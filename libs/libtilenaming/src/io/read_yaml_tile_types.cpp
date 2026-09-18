/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML of project file to the associated
 * data structures
 *******************************************************************/
#include "read_yaml_tile_types.h"

#include <algorithm>
#include <filesystem>
#include <regex>
#include <string>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "yaml-cpp/yaml.h"

namespace arkangel {  // Begin namespace arkangel

/* Go recursively to find the core node where tiles are defined */
static int find_core_node(const YAML::Node& config, YAML::Node& core_node, const bool& verbose) {
  YAML::Node curr_node = config;
  while (1) {
    if (curr_node.size() > 1) {
      VTR_LOGV(verbose, "Found the node where tiles are defined\n");
      break;
    }
    if (curr_node.size() != 1) {
      VTR_LOG_ERROR("Expect only 1 root node in the file. But now have %lu\n", curr_node.size());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    if (!curr_node.IsMap()) {
      VTR_LOG_ERROR("Expect a map under the root node\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    for (YAML::const_iterator it = curr_node.begin(); it != curr_node.end(); ++it) {
      std::string child_name = it->first.as<std::string>();
      curr_node = curr_node[child_name];
      VTR_LOGV(verbose, "Go to the next node '%s' in hierarchy\n", child_name.c_str());
      break;
    }
  }
  core_node = curr_node;

  return openfpga::CMD_EXEC_SUCCESS;
}

static size_t read_yaml_tile_ref_types(const YAML::Node& core_node, TileRef& tile_ref,
                                       const bool& verbose) {
  size_t num_err = 0;
  for (YAML::const_iterator it = core_node.begin(); it != core_node.end(); ++it) {
    std::string tile_name = it->first.as<std::string>();
    YAML::Node tile_node = core_node[tile_name];
    if (tile_node.size() == 0) {
      VTR_LOGV(verbose, "Bypass dummy tile module '%s'\n", tile_name.c_str());
      continue;
    }
    if (!tile_node.IsSequence()) {
      VTR_LOG_ERROR("Expect a list under the tile node '%s'\n", tile_name.c_str());
      num_err++;
      continue;
    }
    /* Get the type */
    std::string type_str;
    for (size_t ichild = 0; ichild < tile_node.size(); ichild++) {
      /* TODO: Make the regex pattern configurable ! */
      std::regex pattern("grid_([a-zA-z.0-9]+)");
      std::smatch match;
      std::string orig_type_str = tile_node[ichild].as<std::string>();
      VTR_LOGV(verbose, "Checking child '%s'\n", orig_type_str.c_str());
      if (std::regex_search(orig_type_str, match, pattern)) {
        type_str = match[1].str();
        break;
      }
    }
    if (!type_str.empty()) {
      VTR_LOGV(verbose, "Set type '%s' to tile '%s'\n", type_str.c_str(), tile_name.c_str());
      tile_ref.set_type(tile_name, type_str);
    }
  }
  return num_err;
}

static size_t read_yaml_tile_subblocks_from_core_node(const YAML::Node& core_node,
                                                      std::vector<std::string>& subblks,
                                                      const bool& verbose) {
  size_t num_err = 0;
  for (YAML::const_iterator it = core_node.begin(); it != core_node.end(); ++it) {
    std::string tile_name = it->first.as<std::string>();
    YAML::Node tile_node = core_node[tile_name];
    if (tile_node.size() == 0) {
      if (subblks.end() == std::find(subblks.begin(), subblks.end(), tile_name)) {
        VTR_LOGV(verbose, "Add subblock module '%s' directly from the core node\n",
                 tile_name.c_str());
        subblks.push_back(tile_name);
      }
      continue;
    }
    if (!tile_node.IsSequence()) {
      VTR_LOG_ERROR("Expect a list under the tile node '%s'\n", tile_name.c_str());
      num_err++;
      continue;
    }
    /* Get the type */
    std::string type_str;
    for (size_t ichild = 0; ichild < tile_node.size(); ichild++) {
      std::string orig_type_str = tile_node[ichild].as<std::string>();
      if (subblks.end() == std::find(subblks.begin(), subblks.end(), orig_type_str)) {
        VTR_LOGV(verbose, "Add subblock module '%s' from tile '%s'\n", orig_type_str.c_str(),
                 tile_name.c_str());
        subblks.push_back(orig_type_str);
      }
    }
  }
  return num_err;
}

/********************************************************************
 * Parse YAML codes to an object of TileRef
 *******************************************************************/
int read_yaml_tile_types(const char* fname, TileRef& tile_ref, const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read fabric hierarchy file to load types of tiles");
  size_t num_err = 0;

  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File '%s' does not exist!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Parse the file */
  YAML::Node config = YAML::LoadFile(std::string(fname));
  try {
    YAML::Node core_node;
    int status = find_core_node(config, core_node, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    num_err += read_yaml_tile_ref_types(core_node, tile_ref, verbose);
  } catch (const YAML::BadFile& e) {
    VTR_LOG_ERROR("%s\n", e.msg.c_str());
    num_err++;
  } catch (const YAML::ParserException& e) {
    VTR_LOG_ERROR("%s\n", e.msg.c_str());
    num_err++;
  }

  if (num_err) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int read_yaml_tile_subblocks(const char* fname, std::vector<std::string>& subblks,
                             const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read fabric hierarchy file to find sub-blocks under tiles");
  size_t num_err = 0;

  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File '%s' does not exist!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Parse the file */
  YAML::Node config = YAML::LoadFile(std::string(fname));
  try {
    YAML::Node core_node;
    int status = find_core_node(config, core_node, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    num_err += read_yaml_tile_subblocks_from_core_node(core_node, subblks, verbose);
  } catch (const YAML::BadFile& e) {
    VTR_LOG_ERROR("%s\n", e.msg.c_str());
    num_err++;
  } catch (const YAML::ParserException& e) {
    VTR_LOG_ERROR("%s\n", e.msg.c_str());
    num_err++;
  }

  if (num_err) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arkangel
