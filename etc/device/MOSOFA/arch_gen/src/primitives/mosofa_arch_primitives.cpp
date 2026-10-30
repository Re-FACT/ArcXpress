/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>
/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
/* Headers from openfpga library */
#include "command_exit_codes.h"
/* Headers from arch gen library */
#include "vpr_arch_primitive.h"
#include "mosofa_arch_primitives.h"
#include "mosofa_constants.h"
#include "fracff_gen.h"

namespace MOSOFA { // begin namespace MOSOFA

static int add_vpr_arch_primitive_pi(arcxpress::ArchGen& arch_gen) {
  int num_err = 0;
  VTR_LOG("Adding primary input models\n");
  arcxpress::VprArchPrimitive pmtv(PI_BLIF_MODEL);
  num_err += pmtv.set_blif_model(std::string(PI_BLIF_MODEL)) ? 0 : 1;
  num_err += pmtv.set_circuit_model(std::string(PI_CIRCUIT_MODEL)) ? 0 : 1;
  num_err += pmtv.set_circuit_model_type(arcxpress::VprArchPrimitive::e_model_type::IO) ? 0 : 1;
  num_err += pmtv.set_physical_model() ? 0 : 1;
  num_err += pmtv.add_output_circuit_port("inpad", 1, "soc_out_o") ? 0 : 1;
  num_err += pmtv.set_output_port_data_io("inpad", true) ? 0 : 1;
  arch_gen.add_arch_primitive(pmtv);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_vpr_arch_primitive_po(arcxpress::ArchGen& arch_gen) {
  int num_err = 0;
  VTR_LOG("Adding primary output models\n");
  arcxpress::VprArchPrimitive pmtv(PO_BLIF_MODEL);
  num_err += pmtv.set_blif_model(std::string(PO_BLIF_MODEL)) ? 0 : 1;
  num_err += pmtv.set_circuit_model(std::string(PO_CIRCUIT_MODEL)) ? 0 : 1;
  num_err += pmtv.set_circuit_model_type(arcxpress::VprArchPrimitive::e_model_type::IO) ? 0 : 1;
  num_err += pmtv.set_physical_model() ? 0 : 1;
  num_err += pmtv.add_input_port("outpad", 1) ? 0 : 1;
  num_err += pmtv.set_input_port_lib_name("outpad", "fpga_out_i") ? 0 : 1;
  arch_gen.add_arch_primitive(pmtv);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_vpr_arch_primitive_lut(arcxpress::ArchGen& arch_gen) {
  int num_err = 0;
  arcxpress::VprArchPrimitive pmtv(FRAC_LUT4_BLIF_MODEL);
  num_err += pmtv.set_blif_model(std::string(FRAC_LUT4_BLIF_MODEL)) ? 0 : 1;
  num_err += pmtv.set_circuit_model(std::string(FRAC_LUT4_CIRCUIT_MODEL)) ? 0 : 1;
  num_err += pmtv.set_circuit_model_type(arcxpress::VprArchPrimitive::e_model_type::LUT) ? 0 : 1;
  num_err += pmtv.set_require_external_netlist(false) ? 0 : 1;
  num_err += pmtv.set_mode_bits(std::string(FRAC_LUT4_BLIF_MODEL), "0") ? 0 : 1;
  num_err += pmtv.set_physical_model() ? 0 : 1;
  num_err += pmtv.add_input_port("in", 4) ? 0 : 1;
  num_err += pmtv.add_output_port("lut3_out", 2) ? 0 : 1;
  num_err += pmtv.set_output_port_frac_level("lut4_out", 3) ? 0 : 1;
  num_err += pmtv.add_output_port("lut4_out", 1) ? 0 : 1;
  num_err += pmtv.set_output_port_frac_level("lut4_out", 4) ? 0 : 1;
  num_err += pmtv.add_input_comb_sink_port("in", "lut3_out") ? 0 : 1;
  num_err += pmtv.add_input_comb_sink_port("in", "lut4_out") ? 0 : 1;
  arch_gen.add_arch_primitive(pmtv);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_vpr_arch_primitive_ff(arcxpress::ArchGen& arch_gen) {
  int num_err = 0;

  for (auto scff : ALL_REGULAR_SCFF_NAMES) {
    FracffGen fff_gen(scff);
    bool incl_phy = false;
    if (std::string(scff) == std::string(PHYSICAL_REGULAR_SCFF_NAME)) {
      incl_phy = true;
    }
    for (auto pmtv : fff_gen.vpr_arch_primitives(incl_phy)) {
      if (arch_gen.is_arch_primitive_exist(pmtv.name())) {
        std::string tag = fff_gen.vpr_physical_model_name();
        arch_gen.set_arch_primitive_mode_bits(pmtv.name(), tag, pmtv.mode_bits(tag));
      } else {
        VTR_LOG("Adding FF model '%s'\n", pmtv.name().c_str());
        arch_gen.add_arch_primitive(pmtv);
      }
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Initialize primitives for Vpr architecture generator
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int create_vpr_arch_primitives(arcxpress::ArchGen& arch_gen) {
  int num_err = 0;

  /* Primary io models */
  VTR_LOG("Adding primary I/O models\n");
  num_err += add_vpr_arch_primitive_pi(arch_gen);
  if (num_err) {
    VTR_LOG_ERROR("Fatal error occur");
    return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
  }
  num_err += add_vpr_arch_primitive_po(arch_gen);
  if (num_err) {
    VTR_LOG_ERROR("Fatal error occur");
    return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG("Adding LUT models\n");
  num_err += add_vpr_arch_primitive_lut(arch_gen);
  if (num_err) {
    VTR_LOG_ERROR("Fatal error occur");
    return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG("Adding Flip-flop models\n");
  num_err += add_vpr_arch_primitive_ff(arch_gen);
  if (num_err) {
    VTR_LOG_ERROR("Fatal error occur");
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // end namespace MOSOFA
