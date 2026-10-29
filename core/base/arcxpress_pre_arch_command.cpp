/********************************************************************
 * Add project-related commands to the Arcxpress shell interface
 *******************************************************************/
#include "arcxpress_pre_arch_command.h"

#include "arcxpress_pre_arch_analysis.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_analyze_logic_resource_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("analyze_logic_resource");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_min_lut_size =
    shell_cmd.add_option("min_lut_size", false, "Specify the lower bound of LUT input size");
  shell_cmd.set_option_require_value(opt_min_lut_size, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_max_lut_size =
    shell_cmd.add_option("max_lut_size", false, "Specify the upper bound of LUT input size");
  shell_cmd.set_option_require_value(opt_max_lut_size, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_rpt_qor =
    shell_cmd.add_option("report_qor", false, "Analyze the results and report to a file");
  shell_cmd.set_option_require_value(opt_rpt_qor, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd,
                      "Run an prediction on the count of logic resources before initiating any "
                      "architecture development");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, analyze_logic_resource);

  return shell_cmd_id;
}

void add_arcxpress_pre_arch_commands(openfpga::Shell<ArcxpressContext>& shell) {
  /* Add a new class of commands */
  openfpga::ShellCommandClassId arcxpress_pre_arch_cmd_class =
    shell.add_command_class("Arcxpress pre-arch evaluation");

  add_arcxpress_analyze_logic_resource_command(shell, arcxpress_pre_arch_cmd_class);
}

}  // namespace arcxpress
