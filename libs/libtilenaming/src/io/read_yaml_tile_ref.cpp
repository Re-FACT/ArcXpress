/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML of project file to the associated
 * data structures
 *******************************************************************/
#include "read_yaml_tile_ref.h"

#include <filesystem>
#include <regex>
#include <string>

#include "command_exit_codes.h"
#include "tile_ref_yaml_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "yaml-cpp/yaml.h"

namespace arcxpress {  // Begin namespace arcxpress

/* Parse detailed module references with sanity checks */
static int read_yaml_module_reference(const YAML::Node& config, const size_t& imod,
                                      TileRef& tile_ref, const std::string& fname,
                                      const bool& verbose) {
  int num_err = 0;
  /* Module name */
  if (!config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_MODULE].IsDefined()) {
    VTR_LOG_ERROR(
      "Expected node '%s' under root '%s' not found!\nThe tile reference file '%s' may be "
      "broken!\n",
      YML_TILEREF_NODE_MODULE, YML_TILEREF_ROOT_NODE_NAME, fname);
    num_err++;
    return num_err;
  }
  std::string module_name =
    config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_MODULE].as<std::string>();

  /* Instance Count */
  if (!config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_COUNT].IsDefined()) {
    VTR_LOG_ERROR(
      "Expected node '%s' under root '%s' not found!\nThe tile reference file '%s' may be "
      "broken!\n",
      YML_TILEREF_NODE_COUNT, YML_TILEREF_ROOT_NODE_NAME, fname);
    num_err++;
    return num_err;
  }
  int inst_count = config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_COUNT].as<int>();

  /* Dimension */
  int height = 1;
  if (config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_HEIGHT].IsDefined()) {
    height = config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_HEIGHT].as<int>();
  }
  int width = 1;
  if (config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_WIDTH].IsDefined()) {
    width = config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_WIDTH].as<int>();
  }

  /* Instances */
  if (!config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_INSTANCES].IsDefined()) {
    VTR_LOG_ERROR(
      "Expected node '%s' under root '%s' not found!\nThe tile reference file '%s' may be "
      "broken!\n",
      YML_TILEREF_NODE_INSTANCES, YML_TILEREF_ROOT_NODE_NAME, fname);
    num_err++;
    return num_err;
  }
  if (!config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_INSTANCES].IsSequence()) {
    VTR_LOG_ERROR(
      "Expected a list under node '%s' of root '%s'!\nThe tile reference file '%s' may be "
      "broken!\n",
      YML_TILEREF_NODE_INSTANCES, YML_TILEREF_ROOT_NODE_NAME, fname);
    num_err++;
    return num_err;
  }
  if (config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_INSTANCES].size() != inst_count) {
    VTR_LOG_ERROR(
      "Number of instances (%lu) in node '%s' of root '%s' does not match the number (%lu) defined "
      "in node '%s' !\nThe tile reference file '%s' may be broken!\n",
      config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_INSTANCES].size(),
      YML_TILEREF_NODE_INSTANCES, YML_TILEREF_ROOT_NODE_NAME, inst_count, YML_TILEREF_NODE_COUNT,
      fname);
    num_err++;
    return num_err;
  }

  /* Load data */
  num_err += tile_ref.add_tile(module_name, height, width) ? 0 : 1;
  if (num_err) {
    return num_err;
  }
  for (size_t iinst = 0;
       iinst < config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_INSTANCES].size();
       ++iinst) {
    std::string inst_name =
      config[YML_TILEREF_ROOT_NODE_NAME][imod][YML_TILEREF_NODE_INSTANCES][iinst].as<std::string>();
    num_err += tile_ref.add_instance(module_name, inst_name) ? 0 : 1;
  }
  VTR_LOGV(verbose, "Found tile '%s' with %lu instances\n", module_name.c_str(), inst_count);

  return num_err;
}

/********************************************************************
 * Parse YAML codes to an object of TileRef
 *******************************************************************/
int read_yaml_tile_ref(const char* fname, TileRef& tile_ref, const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read tile reference file");
  size_t num_err = 0;

  tile_ref.clear();

  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File '%s' does not exist!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Parse the file */
  YAML::Node config = YAML::LoadFile(std::string(fname));
  try {
    /* Find root nodes */
    if (!config[YML_TILEREF_ROOT_NODE_NAME].IsDefined()) {
      VTR_LOG_ERROR(
        "Expected root node '%s' not found!\nThe tile reference file '%s' may be broken!\n",
        YML_TILEREF_ROOT_NODE_NAME, fname);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    if (!config[YML_TILEREF_ROOT_NODE_NAME].IsSequence()) {
      VTR_LOG_ERROR(
        "Expected a list defined under root node '%s'!\nThe tile reference file '%s' may be "
        "broken!\n",
        YML_TILEREF_ROOT_NODE_NAME, fname);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    for (size_t imod = 0; imod < config[YML_TILEREF_ROOT_NODE_NAME].size(); ++imod) {
      num_err += read_yaml_module_reference(config, imod, tile_ref, fname, verbose);
    }
    /* Sanity checks on the summary nodes. This is optional  */
    if (config[YML_TILEREF_NODE_TOTAL].IsDefined()) {
      std::regex pattern("([0-9]+) modules");
      std::smatch match;
      std::string line = config[YML_TILEREF_NODE_TOTAL].as<std::string>();
      if (std::regex_search(line, match, pattern)) {
        if (tile_ref.num_tiles() != std::stoi(match[1].str())) {
          VTR_LOG_ERROR(
            "Mismatch in the number of modules. Found '%lu' but summary line defines '%s'!\n",
            tile_ref.num_tiles(), match[1].str().c_str());
          num_err++;
        }
      } else {
        VTR_LOG_ERROR(
          "Unable to find the number of modules in the summary line: '%s'!\n\tExpect a format like "
          "<int> modules <int> references\n",
          config[YML_TILEREF_NODE_TOTAL].as<std::string>().c_str());
        num_err++;
      }
    }
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

}  // End of namespace arcxpress
