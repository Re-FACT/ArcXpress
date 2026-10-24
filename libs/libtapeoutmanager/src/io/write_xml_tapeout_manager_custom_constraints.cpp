#include "write_xml_tapeout_manager_custom_constraints.h"

#include <filesystem>
#include <map>
#include <set>

#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "vtr_log.h"

namespace {

// Helper function to generate a signature of an XML element including its attributes and text
// content
std::string generate_element_signature(const pugi::xml_node& node) {
  std::string signature = "<" + std::string(node.name());

  // Add all attributes in sorted order to ensure consistent signature
  std::map<std::string, std::string> attrs;
  for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute()) {
    attrs[attr.name()] = attr.value();
  }

  for (const auto& attr_pair : attrs) {
    signature += " " + attr_pair.first + "=\"" + attr_pair.second + "\"";
  }

  signature += ">";

  // Add text content if present
  if (node.text()) {
    signature += node.text().as_string();
  }

  // Process child nodes recursively
  for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
    signature += generate_element_signature(child);
  }

  signature += "</" + std::string(node.name()) + ">";

  return signature;
}

bool region_nodes_equal(const pugi::xml_node& r1, const pugi::xml_node& r2) {
  return r1.attribute("x_low").as_string() == r2.attribute("x_low").as_string() &&
         r1.attribute("y_low").as_string() == r2.attribute("y_low").as_string() &&
         r1.attribute("x_high").as_string() == r2.attribute("x_high").as_string() &&
         r1.attribute("y_high").as_string() == r2.attribute("y_high").as_string() &&
         r1.attribute("subtile").as_string() == r2.attribute("subtile").as_string();
}

bool atom_nodes_equal(const pugi::xml_node& a1, const pugi::xml_node& a2) {
  return a1.attribute("name_pattern").as_string() == a2.attribute("name_pattern").as_string();
}

std::string canonical_path(const std::string& path) {
  try {
    return std::filesystem::weakly_canonical(std::filesystem::path(path)).string();
  } catch (...) {
    return path;
  }
}

}  // anonymous namespace

namespace arcxpress {

void merge_vpr_constraint_files(const std::vector<std::string>& constraint_filenames,
                                const std::string& output_filename) {
  // Create the main document with a new root element
  pugi::xml_document main_doc;
  pugi::xml_node merged_root =
    main_doc.append_child("vpr_constraints");  // Use appropriate root name

  bool has_any_content = false;

  std::set<std::string> visited_files;
  std::string canonical_output = canonical_path(output_filename);

  for (const std::string& filename : constraint_filenames) {
    std::string canonical_input = canonical_path(filename);
    if (canonical_input == canonical_output) {
      // avoid self-merge if output file exists in input list
      continue;
    }
    if (!visited_files.insert(canonical_input).second) {
      // skip identical file visited earlier
      continue;
    }
    if (!std::filesystem::exists(filename)) {
      continue;  // Skip non-existing files
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());

    if (result) {
      // Iterate over the children of the source document's root
      for (pugi::xml_node child : doc.first_child().children()) {
        // Handle different types of nodes differently for smart merging
        std::string element_name = child.name();

        if (element_name == std::string("partition_list")) {
          // For partition_list, create or get the target partition_list node
          pugi::xml_node target_partition_list = merged_root.child("partition_list");
          if (!target_partition_list) {
            target_partition_list = merged_root.append_child("partition_list");
          }

          // Process each partition in the source
          for (pugi::xml_node src_partition : child.children("partition")) {
            std::string src_partition_name = src_partition.attribute("name").as_string();

            // Find if a partition with the same name already exists
            pugi::xml_node existing_partition;
            bool found_existing = false;
            for (pugi::xml_node target_partition : target_partition_list.children("partition")) {
              if (target_partition.attribute("name").as_string() == src_partition_name) {
                existing_partition = target_partition;
                found_existing = true;
                break;
              }
            }

            if (found_existing) {
              // Partition exists, merge content (regions, atoms, etc.) avoiding duplicates

              // Get existing regions and atoms to check against
              std::vector<std::string> existing_region_signatures;
              for (pugi::xml_node existing_region : existing_partition.children("add_region")) {
                existing_region_signatures.push_back(generate_element_signature(existing_region));
              }

              std::vector<std::string> existing_atom_signatures;
              for (pugi::xml_node existing_atom : existing_partition.children("add_atom")) {
                existing_atom_signatures.push_back(generate_element_signature(existing_atom));
              }

              // Add unique regions
              for (pugi::xml_node src_region : src_partition.children("add_region")) {
                std::string region_signature = generate_element_signature(src_region);
                bool exists = false;
                for (const auto& existing_sig : existing_region_signatures) {
                  if (region_signature == existing_sig) {
                    exists = true;
                    break;
                  }
                }
                if (!exists) {
                  existing_partition.append_copy(src_region);
                  has_any_content = true;
                }
              }

              // Add unique atoms
              for (pugi::xml_node src_atom : src_partition.children("add_atom")) {
                std::string atom_signature = generate_element_signature(src_atom);
                bool exists = false;
                for (const auto& existing_sig : existing_atom_signatures) {
                  if (atom_signature == existing_sig) {
                    exists = true;
                    break;
                  }
                }
                if (!exists) {
                  existing_partition.append_copy(src_atom);
                  has_any_content = true;
                }
              }
            } else {
              // Partition doesn't exist, add the whole partition
              target_partition_list.append_copy(src_partition);
              has_any_content = true;
            }
          }
        } else if (element_name == std::string("global_route_constraints")) {
          // For global_route_constraints, create or get the target node
          pugi::xml_node target_grc = merged_root.child("global_route_constraints");
          if (!target_grc) {
            target_grc = merged_root.append_child("global_route_constraints");
          }

          // Get existing signal signatures to check against
          std::vector<std::string> existing_signal_signatures;
          for (pugi::xml_node existing_signal : target_grc.children("set_global_signal")) {
            existing_signal_signatures.push_back(generate_element_signature(existing_signal));
          }

          // Add unique set_global_signal elements
          for (pugi::xml_node src_signal : child.children("set_global_signal")) {
            std::string signal_signature = generate_element_signature(src_signal);
            bool exists = false;
            for (const auto& existing_sig : existing_signal_signatures) {
              if (signal_signature == existing_sig) {
                exists = true;
                break;
              }
            }
            if (!exists) {
              target_grc.append_copy(src_signal);
              has_any_content = true;
            }
          }
        } else {
          // For other types of nodes, check if identical node already exists using signature
          std::set<std::string> existing_signatures;
          for (pugi::xml_node existing_child : merged_root.children(element_name.c_str())) {
            existing_signatures.insert(generate_element_signature(existing_child));
          }

          std::string child_signature = generate_element_signature(child);
          if (existing_signatures.find(child_signature) == existing_signatures.end()) {
            merged_root.append_copy(child);
            has_any_content = true;
          }
        }
      }
    } else {
      VTR_LOG_WARN("Error loading constraint file %s: %s\n", filename.c_str(),
                   result.description());
    }
  }

  if (has_any_content ||
      pugiutil::count_children(merged_root, pugiutil::loc_data{}, pugiutil::ReqOpt::OPTIONAL) > 0) {
    // Save the merged document to a new file
    if (main_doc.save_file(output_filename.c_str())) {
      VTR_LOG("Successfully saved merged constraints to %s\n", output_filename.c_str());
    } else {
      VTR_LOG_ERROR("Failed to save merged constraint file: %s\n", output_filename.c_str());
    }
  } else {
    VTR_LOG_WARN("No constraint content to merge, skipping creation of %s\n",
                 output_filename.c_str());
  }
}

}  // namespace arcxpress