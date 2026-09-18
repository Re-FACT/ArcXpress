/********************************************************************
 * Add project-related commands to the Arkangel shell interface
 *******************************************************************/
#include "arkangel_arch_dev_command.h"

#include "arkangel_arch_dev.h"

/* begin namespace arkangel */
namespace arkangel {

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_create_arch_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("create_arch");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Initialize the development for a new architecture based on templates");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, create_arch);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: create_tapeout
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_setup_arch_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("setup_arch");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Create the data models for an initialized architecture with optional customization applied");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, setup_arch);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_show_arch_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("show_arch");

  openfpga::CommandOptionId opt_type =
    shell_cmd.add_option("type", true, "Specify the type of architecture aspects");
  shell_cmd.set_option_require_value(opt_type, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_fabric =
    shell_cmd.add_option("fabric", false, "Specify the type of fabric");
  shell_cmd.set_option_require_value(opt_fabric, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Show the specified aspect of an eFPGA architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, show_tapeout_arch);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_arch_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_arch");

  openfpga::CommandOptionId opt_type =
    shell_cmd.add_option("type", true, "Specify the type of architecture aspects");
  shell_cmd.set_option_require_value(opt_type, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_fabric =
    shell_cmd.add_option("fabric", false, "Specify the type of fabric");
  shell_cmd.set_option_require_value(opt_fabric, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_file =
    shell_cmd.add_option("file", false,
                         "Specify the handcrafted file to be added. This is optional. If not "
                         "specified, the default setting will be applied which may not be optimal");
  shell_cmd.set_option_require_value(opt_file, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Add a specific attribute to an eFPGA architecture from a file");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_tapeout_arch);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_io_num_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_io_num");

  openfpga::CommandOptionId opt_side = shell_cmd.add_option(
    "side", true,
    "Specify the side of the eFPGA where the I/O number constraint should be applied");
  shell_cmd.set_option_require_value(opt_side, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_type =
    shell_cmd.add_option("type", true, "Specify the type of I/Os. Could be [ input | output ]");
  shell_cmd.set_option_require_value(opt_type, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_num = shell_cmd.add_option("num", true, "Specify the I/O number");
  shell_cmd.set_option_require_value(opt_num, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Specify the number of I/Os by type and side for an FPGA architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_arch_io_num);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_sync_dp_dimension_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_sync_dp_dimension");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd,
    "Specify the dp fabric dimension should be the same as the ultimate fabric. This helps to "
    "reduce the risk on consistency failures. By default, it is disabled");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_arch_sync_dp_dimension);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_core_dimension_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_core_dimension");

  openfpga::CommandOptionId opt_height = shell_cmd.add_option(
    "height", true, "Specify the number of rows in the core fabric, excluding I/Os");
  shell_cmd.set_option_require_value(opt_height, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_width = shell_cmd.add_option(
    "width", true, "Specify the number of columns in the core fabric, excluding I/Os");
  shell_cmd.set_option_require_value(opt_width, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_sync_dp_dim = shell_cmd.add_option(
    "sync_dp_dimension", false,
    "Specify the dp fabric dimension should be the same as the ultimate fabric. This helps to "
    "reduce the risk on consistency failures. By default, it is disabled");

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd,
                      "Specify the height and width for the core fabric of an FPGA architecture. "
                      "This excludes the I/O rows and columns");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_arch_core_dimension);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_num_prog_clock_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_num_prog_clock");

  openfpga::CommandOptionId opt_num =
    shell_cmd.add_option("num", true, "Specify the number of programming clocks");
  shell_cmd.set_option_require_value(opt_num, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Specify the number of programming clocks");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_arch_num_pclk);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_num_clock_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_num_clock");

  openfpga::CommandOptionId opt_num =
    shell_cmd.add_option("num", true, "Specify the number of operating clocks");
  shell_cmd.set_option_require_value(opt_num, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Specify the number of operating clocks");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_arch_num_clk);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_num_reset_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_num_reset");

  openfpga::CommandOptionId opt_num =
    shell_cmd.add_option("num", true, "Specify the number of operating resets");
  shell_cmd.set_option_require_value(opt_num, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Specify the number of operating resets");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_arch_num_rst);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_set_num_config_region_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("set_num_config_region");

  openfpga::CommandOptionId opt_num =
    shell_cmd.add_option("num", true, "Specify the number of configurable regions");
  shell_cmd.set_option_require_value(opt_num, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id =
    shell.add_command(shell_cmd, "Specify the number of configurable regions.");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, set_arch_num_config_region);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: show_status
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_remove_io_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("remove_io");

  openfpga::CommandOptionId opt_side = shell_cmd.add_option(
    "side", true,
    "Specify the side of the eFPGA where the I/O number constraint should be applied");
  shell_cmd.set_option_require_value(opt_side, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Remove all the I/Os on the selected side for an FPGA architecture");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, remove_arch_io);

  return shell_cmd_id;
}

/********************************************************************
 * - Add a command to Shell environment: validate_arch
 * - Add associated options
 * - Add command dependency
 *******************************************************************/
static openfpga::ShellCommandId add_arkangel_eval_arch_command(
  openfpga::Shell<ArkangelContext>& shell, const openfpga::ShellCommandClassId& cmd_class_id) {
  openfpga::Command shell_cmd("eval_arch");

  openfpga::CommandOptionId opt_run =
    shell_cmd.add_option("run", false, "Specify the name of run to start with");
  shell_cmd.set_option_require_value(opt_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_arch_run =
    shell_cmd.add_option("arch_run", false, "Specify the name of archtecture run to start with");
  shell_cmd.set_option_require_value(opt_arch_run, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_corner = shell_cmd.add_option(
    "corner", false,
    "Specify the corner name to be considered. If not specified, all the corners "
    "will be considered");
  shell_cmd.set_option_require_value(opt_corner, openfpga::OPT_STRING);

  shell_cmd.add_option(
    "force_copy_arch_data", false,
    "Instead of creating a link to the architecture data, copy the data to the runtime directory. "
    "This improves the portability of project workspace but at a cost of additional disk space. "
    "This is automatically enabled on windows system.");

  openfpga::CommandOptionId opt_rpt_qor =
    shell_cmd.add_option("report_qor", false, "Analyze the results and report to a file");
  shell_cmd.set_option_require_value(opt_rpt_qor, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    shell_cmd.add_option("verbosity", false, "Enable verbose log");
  shell_cmd.set_option_require_value(opt_verbosity, openfpga::OPT_INT);

  openfpga::CommandOptionId opt_max_processor =
    shell_cmd.add_option("jobs", false,
                         "Specify the maximum number of processors to be used. If not specified, "
                         "the default is 1 processor");
  shell_cmd.set_option_require_value(opt_max_processor, openfpga::OPT_INT);

  /* Add command to the Shell */
  openfpga::ShellCommandId shell_cmd_id = shell.add_command(
    shell_cmd, "Evaluate an architecture by generating golden bitstream and reports");
  shell.set_command_class(shell_cmd_id, cmd_class_id);
  shell.set_command_execute_function(shell_cmd_id, eval_tapeout_arch);

  return shell_cmd_id;
}

void add_arkangel_arch_dev_commands(openfpga::Shell<ArkangelContext>& shell) {
  /* Add a new class of commands */
  openfpga::ShellCommandClassId arkangel_arch_dev_cmd_class =
    shell.add_command_class("Arkangel architecture development");

  add_arkangel_create_arch_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_setup_arch_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_show_arch_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_arch_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_io_num_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_core_dimension_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_sync_dp_dimension_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_num_clock_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_num_reset_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_num_prog_clock_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_set_num_config_region_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_remove_io_command(shell, arkangel_arch_dev_cmd_class);
  add_arkangel_eval_arch_command(shell, arkangel_arch_dev_cmd_class);
}

}  // namespace arkangel
