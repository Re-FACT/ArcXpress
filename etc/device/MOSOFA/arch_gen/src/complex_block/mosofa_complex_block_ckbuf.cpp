#include <algorithm>
#include <filesystem>
#include <string>
#include "mosofa_complex_block_ckbuf.h"
#include "mosofa_constants.h"
#include "mosofa_naming.h"
#include "command_exit_codes.h"
#include "vpr_arch_layout.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA { // begin namespace MOSOFA

int assemble_ckbuf_complex_block(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;

  std::string top_pb = std::string(CKBUF_BLIF_MODEL);
  VTR_LOGV(verbose, "Assemble top-level pb_type '%s'\n", top_pb.c_str());
  std::string phy_mode = arcxpress::gen_physical_mode_name(top_pb);
  std::string prim_pb = arch_gen.find_primitive_pb_type_name(std::string(CKBUF_BLIF_MODEL));
  /* Physical modes */
  num_err += arch_gen.add_pb_type_mode(top_pb, phy_mode, true) ? 0 : 1;
  num_err += arch_gen.set_pb_type_circuit_model(prim_pb, CKBUF_CIRCUIT_MODEL) ? 0 : 1;
  num_err += arch_gen.add_pb_type_to_mode(phy_mode, prim_pb, 1) ? 0 : 1;
  num_err += arch_gen.auto_wire_pb_type_mode(phy_mode) ? 0 : 1;
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // end namespace MOSOFA
