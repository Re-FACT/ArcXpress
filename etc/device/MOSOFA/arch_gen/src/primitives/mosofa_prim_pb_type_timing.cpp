/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "sofahd_constants.h"
#include "sofahd_prim_pb_type_timing.h"
#include "command_exit_codes.h"
#include "fracff_gen.h"
#include "vpr_arch_layout.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace SofaHD { // begin namespace SofaHD

static int add_frac_lut4_primitive_pb_type_timing(arcxpress::ArchGen& arch_gen) {
  int num_err = 0;
  std::string prim_pb = arch_gen.find_primitive_pb_type_name(std::string(FRAC_LUT4_BLIF_MODEL));
  if (prim_pb.empty()) {
    VTR_LOG_ERROR("The primitive pb_type '%s' was not successfully added!\n", FRAC_LUT4_BLIF_MODEL);
  }
  openfpga::BasicPort in_port("in", 4);
  openfpga::BasicPort lut4_out_port("lut3_out", 2);
  openfpga::BasicPort lut5_out_port("lut4_out", 1);

  std::string lut3_delay_var = arch_gen.lut_comb_delay_variable(4);
  std::string lut4_delay_var = arch_gen.lut_comb_delay_variable(5);
  num_err +=
    arch_gen.add_pb_type_comb_delay(prim_pb, in_port, lut3_out_port, lut4_delay_var) ? 0 : 1;
  num_err +=
    arch_gen.add_pb_type_comb_delay(prim_pb, in_port, lut3_out_port, lut4_delay_var) ? 0 : 1;
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_dff_primitive_pb_type_timing(arcxpress::ArchGen& arch_gen,
                                            const std::string& vpr_model_name) {
  int num_err = 0;
  for (std::string prim_pb : arch_gen.find_primitive_pb_type_names(vpr_model_name)) {
    if (prim_pb.empty()) {
      VTR_LOG_ERROR("The primitive pb_type '%s' was not successfully added!\n",
                    vpr_model_name.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    openfpga::BasicPort d_port("D", 1);
    openfpga::BasicPort q_port("Q", 1);
    openfpga::BasicPort ck_port("C", 1);
    /* Port class: TODO: may be moved to a separated function */
    num_err +=
      arch_gen.set_pb_type_port_class(prim_pb, d_port, arcxpress::ArchGen::e_port_class::FF_D) ? 0
                                                                                              : 1;
    num_err +=
      arch_gen.set_pb_type_port_class(prim_pb, q_port, arcxpress::ArchGen::e_port_class::FF_Q) ? 0
                                                                                              : 1;
    num_err +=
      arch_gen.set_pb_type_port_class(prim_pb, ck_port, arcxpress::ArchGen::e_port_class::FF_CLOCK)
        ? 0
        : 1;
    /* Timing */
    num_err += arch_gen.add_pb_type_setup_delay(prim_pb, d_port, ck_port, "${FF_T_SETUP}") ? 0 : 1;
    num_err += arch_gen.add_pb_type_hold_delay(prim_pb, d_port, ck_port, "${FF_T_HOLD}") ? 0 : 1;
    num_err +=
      arch_gen.add_pb_type_clk2q_delay(prim_pb, q_port, ck_port, "${FF_T_CLK2Q}", "${FF_T_CLK2Q}")
        ? 0
        : 1;
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_primitive_pb_type_timing(arcxpress::ArchGen& arch_gen, const bool& verbose) {
  int num_err = 0;
  VTR_LOGV(verbose, "Adding timing information to common primitive pb_types\n");
  num_err += add_frac_lut4_primitive_pb_type_timing(arch_gen);
  for (auto scff : ALL_REGULAR_SCFF_NAMES) {
    FracffGen fff_gen(scff);
    for (auto pmtv : fff_gen.vpr_arch_primitives(true)) {
      num_err += add_dff_primitive_pb_type_timing(arch_gen, pmtv.name());
    }
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace SofaHD
