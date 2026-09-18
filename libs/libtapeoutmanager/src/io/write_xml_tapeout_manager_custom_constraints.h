#ifndef WRITE_XML_TAPEOUT_MANAGER_CUSTOM_CONSTRAINTS_H
#define WRITE_XML_TAPEOUT_MANAGER_CUSTOM_CONSTRAINTS_H

#include <string>
#include <vector>

#include "pugixml.hpp"

/* namespace arkangel begins */
namespace arkangel {

void merge_vpr_constraint_files(const std::vector<std::string>& constraint_filenames,
                                const std::string& output_filename);

void dedupe_vpr_constraints(pugi::xml_node vpr_constraints_root);

}  // namespace arkangel

#endif