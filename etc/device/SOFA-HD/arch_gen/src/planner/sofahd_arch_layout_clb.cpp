/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "vpr_arch_layout.h"
#include "sofa_arch_layout_clb.h"
#include "sofahd_arch_layout_clb.h"
#include "sofahd_constants.h"
#include "command_exit_codes.h"
#include "add_hidden_primitive_pb_types.h"

namespace SofaHD { // begin namespace SofaHD

static int add_clb_tile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                           const SofaHD::ArchPlanner& arch_plan) {
  int num_err = 0;

  std::string tile_name("clb");
  num_err += va_layout.create_tile(tile_name, arcxpress::VprArchLayout::e_tile_type::CLB) ? 0 : 1;
  num_err += sofa::add_clb_clb_tile_to_vpr_arch_layout(va_layout, tile_name);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_clb_tiles_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                    const SofaHD::ArchPlanner& arch_plan) {
  int num_err = 0;

  num_err += alkaid::add_clb_pb_type_to_vpr_arch_layout(va_layout);
  num_err += add_clb_tile_to_vpr_arch_layout(va_layout, arch_plan);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace SofaHD
