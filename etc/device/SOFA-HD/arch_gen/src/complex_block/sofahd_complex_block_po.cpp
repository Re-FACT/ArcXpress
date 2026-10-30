/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "sofahd_complex_block_po.h"
#include "sofahd_constants.h"
#include "sofahd_naming.h"
#include "command_exit_codes.h"
#include "vpr_arch_layout.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace SofaHD { // begin namespace SofaHD

static int assemble_po_complex_block_physical_mode(
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

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int assemble_po_complex_block_operating_modes(
  arcxpress::ArchGen& arch_gen, const std::string& top_pb, const std::string& op_pb_name,
  const std::string& op_pad_name, const std::string& phy_pad_name,
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
  std::string pad_pb = op_pad_name;
  /* Bind to physical pb_type */
  num_err += arch_gen.set_pb_type_physical_pb_type(pad_pb, phy_pad_name) ? 0 : 1;

  /* Physical mode detailed pb_type tree */
  std::string op_pb_default_mode = arcxpress::gen_default_mode_name(op_pb);
  num_err += arch_gen.add_pb_type_mode(op_pb, op_pb_default_mode) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(op_pb_default_mode, pad_pb, 1) ? 0 : 1;

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

  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int assemble_po_complex_block(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string top_pb = "io_po";
  VTR_LOGV(verbose, "Assemble top-level pb_type '%s'\n", top_pb.c_str());
  std::string po_pb = arch_gen.find_primitive_pb_type_name(std::string(PO_BLIF_MODEL));
  PhyPbInfo pad_pb_info = {po_pb, PO_CIRCUIT_MODEL};
  num_err += assemble_po_complex_block_physical_mode(arch_gen, top_pb, "po_pad", 
                                                     pad_pb_info, verbose);
  num_err += assemble_po_complex_block_operating_modes(arch_gen, top_pb, "io_output",
                                                       std::string(PO_NAME), po_pb, verbose);
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace SofaHD
