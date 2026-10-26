/********************************************************************
 * Add simulation-related commands to the OpenFPGA shell interface
 *******************************************************************/
#include "arcxpress_simulation_command.h"

#include "arcxpress_simulation.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * - Add a command to Shell environment: sim
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arcxpress_simulation_command(
  openfpga::Shell<ArcxpressContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id,
  const std::vector<openfpga::ShellCommandId>& dependent_cmds) {
  openfpga::Command shell_cmd("run_benchmark_simulation");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, all the corners "
    "will be considered");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  /* Add an option '--benchmark' */
  openfpga::CommandOptionId opt_benchmark_name =
    shell_cmd.add_option("benchmark", true, "Specify the name of the benchmark to be simulated");
  shell_cmd.set_option_require_value(opt_benchmark_name, openfpga::OPT_STRING);

  /* Add an option '--simulator' */
  openfpga::CommandOptionId opt_simulator =
    shell_cmd.add_option("simulator", false,
                         "Allow user to choose a supported simulator, could vcs, icarus, modelsim "
                         "and questa. By default, it will be vcs");
  shell_cmd.set_option_require_value(opt_simulator, openfpga::OPT_STRING);

  /* Add an option '--dump_waveform' */
  openfpga::CommandOptionId opt_dw = shell_cmd.add_option(
    "dump_waveform", false,
    "Specify if the waveform file should be outputted when running simulation. Available options "
    "are [ none | fsdb | vcd ]. By default is none, implicating no waveform files are outputted.");
  shell_cmd.set_option_require_value(opt_dw, openfpga::OPT_STRING);

  /* Add an option '--type' */
  openfpga::CommandOptionId opt_type = shell_cmd.add_option(
    "type", true, "Simulation type, values can be rtl, post_synth and post_implementation.");
  shell_cmd.set_option_require_value(opt_type, openfpga::OPT_STRING);

  /* Add an option '--verbose' */
  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Log out simulation related message.");

  /* Register command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Start simulator to verify a design");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, run_app_hdl_simulation);

  /* Add command dependency to the Shell */
  shell.set_command_dependency(shell_cmd_id, dependent_cmds);

  return shell_cmd_id;
}

void add_simulation_commands(openfpga::Shell<ArcxpressContext>& shell) {
  /* Add a new class of commands */
  openfpga::ShellCommandClassId sim_cmd_class = shell.add_command_class("simulation");

  std::vector<openfpga::ShellCommandId> sim_cmd_deps;
  add_arcxpress_simulation_command(shell, sim_cmd_class, sim_cmd_deps);

  /* TODO: add pack, place and route commands */
}

} /* end namespace arcxpress */
