/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "command_exit_codes.h"
#include "mosofa_complex_block_ff.h"
#include "mosofa_complex_block_pi.h"
#include "mosofa_constants.h"
#include "mosofa_naming.h"
#include "fracff_gen.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA {

static int add_pi_ff_pb_type(arcxpress::ArchGen& arch_gen, const std::string& ff_pb,
                             const bool& verbose) {
  int num_err = 0;

  VTR_LOGV(verbose, "Adding intermediate pb_type '%s'\n", ff_pb.c_str());
  num_err += arch_gen.add_pb_type(ff_pb) ? 0 : 1;
  // TODO: Double check the port name to match the actual pff ports
  openfpga::BasicPort d_port("D", 1);
  openfpga::BasicPort ff_en_port("ff_en", 1);
  openfpga::BasicPort clr_port("R", 1);
  openfpga::BasicPort q_port("Q", 1);
  openfpga::BasicPort ck_port("C", 1);
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, d_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, ff_en_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, clr_port, arcxpress::VprArchLayout::e_port_type::RESET) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, q_port, arcxpress::VprArchLayout::e_port_type::OUTPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, ck_port, arcxpress::VprArchLayout::e_port_type::CLOCK) ? 0 : 1;

  FracffGen fff_gen(OP2PHY_PB_TYPE_MAP.at(ff_pb.c_str()));
  std::string tag = fff_gen.vpr_physical_model_name();
  std::string phy_pb_type_name = arch_gen.find_primitive_pb_type_name(tag);

  { /* Latch */
    std::string pb_mode =
      arcxpress::gen_pb_mode_name(ff_pb, std::string(OPERATING_BUILTIN_LATCH_NAME));
    num_err += arch_gen.add_pb_type_mode(ff_pb, pb_mode) ? 0 : 1;
    std::string latch_pb = gen_latch_pb_name(ff_pb);
    num_err += arch_gen.add_pb_type_to_mode(pb_mode, latch_pb, 1) ? 0 : 1;
    num_err += wire_ff_pb_type_mode(arch_gen, pb_mode, latch_pb, true, false);
    /* Set binding to the its physical pb_type */
    num_err += bind_ff_pb_type_to_physical(arch_gen, ff_pb, latch_pb, phy_pb_type_name, verbose);
  }

  /* Add fracturable ff primitive */
  for (auto pmtv : fff_gen.vpr_arch_primitives(false)) {
    std::string pb_mode = arcxpress::gen_pb_mode_name(ff_pb, pmtv.name());
    num_err += arch_gen.add_pb_type_mode(ff_pb, pb_mode) ? 0 : 1;
    num_err += arch_gen.add_pb_type_to_mode(pb_mode, pmtv.pb_type_name(tag), 1) ? 0 : 1;
    num_err += wire_ff_pb_type_mode(arch_gen, pb_mode, pmtv.pb_type_name(tag), true, false);
    /* Set binding to the its physical pb_type */
    num_err += bind_ff_pb_type_to_physical(arch_gen, ff_pb, pmtv.pb_type_name(tag),
                                           phy_pb_type_name, verbose);
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_pi_complex_block_physical_mode(arcxpress::ArchGen& arch_gen,
                                                   const std::string& top_pb,
                                                   const std::string& phy_pb_name,
                                                   const PhyPbInfo& pad_pb_name,
                                                   const PhyPbInfo& ff_pb_name,
                                                   const bool& verbose) {
  int num_err = 0;

  VTR_LOGV(verbose, "Assemble physical mode for PO '%s'...\n", top_pb.c_str());

  /* physical modes and its child pb_type */
  std::string pb_mode = arcxpress::gen_physical_mode_name(top_pb);
  num_err += arch_gen.add_pb_type_mode(top_pb, pb_mode, true) ? 0 : 1;
  std::string phy_pb = phy_pb_name;
  num_err += arch_gen.copy_pb_type(phy_pb, top_pb) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, phy_pb, 1) ? 0 : 1;
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;

  /* Physical mode detailed pb_type tree */
  std::string phy_pb_default_mode = arcxpress::gen_default_mode_name(phy_pb);
  num_err += arch_gen.add_pb_type_mode(phy_pb, phy_pb_default_mode) ? 0 : 1;
  std::string pad_pb = std::get<0>(pad_pb_name);
  num_err += arch_gen.add_pb_type_to_mode(phy_pb_default_mode, pad_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_circuit_model(pad_pb, std::get<1>(pad_pb_name)) ? 0 : 1;
  std::string ff_pb = std::get<0>(ff_pb_name);
  num_err += arch_gen.add_pb_type_to_mode(phy_pb_default_mode, ff_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_circuit_model(ff_pb, std::get<1>(ff_pb_name)) ? 0 : 1;

  openfpga::BasicPort src_inpad_port;
  if (!arch_gen.find_pb_type_port(pad_pb, "inpad", src_inpad_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_a2f_port;
  if (!arch_gen.find_pb_type_port(phy_pb, "a2f_o", des_a2f_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(phy_pb_default_mode, pad_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {pad_pb, ipb, src_inpad_port};
    num_err += arch_gen.add_out2out_edge(phy_pb_default_mode, src_edge_port, des_a2f_port) ? 0 : 1;
  }
  /* Interconnect */
  num_err +=
    arch_gen.auto_wire_control_signals_pb_type_mode_child(phy_pb_default_mode, ff_pb) ? 0 : 1;
  /* Data port */
  std::map<std::string, std::string> data_port_map;
  data_port_map["reset"] = "R";
  for (auto pair : data_port_map) {
    openfpga::BasicPort src_d_port;
    if (!arch_gen.find_pb_type_port(phy_pb, pair.first, src_d_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    openfpga::BasicPort des_d_port;
    if (!arch_gen.find_pb_type_port(ff_pb, pair.second, des_d_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    for (auto ipb : arch_gen.child_pbs(phy_pb_default_mode, ff_pb)) {
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_d_port};
      num_err += arch_gen.add_in2in_edge(phy_pb_default_mode, src_d_port, des_edge_port) ? 0 : 1;
    }
  }
  /* Output mux */
  openfpga::BasicPort src_q_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "Q", src_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(phy_pb_default_mode, pad_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, ipb, src_q_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"0.429e-9", "0.166e-9"};
    num_err += arch_gen.add_out2out_edge(phy_pb_default_mode, src_edge_port, des_a2f_port,
                                         PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                 ? 0
                 : 1;
  }
  openfpga::BasicPort des_d_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "D", des_d_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(phy_pb_default_mode, ff_pb)) {
    for (auto jpb : arch_gen.child_pbs(phy_pb_default_mode, pad_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {pad_pb, jpb, src_inpad_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_d_port};
      num_err +=
        arch_gen.add_internal_edge(phy_pb_default_mode, src_edge_port, des_edge_port) ? 0 : 1;
    }
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_pi_complex_block_operating_modes(
  arcxpress::ArchGen& arch_gen, const std::string& top_pb, const std::string& op_pb_name,
  const std::string& op_pad_name, const std::string& op_ff_name, const std::string& phy_pad_name, const bool& verbose) {
  int num_err = 0;

  VTR_LOGV(verbose, "Assemble operating mode for PI '%s'...\n", top_pb.c_str());

  /* Operating mode */
  std::string pb_mode = op_pb_name;
  num_err += arch_gen.add_pb_type_mode(top_pb, pb_mode) ? 0 : 1;
  std::string op_pb = op_pb_name;
  num_err += arch_gen.copy_pb_type(op_pb, top_pb) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, op_pb, 1) ? 0 : 1;
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;

  /* Build child pb type */
  std::string ff_pb = op_ff_name;
  num_err += add_pi_ff_pb_type(arch_gen, ff_pb, verbose);

  /* Operating mode detailed pb_type tree */
  std::string op_pb_default_mode = arcxpress::gen_default_mode_name(op_pb);
  num_err += arch_gen.add_pb_type_mode(op_pb, op_pb_default_mode) ? 0 : 1;
  std::string pad_pb = op_pad_name;
  num_err += arch_gen.add_pb_type_to_mode(op_pb_default_mode, pad_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_physical_pb_type(pad_pb, phy_pad_name) ? 0 : 1;

  /* Interconnect */
  num_err +=
    arch_gen.auto_wire_control_signals_pb_type_mode_child(op_pb_default_mode, ff_pb) ? 0 : 1;
  /* Data port */
  openfpga::BasicPort src_inpad_port;
  if (!arch_gen.find_pb_type_port(pad_pb, "inpad", src_inpad_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_a2f_port;
  if (!arch_gen.find_pb_type_port(op_pb, "a2f_o", des_a2f_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(op_pb_default_mode, pad_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {pad_pb, ipb, src_inpad_port};
    num_err += arch_gen.add_out2out_edge(op_pb_default_mode, src_edge_port, des_a2f_port) ? 0 : 1;
  }
  /* Output mux */
  openfpga::BasicPort src_q_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "Q", src_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(op_pb_default_mode, pad_pb)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, ipb, src_q_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"0.429e-9", "0.166e-9"};
    num_err +=
      arch_gen.add_out2out_edge(op_pb_default_mode, src_edge_port, des_a2f_port, edge_delay) ? 0 : 1;
  }
  openfpga::BasicPort des_d_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "D", des_d_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(op_pb_default_mode, ff_pb)) {
    for (auto jpb : arch_gen.child_pbs(op_pb_default_mode, pad_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {pad_pb, jpb, src_inpad_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_d_port};
      num_err +=
        arch_gen.add_internal_edge(op_pb_default_mode, src_edge_port, des_edge_port) ? 0 : 1;
      /* Add pack pattern for latches */
      {
        std::string latch_pb = gen_latch_pb_name(ff_pb);
        num_err += arch_gen.set_internal_edge_pack_pattern(
                     op_pb_default_mode, src_edge_port, des_edge_port,
                     arcxpress::gen_ioff_pack_pattern_name(latch_pb, "D"))
                     ? 0
                     : 1;
      }

      FracffGen fff_gen(OP2PHY_PB_TYPE_MAP.at(ff_pb.c_str()));
      for (auto pmtv : fff_gen.vpr_arch_primitives(false)) {
        num_err += arch_gen.set_internal_edge_pack_pattern(
                     op_pb_default_mode, src_edge_port, des_edge_port,
                     arcxpress::gen_ioff_pack_pattern_name(
                       pmtv.pb_type_name(fff_gen.vpr_physical_model_name()), "D"))
                     ? 0
                     : 1;
      }
    }
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int assemble_pi_complex_block(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string top_pb = "io_pi";
  VTR_LOGV(verbose, "Assemble top-level pb_type '%s'\n", top_pb.c_str());

  /* Build pi model binding */
  std::string pi_pb = arch_gen.find_primitive_pb_type_name(std::string(PI_BLIF_MODEL));
  PhyPbInfo pad_pb_info = {pi_pb, PI_CIRCUIT_MODEL};
  /* Build ff model binding */
  FracffGen fff_gen(PHYSICAL_PI_SCFF_NAME);
  std::string ff_pb = fff_gen.vpr_arch_physical_primitive().pb_type_name(std::string(PHYSICAL_PI_SCFF_NAME));
  PhyPbInfo ff_pb_info = {ff_pb, fff_gen.circuit_model_name()};
  /* Build physical mode */
  num_err +=
    assemble_pi_complex_block_physical_mode(arch_gen, top_pb, "pi_pad", pad_pb_info, ff_pb_info, verbose);
  /* Build operating mode */
  num_err += assemble_pi_complex_block_operating_modes(arch_gen, top_pb, "io_input",
                                                       std::string(PI_NAME),
                                                       std::string(OPERATING_PI_SCFF_NAME),
                                                       pi_pb, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace MOSOFA
