/********************************************************************
 * Add project-related commands to the Arcxpress shell interface
 *******************************************************************/
#include "arcxpress_tapeout_command.h"

#include "arcxpress_analyses.h"
#include "arcxpress_backannotate.h"
#include "arcxpress_export.h"
#include "arcxpress_report.h"
#include "arcxpress_tapeout.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * - Add a command to Shell environment: save_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_save_tapeout_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("save_tapeout");

  /* Add an option '--config_file' */
  openfpga::CommandOptionId opt_config_file =
    shell_cmd.add_option("config_file", false,
                         "file path to the project description. If not specifed, will use the same "
                         "file when create/load the project from");
  shell_cmd.set_option_require_value(opt_config_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Save the tapeout project to a given configuration file");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, save_tapeout);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_create_tapeout_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("create_tapeout");

  /* Add an option '--config_file' */
  openfpga::CommandOptionId opt_config_file =
    shell_cmd.add_option("config_file", true, "file path to the project description");
  shell_cmd.set_option_require_value(opt_config_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "create a new tapeout project with a given configuration file");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, create_tapeout);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: load_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_load_tapeout_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("load_tapeout");

  /* Add an option '--config_file' */
  openfpga::CommandOptionId opt_config_file =
    shell_cmd.add_option("config_file", true, "file path to the project description");
  shell_cmd.set_option_require_value(opt_config_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_no_validation =
    shell_cmd.add_option("no_validation", false,
                         "Skip the validation step, which may cause errors or extra debugging "
                         "effort during development ");

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Load an existing tapeout project with a given configuration file");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, load_tapeout);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_show_status_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("show_status");

  /* Add an option '--step' */
  openfpga::CommandOptionId opt_step = shell_cmd.add_option(
    "step", true, "Specify the step name whose development status to be shown");
  shell_cmd.set_option_require_value(opt_step, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name whose development status to be shown");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Show the development status of a tapeout project");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, show_tapeout_status);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_show_dashboard_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("show_dashboard");

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false, "File path where the dashboard reports are written to");
  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Show the dashboard on the latest development status of a tapeout project");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, show_tapeout_dashboard);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_corners
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_corners_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_corners");

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false, "File path where the corner reports are written to");

  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);
  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Show all the corners defined in a tapeout project");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_corners);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_corners
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_module_names_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_module_names");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of architecture development");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Show all the module names for selected architecture development run");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_module_names);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_corners
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_ref_module_names_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_reference_module_names");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of architecture development");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Show all the reference module names for selected architecture development run");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_ref_module_names);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_io
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_io_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_io");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of architecture development");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false, "File path where the io reports are written to");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);
  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Show all the I/O information for selected architecture development run");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_io);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_static_power
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_static_power_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_static_power");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of netlist development");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corner will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false, "File path where the static power reports are written to");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);
  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Report static power consumption for selected netlist development run and corner");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_static_power);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_static_power
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_benchmark_power_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_benchmark_power");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of design verification");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corner will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_file = shell_cmd.add_option(
    "file", false, "File path where the benchmark power reports are written to");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_name =
    shell_cmd.add_option("benchmark", true, "Specify the name of the benchmark to be considered");
  shell_cmd.set_option_require_value(opt_name, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Report benchmark power consumption for selected design verification run and corner");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_benchmark_power);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_config_power
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_config_power_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_config_power");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of netlist development");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corner will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false, "File path where the config power reports are written to");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Report configuration power consumption for selected netlist development run and corner");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_config_power);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_config_stats
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_config_stats_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_config_stats");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of netlist development");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_pclk_freq = shell_cmd.add_option(
    "pclk_freq", true,
    "Specify clock frequency to be applied on the programming clocks. The unit is [Hz]");
  shell_cmd.set_option_require_value(opt_pclk_freq, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_precision =
    shell_cmd.add_option("precision", false, "Precision of data values");
  shell_cmd.set_option_require_value(opt_precision, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false, "File path where the config status reports are written to");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd,
                      "Report the configuration time and detailed statistics about the "
                      "configuration protocol of eFPGA for selected netlist development run");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_config_stats);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_corners
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_export_netlist_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("export_netlist");

  /* Add an option '--file' */
  openfpga::CommandOptionId opt_file = shell_cmd.add_option(
    "file", true, "file path where the netlist package are written to. Must be ended with .tar.gz");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of netlist development.");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  shell_cmd.add_option(
    "force_overwrite", false,
    "Force to overwrite existing package if possible. This may risk on potential data loss");

  shell_cmd.add_option("reduce_error_to_warning", false,
                       "Reduce errors to warnings when exporting netlists. It may cause incorrect "
                       "netlists to be outputted. Be cautious when enable this option!");

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd,
                      "Export the gate-level netlists in a package for external use. Must have a "
                      "successful netlist development");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, export_netlist);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_corners
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_export_arch_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("export_arch");

  /* Add an option '--file' */
  openfpga::CommandOptionId opt_file = shell_cmd.add_option(
    "file", true,
    "file path where the architecture package are written to. Must be ended with .tar.gz");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of architecture development.");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  shell_cmd.add_option(
    "force_overwrite", false,
    "Force to overwrite existing package if possible. This may risk on potential data loss");

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd,
                      "Export the architecture data in a package for external use. Must have a "
                      "successful architecture development");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, export_arch);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: backannotate
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_backannotate_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("backannotate");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of netlist development.");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corner will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false, "File path where the backannotate reports are written to");
  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Back-annotate the post-synthesis QoR results to FPGA architecture. Once the back-annotation "
    "is done, the architecture development flow should be re-run to initialize the database and "
    "evaluation flow should be re-run to compare the difference before and after back-annotation");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, backannotate);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: disable_benchmark
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_disable_benchmark_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("disable_benchmark");

  openfpga::CommandOptionId opt_name = shell_cmd.add_option(
    "name", true,
    "Specify the name of the benchmark to be disabled during evaluation and design verification");
  shell_cmd.set_option_require_value(opt_name, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Disable a selected benchmark for evaluation and design verification");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, disable_benchmark);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: disable_corner
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_disable_corner_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("disable_corner");

  openfpga::CommandOptionId opt_name = shell_cmd.add_option(
    "name", true,
    "Specify the name of the corner to be disabled during evaluation and design verification");
  shell_cmd.set_option_require_value(opt_name, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Disable a selected corner for evaluation and design verification");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, disable_corner);
  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: analyze_pre_arch_qor
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_analyze_pre_arch_qor_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("analyze_pre_arch_qor");

  /* Add an option '--file' */
  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false,
                         "file path where QoR results are written to. If not specified, QoR "
                         "results will be shown in shell");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of pre-arch analysis");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Analyze the Qualit-of-Result for the pre-arch analysis");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, analyze_pre_arch_qor);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: analyze_qor
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_analyze_qor_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("analyze_qor");

  /* Add an option '--file' */
  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false,
                         "file path where QoR results are written to. If not specified, QoR "
                         "results will be shown in shell");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of architecture evaluation");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_precision =
    shell_cmd.add_option("precision", false, "Precision of data values");
  shell_cmd.set_option_require_value(opt_precision, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corners will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Analyze the Quality-of-Result for the selected architecture evaluation");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, analyze_arch_eval_qor);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: report_qor_scale
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_report_qor_scale_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("report_qor_scale");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of netlist run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_type =
    shell_cmd.add_option("type", false,
                         "Specify the type of PPA to be extracted. Can be [timing | leakage | "
                         "power ]. If not specified, all the PPA metrics will be extracted.");
  shell_cmd.set_option_require_value(opt_type, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corners will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_report_file =
    shell_cmd.add_option("file", false, "Output file path of the generated qor reports");
  shell_cmd.set_option_require_value(opt_report_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_precision =
    shell_cmd.add_option("precision", false, "Precision of scale factor");
  shell_cmd.set_option_require_value(opt_precision, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "report qor scale factor to a certain file or on screen");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, report_qor_scale);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: export_benchmark_schematic
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
// If Graphviz is found, enable the following functionality
// This is a temporay fix due to the graphviz package issue on CentOS 7.9
#ifdef USE_GRAPHVIZ
static openfpga::ShellCommandId add_arcxpress_export_benchmark_schematic_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("export_benchmark_schematic");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false,
                         "Specify the run id of architecture evaluation.If not specified, the "
                         "default run will be considered.");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner =
    shell_cmd.add_option("corner", false,
                         "Specify the name of corner to be considered.If not specified, the "
                         "default corner will be considered.");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_file = shell_cmd.add_option(
    "file", true, "Specify the file path to the schematic to be outputted. Can be [ png | svg ].");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_benchmark =
    shell_cmd.add_option("benchmark", true, "Specify the benchmark name to be exported");
  shell_cmd.set_option_require_value(opt_benchmark, openfpga::OPT_STRING);
  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);
  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Export block diagram of a selected benchmark after synthesis");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, export_benchmark_schematic);

  return shell_cmd_id;
}
#endif

/********************************************************************
 * - Add a command to Shell environment: add_benchmark_vpr_constraints
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_benchmark_vpr_constraints_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("add_benchmark_vpr_constraints");

  // Option to specify the run name of the architecture evaluation
  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false,
                         "Specify the run name for architecture evaluation. If not specified, the "
                         "latest run will be used.");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  // Option to specify the corner to be used for this run
  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner defined through project file. If not specified, use the default corner.");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  // Option to specify the benchmark to be applied
  openfpga::CommandOptionId opt_benchmark = shell_cmd.add_option(
    "benchmark", true, "Specify the name of the benchmark to be applied. This is required.");
  shell_cmd.set_option_require_value(opt_benchmark, openfpga::OPT_STRING);

  // Option to specify the path to the VPR constraint file
  openfpga::CommandOptionId opt_file = shell_cmd.add_option(
    "file", true, "Specify the path to the VPR constraint file to be applied.");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  // Option for verbosity level
  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Specify the verbosity level in log. Default is 1.");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  // Add the command to the Shell
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Add custom VPR constraints to a benchmark for architecture evaluation");

  // Set the command's execution function
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, add_benchmark_vpr_constraints);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: modify utilization
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_set_benchmark_custom_pin_utilization_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_benchmark_custom_pin_utilization");
  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, all the corners "
    "will be considered");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  // specify the benchmark to be applied
  openfpga::CommandOptionId opt_benchmark = shell_cmd.add_option(
    "benchmark", true, "Specify the name of the benchmark to be applied. This is required.");
  shell_cmd.set_option_require_value(opt_benchmark, openfpga::OPT_STRING);

  // specify the tile to be applied
  openfpga::CommandOptionId opt_tile = shell_cmd.add_option(
    "tile", true, "Specify the name of the tile to be applied. This is required.");
  shell_cmd.set_option_require_value(opt_tile, openfpga::OPT_STRING);

  // specify the input pin utilization
  openfpga::CommandOptionId opt_input_util =
    shell_cmd.add_option("input_pin_util", false,
                         "Specify the input pin utilization to be applied. If not specified, will "
                         "use default value from arch template.");
  shell_cmd.set_option_require_value(opt_input_util, openfpga::OPT_FLOAT);

  // specify the output pin utilization
  openfpga::CommandOptionId opt_output_util =
    shell_cmd.add_option("output_pin_util", false,
                         "Specify the output pin utilization to be applied. If not specified, will "
                         "use default value from arch template.");
  shell_cmd.set_option_require_value(opt_output_util, openfpga::OPT_FLOAT);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Evaluate an architecture by generating golden bitstream and reports");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_benchmark_custom_pin_utilization);

  return shell_cmd_id;
}

void add_arcxpress_tapeout_commands(openfpga::Shell<ArcxpressContext>& shell) {
  /* Add a new class of commands */
  openfpga::ShellCommandClassId arcxpress_tapeout_cmd_class =
    shell.add_command_class("Arcxpress tapeout");

  add_arcxpress_create_tapeout_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_load_tapeout_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_save_tapeout_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_show_status_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_show_dashboard_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_corners_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_module_names_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_ref_module_names_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_io_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_static_power_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_benchmark_power_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_config_power_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_config_stats_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_export_netlist_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_export_arch_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_backannotate_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_disable_benchmark_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_disable_corner_command(shell, arcxpress_tapeout_cmd_class);
  // TODO: These commands should be activated when loading tapeout can check existing result files
  //  add_arcxpress_analyze_pre_arch_qor_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_analyze_qor_command(shell, arcxpress_tapeout_cmd_class);
  add_arcxpress_report_qor_scale_command(shell, arcxpress_tapeout_cmd_class);
// If Graphviz is found, enable the following functionality
// This is a temporay fix due to the graphviz package issue on CentOS 7.9
#ifdef USE_GRAPHVIZ
  add_arcxpress_export_benchmark_schematic_command(shell, arcxpress_tapeout_cmd_class);
#endif
  add_benchmark_vpr_constraints_command(shell, arcxpress_tapeout_cmd_class);
  add_set_benchmark_custom_pin_utilization_command(shell, arcxpress_tapeout_cmd_class);
}

}  // namespace arcxpress
