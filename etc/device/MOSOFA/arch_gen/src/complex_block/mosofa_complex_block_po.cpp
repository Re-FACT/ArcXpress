#include <algorithm>
#include <filesystem>
#include <string>
#include "mosofa_complex_block_ff.h"
#include "mosofa_complex_block_po.h"
#include "mosofa_constants.h"
#include "mosofa_naming.h"
#include "command_exit_codes.h"
#include "fracff_gen.h"
#include "vpr_arch_layout.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA { // begin namespace MOSOFA

static int add_po_ff_pb_type(arcxpress::ArchGen& arch_gen, const std::string& ff_pb,
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
    num_err += wire_ff_pb_type_mode(arch_gen, pb_mode, latch_pb, false, true);
    /* Set binding to the its physical pb_type */
    num_err += bind_ff_pb_type_to_physical(arch_gen, ff_pb, latch_pb, phy_pb_type_name, verbose);
  }

  /* Add mmff primitive */
  for (auto pmtv : fff_gen.vpr_arch_primitives(false)) {
    std::string pb_mode = arcxpress::gen_pb_mode_name(ff_pb, pmtv.name());
    num_err += arch_gen.add_pb_type_mode(ff_pb, pb_mode) ? 0 : 1;
    num_err += arch_gen.add_pb_type_to_mode(pb_mode, pmtv.pb_type_name(tag), 1) ? 0 : 1;
    num_err += wire_ff_pb_type_mode(arch_gen, pb_mode, pmtv.pb_type_name(tag), false, true);
    /* Set binding to the its physical pb_type */
    num_err += bind_ff_pb_type_to_physical(arch_gen, ff_pb, pmtv.pb_type_name(tag),
                                           phy_pb_type_name, verbose);
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_po_complex_block_physical_mode(
  arcxpress::ArchGen& arch_gen, const std::string& top_pb, const std::string& phy_pb_name,
  const PhyPbInfo& ff_pb_name, const PhyPbInfo& pad_pb_name, const bool& verbose) {
  int num_err = 0;

  VTR_LOGV(verbose, "Assemble physical mode for PO '%s'...\n", top_pb.c_str());

  /* physical modes and its child pb_type */
  std::string pb_mode = arcxpress::gen_physical_mode_name(top_pb);
  num_err += arch_gen.add_pb_type_mode(top_pb, pb_mode, true) ? 0 : 1;
  std::string phy_pb = phy_pb_name;
  num_err += arch_gen.copy_pb_type(phy_pb, top_pb) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, phy_pb, 1) ? 0 : 1;
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;
  {
    openfpga::BasicPort src_port;
    if (!arch_gen.find_pb_type_port(top_pb, "clk", src_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    openfpga::BasicPort des_port;
    if (!arch_gen.find_pb_type_port(phy_pb, "clk", des_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    arcxpress::ArchGen::EdgePort des_edge_port = {phy_pb, 0, des_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${IO_CLK_MAX_DELAY}", "${IO_CLK_MIN_DELAY}"};
    num_err += arch_gen.set_in2in_edge_delay(pb_mode, src_port, des_edge_port, edge_delay) ? 0 : 1;
  }
  {
    openfpga::BasicPort src_port;
    if (!arch_gen.find_pb_type_port(top_pb, "reset", src_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    openfpga::BasicPort des_port;
    if (!arch_gen.find_pb_type_port(phy_pb, "reset", des_port)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    arcxpress::ArchGen::EdgePort des_edge_port = {phy_pb, 0, des_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${IO_RST_MAX_DELAY}", "${IO_RST_MIN_DELAY}"};
    num_err += arch_gen.set_in2in_edge_delay(pb_mode, src_port, des_edge_port, edge_delay) ? 0 : 1;
  }

  /* Physical mode detailed pb_type tree */
  std::string phy_pb_default_mode = arcxpress::gen_default_mode_name(phy_pb);
  num_err += arch_gen.add_pb_type_mode(phy_pb, phy_pb_default_mode) ? 0 : 1;
  std::string ff_pb = std::get<0>(ff_pb_name);
  num_err += arch_gen.add_pb_type_to_mode(phy_pb_default_mode, ff_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_circuit_model(ff_pb, std::get<1>(ff_pb_name)) ? 0 : 1;
  std::string pad_pb = std::get<0>(pad_pb_name);
  num_err += arch_gen.add_pb_type_to_mode(phy_pb_default_mode, pad_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_circuit_model(pad_pb, std::get<1>(pad_pb_name)) ? 0 : 1;
  /* Interconnect */
  num_err +=
    arch_gen.auto_wire_control_signals_pb_type_mode_child(phy_pb_default_mode, ff_pb) ? 0 : 1;
  /* Data ports */
  std::map<std::string, std::string> data_port_map;
  data_port_map["f2a_i"] = "D";
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
  openfpga::BasicPort src_f2a_port;
  if (!arch_gen.find_pb_type_port(phy_pb, "f2a_i", src_f2a_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_q_port;
  if (!arch_gen.find_pb_type_port(pad_pb, "outpad", des_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(phy_pb_default_mode, pad_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {pad_pb, ipb, des_q_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"0.436e-9", "0.245e-9"};
    num_err += arch_gen.add_in2in_edge(phy_pb_default_mode, src_f2a_port, des_edge_port,
                                       PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                 ? 0
                 : 1;
  }
  openfpga::BasicPort src_q_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "Q", src_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(phy_pb_default_mode, ff_pb)) {
    for (auto jpb : arch_gen.child_pbs(phy_pb_default_mode, pad_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, ipb, src_q_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {pad_pb, jpb, des_q_port};
      arcxpress::ArchGen::EdgeDelay edge_delay = {"0.120e-9", "0.085e-9"};
      num_err += arch_gen.add_internal_edge(phy_pb_default_mode, src_edge_port, des_edge_port,
                                            PB_MUX_HIGHLOAD_CIRCUIT_MODEL_NAME, edge_delay)
                   ? 0
                   : 1;
    }
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_po_complex_block_operating_modes(
  arcxpress::ArchGen& arch_gen, const std::string& top_pb, const std::string& op_pb_name,
  const std::string& op_ff_name, const std::string& op_pad_name, const std::string& phy_pad_name,
  const bool& verbose) {
  int num_err = 0;

  VTR_LOGV(verbose, "Assemble operating mode for PO '%s'...\n", top_pb.c_str());

  /* Operating mode */
  std::string pb_mode = op_pb_name;
  num_err += arch_gen.add_pb_type_mode(top_pb, pb_mode) ? 0 : 1;
  std::string op_pb = op_pb_name;
  num_err += arch_gen.copy_pb_type(op_pb, top_pb) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, op_pb, 1) ? 0 : 1;
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;

  /* Build child pb */
  std::string ff_pb = op_ff_name;
  num_err += add_po_ff_pb_type(arch_gen, ff_pb, verbose);
  std::string pad_pb = op_pad_name;
  /* Bind to physical pb_type */
  num_err += arch_gen.set_pb_type_physical_pb_type(pad_pb, phy_pad_name) ? 0 : 1;

  /* Physical mode detailed pb_type tree */
  std::string op_pb_default_mode = arcxpress::gen_default_mode_name(op_pb);
  num_err += arch_gen.add_pb_type_mode(op_pb, op_pb_default_mode) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(op_pb_default_mode, ff_pb, 1) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(op_pb_default_mode, pad_pb, 1) ? 0 : 1;
  /* Interconnect */
  num_err +=
    arch_gen.auto_wire_control_signals_pb_type_mode_child(op_pb_default_mode, ff_pb) ? 0 : 1;

  /* Data port */
  openfpga::BasicPort src_d_port;
  if (!arch_gen.find_pb_type_port(op_pb, "f2a_i", src_d_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_d_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "D", des_d_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(op_pb_default_mode, ff_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb, ipb, des_d_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${IO_OUTPAD_MAX_DELAY}", "${IO_OUTPAD_MIN_DELAY}"};
    num_err +=
      arch_gen.add_in2in_edge(op_pb_default_mode, src_d_port, des_edge_port, edge_delay) ? 0 : 1;
  }
  /* Output mux */
  openfpga::BasicPort src_f2a_port;
  if (!arch_gen.find_pb_type_port(op_pb, "f2a_i", src_f2a_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort des_q_port;
  if (!arch_gen.find_pb_type_port(pad_pb, "outpad", des_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(op_pb_default_mode, pad_pb)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {pad_pb, ipb, des_q_port};
    arcxpress::ArchGen::EdgeDelay edge_delay = {"${IO_O2O_MAX_DELAY}", "${IO_O2O_MIN_DELAY}"};
    num_err +=
      arch_gen.add_in2in_edge(op_pb_default_mode, src_f2a_port, des_edge_port, edge_delay) ? 0 : 1;
  }
  openfpga::BasicPort src_q_port;
  if (!arch_gen.find_pb_type_port(ff_pb, "Q", src_q_port)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto ipb : arch_gen.child_pbs(op_pb_default_mode, ff_pb)) {
    for (auto jpb : arch_gen.child_pbs(op_pb_default_mode, pad_pb)) {
      arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb, ipb, src_q_port};
      arcxpress::ArchGen::EdgePort des_edge_port = {pad_pb, jpb, des_q_port};
      arcxpress::ArchGen::EdgeDelay edge_delay = {"${IO_FF2O_MAX_DELAY}", "${IO_FF2O_MIN_DELAY}"};
      num_err +=
        arch_gen.add_internal_edge(op_pb_default_mode, src_edge_port, des_edge_port, edge_delay)
          ? 0
          : 1;
      /* Add pack pattern for latches */
      {
        std::string latch_pb = gen_latch_pb_name(ff_pb);
        num_err += arch_gen.set_internal_edge_pack_pattern(
                     op_pb_default_mode, src_edge_port, des_edge_port,
                     arcxpress::gen_ioff_pack_pattern_name(latch_pb, "Q"))
                     ? 0
                     : 1;
      }
      FracffGen fff_gen(OP2PHY_PB_TYPE_MAP.at(ff_pb.c_str()));
      for (auto pmtv : fff_gen.vpr_arch_primitives(false)) {
        num_err += arch_gen.set_internal_edge_pack_pattern(
                     op_pb_default_mode, src_edge_port, des_edge_port,
                     arcxpress::gen_ioff_pack_pattern_name(
                       pmtv.pb_type_name(fff_gen.vpr_physical_model_name()), "Q"))
                     ? 0
                     : 1;
      }
    }
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int assemble_po_complex_block(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string top_pb = "io_po";
  VTR_LOGV(verbose, "Assemble top-level pb_type '%s'\n", top_pb.c_str());
  FracffGen fff_gen(PHYSICAL_PO_SCFF_NAME, PHYSICAL_PO_SCFF_NAME);
  std::string ff_pb =
    fff_gen.vpr_arch_physical_primitive().pb_type_name(std::string(PHYSICAL_PO_SCFF_NAME));
  PhyPbInfo ff_pb_info = {ff_pb, fff_gen.circuit_model_name()};
  std::string po_pb = arch_gen.find_primitive_pb_type_name(std::string(PO_BLIF_MODEL));
  PhyPbInfo pad_pb_info = {po_pb, PO_CIRCUIT_MODEL};
  num_err += assemble_po_complex_block_physical_mode(arch_gen, top_pb, "po_pad", ff_pb_info,
                                                     pad_pb_info, verbose);
  num_err += assemble_po_complex_block_operating_modes(arch_gen, top_pb, "io_output",
                                                       std::string(OPERATING_PO_SCFF_NAME),
                                                       std::string(PO_NAME), po_pb, verbose);
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // end namespace MOSOFA
