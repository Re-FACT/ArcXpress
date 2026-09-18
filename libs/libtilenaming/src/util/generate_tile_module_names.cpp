#include "generate_tile_module_names.h"

#include "command_exit_codes.h"
#include "read_xml_module_name_map.h"
#include "read_yaml_tile_ref.h"
#include "read_yaml_tile_types.h"
#include "vtr_log.h"
#include "write_xml_module_name_map.h"

namespace arkangel {  // Begin namespace arkangel

/* Generate a tile module names based on tile references, reference module names */
int generate_tile_module_names(const std::string& i_module_names_fname,
                               const std::string& i_fabric_hie_fname,
                               const std::string& i_tile_ref_fname, const std::string& o_tile_fname,
                               const bool& verbose) {
  /* Read the io coordinate file */
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  TileRef tile_ref;
  status = read_yaml_tile_ref(i_tile_ref_fname.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = read_yaml_tile_types(i_fabric_hie_fname.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::ModuleNameMap ref_module_names;
  status = openfpga::read_xml_module_name_map(i_module_names_fname.c_str(), ref_module_names);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Create a custom reference module names */
  VTR_LOG("Generating custom module names...\n");
  openfpga::ModuleNameMap final_module_names;
  for (std::string tile : tile_ref.tiles()) {
    if (tile.rfind("tile", 0) != 0) {
      continue; /* Bypass non-tile modules */
    }
    if (!ref_module_names.tag_exist(tile)) {
      VTR_LOG_ERROR("Expect tile name '%s' does not exist in reference module names!\n",
                    tile.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Create a custom name */
    std::string cust_tile_name = tile_ref.custom_tile_name(tile);
    VTR_LOG("Adding custom module name '%s'\n", cust_tile_name.c_str());
    final_module_names.set_tag_to_name_pair(tile, cust_tile_name);
  }
  status =
    openfpga::write_xml_module_name_map(o_tile_fname.c_str(), final_module_names, true, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arkangel
