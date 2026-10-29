#include "arcxpress_shell.h"

#include "arcxpress_arch_dev_command.h"
#include "arcxpress_basic_command.h"
#include "arcxpress_dv_command.h"
#include "arcxpress_file_os.h"
#include "arcxpress_init.h"
#include "arcxpress_netlist_dev_command.h"
#include "arcxpress_openfpga_sdc_command.h"
#include "arcxpress_openfpga_setup_command.h"
#include "arcxpress_openfpga_verilog_command.h"
#include "arcxpress_pre_arch_command.h"
#include "arcxpress_simulation_command.h"
#include "arcxpress_tapeout_command.h"
#include "arcxpress_title.h"
#include "arcxpress_vpr_command.h"
#include "command_echo.h"
#include "command_parser.h"
#include "signal_handler_manager.h"

ArcXpressShell::ArcXpressShell() {
  shell_.set_name("ArcXpress");

  /* Print out title */
  VTR_LOG("\n%s\n", create_arcxpress_title().c_str());
  fflush(stdout);

  /* Add tape-out flow related commands */
  arcxpress::add_arcxpress_tapeout_commands(shell_);
  arcxpress::add_arcxpress_pre_arch_commands(shell_);
  arcxpress::add_arcxpress_arch_dev_commands(shell_);
  arcxpress::add_arcxpress_netlist_dev_commands(shell_);
  arcxpress::add_arcxpress_dv_commands(shell_);

  /* Add implementation commands */
  arcxpress::add_vpr_commands(shell_);

  /* Add openfpga setup commands */
  arcxpress::add_openfpga_setup_commands(shell_);

  /* Add openfpga verilog commands */
  arcxpress::add_openfpga_verilog_commands(shell_);

  /* Add openfpga sdc commands */
  arcxpress::add_openfpga_sdc_commands(shell_);

  /*Add simulation commds*/
  arcxpress::add_simulation_commands(shell_);

  /* Add basic commands: exit, help, etc.
   * Note:
   * This MUST be the last command group to be added!
   */
  arcxpress::add_basic_commands(shell_);
}

int ArcXpressShell::run_command(const char* cmd_line) {
  bool gui_mode = false;
  int ret = arcxpress::SignalHandlerManager::set_recover_handler(gui_mode);
  if (0 == ret) {
    arcxpress::SignalHandlerManager::set_running_cmd_flag(true);
    ret = shell_.execute_command(cmd_line, arcxpress_ctx_);
    arcxpress::SignalHandlerManager::set_running_cmd_flag(false);
  }
  if (arcxpress::CMD_EXEC_SIGINT_HANDLED == ret) {
    ret = 0;
  }
  return ret;
}

void ArcXpressShell::reset() {
  /* TODO: reset the shell status */
  /* TODO: reset the data storage */
}

int ArcXpressShell::start(int argc, char** argv) {
  /* Ensure a clean start */
  reset();
  /* Create the command to launch shell in different modes */
  openfpga::Command start_cmd("ArcXpress");
  /* Add options to openfpga shell interface */
  /* '--interactive', -i': launch the interactive mode */
  openfpga::CommandOptionId opt_interactive =
    start_cmd.add_option("interactive", false, "Launch ArcXpress in interactive mode");
  start_cmd.set_option_short_name(opt_interactive, "i");

  /* '--file', -f': launch the script mode */
  openfpga::CommandOptionId opt_script_mode =
    start_cmd.add_option("file", false, "Launch ArcXpress in script mode");
  start_cmd.set_option_require_value(opt_script_mode, openfpga::OPT_STRING);
  start_cmd.set_option_short_name(opt_script_mode, "f");

  /* '--boot_config', -bc': boot configuration setting */
  openfpga::CommandOptionId opt_boot_cfg =
    start_cmd.add_option("boot_config", true, "The boot configuration file");
  start_cmd.set_option_require_value(opt_boot_cfg, openfpga::OPT_STRING);
  start_cmd.set_option_short_name(opt_boot_cfg, "bc");

  /* '--root_path': specify the root path of device lib */
  openfpga::CommandOptionId opt_root_path =
    start_cmd.add_option("root_path", true, "Specify the root directory of installation/build");
  start_cmd.set_option_require_value(opt_root_path, openfpga::OPT_STRING);

  /* '--batch_execution': execute the script in batch mode.
   * Will exit immediately when fatal errors occurred
   */
  openfpga::CommandOptionId opt_batch_exec = start_cmd.add_option(
    "batch_execution", false, "Launch ArcXpress in batch mode when running scripts");
  start_cmd.set_option_short_name(opt_batch_exec, "batch");

  /* '--version', -v': print version information */
  openfpga::CommandOptionId opt_version =
    start_cmd.add_option("version", false, "Show ArcXpress version");
  start_cmd.set_option_short_name(opt_version, "v");

  /* '--help', -h': print help desk */
  openfpga::CommandOptionId opt_help = start_cmd.add_option("help", false, "Help desk");
  start_cmd.set_option_short_name(opt_help, "h");

  /* Parse the option, to avoid issues, we use the command name to replace the
   * argv[0] */
  std::vector<std::string> cmd_opts;
  cmd_opts.push_back(start_cmd.name());
  for (int iarg = 1; iarg < argc; ++iarg) {
    cmd_opts.push_back(std::string(argv[iarg]));
  }

  openfpga::CommandContext start_cmd_context(start_cmd);
  if (false == parse_command(cmd_opts, start_cmd, start_cmd_context)) {
    /* Parse fail: Echo the command */
    openfpga::print_command_options(start_cmd);
  } else {
    /* Format the root path in case windows-style is found */
    std::string formatted_root_path =
      arcxpress::format_windows_path(start_cmd_context.option_value(start_cmd, opt_root_path));
    std::string formatted_boot_cfg_path =
      arcxpress::format_windows_path(start_cmd_context.option_value(start_cmd, opt_boot_cfg));

    /* Parse succeed. Initialize and branch on options */
    if (arcxpress::arcxpress_init(arcxpress_ctx_, formatted_root_path, formatted_boot_cfg_path) ==
        openfpga::CMD_EXEC_FATAL_ERROR) {
      return 1;
    };
    /* Show version */
    if (true == start_cmd_context.option_enable(start_cmd, opt_version)) {
      print_arcxpress_version_info();
      return 0;
    }
    /* Start a shell */
    if (true == start_cmd_context.option_enable(start_cmd, opt_interactive)) {
      run_interactive_mode();
      return shell_.exit_code();
    }

    if (true == start_cmd_context.option_enable(start_cmd, opt_script_mode)) {
      run_script_mode(start_cmd_context.option_value(start_cmd, opt_script_mode).c_str(),
                      start_cmd_context.option_enable(start_cmd, opt_batch_exec));
      return shell_.exit_code();
    }
    /* Reach here there is something wrong, show the help desk */
    openfpga::print_command_options(start_cmd);
  }

  /* Reach here, it means shell execution has critical errors.
   * Return a code with fatal errors
   */
  return 1;
}

void ArcXpressShell::run_interactive_mode() {
  bool gui_mode = false;
  arcxpress::SignalHandlerManager::set_sig_handler(gui_mode);
  int ret =
    arcxpress::SignalHandlerManager::set_fatal_handler("run_interactive_mode_signal_handler");
  /* If fatal error such as segment fault occurs, signal handler handles it and returns to
     interactive mode. If fatal error occurs again, the above process runs again in the below loop.
     The exit commmand can break this loop. */
  while ((arcxpress::CMD_EXEC_SUCCESS == ret) || (arcxpress::CMD_EXEC_FATAL_ERROR_HANDLED == ret)) {
    int status = arcxpress::SignalHandlerManager::set_recover_handler(gui_mode);
    shell_.run_interactive_mode(arcxpress_ctx_);
  }
}

void ArcXpressShell::run_script_mode(const char* script_file_name, const bool& batch_mode) {
  bool gui_mode = false;
  arcxpress::SignalHandlerManager::set_sig_handler(gui_mode);
  int status = arcxpress::CMD_EXEC_SUCCESS;
  int ret = arcxpress::SignalHandlerManager::set_fatal_handler("run_script_mode_signal_handler");
  if (arcxpress::CMD_EXEC_SUCCESS == ret) {
    status = arcxpress::SignalHandlerManager::set_recover_handler(gui_mode);
    shell_.run_script_mode(script_file_name, arcxpress_ctx_, batch_mode);
  }
  run_interactive_mode();
}
