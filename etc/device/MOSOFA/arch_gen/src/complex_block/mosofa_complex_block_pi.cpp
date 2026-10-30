/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "mosofa_complex_block_pi.h"
#include "mosofa_constants.h"
#include "mosofa_naming.h"
#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA {

static int assemble_pi_complex_block_physical_mode(
  arcxpress::ArchGen& arch_gen, const std::string& top_pb, const std::string& phy_pb_name,
  const PhyPbInfo& pad_pb_name, const bool& verbose) {
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
    num_err += arch_gen.add_out2out_edge(phy_pb_default_mode, src_edge_port, des_a2f_port)
                 ? 0
                 : 1;
  }

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_pi_complex_block_operating_modes(
  arcxpress::ArchGen& arch_gen, const std::string& top_pb, const std::string& op_pb_name,
  const std::string& op_pad_name, const std::string& phy_pad_name,
  const bool& verbose) {
  int num_err = 0;

  VTR_LOGV(verbose, "Assemble operating mode for PI '%s'...\n", top_pb.c_str());

  /* Operating mode */
  std::string pb_mode = op_pb_name;
  num_err += arch_gen.add_pb_type_mode(top_pb, pb_mode) ? 0 : 1;
  std::string op_pb = op_pb_name;
  num_err += arch_gen.copy_pb_type(op_pb, top_pb) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(pb_mode, op_pb, 1) ? 0 : 1;
  num_err += arch_gen.auto_wire_pb_type_mode(pb_mode) ? 0 : 1;

  /* Physical mode detailed pb_type tree */
  std::string op_pb_default_mode = arcxpress::gen_default_mode_name(op_pb);
  num_err += arch_gen.add_pb_type_mode(op_pb, op_pb_default_mode) ? 0 : 1;
  std::string pad_pb = op_pad_name;
  num_err += arch_gen.add_pb_type_to_mode(op_pb_default_mode, pad_pb, 1) ? 0 : 1;
  num_err += arch_gen.set_pb_type_physical_pb_type(pad_pb, phy_pad_name) ? 0 : 1;
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

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int assemble_pi_complex_block(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string top_pb = "io_pi";
  VTR_LOGV(verbose, "Assemble top-level pb_type '%s'\n", top_pb.c_str());

  std::string pi_pb = arch_gen.find_primitive_pb_type_name(std::string(PI_BLIF_MODEL));
  PhyPbInfo pad_pb_info = {pi_pb, PI_CIRCUIT_MODEL};
  num_err += assemble_pi_complex_block_physical_mode(arch_gen, top_pb, "pi_pad",
                                                     pad_pb_info, verbose);
  num_err += assemble_pi_complex_block_operating_modes(arch_gen, top_pb, "io_input",
                                                       std::string(PI_NAME), pi_pb, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace MOSOFA
