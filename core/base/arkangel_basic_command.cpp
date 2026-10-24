/********************************************************************
 * Add basic commands to the ArkAngel shell interface, including:
 * - exit
 * - version
 * - help
 *******************************************************************/
#include "arkangel_basic_command.h"

#include "arkangel_title.h"

/* begin namespace arcxpress */
namespace arcxpress {

void add_basic_commands(openfpga::Shell<ArkangelContext>& shell) {
  /* Add a new class of commands */
  openfpga::ShellCommandClassId basic_cmd_class = shell.add_command_class("Basic");

  openfpga::Command shell_cmd_exit("exit");
  openfpga::ShellCommandId shell_cmd_exit_id = shell.add_command(shell_cmd_exit, "Exit the shell");
  shell.set_command_class(shell_cmd_exit_id, basic_cmd_class);
  shell.set_command_execute_function(shell_cmd_exit_id, [shell]() { shell.exit(); });

  /* Version */
  openfpga::Command shell_cmd_version("version");
  openfpga::ShellCommandId shell_cmd_version_id =
    shell.add_command(shell_cmd_version, "Show version information");
  shell.set_command_class(shell_cmd_version_id, basic_cmd_class);
  shell.set_command_execute_function(shell_cmd_version_id, print_arkangel_version_info);

  /* Note:
   * help MUST be the last to add because the linking to execute function will
   * do a snapshot on the shell
   */
  openfpga::Command shell_cmd_help("help");
  openfpga::ShellCommandId shell_cmd_help_id =
    shell.add_command(shell_cmd_help, "Launch help desk");
  shell.set_command_class(shell_cmd_help_id, basic_cmd_class);
  shell.set_command_execute_function(shell_cmd_help_id, [shell]() { shell.print_commands(); });
}

}  // namespace arcxpress
