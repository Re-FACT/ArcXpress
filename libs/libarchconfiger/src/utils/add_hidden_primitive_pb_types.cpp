/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "add_hidden_primitive_pb_types.h"
#include "arch_gen_constants.h"
#include "command_exit_codes.h"
#include "openfpga_port.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arkangel {

int add_builtin_pi_primitive_pb_type(arkangel::ArchGen& arch_gen, const std::string& pb_name,
                                     const std::string& mode_bits, const bool& verbose) {
  int num_err = 0;

  /* Port modeling */
  num_err += arch_gen.add_pb_type(pb_name) ? 0 : 1;
  num_err += arch_gen.set_pb_type_blif_model(pb_name, std::string(PI_BLIF_MODEL)) ? 0 : 1;
  if (!mode_bits.empty()) {
    num_err += arch_gen.set_pb_type_mode_bits(pb_name, mode_bits) ? 0 : 1;
  }
  openfpga::BasicPort o_port("inpad", 1);
  num_err += arch_gen.add_pb_type_port(pb_name, o_port, VprArchLayout::e_port_type::OUTPUT) ? 0 : 1;

  VTR_LOGV(verbose, "Added hidden primitive pb_type '%s'\n", pb_name.c_str());

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_builtin_po_primitive_pb_type(arkangel::ArchGen& arch_gen, const std::string& pb_name,
                                     const bool& verbose) {
  int num_err = 0;

  /* Port modeling */
  num_err += arch_gen.add_pb_type(pb_name) ? 0 : 1;
  num_err += arch_gen.set_pb_type_blif_model(pb_name, std::string(PO_BLIF_MODEL)) ? 0 : 1;
  openfpga::BasicPort o_port("outpad", 1);
  num_err += arch_gen.add_pb_type_port(pb_name, o_port, VprArchLayout::e_port_type::INPUT) ? 0 : 1;

  VTR_LOGV(verbose, "Added hidden primitive pb_type '%s'\n", pb_name.c_str());

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_builtin_latch_primitive_pb_type(arkangel::ArchGen& arch_gen, const std::string& pb_name,
                                        const bool& verbose) {
  int num_err = 0;

  /* Port modeling */
  num_err += arch_gen.add_pb_type(pb_name) ? 0 : 1;
  num_err += arch_gen.set_pb_type_blif_model(pb_name, std::string(LATCH_BLIF_MODEL)) ? 0 : 1;
  openfpga::BasicPort d_port("D", 1);
  openfpga::BasicPort q_port("Q", 1);
  openfpga::BasicPort ck_port("clk", 1);
  num_err += arch_gen.add_pb_type_port(pb_name, d_port, VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err += arch_gen.set_pb_type_port_class(pb_name, d_port, ArchGen::e_port_class::FF_D) ? 0 : 1;
  num_err += arch_gen.add_pb_type_port(pb_name, q_port, VprArchLayout::e_port_type::OUTPUT) ? 0 : 1;
  num_err += arch_gen.set_pb_type_port_class(pb_name, q_port, ArchGen::e_port_class::FF_Q) ? 0 : 1;
  num_err += arch_gen.add_pb_type_port(pb_name, ck_port, VprArchLayout::e_port_type::CLOCK) ? 0 : 1;
  num_err +=
    arch_gen.set_pb_type_port_class(pb_name, ck_port, ArchGen::e_port_class::FF_CLOCK) ? 0 : 1;
  /* Timing */
  num_err +=
    arch_gen.add_pb_type_setup_delay(pb_name, d_port, ck_port, "${IO_LATCH_TSETUP}") ? 0 : 1;
  num_err += arch_gen.add_pb_type_hold_delay(pb_name, d_port, ck_port, "${IO_LATCH_THOLD}") ? 0 : 1;
  num_err += arch_gen.add_pb_type_clk2q_delay(pb_name, q_port, ck_port, "${IO_LATCH_CLK2Q_MAX}",
                                              "${IO_LATCH_CLK2Q_MIN}")
               ? 0
               : 1;

  VTR_LOGV(verbose, "Added hidden primitive pb_type '%s'\n", pb_name.c_str());

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_builtin_lut_primitive_pb_type(arkangel::ArchGen& arch_gen, const short& lut_size,
                                      const std::string& mode_bits, const bool& verbose) {
  int num_err = 0;
  VTR_LOGV(verbose, "Adding primitive pb_types for VPR built-in LUTs...\n");
  /* Sanity checks */
  if (lut_size < 2 || lut_size > 8) {
    VTR_LOG_ERROR("The LUT input size '%d' is out of the range [2, 8]!\n", lut_size);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Port modeling */
  std::string pb_name = arch_gen.lut_primitive_pb_type_name(lut_size);
  num_err += arch_gen.add_pb_type(pb_name) ? 0 : 1;
  num_err += arch_gen.set_pb_type_blif_model(pb_name, std::string(LUT_BLIF_MODEL)) ? 0 : 1;
  num_err += arch_gen.set_pb_type_class(pb_name, ArchGen::e_pb_type_class::LUT) ? 0 : 1;
  if (!mode_bits.empty()) {
    num_err += arch_gen.set_pb_type_mode_bits(pb_name, mode_bits) ? 0 : 1;
  }
  openfpga::BasicPort in_port("in", lut_size);
  openfpga::BasicPort out_port("out", 1);
  num_err += arch_gen.add_pb_type_port(pb_name, in_port, VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err +=
    arch_gen.set_pb_type_port_class(pb_name, in_port, ArchGen::e_port_class::LUT_IN) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(pb_name, out_port, VprArchLayout::e_port_type::OUTPUT) ? 0 : 1;
  num_err +=
    arch_gen.set_pb_type_port_class(pb_name, out_port, ArchGen::e_port_class::LUT_OUT) ? 0 : 1;
  /* Timing */
  std::string delay_var = arch_gen.lut_comb_delay_variable(lut_size);
  for (auto pin : in_port.pins()) {
    openfpga::BasicPort curr_in_port(in_port.get_name(), pin, pin);
    num_err += arch_gen.add_pb_type_comb_delay(pb_name, curr_in_port, out_port, delay_var) ? 0 : 1;
  }

  VTR_LOGV(verbose, "Added hidden primitive pb_type '%s'\n", pb_name.c_str());
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arkangel
