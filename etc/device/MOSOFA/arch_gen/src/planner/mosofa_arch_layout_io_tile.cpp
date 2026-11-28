/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "command_exit_codes.h"
#include "mosofa_arch_layout_io_subtile.h"
#include "mosofa_arch_layout_io_tile.h"
#include "mosofa_constants.h"
#include "mosofa_naming.h"
#include "side_manager.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA {  // begin namespace MOSOFA

static int add_io_tile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                          const ArchPlanner& arch_plan, const e_side& io_side) {
  int num_err = 0;

  std::string tile_name = gen_io_tile_name(io_side);
  num_err +=
    va_layout.create_tile(tile_name, arcxpress::VprArchLayout::e_tile_type::IO, io_side) ? 0 : 1;

  std::string subtile_pi_name = gen_pi_subtile_name(io_side);
  size_t num_pi = arch_plan.num_input_per_tile(io_side);
  std::string pi_phy_pb_type_name = gen_pi_pb_type_name();
  std::vector<std::string> pi_equ_pb_type_names;
  num_err += va_layout.add_subtile(tile_name, subtile_pi_name, pi_phy_pb_type_name,
                                   pi_equ_pb_type_names, num_pi)
               ? 0
               : 1;
  e_side pi_pin_side = SideManager(io_side).get_opposite();
  // Currently all the global signal should access from the right side of I/Os. However, at the right side of eFPGA, the access point should be adjusted to the left side
  e_side pi_gclk_pin_side = RIGHT;
  if (io_side == RIGHT) {
    pi_gclk_pin_side = LEFT;
  }
  for (size_t ipi = 0; ipi < num_pi; ++ipi) {
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_pi_name, ipi, "a2f_o", 0, pi_pin_side)
        ? 0
        : 1;
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_pi_name, ipi, "ff_en", 0, pi_pin_side)
        ? 0
        : 1;
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_pi_name, ipi, "clk", 0, pi_gclk_pin_side)
        ? 0
        : 1;
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_pi_name, ipi, "reset", 0, pi_gclk_pin_side)
        ? 0
        : 1;
  }

  std::string subtile_po_name = gen_po_subtile_name(io_side);
  size_t num_po = arch_plan.num_output_per_tile(io_side);
  std::string po_phy_pb_type_name = gen_po_pb_type_name();
  std::vector<std::string> po_equ_pb_type_names;
  num_err += va_layout.add_subtile(tile_name, subtile_po_name, po_phy_pb_type_name,
                                   po_equ_pb_type_names, num_po)
               ? 0
               : 1;
  e_side po_pin_side = SideManager(io_side).get_opposite();
  // Currently all the global signal should access from the right side of I/Os. However, at the right side of eFPGA, the access point should be adjusted to the left side
  e_side po_gclk_pin_side = RIGHT;
  if (io_side == RIGHT) {
    po_gclk_pin_side = LEFT;
  }
  for (size_t ipo = 0; ipo < num_po; ++ipo) {
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_po_name, ipo, "f2a_i", 0, po_pin_side)
        ? 0
        : 1;
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_po_name, ipo, "ff_en", 0, po_pin_side)
        ? 0
        : 1;
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_po_name, ipo, "clk", 0, po_gclk_pin_side)
        ? 0
        : 1;
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_po_name, ipo, "reset", 0, po_gclk_pin_side)
        ? 0
        : 1;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_io_tiles_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                    const ArchPlanner& arch_plan) {
  int num_err = 0;

  VTR_LOG("Adding pi_pb_type\n");
  num_err += add_pi_pb_type_to_vpr_arch_layout(va_layout);
  VTR_LOG("Adding po_pb_type\n");
  num_err += add_po_pb_type_to_vpr_arch_layout(va_layout);

  for (e_side curr_side : TOTAL_2D_SIDES) {
    VTR_LOG("Adding io tile\n");
    num_err += add_io_tile_to_vpr_arch_layout(va_layout, arch_plan, curr_side);
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace MOSOFA
