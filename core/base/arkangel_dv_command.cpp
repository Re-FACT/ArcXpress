/********************************************************************
 * Add project-related commands to the Arkangel shell interface
 *******************************************************************/
#include "arkangel_dv_command.h"

#include "arkangel_dv.h"

/* begin namespace arkangel */
namespace arkangel {

/********************************************************************
 * - Add a command to Shell environment: setup_dv
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_setup_dv_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("setup_design_verification");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If no specified, use the default corner");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_bitstream_run =
    shell_cmd.add_option("bitstream_run", false, "Specify the name of bitstream run to start with");
  shell_cmd.set_option_require_value(opt_bitstream_run, openfpga::OPT_STRING);

  /* TODO: Consider the netlist run and pd run!!! This is create the links to netlists */
  openfpga::CommandOptionId opt_nlist_run =
    shell_cmd.add_option("netlist_run", false, "Specify the name of netlist run to start with");
  shell_cmd.set_option_require_value(opt_nlist_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_pd_run =
    shell_cmd.add_option("pd_run", false, "Specify the name of physical design run to start with");
  shell_cmd.set_option_require_value(opt_pd_run, openfpga::OPT_STRING);

  shell_cmd.add_option("dryrun", false,
                       "Specify if setup design verification should be dryrun only");

  openfpga::CommandOptionId opt_j = shell_cmd.add_option(
    "jobs", false, "Specify the max. number of jobs to be applied on design verification");
  shell_cmd.set_option_require_value(opt_j, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_ntwt =
    shell_cmd.add_option("new_thread_wait_time", false,
                         "Specify the time (in sec.) to wait before starting a new thread (when "
                         "--jobs > 2). By default, it is 1 sec.");
  shell_cmd.set_option_require_value(opt_ntwt, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_sswt =
    shell_cmd.add_option("sim_start_wait_time", false,
                         "Specify the time (in sec.) to wait before starting a new thread for "
                         "simulation (when --jobs > 2). By default, it is 1 sec.");
  shell_cmd.set_option_require_value(opt_sswt, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Setup design verification enviornment for selected bitstream and netlists");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, setup_dv);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: run_preconfig_sim
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_run_preconfig_sim_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("run_preconfigured_simulation");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If no specified, use the default corner");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_name =
    shell_cmd.add_option("benchmark", false,
                         "Specify the name of the benchmark to be considered. If not specified, "
                         "all the benchmarks (not disabled) will be included");
  shell_cmd.set_option_require_value(opt_name, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_j = shell_cmd.add_option(
    "jobs", false, "Specify the max. number of jobs to be applied on synthesis");
  shell_cmd.set_option_require_value(opt_j, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_ntwt = shell_cmd.add_option(
    "new_thread_wait_time", false,
    "Specify the time (in sec.) to wait before starting a new thread (when --jobs > 2)");
  shell_cmd.set_option_require_value(opt_ntwt, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_sswt =
    shell_cmd.add_option("sim_start_wait_time", false,
                         "Specify the time (in sec.) to wait before starting a new thread for "
                         "simulation (when --jobs > 2)");
  shell_cmd.set_option_require_value(opt_sswt, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_lcwt =
    shell_cmd.add_option("log_check_wait_time", false,
                         "Specify the time (in sec.) to wait before starting a new thread for "
                         "checking simulation logs (when --jobs > 2)");
  shell_cmd.set_option_require_value(opt_lcwt, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_sim = shell_cmd.add_option(
    "simulator", false,
    "Specify the simulator to be used when running simulation. Available options are [ "
    "synopsys_vcs | "
    "mentor_modelsim | mentor_questa ]. By default, synopsys_vcs is considered");
  shell_cmd.set_option_require_value(opt_sim, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_nt =
    shell_cmd.add_option("netlist_type", false,
                         "Specify the type of netlist to be considered when running simulation. "
                         "Available options are [ rtl | gl | pl ]. By default, rtl is considered");
  shell_cmd.set_option_require_value(opt_nt, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_dw = shell_cmd.add_option(
    "dump_waveform", false,
    "Specify if the waveform file should be outputted when running simulation. Available options "
    "are [ none | fsdb | vcd ]. By default is none, implicating no waveform files are outputted.");
  shell_cmd.set_option_require_value(opt_dw, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Run preconfigured simulation for selected design verification run");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, run_preconfig_dv);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: run_benchmark_power_analysis
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_run_benchmark_power_analysis_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("run_benchmark_power_analysis");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of design verification");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corner will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_name = shell_cmd.add_option(
    "benchmark", true,
    "Specify the name of the benchmark to be considered during dynamic power analysis");
  shell_cmd.set_option_require_value(opt_name, openfpga::OPT_STRING);

  shell_cmd.add_option("dryrun", false, "Specify if dynamic power analysis should be dryrun only");

  openfpga::CommandOptionId opt_j = shell_cmd.add_option(
    "jobs", false, "Specify the max. number of jobs to be applied on dynamic power analysis");
  shell_cmd.set_option_require_value(opt_j, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Report dynamic power analysis for selected design verification run, benchmark and corner");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, run_benchmark_power_analysis);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: run_benchmark_sta
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_run_benchmark_sta_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("run_benchmark_sta");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the run name of design verification");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, the default "
    "corner will be considered. Use the keyword all to sweep all the available corners");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_clock_freq = shell_cmd.add_option(
    "clock_freq", false,
    " Specify the clock frequency to be applied. The clock frequency is expected to be provided in "
    "MHz. If not specified, a default value 100 MHz is used.");
  shell_cmd.set_option_require_value(opt_clock_freq, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_netlist =
    shell_cmd.add_option("netlist_type ", false,
                         "Specify the type of the netlist. Available options are [ "
                         "gl | pl ].");
  shell_cmd.set_option_require_value(opt_clock_freq, openfpga::OPT_STRING);
  openfpga::CommandOptionId opt_name = shell_cmd.add_option(
    "benchmark", true, "Specify the name of the benchmark to be considered during STA analysis");
  shell_cmd.set_option_require_value(opt_name, openfpga::OPT_STRING);

  shell_cmd.add_option("dryrun", false, "Specify if STA analysis should be dryrun only");

  openfpga::CommandOptionId opt_j = shell_cmd.add_option(
    "jobs", false, "Specify the max. number of jobs to be applied on STA analysis");
  shell_cmd.set_option_require_value(opt_j, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Report STA analysis for selected design verification run, benchmark and corner");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, run_benchmark_sta);

  return shell_cmd_id;
}

void add_arkangel_dv_commands(openfpga::Shell<ArkangelContext>& shell) {
  /* Add a new class of commands */
  openfpga::ShellCommandClassId arkangel_dv_cmd_class =
    shell.add_command_class("Arkangel tapeout design verification");

  add_arkangel_setup_dv_command(shell, arkangel_dv_cmd_class);
  add_arkangel_run_preconfig_sim_command(shell, arkangel_dv_cmd_class);
  add_arkangel_run_benchmark_power_analysis_command(shell, arkangel_dv_cmd_class);
  add_arkangel_run_benchmark_sta_command(shell, arkangel_dv_cmd_class);
}

}  // namespace arkangel
