/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
#include "arch_gen_command_interface.h"
#include "arch_gen.h"
#include "command_exit_codes.h"

int main(int argc, char** argv) {
  /*************************************
   * General purpose interface API for architecture generator
   * Instantiate the interface with the command name
   */
  arcxpress::ArchGenCommandInterface cmd_interface("arch_gen");
  openfpga::CommandContext exec_cmd_ctx(cmd_interface.command());

  // Parse command-line options
  if (!cmd_interface.parse(argc, argv, exec_cmd_ctx)) {
    VTR_LOG_ERROR("Failed in parsing architecture generator command-line options\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* The standard arch genertor and status flag to start device-specific customization */
  arcxpress::ArchGen arch_gen;
  int status = 0;

  /*************************************
   * Run device-specific arch generation
   */
  // TODO: merge the vpr_arch_primitive to init_vpr_arch_layout
  status = SofaHD::init_vpr_arch_primitives(arch_gen, false, false);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in creating primitive details\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = SofaHD::init_vpr_arch_layout(arch_gen, height, width, num_clks, num_rsts, num_inputs, num_outputs, num_pclks, num_regions, sync_dp_dimension);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in creating layout details\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /*************************************
   * General purpose interface API for architecture generator
   * Generate and write all XML template files, networks, and maps
   */
  status = cmd_interface.write_xml_templates(exec_cmd_ctx, arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in writing architecture file packages\n");
    return status;
  }

  return status;
}
