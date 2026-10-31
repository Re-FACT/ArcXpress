/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "mosofa_arch_layout_clb_subtile.h"
#include "mosofa_constants.h"
#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA { // begin namespace MOSOFA

int add_clb_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                       const ArchPlanner& arch_planner) {
  int num_err = 0;

  size_t num_fle = arch_planner.num_fle_per_clb();
  size_t lut_size = arch_planner.max_lut_size();
  size_t num_outputs = num_fle * 2;

  std::string pb_type_name("clb");
  num_err +=
    va_layout.create_pb_type(pb_type_name, arcxpress::VprArchLayout::e_subtile_type::CLB) ? 0 : 1;
  size_t equ_input_width = lut_size - 1;
  for (size_t iin = 0; iin < num_fle; ++iin) {
    std::string port_name = std::string("I") + std::to_string(iin);
    num_err +=
      va_layout.add_pb_type_port(pb_type_name, port_name,
                                 arcxpress::VprArchLayout::e_port_type::INPUT, equ_input_width)
        ? 0
        : 1;
    num_err += va_layout.set_pb_type_port_logic_equivalent(pb_type_name, port_name, true) ? 0 : 1;
  }
  /* Last input of LUTs */
  std::string Ix_port_name = std::string("Ix");
  num_err += va_layout.add_pb_type_port(pb_type_name, Ix_port_name,
                                        arcxpress::VprArchLayout::e_port_type::INPUT, num_fle)
               ? 0
               : 1;
  /* Cin */
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "cin", arcxpress::VprArchLayout::e_port_type::CI) ? 0
                                                                                              : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "reg_in", arcxpress::VprArchLayout::e_port_type::CI) ? 0
                                                                                              : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "sc_in", arcxpress::VprArchLayout::e_port_type::CI) ? 0
                                                                                              : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "reset", arcxpress::VprArchLayout::e_port_type::RESET)
      ? 0
      : 1;
  num_err += va_layout.add_pb_type_port(pb_type_name, "O",
                                        arcxpress::VprArchLayout::e_port_type::OUTPUT, num_outputs)
               ? 0
               : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "cout", arcxpress::VprArchLayout::e_port_type::CO) ? 0
                                                                                               : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "reg_out", arcxpress::VprArchLayout::e_port_type::CO) ? 0
                                                                                               : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "sc_out", arcxpress::VprArchLayout::e_port_type::CO) ? 0
                                                                                               : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "clk", arcxpress::VprArchLayout::e_port_type::CLOCK)
      ? 0
      : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_clb_subtile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                       const ArchPlanner& arch_planner,
                                       const std::string& tile_name) {
  int num_err = 0;

  size_t num_fle = arch_planner.num_fle_per_clb();
  size_t lut_size = arch_planner.max_lut_size();
  size_t num_outputs = num_fle * 2;

  size_t equ_input_width = lut_size - 1;

  std::string subtile_name("clb");
  num_err += va_layout.add_subtile(tile_name, subtile_name, "clb", {}) ? 0 : 1;
  /* Add custom pin location */
  num_err +=
    va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "clk", 0, RIGHT) ? 0 : 1;
  /* Add right side inputs and outputs, make it even on two sides */
  for (size_t iport = 0; iport < num_fle; ++iport) {
    std::string port_name = std::string("I") + std::to_string(iport);
    /* FIXME: This is a temporarily patch to resolve the issue that global signals are not routable
     * to CLB inputs (to drive LUTs). This allows more LUT inputs (=4, all in one group, being
     * swappable by router) to be driven by global signal network. Ideally, all the LUT inputs
     * should be driven but at the cost of routability issues (all the CLB inputs are one side,
     * which should be avoided) */
    for (size_t ipin = 0; ipin < equ_input_width / 2 + 2; ++ipin) {
      num_err +=
        va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, port_name, ipin, RIGHT) ? 0
                                                                                               : 1;
    }
  }
  /* To balance pin location, Ix locates on top side only*/
  std::string Ix_port_name = std::string("Ix");
  for (size_t ipin = 0; ipin < num_fle / 2; ++ipin) {
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, Ix_port_name, ipin, TOP) ? 0
                                                                                              : 1;
  }
  for (size_t ipin = 0; ipin < num_outputs / 2; ++ipin) {
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "O", ipin, RIGHT) ? 0 : 1;
  }
  num_err +=
    va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "reset", 0, RIGHT) ? 0 : 1;
  /* Add top side  */
  for (size_t iport = 0; iport < num_fle; ++iport) {
    std::string port_name = std::string("I") + std::to_string(iport);
    for (size_t ipin = equ_input_width / 2 + 2; ipin < equ_input_width; ++ipin) {
      num_err +=
        va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, port_name, ipin, TOP) ? 0
                                                                                             : 1;
    }
  }
  for (size_t ipin = num_fle / 2; ipin < num_fle; ++ipin) {
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, Ix_port_name, ipin, TOP) ? 0
                                                                                              : 1;
  }
  for (size_t ipin = num_outputs / 2; ipin < num_outputs; ++ipin) {
    num_err +=
      va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "O", ipin, TOP) ? 0 : 1;
  }

  num_err += va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "cin", 0, TOP) ? 0 : 1;
  num_err += va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "reg_in", 0, TOP) ? 0 : 1;
  num_err += va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "sc_in", 0, TOP) ? 0 : 1;
  num_err +=
    va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "cout", 0, BOTTOM) ? 0 : 1;
    va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "reg_out", 0, BOTTOM) ? 0 : 1;
    va_layout.add_subtile_pin_location(tile_name, subtile_name, 0, "sc_out", 0, BOTTOM) ? 0 : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // end namespace Sofa
