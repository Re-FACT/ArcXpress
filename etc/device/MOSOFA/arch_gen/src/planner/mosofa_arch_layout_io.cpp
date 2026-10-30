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
#include "sofahd_constants.h"
#include "sofahd_naming.h"
#include "sofahd_arch_layout_io.h"
#include "sofahd_arch_layout_io.h"
#include "command_exit_codes.h"
#include "side_manager.h"

namespace SofaHD { // begin namespace SofaHD

static int add_io_tile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                           const SofaHD::ArchPlanner& arch_plan,
                                           const e_side& io_side) {
  int num_err = 0;

  std::string tile_name = SofaHD::gen_io_tile_name(io_side);
  num_err += va_layout.create_tile(tile_name, arcxpress::VprArchLayout::e_tile_type::IO, io_side) ? 0 : 1;

  std::string subtile_pio_name = SofaHD::gen_io_subtile_name(io_side, 0, false, false);
  size_t num_pio = arch_plan.num_io_per_tile(io_side);
  std::string pio_phy_pb_type_name = SofaHD::gen_pio_pb_type_name(0, false, false);
  std::vector<std::string> pio_equ_pb_type_names;
  num_err += va_layout.add_subtile(tile_name, subtile_pio_name, pio_phy_pb_type_name, pio_equ_pb_type_names, num_pio) ? 0 : 1;
  e_side io_pin_side = SideManager(io_side).opposite();
  for (size_t ipio = 0; ipio < num_pio; ++ipio) {
    num_err += va_layout.add_subtile_pin_location(tile_name, subtile_pio_name, ipio, "outpad", 0, io_pin_side) ? 0 : 1;
    num_err += va_layout.add_subtile_pin_location(tile_name, subtile_pio_name, ipio, "inpad", 0, io_pin_side) ? 0 : 1;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_io_tiles_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                           const SofaHD::ArchPlanner& arch_plan) {
  int num_err = 0;

  num_err += SofaHD::add_pio_pb_type_to_vpr_arch_layout(va_layout);

  for (e_side curr_side : TOTAL_2D_SIDES) {
    num_err += add_io_tile_to_vpr_arch_layout(va_layout, arch_plan, curr_side);
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace SofaHD
