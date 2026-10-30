/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
#include "arch_gen_command_interface.h"
#include "arch_gen.h"
#include "command_exit_codes.h"
#include "mosofa_arch_layout.h"

int main(int argc, char** argv) {
  /*************************************
   * General purpose interface API for architecture generator
   * Instantiate the interface with the command name
   */
  arcxpress::ArchGenCommandInterface cmd_interface;
  // Parse command-line options
  if (!cmd_interface.parse("arch_gen", argc, argv)) {
    VTR_LOG_ERROR("Failed in parsing architecture generator command-line options\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* The standard arch genertor and status flag to start device-specific customization */
  arcxpress::ArchGen arch_gen;
  int status = 0;
  arcxpress::ArchGenInputParams arch_params = cmd_interface.arch_parameters(); 

  /*************************************
   * Run device-specific arch generation
   */
  status = MOSOFA::create_vpr_arch_layout(arch_gen, arch_params);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in creating layout details\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /*************************************
   * General purpose interface API for architecture generator
   * Generate and write all XML template files, networks, and maps
   */
  status = cmd_interface.write_xml_templates(arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in writing architecture file packages\n");
    return status;
  }

  return status;
}
