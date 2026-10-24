/********************************************************************
 * Add project-related commands to the Arkangel shell interface
 *******************************************************************/
#include "arkangel_netlist_dev_command.h"

#include "arkangel_netlist_dev.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_generate_netlist_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("generate_netlist");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_arch_run =
    shell_cmd.add_option("arch_run", false, "Specify the name of archtecture run to start with");
  shell_cmd.set_option_require_value(opt_arch_run, openfpga::OPT_STRING);

  shell_cmd.add_option("netlist_makeup", false,
                       "Enable netlist makeup step, which leads to better QoR in physical design");

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Generate RTL netlist based on the selected architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, generate_netlist);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_netlist_optimization_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("optimize_netlist");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_stage =
    shell_cmd.add_option("stage", false, "Specify the stage of the optimization to be run");
  shell_cmd.set_option_require_value(opt_stage, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_strtg =
    shell_cmd.add_option("strategy", false,
                         "Specify the custom synthesis strategy file, when the default synthesis "
                         "strategy cannot satisfy your needs");
  shell_cmd.set_option_require_value(opt_strtg, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner =
    shell_cmd.add_option("corner", false,
                         "Specify the corner name to be considered. If not specified, the default "
                         "corner will be considered");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  shell_cmd.add_option("dryrun", false, "Specify if netlist synthesis should be dryrun only");

  openfpga::CommandOptionId opt_j = shell_cmd.add_option(
    "jobs", false, "Specify the max. number of jobs to be applied on synthesis");
  shell_cmd.set_option_require_value(opt_j, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Optimize RTL netlist based on the selected architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, netlist_opt);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_coarse_analyze_netlist_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("coarse_analyze_netlist");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  shell_cmd.add_option("dryrun", false, "Specify if area analysis should be dryrun only");

  openfpga::CommandOptionId opt_j = shell_cmd.add_option(
    "jobs", false, "Specify the max. number of jobs to be applied on analyzing");
  shell_cmd.set_option_require_value(opt_j, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_corner =
    shell_cmd.add_option("corner", false,
                         "Specify the corner name to be considered. If not specified, the default "
                         "corner will be considered");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Analyze the tile area for a gate-level netlist based on the selected architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, analyze_tile_area);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_coarse_floorplan_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("coarse_floorplan");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_rptf =
    shell_cmd.add_option("report_file", false,
                         "Specify the path to the CSV file where floorplanning results will be "
                         "written. If not specified, results will be shown on screen");
  shell_cmd.set_option_require_value(opt_rptf, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_rptf_shaping = shell_cmd.add_option(
    "shaping_report_file", false,
    "Specify the path to the shaping.txt file where floorplanning instance and coordinate "
    "information will be written. If not provided, the file will be saved in the netlist "
    "report directory.");
  shell_cmd.set_option_require_value(opt_rptf_shaping, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_precision = shell_cmd.add_option(
    "precision", false,
    "Specify the precision for all the numbers in floorplan report. By default, it is 2");
  shell_cmd.set_option_require_value(opt_precision, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_strategy =
    shell_cmd.add_option("strategy", false,
                         "Specify the XML file containing the custom floorplan strategy,If not "
                         "specified, a default strategy will be applied.");
  shell_cmd.set_option_require_value(opt_strategy, openfpga::OPT_STRING);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Perform a coarse floorplan for a gate-level netlist based on the selected architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, coarse_floorplan);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_extract_netlist_qor_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("extract_netlist_qor");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of netlist run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_type = shell_cmd.add_option(
    "type", false,
    "Specify the type of PPA to be extracted. Can be [timing | leakage | "
    "config_power ]. If not specified, all the PPA metrics will be extracted.");
  shell_cmd.set_option_require_value(opt_type, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_task_name = shell_cmd.add_option(
    "task_name", false,
    "Specify the name of PPA task to be extracted. Must be a valid one specified in the selected "
    "type. If not specified, all the tasks under the selected type will be extracted.");
  shell_cmd.set_option_require_value(opt_task_name, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corners will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_clock_freq =
    shell_cmd.add_option("clock_freq", false,
                         " Specify the clock frequency to be applied for each global clock of the "
                         "eFPGA fabric, in the unit of "
                         "[MHz]. If not specified, a default value of 100 [MHz] is used.");
  shell_cmd.set_option_require_value(opt_clock_freq, openfpga::OPT_STRING);

  shell_cmd.add_option("dryrun", false, "Specify if area analysis should be dryrun only");

  openfpga::CommandOptionId opt_j = shell_cmd.add_option(
    "jobs", false, "Specify the max. number of jobs to be applied on analyzing");
  shell_cmd.set_option_require_value(opt_j, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Run timing and power analyze for a gate-level netlist based on the selected architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, extract_netlist_qor);

  return shell_cmd_id;
}

void add_arkangel_netlist_dev_commands(openfpga::Shell<ArkangelContext>& shell) {
  /* Add a new class of commands */
  openfpga::ShellCommandClassId arkangel_netlist_dev_cmd_class =
    shell.add_command_class("Arkangel netlist development");

  add_arkangel_generate_netlist_command(shell, arkangel_netlist_dev_cmd_class);
  add_arkangel_netlist_optimization_command(shell, arkangel_netlist_dev_cmd_class);
  add_arkangel_coarse_analyze_netlist_command(shell, arkangel_netlist_dev_cmd_class);
  add_arkangel_coarse_floorplan_command(shell, arkangel_netlist_dev_cmd_class);
  add_arkangel_extract_netlist_qor_command(shell, arkangel_netlist_dev_cmd_class);
}

}  // namespace arcxpress