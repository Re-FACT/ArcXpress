/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "mosofa_complex_block_clb.h"
#include "mosofa_complex_block_ff.h"
#include "mosofa_constants.h"
#include "mosofa_naming.h"
#include "command_exit_codes.h"
#include "fracff_gen.h"
#include "vpr_arch_layout.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA {

static int add_frac_logic_pb_type(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string frac_logic_pb = std::string(PHYSICAL_FRAC_LOGIC_NAME);
  VTR_LOGV(verbose, "Adding intermediate pb_type '%s'\n", frac_logic_pb.c_str());
  num_err += arch_gen.add_pb_type(frac_logic_pb) ? 0 : 1;
  openfpga::BasicPort in_port("in", 4);
  openfpga::BasicPort out_port("out", 2);
  num_err +=
    arch_gen.add_pb_type_port(frac_logic_pb, in_port, arcxpress::VprArchLayout::e_port_type::INPUT)
      ? 0
      : 1;
  num_err +=
    arch_gen.add_pb_type_port(frac_logic_pb, out_port, arcxpress::VprArchLayout::e_port_type::OUTPUT)
      ? 0
      : 1;

  /* Default mode */
  std::string default_mode = arcxpress::gen_default_mode_name(frac_logic_pb);
  std::string frac_lut_pb = arch_gen.find_primitive_pb_type_name(std::string(FRAC_LUT4_BLIF_MODEL));
  num_err += arch_gen.add_pb_type_mode(frac_logic_pb, default_mode) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(default_mode, frac_lut_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_circuit_model(frac_lut_pb, FRAC_LUT4_CIRCUIT_MODEL) ? 0 : 1;
  /* Interconnect*/
  openfpga::BasicPort des_in_port;
  if (!arch_gen.find_pb_type_port(frac_lut_pb, "in", des_in_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto pin : des_in_port.pins()) {
    openfpga::BasicPort src_in_pin(des_in_port.get_name(), pin, pin);
    openfpga::BasicPort des_in_pin(des_in_port.get_name(), pin, pin);
    for (auto ipb : arch_gen.child_pbs(default_mode, frac_lut_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {frac_lut_pb, ipb, des_in_pin};
      num_err += arch_gen.add_in2in_edge(default_mode, src_in_pin, des_edge_port) ? 0 : 1;
    }
  }
  openfpga::BasicPort src_lut3_out_port;
  if (!arch_gen.find_pb_type_port(frac_lut_pb, "lut3_out", src_lut3_out_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Lut3[1] -> out[1] */
  for (auto ipb : arch_gen.child_pbs(default_mode, frac_lut_pb)) {
    openfpga::BasicPort src_out_port(src_lut3_out_port.get_name(), 1, 1);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_lut_pb, ipb, src_out_port};
    openfpga::BasicPort des_out_port(out_port.get_name(), 1, 1);
    num_err += arch_gen.add_out2out_edge(default_mode, src_edge_port, des_out_port) ? 0 : 1;
  }
  /* MUX: Lut3[0] -> out[0] */
  for (auto ipb : arch_gen.child_pbs(default_mode, frac_lut_pb)) {
    openfpga::BasicPort src_out_port(src_lut3_out_port.get_name(), 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_lut_pb, ipb, src_out_port};
    openfpga::BasicPort des_out_port(out_port.get_name(), 0, 0);
    num_err += arch_gen.add_out2out_edge(default_mode, src_edge_port, des_out_port,
                                         PB_MUX_NO_CONST_INPUT_CIRCUIT_MODEL_NAME)
                 ? 0
                 : 1;
  }
  /* MUX: Lut4[0] -> out[0] */
  openfpga::BasicPort src_lut4_out_port;
  if (!arch_gen.find_pb_type_port(frac_lut_pb, "lut4_out", src_lut4_out_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(default_mode, frac_lut_pb)) {
    openfpga::BasicPort src_out_port(src_lut4_out_port.get_name(), 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_lut_pb, ipb, src_out_port};
    openfpga::BasicPort des_out_port(out_port.get_name(), 0, 0);
    num_err += arch_gen.add_out2out_edge(default_mode, src_edge_port, des_out_port,
                                         PB_MUX_NO_CONST_INPUT_CIRCUIT_MODEL_NAME)
                 ? 0
                 : 1;
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_ble3_pb_type(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string ble3_pb = std::string(OPERATING_BLE4_NAME);
  VTR_LOGV(verbose, "Adding intermediate pb_type '%s'\n", ble3_pb.c_str());
  num_err += arch_gen.add_pb_type(ble3_pb) ? 0 : 1;
  openfpga::BasicPort in_port("in", 4);
  openfpga::BasicPort rst_port("reset", 1);
  openfpga::BasicPort ck_port("clk", 1);
  openfpga::BasicPort out_port("out", 1);
  num_err +=
    arch_gen.add_pb_type_port(ble3_pb, in_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0
                                                                                             : 1;
  num_err +=
    arch_gen.add_pb_type_port(ble3_pb, rst_port, arcxpress::VprArchLayout::e_port_type::RESET) ? 0
                                                                                              : 1;
  num_err +=
    arch_gen.add_pb_type_port(ble3_pb, ck_port, arcxpress::VprArchLayout::e_port_type::CLOCK) ? 0
                                                                                             : 1;
  num_err +=
    arch_gen.add_pb_type_port(ble3_pb, out_port, arcxpress::VprArchLayout::e_port_type::OUTPUT) ? 0
                                                                                               : 1;

  /* Default mode */
  std::string pb_mode = arcxpress::gen_default_mode_name(ble3_pb);
  num_err += arch_gen.add_pb_type_mode(ble3_pb, pb_mode) ? 0 : 1;

  /* Build child pbs */
  std::string lut_pb = arch_gen.lut_primitive_pb_type_name(4);
  std::string phy_lut_pb = arch_gen.find_primitive_pb_type_name(FRAC_LUT4_BLIF_MODEL);
  num_err += arch_gen.set_pb_type_physical_pb_type(lut_pb, phy_lut_pb) ? 0 : 1;
  num_err += arch_gen.set_pb_type_physical_pb_type_index_factor(lut_pb, "0.5") ? 0 : 1;
  num_err +=
    arch_gen.set_pb_type_physical_port_binding(lut_pb, "in", openfpga::BasicPort("in", 0, 3)) ? 0
                                                                                              : 1;
  num_err += arch_gen.set_pb_type_physical_port_binding(lut_pb, "out",
                                                        openfpga::BasicPort("lut4_out", 0, 0), "1")
               ? 0
               : 1;
  /* TODO: Bind physical ports */
  std::string ff_pb = std::string(OPERATING_BLE4_FF_NAME);
  num_err += add_ff_pb_type(arch_gen, ff_pb, true);

  /* Add child pbs */
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, lut_pb, 1) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, ff_pb, 1) ? 0 : 1;

  /* TODO: Interconnect */
  openfpga::BasicPort des_in_port;
  if (!arch_gen.find_pb_type_port(lut_pb, "in", des_in_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto pin : des_in_port.pins()) {
    openfpga::BasicPort src_pin(des_in_port.get_name(), pin, pin);
    openfpga::BasicPort des_pin(des_in_port.get_name(), pin, pin);
    for (auto ipb : arch_gen.child_pbs(pb_mode, lut_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {lut_pb, ipb, des_pin};
      num_err += arch_gen.add_in2in_edge(pb_mode, src_pin, des_edge_port) ? 0 : 1;
    }
  }
  openfpga::BasicPort des_rst_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "R", des_rst_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_rst_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, rst_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort des_ck_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "C", des_ck_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_ck_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, ck_port, des_edge_port) ? 0 : 1;
  }

  openfpga::BasicPort src_lutout_port;
  if (!arch_gen.find_pb_type_port(lut_pb, "out", src_lutout_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_d_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "D", des_d_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, lut_pb)) {
    for (auto jpb : arch_gen.child_pbs(pb_mode, ff_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {lut_pb, ipb, src_lutout_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, jpb, des_d_port};
      num_err += arch_gen.add_internal_edge(pb_mode, src_edge_port, des_edge_port) ? 0 : 1;
      num_err += arch_gen.set_internal_edge_pack_pattern(pb_mode, src_edge_port, des_edge_port,
                                                         std::string("ble3"))
                   ? 0
                   : 1;
    }
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, lut_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {lut_pb, ipb, src_lutout_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, out_port) ? 0 : 1;
  }

  openfpga::BasicPort src_q_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "Q", src_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, ipb, src_q_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, out_port) ? 0 : 1;
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_ale_pb_type(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string ale_pb = std::string(OPERATING_ALE_NAME);
  VTR_LOGV(verbose, "Adding intermediate pb_type '%s'\n", ale_pb.c_str());
  num_err += arch_gen.add_pb_type(ale_pb) ? 0 : 1;
  openfpga::BasicPort in_port("in", 3);
  openfpga::BasicPort cin_port("cin", 1);
  openfpga::BasicPort cout_port("cout", 1);
  openfpga::BasicPort rst_port("reset", 1);
  openfpga::BasicPort ck_port("clk", 1);
  openfpga::BasicPort out_port("out", 1);
  num_err +=
    arch_gen.add_pb_type_port(ale_pb, in_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ale_pb, cin_port, arcxpress::VprArchLayout::e_port_type::CI) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ale_pb, cout_port, arcxpress::VprArchLayout::e_port_type::CO) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ale_pb, rst_port, arcxpress::VprArchLayout::e_port_type::RESET) ? 0
                                                                                             : 1;
  num_err +=
    arch_gen.add_pb_type_port(ale_pb, ck_port, arcxpress::VprArchLayout::e_port_type::CLOCK) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ale_pb, out_port, arcxpress::VprArchLayout::e_port_type::OUTPUT) ? 0
                                                                                              : 1;

  /* Default mode */
  std::string pb_mode = arcxpress::gen_default_mode_name(ale_pb);
  num_err += arch_gen.add_pb_type_mode(ale_pb, pb_mode) ? 0 : 1;

  /* Build child pbs */
  std::string ff_pb = std::string(OPERATING_ALE_FF_NAME);
  num_err += add_ff_pb_type(arch_gen, ff_pb, true);
  std::string lut_pb = arch_gen.lut_primitive_pb_type_name(4);
  std::string phy_lut_pb = arch_gen.find_primitive_pb_type_name(FRAC_LUT4_BLIF_MODEL);
  num_err += arch_gen.set_pb_type_physical_pb_type(lut_pb, phy_lut_pb) ? 0 : 1;
  /* Bind physical port */
  num_err += arch_gen.set_pb_type_physical_pb_type_index_factor(lut_pb, "0.5") ? 0 : 1;
  num_err +=
    arch_gen.set_pb_type_physical_port_binding(lut_pb, "in", openfpga::BasicPort("in", 0, 3)) ? 0
                                                                                              : 1;
  num_err += arch_gen.set_pb_type_physical_port_binding(lut_pb, "out",
                                                        openfpga::BasicPort("lut3_out", 0, 0), "1")
               ? 0
               : 1;
  std::string adder_pb = arch_gen.find_primitive_pb_type_name(std::string(ADDER_BLIF_MODEL));
  std::string phy_adder_pb = arch_gen.find_primitive_pb_type_name(PHYSICAL_ADDER_BLIF_MODEL);
  num_err += arch_gen.set_pb_type_physical_pb_type(adder_pb, phy_adder_pb) ? 0 : 1;

  /* Add child pbs */
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, lut_pb, 2) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, adder_pb, 1) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, ff_pb, 1) ? 0 : 1;

  /* TODO: Interconnect */
  openfpga::BasicPort des_in_port;
  if (!arch_gen.find_pb_type_port(lut_pb, "in", des_in_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto pin : des_in_port.pins()) {
    openfpga::BasicPort src_pin(des_in_port.get_name(), pin, pin);
    openfpga::BasicPort des_pin(des_in_port.get_name(), pin, pin);
    for (auto ipb : arch_gen.child_pbs(pb_mode, lut_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {lut_pb, ipb, des_pin};
      num_err += arch_gen.add_in2in_edge(pb_mode, src_pin, des_edge_port) ? 0 : 1;
    }
  }
  openfpga::BasicPort des_rst_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "R", des_rst_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_rst_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, rst_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort des_ck_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "C", des_ck_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_ck_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, ck_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort des_cin_port;
  if (!arch_gen.find_pb_type_port(adder_pb, "cin", des_cin_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, adder_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {adder_pb, ipb, des_cin_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, cin_port, des_edge_port) ? 0 : 1;
    num_err +=
      arch_gen.set_in2in_edge_pack_pattern(pb_mode, cin_port, des_edge_port, std::string("chain"))
        ? 0
        : 1;
  }

  openfpga::BasicPort src_lutout_port;
  if (!arch_gen.find_pb_type_port(lut_pb, "out", src_lutout_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_a_port;
  if (!arch_gen.find_pb_type_port(adder_pb, "a", des_a_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_b_port;
  if (!arch_gen.find_pb_type_port(adder_pb, "b", des_b_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  {
    arcxpress::ArchGen::EdgePort src_edge_port = {lut_pb, 0, src_lutout_port};
    arcxpress::ArchGen::EdgePort des_edge_port = {adder_pb, 0, des_a_port};
    num_err += arch_gen.add_internal_edge(pb_mode, src_edge_port, des_edge_port) ? 0 : 1;
  }
  {
    arcxpress::ArchGen::EdgePort src_edge_port = {lut_pb, 1, src_lutout_port};
    arcxpress::ArchGen::EdgePort des_edge_port = {adder_pb, 0, des_b_port};
    num_err += arch_gen.add_internal_edge(pb_mode, src_edge_port, des_edge_port) ? 0 : 1;
  }

  openfpga::BasicPort src_sumout_port;
  if (!arch_gen.find_pb_type_port(adder_pb, "sumout", src_sumout_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_d_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "D", des_d_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, adder_pb)) {
    for (auto jpb : arch_gen.child_pbs(pb_mode, ff_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {adder_pb, ipb, src_sumout_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, jpb, des_d_port};
      num_err += arch_gen.add_internal_edge(pb_mode, src_edge_port, des_edge_port) ? 0 : 1;
      num_err += arch_gen.set_internal_edge_pack_pattern(pb_mode, src_edge_port, des_edge_port,
                                                         std::string("arith"))
                   ? 0
                   : 1;
    }
  }

  openfpga::BasicPort src_cout_port;
  if (!arch_gen.find_pb_type_port(adder_pb, "cout", src_cout_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, adder_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {adder_pb, ipb, src_cout_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, cout_port) ? 0 : 1;
    num_err += arch_gen.set_out2out_edge_pack_pattern(pb_mode, src_edge_port, cout_port,
                                                      std::string("chain"))
                 ? 0
                 : 1;
  }

  openfpga::BasicPort src_q_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "Q", src_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, ipb, src_q_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, out_port) ? 0 : 1;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, adder_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {adder_pb, ipb, src_sumout_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, out_port) ? 0 : 1;
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_lut3inter_pb_type(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string top_pb = std::string(OPERATING_LUT3INTER_NAME);
  VTR_LOGV(verbose, "Adding intermediate pb_type '%s'\n", top_pb.c_str());
  num_err += arch_gen.add_pb_type(top_pb) ? 0 : 1;
  openfpga::BasicPort in_port("in", 3);
  openfpga::BasicPort rst_port("reset", 1);
  openfpga::BasicPort ck_port("clk", 1);
  openfpga::BasicPort out_port("out", 2);
  num_err +=
    arch_gen.add_pb_type_port(top_pb, in_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(top_pb, rst_port, arcxpress::VprArchLayout::e_port_type::RESET) ? 0
                                                                                             : 1;
  num_err +=
    arch_gen.add_pb_type_port(top_pb, ck_port, arcxpress::VprArchLayout::e_port_type::CLOCK) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(top_pb, out_port, arcxpress::VprArchLayout::e_port_type::OUTPUT) ? 0
                                                                                              : 1;

  /* Default mode */
  std::string pb_mode = arcxpress::gen_default_mode_name(top_pb);
  num_err += arch_gen.add_pb_type_mode(top_pb, pb_mode) ? 0 : 1;

  /* Add child pbs */
  std::string ble3_pb = std::string(OPERATING_BLE3_NAME);
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, ble3_pb, 2) ? 0 : 1;

  /* Interconnect */
  openfpga::BasicPort des_in_port;
  if (!arch_gen.find_pb_type_port(ble3_pb, "in", des_in_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto pin : des_in_port.pins()) {
    openfpga::BasicPort src_pin(des_in_port.get_name(), pin, pin);
    openfpga::BasicPort des_pin(des_in_port.get_name(), pin, pin);
    for (auto ipb : arch_gen.child_pbs(pb_mode, ble3_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {ble3_pb, ipb, des_pin};
      num_err += arch_gen.add_in2in_edge(pb_mode, src_pin, des_edge_port) ? 0 : 1;
    }
  }
  openfpga::BasicPort des_rst_port;
  if (!arch_gen.find_pb_type_port(ble3_pb, "reset", des_rst_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ble3_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ble3_pb, ipb, des_rst_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, rst_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort des_ck_port;
  if (!arch_gen.find_pb_type_port(ble3_pb, "clk", des_ck_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ble3_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ble3_pb, ipb, des_ck_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, ck_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort src_out_port;
  if (!arch_gen.find_pb_type_port(ble3_pb, "out", src_out_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ble3_pb)) {
    openfpga::BasicPort des_out_pin(src_out_port.get_name(), ipb, ipb);
    arcxpress::ArchGen::EdgePort src_edge_port = {ble3_pb, ipb, src_out_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, des_out_pin) ? 0 : 1;
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_ble4_pb_type(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string ble4_pb = std::string(OPERATING_BLE4_NAME);
  VTR_LOGV(verbose, "Adding intermediate pb_type '%s'\n", ble4_pb.c_str());
  num_err += arch_gen.add_pb_type(ble4_pb) ? 0 : 1;
  openfpga::BasicPort in_port("in", 4);
  openfpga::BasicPort rst_port("reset", 1);
  openfpga::BasicPort ck_port("clk", 1);
  openfpga::BasicPort out_port("out", 1);
  num_err +=
    arch_gen.add_pb_type_port(ble4_pb, in_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0
                                                                                             : 1;
  num_err +=
    arch_gen.add_pb_type_port(ble4_pb, rst_port, arcxpress::VprArchLayout::e_port_type::RESET) ? 0
                                                                                              : 1;
  num_err +=
    arch_gen.add_pb_type_port(ble4_pb, ck_port, arcxpress::VprArchLayout::e_port_type::CLOCK) ? 0
                                                                                             : 1;
  num_err +=
    arch_gen.add_pb_type_port(ble4_pb, out_port, arcxpress::VprArchLayout::e_port_type::OUTPUT) ? 0
                                                                                               : 1;

  /* Default mode */
  std::string pb_mode = arcxpress::gen_default_mode_name(ble4_pb);
  num_err += arch_gen.add_pb_type_mode(ble4_pb, pb_mode) ? 0 : 1;

  /* Build child pbs */
  std::string ff_pb = std::string(OPERATING_BLE4_FF_NAME);
  num_err += add_ff_pb_type(arch_gen, ff_pb, true);
  std::string lut_pb = arch_gen.lut_primitive_pb_type_name(4);
  std::string phy_lut_pb = arch_gen.find_primitive_pb_type_name(FRAC_LUT4_BLIF_MODEL);
  num_err += arch_gen.set_pb_type_physical_pb_type(lut_pb, phy_lut_pb) ? 0 : 1;
  /* TODO: Bind physical ports */
  num_err +=
    arch_gen.set_pb_type_physical_port_binding(lut_pb, "in", openfpga::BasicPort("in", 0, 4)) ? 0
                                                                                              : 1;
  num_err +=
    arch_gen.set_pb_type_physical_port_binding(lut_pb, "out", openfpga::BasicPort("lut4_out", 0, 0))
      ? 0
      : 1;

  /* Add child pbs */
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, lut_pb, 1) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, ff_pb, 1) ? 0 : 1;

  /* Interconnect */
  openfpga::BasicPort des_in_port;
  if (!arch_gen.find_pb_type_port(lut_pb, "in", des_in_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto pin : des_in_port.pins()) {
    openfpga::BasicPort src_pin(des_in_port.get_name(), pin, pin);
    openfpga::BasicPort des_pin(des_in_port.get_name(), pin, pin);
    for (auto ipb : arch_gen.child_pbs(pb_mode, lut_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {lut_pb, ipb, des_pin};
      num_err += arch_gen.add_in2in_edge(pb_mode, src_pin, des_edge_port) ? 0 : 1;
    }
  }
  openfpga::BasicPort des_rst_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "R", des_rst_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_rst_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, rst_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort des_ck_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "C", des_ck_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_ck_port};
    num_err += arch_gen.add_in2in_edge(pb_mode, ck_port, des_edge_port) ? 0 : 1;
  }

  openfpga::BasicPort src_lutout_port;
  if (!arch_gen.find_pb_type_port(lut_pb, "out", src_lutout_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_d_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "D", des_d_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, lut_pb)) {
    for (auto jpb : arch_gen.child_pbs(pb_mode, ff_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {lut_pb, ipb, src_lutout_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, jpb, des_d_port};
      num_err += arch_gen.add_internal_edge(pb_mode, src_edge_port, des_edge_port) ? 0 : 1;
      num_err += arch_gen.set_internal_edge_pack_pattern(pb_mode, src_edge_port, des_edge_port,
                                                         std::string("ble4"))
                   ? 0
                   : 1;
    }
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, lut_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {lut_pb, ipb, src_lutout_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, out_port) ? 0 : 1;
  }

  openfpga::BasicPort src_q_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "Q", src_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(pb_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, ipb, src_q_port};
    num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, out_port) ? 0 : 1;
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_fle_complex_block_physical_mode(arcxpress::ArchGen& arch_gen,
                                                    const std::string& fle_pb) {
  int num_err = 0;

  /* physical modes and its child pb_type */
  std::string pb_mode = arcxpress::gen_physical_mode_name(fle_pb);
  num_err += arch_gen.add_pb_type_mode(fle_pb, pb_mode, true) ? 0 : 1;

  std::string fabric_pb = std::string(OPERATING_FABRIC_NAME);
  num_err += arch_gen.copy_pb_type(fabric_pb, fle_pb) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, fabric_pb, 1) ? 0 : 1;
  std::string fabric_mode = arcxpress::gen_default_mode_name(fabric_pb);
  num_err += arch_gen.add_pb_type_mode(fabric_pb, fabric_mode, true) ? 0 : 1;
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;

  /* Build child logic */
  num_err += add_frac_logic_pb_type(arch_gen, true);

  /* Child pbs */
  std::string frac_logic_pb = std::string(PHYSICAL_FRAC_LOGIC_NAME);
  num_err += arch_gen.add_pb_type_to_mode(fabric_mode, frac_logic_pb, 1) ? 0 : 1;
  std::string ff_pb = arch_gen.find_primitive_pb_type_name(std::string(PHYSICAL_REGULAR_SCFF_NAME));
  num_err += arch_gen.add_pb_type_to_mode(fabric_mode, ff_pb, 2) ? 0 : 1;
  FracffGen phy_fff_gen(PHYSICAL_REGULAR_SCFF_NAME);
  num_err += arch_gen.set_pb_type_circuit_model(ff_pb, phy_fff_gen.circuit_model_name()) ? 0 : 1;
  std::string adder_pb = arch_gen.find_primitive_pb_type_name((PHYSICAL_ADDER_BLIF_MODEL));
  num_err += arch_gen.add_pb_type_to_mode(fabric_mode, adder_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_circuit_model(adder_pb, PHYSICAL_ADDER_CIRCUIT_MODEL) ? 0 : 1;
  /* TODO: Interconnect */
  openfpga::BasicPort src_in_port("in", 4);
  openfpga::BasicPort des_in_port;
  if (!arch_gen.find_pb_type_port(frac_logic_pb, "in", des_in_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto pin : des_in_port.pins()) {
    openfpga::BasicPort src_pin(des_in_port.get_name(), pin, pin);
    openfpga::BasicPort des_pin(des_in_port.get_name(), pin, pin);
    for (auto ipb : arch_gen.child_pbs(fabric_mode, frac_logic_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {frac_logic_pb, ipb, des_pin};
      num_err += arch_gen.add_in2in_edge(fabric_mode, src_pin, des_edge_port) ? 0 : 1;
    }
  }
  openfpga::BasicPort src_cin_port("cin", 1);
  openfpga::BasicPort des_cin_port;
  if (!arch_gen.find_pb_type_port(adder_pb, "cin", des_cin_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(fabric_mode, adder_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {adder_pb, ipb, des_cin_port};
    num_err += arch_gen.add_in2in_edge(fabric_mode, src_cin_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort des_cout_port("cout", 1);
  openfpga::BasicPort src_cout_port;
  if (!arch_gen.find_pb_type_port(adder_pb, "cout", src_cout_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(fabric_mode, adder_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {adder_pb, ipb, src_cout_port};
    num_err += arch_gen.add_out2out_edge(fabric_mode, src_edge_port, des_cout_port) ? 0 : 1;
  }
  openfpga::BasicPort src_ck_port("clk", 1);
  openfpga::BasicPort des_ck_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "C", des_ck_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(fabric_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_ck_port};
    num_err += arch_gen.add_in2in_edge(fabric_mode, src_ck_port, des_edge_port) ? 0 : 1;
  }
  openfpga::BasicPort src_rst_port("reset", 1);
  openfpga::BasicPort des_rst_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "R", des_rst_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(fabric_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_rst_port};
    num_err += arch_gen.add_in2in_edge(fabric_mode, src_rst_port, des_edge_port) ? 0 : 1;
  }
  {
    openfpga::BasicPort src_out_port("out", 0, 0);
    openfpga::BasicPort des_a_port("a", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_logic_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgePort des_edge_port = {adder_pb, 0, des_a_port};
    num_err += arch_gen.add_internal_edge(fabric_mode, src_edge_port, des_edge_port) ? 0 : 1;
  }
  {
    openfpga::BasicPort src_out_port("out", 1, 1);
    openfpga::BasicPort des_b_port("b", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_logic_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgePort des_edge_port = {adder_pb, 0, des_b_port};
    num_err += arch_gen.add_internal_edge(fabric_mode, src_edge_port, des_edge_port) ? 0 : 1;
  }
  {
    openfpga::BasicPort src_out_port("out", 0, 0);
    openfpga::BasicPort des_d_port("D", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_logic_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, 0, des_d_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${LUT_OUT0_TO_FF_D_DELAY}", "0"};
    num_err +=
      arch_gen.add_internal_edge(fabric_mode, src_edge_port, des_edge_port, edge_delay) ? 0 : 1;
  }
  {
    openfpga::BasicPort src_out_port("out", 1, 1);
    openfpga::BasicPort des_d_port("D", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_logic_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, 1, des_d_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${LUT_OUT1_TO_FF_D_DELAY}", "0"};
    num_err +=
      arch_gen.add_internal_edge(fabric_mode, src_edge_port, des_edge_port, edge_delay) ? 0 : 1;
  }
  {
    openfpga::BasicPort src_out_port("sumout", 0, 0);
    openfpga::BasicPort des_d_port("D", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {adder_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, 1, des_d_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${LUT_OUT1_TO_FF_D_DELAY}", "0"};
    num_err +=
      arch_gen.add_internal_edge(fabric_mode, src_edge_port, des_edge_port, edge_delay) ? 0 : 1;
  }
  {
    openfpga::BasicPort des_out_port("out", 0, 0);
    openfpga::BasicPort src_q_port("Q", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, 0, src_q_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${FF0_Q_TO_FLE_OUT_DELAY}", "0"};
    num_err += arch_gen.add_out2out_edge(fabric_mode, src_edge_port, des_out_port,
                                         PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                 ? 0
                 : 1;
  }
  {
    openfpga::BasicPort des_out_port("out", 0, 0);
    openfpga::BasicPort src_out_port("out", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_logic_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${LUT_OUT0_TO_FLE_OUT_DELAY}", "0"};
    num_err += arch_gen.add_out2out_edge(fabric_mode, src_edge_port, des_out_port,
                                         PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                 ? 0
                 : 1;
  }
  {
    openfpga::BasicPort des_out_port("out", 1, 1);
    openfpga::BasicPort src_q_port("Q", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, 1, src_q_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${FF1_Q_TO_FLE_OUT_DELAY}", "0"};
    num_err += arch_gen.add_out2out_edge(fabric_mode, src_edge_port, des_out_port,
                                         PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                 ? 0
                 : 1;
  }
  {
    openfpga::BasicPort des_out_port("out", 1, 1);
    openfpga::BasicPort src_out_port("sumout", 0, 0);
    arcxpress::ArchGen::EdgePort src_edge_port = {adder_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${LUT_OUT1_TO_FLE_OUT_DELAY}", "0"};
    num_err += arch_gen.add_out2out_edge(fabric_mode, src_edge_port, des_out_port,
                                         PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                 ? 0
                 : 1;
  }
  {
    openfpga::BasicPort des_out_port("out", 1, 1);
    openfpga::BasicPort src_out_port("out", 1, 1);
    arcxpress::ArchGen::EdgePort src_edge_port = {frac_logic_pb, 0, src_out_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${LUT_OUT1_TO_FLE_OUT_DELAY}", "0"};
    num_err += arch_gen.add_out2out_edge(fabric_mode, src_edge_port, des_out_port,
                                         PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                 ? 0
                 : 1;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_fle_complex_block_operating_mode_n2_lut3(arcxpress::ArchGen& arch_gen,
                                                             const std::string& fle_pb) {
  int num_err = 0;

  /* build child pb_types */
  num_err += add_ble3_pb_type(arch_gen, true);
  num_err += add_lut3inter_pb_type(arch_gen, true);

  /* physical modes and its child pb_type */
  std::string lut3inter_pb = std::string(OPERATING_LUT3INTER_NAME);
  std::string pb_mode = arcxpress::gen_pb_mode_name(fle_pb, std::string("n2_lut3"));
  num_err += arch_gen.add_pb_type_mode(fle_pb, pb_mode) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, lut3inter_pb, 1) ? 0 : 1;
  /* Interconnect */
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_fle_complex_block_operating_mode_n1_lut4(arcxpress::ArchGen& arch_gen,
                                                             const std::string& fle_pb) {
  int num_err = 0;

  /* Build child pb type */
  num_err += add_ble4_pb_type(arch_gen, true);
  /* physical modes and its child pb_type */
  std::string pb_mode = arcxpress::gen_pb_mode_name(fle_pb, std::string("n1_lut4"));
  num_err += arch_gen.add_pb_type_mode(fle_pb, pb_mode) ? 0 : 1;
  std::string ble4_pb = std::string(OPERATING_BLE4_NAME);
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, ble4_pb, 1) ? 0 : 1;
  /* Interconnect */
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_fle_complex_block_operating_mode_arith(arcxpress::ArchGen& arch_gen,
                                                           const std::string& fle_pb) {
  int num_err = 0;

  /* Build child pb_type */
  num_err += add_ale_pb_type(arch_gen, true);
  /* physical modes and its child pb_type */
  std::string pb_mode = arcxpress::gen_pb_mode_name(fle_pb, std::string("arith"));
  num_err += arch_gen.add_pb_type_mode(fle_pb, pb_mode) ? 0 : 1;
  std::string ale_pb = std::string(OPERATING_ALE_NAME);
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, ale_pb, 1) ? 0 : 1;
  /* Interconnect */
  /* Add pack pattern */
  std::vector<std::string> regular_inputs = {"in", "clk", "reset"};
  for (std::string ale_pin : regular_inputs) {
    openfpga::BasicPort des_in_port;
    if (!arch_gen.find_pb_type_port(ale_pb, ale_pin, des_in_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    for (auto pin : des_in_port.pins()) {
      openfpga::BasicPort src_pin(des_in_port.get_name(), pin, pin);
      openfpga::BasicPort des_pin(des_in_port.get_name(), pin, pin);
      for (auto ipb : arch_gen.child_pbs(pb_mode, ale_pb)) {
        arcxpress::ArchGen::EdgePort des_edge_port = {ale_pb, ipb, des_pin};
        num_err += arch_gen.add_in2in_edge(pb_mode, src_pin, des_edge_port) ? 0 : 1;
      }
    }
  }
  {
    openfpga::BasicPort des_cin_port;
    if (!arch_gen.find_pb_type_port(ale_pb, "cin", des_cin_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    for (auto ipb : arch_gen.child_pbs(pb_mode, ale_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {ale_pb, ipb, des_cin_port};
      num_err += arch_gen.add_in2in_edge(pb_mode, des_cin_port, des_edge_port) ? 0 : 1;
      // TODO: cin_chain should be generated from a function
      num_err += arch_gen.set_in2in_edge_pack_pattern(pb_mode, des_cin_port, des_edge_port,
                                                      std::string("cin_chain"))
                   ? 0
                   : 1;
    }
  }
  {
    openfpga::BasicPort src_cout_port;
    if (!arch_gen.find_pb_type_port(ale_pb, "cout", src_cout_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    for (auto ipb : arch_gen.child_pbs(pb_mode, ale_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {ale_pb, ipb, src_cout_port};
      num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, src_cout_port) ? 0 : 1;
      // TODO: cin_chain should be generated from a function
      num_err += arch_gen.set_out2out_edge_pack_pattern(pb_mode, src_edge_port, src_cout_port,
                                                        std::string("cin_chain"))
                   ? 0
                   : 1;
    }
  }
  {
    openfpga::BasicPort src_out_port;
    if (!arch_gen.find_pb_type_port(ale_pb, "out", src_out_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    openfpga::BasicPort des_out_port(src_out_port.get_name(), 1, 1);

    for (auto ipb : arch_gen.child_pbs(pb_mode, ale_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {ale_pb, ipb, src_out_port};
      num_err += arch_gen.add_out2out_edge(pb_mode, src_edge_port, des_out_port) ? 0 : 1;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int assemble_clb_complex_block(arcxpress::ArchGen& arch_gen,
                               const ArchPlanner& arch_planner,
                               const bool& verbose) {
  int num_err = 0;

  std::string top_pb = "clb";
  VTR_LOGV(verbose, "Assemble top-level pb_type '%s'\n", top_pb.c_str());

  std::string clb_mode = arcxpress::gen_default_mode_name(top_pb);
  num_err += arch_gen.add_pb_type_mode(top_pb, clb_mode, true) ? 0 : 1;
  std::string fle_pb = std::string(OPERATING_FLE_NAME);
  num_err += arch_gen.add_pb_type(fle_pb) ? 0 : 1;
  size_t num_fle = arch_planner.num_fle_per_clb();
  num_err += arch_gen.add_pb_type_to_mode(clb_mode, fle_pb, num_fle) ? 0 : 1;
  openfpga::BasicPort in_port("in", 4);
  openfpga::BasicPort rst_port("reset", 1);
  openfpga::BasicPort cin_port("cin", 1);
  openfpga::BasicPort reg_in_port("reg_in", 1);
  openfpga::BasicPort sc_in_port("sc_in", 1);
  openfpga::BasicPort ck_port("clk", 1);
  openfpga::BasicPort out_port("out", 2);
  openfpga::BasicPort cout_port("cout", 1);
  openfpga::BasicPort reg_out_port("reg_out", 1);
  openfpga::BasicPort sc_out_port("sc_out", 1);
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, in_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, rst_port, arcxpress::VprArchLayout::e_port_type::RESET) ? 0
                                                                                             : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, cin_port, arcxpress::VprArchLayout::e_port_type::CI) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, reg_in_port, arcxpress::VprArchLayout::e_port_type::CI) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, sc_in_port, arcxpress::VprArchLayout::e_port_type::CI) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, ck_port, arcxpress::VprArchLayout::e_port_type::CLOCK) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, cout_port, arcxpress::VprArchLayout::e_port_type::CO) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, reg_out_port, arcxpress::VprArchLayout::e_port_type::CO) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, sc_out_port, arcxpress::VprArchLayout::e_port_type::CO) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(fle_pb, out_port, arcxpress::VprArchLayout::e_port_type::OUTPUT) ? 0
                                                                                              : 1;

  num_err += assemble_fle_complex_block_physical_mode(arch_gen, fle_pb);
  num_err += assemble_fle_complex_block_operating_mode_n2_lut3(arch_gen, fle_pb);
  num_err += assemble_fle_complex_block_operating_mode_n1_lut4(arch_gen, fle_pb);
  num_err += assemble_fle_complex_block_operating_mode_arith(arch_gen, fle_pb);

  /* Interconnect fle to clb */
  for (size_t ifle = 0; ifle < num_fle; ++ifle) {
    std::string I_name = std::string("I") + std::to_string(ifle);
    openfpga::BasicPort src_in_port(I_name, 4);
    for (size_t pin : src_in_port.pins()) {
      openfpga::BasicPort src_I_port(I_name, pin, pin);
      if (pin == 3) {
        I_name = std::string("Ix");
        src_I_port.set_name(I_name);
        src_I_port.set_lsb(ifle);
        src_I_port.set_msb(ifle);
      }
      openfpga::BasicPort des_in_port("in", pin, pin);
      arcxpress::ArchGen::EdgePort des_edge_port = {fle_pb, ifle, des_in_port};
      num_err += arch_gen.add_in2in_edge(clb_mode, src_I_port, des_edge_port) ? 0 : 1;
    }
  }
  for (size_t ifle = 0; ifle < num_fle; ++ifle) {
    openfpga::BasicPort src_port("clk", 0, 0);
    openfpga::BasicPort des_port("clk", 0, 0);
    arcxpress::ArchGen::EdgePort des_edge_port = {fle_pb, ifle, des_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${CLB_CLK_MUX_DELAY}", "0"};
    num_err += arch_gen.add_in2in_edge(clb_mode, src_port, des_edge_port, edge_delay) ? 0 : 1;
  }
  for (size_t ifle = 0; ifle < num_fle; ++ifle) {
    openfpga::BasicPort src_port("reset", 0, 0);
    openfpga::BasicPort des_port("reset", 0, 0);
    arcxpress::ArchGen::EdgePort des_edge_port = {fle_pb, ifle, des_port};
    num_err += arch_gen.add_in2in_edge(clb_mode, src_port, des_edge_port) ? 0 : 1;
  }
  for (size_t ifle = 0; ifle < num_fle; ++ifle) {
    openfpga::BasicPort src_out_port("out", 2);
    for (size_t pin : src_out_port.pins()) {
      openfpga::BasicPort src_port("out", pin, pin);
      size_t o_pin_idx = ifle * 2 + pin;
      openfpga::BasicPort des_o_port("O", o_pin_idx, o_pin_idx);
      arcxpress::ArchGen::EdgePort src_edge_port = {fle_pb, ifle, src_port};
      num_err += arch_gen.add_out2out_edge(clb_mode, src_edge_port, des_o_port) ? 0 : 1;
    }
  }
  /* Carry chain, shift register chain and scan-chain */
  std::map<std::string, std::string> chain_port_pairs = {
    {"cin", "cout"},
    {"reg_in", "reg_out"},
    {"sc_in", "sc_out"}
  };
  for (const auto& [iport, oport] : chain_port_pairs) {
    std::string pack_pattern_name = iport + std::string("chain");
    {
      openfpga::BasicPort src_port(iport, 0, 0);
      openfpga::BasicPort des_port(iport, 0, 0);
      arcxpress::ArchGen::EdgePort des_edge_port = {fle_pb, 0, des_port};
      num_err += arch_gen.add_in2in_edge(clb_mode, src_port, des_edge_port) ? 0 : 1;
      num_err +=
        arch_gen.set_in2in_edge_pack_pattern(clb_mode, src_port, des_edge_port, pack_pattern_name)
          ? 0
          : 1;
    }
    for (size_t ifle = 0; ifle < num_fle - 1; ++ifle) {
      openfpga::BasicPort src_port(oport, 0, 0);
      openfpga::BasicPort des_port(iport, 0, 0);
      arcxpress::ArchGen::EdgePort src_edge_port = {fle_pb, ifle, src_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {fle_pb, ifle + 1, des_port};
      num_err += arch_gen.add_internal_edge(clb_mode, src_edge_port, des_edge_port) ? 0 : 1;
      num_err += arch_gen.set_internal_edge_pack_pattern(clb_mode, src_edge_port, des_edge_port,
                                                         pack_pattern_name)
                   ? 0
                   : 1;
    }
    {
      openfpga::BasicPort src_port(oport, 0, 0);
      openfpga::BasicPort des_port(oport, 0, 0);
      arcxpress::ArchGen::EdgePort src_edge_port = {fle_pb, num_fle - 1, src_port};
      num_err += arch_gen.add_out2out_edge(clb_mode, src_edge_port, des_port) ? 0 : 1;
      num_err += arch_gen.set_out2out_edge_pack_pattern(clb_mode, src_edge_port, des_port,
                                                        pack_pattern_name)
                   ? 0
                   : 1;
    }
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace MOSOFA
