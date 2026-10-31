/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "add_hidden_primitive_pb_types.h"
#include "mosofa_complex_block_ff.h"
#include "mosofa_constants.h"
#include "mosofa_naming.h"
#include "command_exit_codes.h"
#include "fracff_gen.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA { // begin namespace MOSOFA

/* A unified function to do wiring between ff pb_type and its parent pb_type under a mode */
int wire_ff_pb_type_mode(arcxpress::ArchGen& arch_gen, const std::string& ff_mode,
                         const std::string& ff_pb_name, const bool& add_ipack_pattern,
                         const bool& add_opack_pattern) {
  int num_err = 0;
  openfpga::BasicPort d_port("D", 1);
  openfpga::BasicPort clr_port("R", 1);
  openfpga::BasicPort q_port("Q", 1);
  openfpga::BasicPort ck_port("C", 1);
  for (auto ipb : arch_gen.child_pbs(ff_mode, ff_pb_name)) {
    arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb_name, ipb, d_port};
    num_err += arch_gen.add_in2in_edge(ff_mode, d_port, des_edge_port) ? 0 : 1;
    if (add_ipack_pattern) {
      num_err +=
        arch_gen.set_in2in_edge_pack_pattern(ff_mode, d_port, des_edge_port,
                                             arcxpress::gen_ioff_pack_pattern_name(ff_pb_name, "D"))
          ? 0
          : 1;
    }
  }
  for (auto ipb : arch_gen.child_pbs(ff_mode, ff_pb_name)) {
    openfpga::BasicPort des_ck_port;
    for (std::string des_ck_name : {"clk", "C"}) {
      if (!arch_gen.find_pb_type_port(ff_pb_name, des_ck_name, des_ck_port)) {
        continue;
      }
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb_name, ipb, des_ck_port};
      num_err += arch_gen.add_in2in_edge(ff_mode, ck_port, des_edge_port) ? 0 : 1;
    }
  }
  for (auto ipb : arch_gen.child_pbs(ff_mode, ff_pb_name)) {
    arcxpress::ArchGen::EdgePort src_edge_port = {ff_pb_name, ipb, q_port};
    num_err += arch_gen.add_out2out_edge(ff_mode, src_edge_port, q_port) ? 0 : 1;
    if (add_opack_pattern) {
      num_err +=
        arch_gen.set_out2out_edge_pack_pattern(
          ff_mode, src_edge_port, q_port, arcxpress::gen_ioff_pack_pattern_name(ff_pb_name, "Q"))
          ? 0
          : 1;
    }
  }
  /* Handle the optional reset/set signals */
  for (auto ipb : arch_gen.child_pbs(ff_mode, ff_pb_name)) {
    for (std::string des_clr_name : {"R", "RN", "S", "SN"}) {
      openfpga::BasicPort des_clr_port;
      if (!arch_gen.find_pb_type_port(ff_pb_name, des_clr_name, des_clr_port)) {
        continue;
      }
      arcxpress::ArchGen::EdgePort des_edge_port = {ff_pb_name, ipb, des_clr_port};
      num_err += arch_gen.add_in2in_edge(ff_mode, clr_port, des_edge_port) ? 0 : 1;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int bind_ff_pb_type_to_physical(arcxpress::ArchGen& arch_gen, const std::string& ff_pb_tag,
                                const std::string& ff_pb_type, const std::string& phy_pb_type_name,
                                const bool& verbose) {
  int num_err = 0;
  /* Bind physical pb_type */
  VTR_LOGV(verbose, "Binding pb_type '%s' (tag: '%s') to its physical '%s'\n", ff_pb_type.c_str(),
           ff_pb_tag.c_str(), phy_pb_type_name.c_str());
  num_err += arch_gen.set_pb_type_physical_pb_type(ff_pb_type, phy_pb_type_name) ? 0 : 1;
  if (std::string(ff_pb_tag) == std::string(OPERATING_BLE3_FF_NAME)) {
    /* Intended to be empty */
  } else if (std::string(ff_pb_tag) == std::string(OPERATING_BLE4_FF_NAME)) {
    num_err += arch_gen.set_pb_type_physical_pb_type_index_factor(ff_pb_type, "2") ? 0 : 1;
    num_err += arch_gen.set_pb_type_physical_pb_type_index_offset(ff_pb_type, "0") ? 0 : 1;
  } else if (std::string(ff_pb_tag) == std::string(OPERATING_ALE_FF_NAME)) {
    num_err += arch_gen.set_pb_type_physical_pb_type_index_factor(ff_pb_type, "2") ? 0 : 1;
    num_err += arch_gen.set_pb_type_physical_pb_type_index_offset(ff_pb_type, "1") ? 0 : 1;
  }
  /* Bind physical ports */
  {
    openfpga::BasicPort src_port;
    openfpga::BasicPort phy_port("C", 1);
    if (arch_gen.find_pb_type_port(ff_pb_type, "clk", src_port)) {
      num_err +=
        arch_gen.set_pb_type_physical_port_binding(ff_pb_type, src_port.get_name(), phy_port) ? 0
                                                                                              : 1;
    }
  }
  {
    openfpga::BasicPort src_port;
    openfpga::BasicPort phy_port("R", 1);
    if (arch_gen.find_pb_type_port(ff_pb_type, "RN", src_port)) {
      num_err +=
        arch_gen.set_pb_type_physical_port_binding(ff_pb_type, src_port.get_name(), phy_port) ? 0
                                                                                              : 1;
    }
  }
  {
    openfpga::BasicPort src_port;
    openfpga::BasicPort phy_port("R", 1);
    if (arch_gen.find_pb_type_port(ff_pb_type, "S", src_port)) {
      num_err +=
        arch_gen.set_pb_type_physical_port_binding(ff_pb_type, src_port.get_name(), phy_port) ? 0
                                                                                              : 1;
    }
  }
  {
    openfpga::BasicPort src_port;
    openfpga::BasicPort phy_port("R", 1);
    if (arch_gen.find_pb_type_port(ff_pb_type, "SN", src_port)) {
      num_err +=
        arch_gen.set_pb_type_physical_port_binding(ff_pb_type, src_port.get_name(), phy_port) ? 0
                                                                                              : 1;
    }
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_ff_pb_type(arcxpress::ArchGen& arch_gen, const std::string& ff_pb, const bool& verbose) {
  int num_err = 0;

  VTR_LOGV(verbose, "Adding intermediate pb_type '%s'\n", ff_pb.c_str());
  num_err += arch_gen.add_pb_type(ff_pb) ? 0 : 1;
  openfpga::BasicPort d_port("D", 1);
  openfpga::BasicPort clr_port("R", 1);
  openfpga::BasicPort q_port("Q", 1);
  openfpga::BasicPort ck_port("C", 1);
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, d_port, arcxpress::VprArchLayout::e_port_type::INPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, clr_port, arcxpress::VprArchLayout::e_port_type::RESET) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, q_port, arcxpress::VprArchLayout::e_port_type::OUTPUT) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_port(ff_pb, ck_port, arcxpress::VprArchLayout::e_port_type::CLOCK) ? 0 : 1;

  FracffGen phy_fff_gen(PHYSICAL_REGULAR_SCFF_NAME);
  std::string phy_tag = phy_fff_gen.vpr_physical_model_name();
  std::string phy_pb_type_name = arch_gen.find_primitive_pb_type_name(phy_tag);

  { /* Latch */
    std::string pb_mode =
      arcxpress::gen_pb_mode_name(ff_pb, std::string(OPERATING_BUILTIN_LATCH_NAME));
    num_err += arch_gen.add_pb_type_mode(ff_pb, pb_mode) ? 0 : 1;
    std::string latch_pb = gen_latch_pb_name(ff_pb);
    num_err += arch_gen.add_pb_type_to_mode(pb_mode, latch_pb, 1) ? 0 : 1;
    num_err += wire_ff_pb_type_mode(arch_gen, pb_mode, latch_pb, false, false);
    num_err += bind_ff_pb_type_to_physical(arch_gen, ff_pb, latch_pb, phy_pb_type_name, verbose);
  }

  /* Add mmff primitive */
  FracffGen fff_gen(ff_pb);
  std::string tag = fff_gen.vpr_physical_model_name();
  for (auto pmtv : fff_gen.vpr_arch_primitives(false)) {
    std::string pb_mode = arcxpress::gen_pb_mode_name(ff_pb, pmtv.name());
    num_err += arch_gen.add_pb_type_mode(ff_pb, pb_mode) ? 0 : 1;
    num_err += arch_gen.add_pb_type_to_mode(pb_mode, pmtv.pb_type_name(tag), 1) ? 0 : 1;
    num_err += wire_ff_pb_type_mode(arch_gen, pb_mode, pmtv.pb_type_name(tag), false, false);
    num_err += bind_ff_pb_type_to_physical(arch_gen, ff_pb, pmtv.pb_type_name(tag),
                                           phy_pb_type_name, verbose);
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_mosofa_builtin_latch_primitive_pb_type(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;
  VTR_LOGV(verbose, "Adding pb_types for vpr built-in model 'latches'...\n");
  for (auto scff_name : ALL_REGULAR_SCFF_NAMES) {
    std::string pb_name = MOSOFA::gen_latch_pb_name(scff_name);
    num_err += add_builtin_latch_primitive_pb_type(arch_gen, pb_name, true);
    FracffGen fff_gen(scff_name);
    std::string mode_bits = fff_gen.default_mode_bits();
    num_err += arch_gen.set_pb_type_mode_bits(pb_name, mode_bits) ? 0 : 1;
  }
  VTR_LOGV(verbose, "Done\n");
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // end namespace MOSOFA
