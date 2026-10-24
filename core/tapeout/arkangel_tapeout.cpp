/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"
/* Headers from openfpgashell library */
#include <filesystem>
#include <map>

#include "arkangel_constants.h"
#include "arkangel_file_os.h"
#include "arkangel_save_utils.h"
#include "arkangel_sys_env.h"
#include "arkangel_tapeout.h"
#include "arkangel_tapeout_utils.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "read_xml_benchmark_manager.h"
#include "read_xml_pdk_manager.h"
#include "read_xml_tapeout_manager.h"
#include "tableprinter.h"
#include "write_xml_benchmark_manager.h"
#include "write_xml_pdk_manager.h"
#include "write_xml_tapeout_manager.h"
/* begin namespace arcxpress */
namespace arcxpress {
/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int show_tapeout_dashboard(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                           const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_report_file = cmd.option("file");
  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_report_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_report_file);
  }
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  VTR_LOG("Dashboard of tapeout project '%s'\n", to_mgr.name().c_str());
  std::string step_tag = "Step";
  std::string substep_tag = "Substep";
  std::string status_tag = "Status";
  std::string latest_run_tag = "Latest Run";
  std::string frozen_tag = "Frozen";
  /* Table Header */
  TablePrinter tp({step_tag, substep_tag, status_tag, latest_run_tag, frozen_tag});
  std::string TAG_NA("N/A");
  for (auto curr_step : to_mgr.dev_steps()) {
    std::string step_name = to_mgr.step_name2str(curr_step);
    /* If no run is available, skip it */
    if (to_mgr.step_runs(curr_step).empty()) {
      std::string substep_na = TAG_NA;
      std::string latest_run_na = TAG_NA;
      std::string status_na = to_mgr.status2str(TapeoutManager::e_run_status::FRESH);
      std::string frozen_na = TAG_NA;
      tp.add_row({step_name, substep_na, status_na, latest_run_na, frozen_na});
      continue;
    }
    std::string latest_run = to_mgr.step_latest_run(curr_step);
    std::string latest_run_str = latest_run;
    std::string frozen_run = to_mgr.step_run_frozen(curr_step, latest_run) ? "Y" : "N";
    std::string substep_name = TAG_NA;
    std::string status_str = to_mgr.step_run_status_str(curr_step, latest_run);
    tp.add_row({step_name, substep_name, status_str, latest_run_str, frozen_run});
    for (auto curr_substep : to_mgr.substeps(curr_step, latest_run)) {
      substep_name = to_mgr.substep_name2str(curr_substep);
      std::string s_status_str = to_mgr.substep_status_str(curr_step, latest_run, curr_substep);
      tp.add_row({step_name, substep_name, s_status_str, latest_run_str, frozen_run});
    }
  }
  /* Estimate a progress bar: May become a method of TapeoutManager later! */
  float num_pass_stages = 0.;
  for (auto curr_step : to_mgr.dev_steps()) {
    if (to_mgr.step_runs(curr_step).empty()) {
      continue;
    }
    std::string latest_run = to_mgr.step_latest_run(curr_step);
    if (TapeoutManager::e_run_status::SUCCESS == to_mgr.step_run_status(curr_step, latest_run)) {
      num_pass_stages += 1;
    }
  }
  float progress = 100. * num_pass_stages / to_mgr.dev_steps().size();
  VTR_LOG("[Tapeout progress] %.2f\%\n", progress);
  tp.print(report_file_path);
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int show_tapeout_status(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_step = cmd.option("step");
  VTR_ASSERT(true == cmd_ctx.option_enable(cmd, opt_step));
  /* Show status of a selected step */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  std::string curr_step_name = cmd_ctx.option_value(cmd, opt_step);
  TapeoutManager::e_step_name curr_step = to_mgr.str2step_name(curr_step_name, true);
  if (!to_mgr.valid_step_name(curr_step)) {
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  openfpga::CommandOptionId opt_run = cmd.option("run");
  std::string target_run;
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    target_run = cmd_ctx.option_value(cmd, opt_run);
  }
  VTR_LOG("Step name: %s\n", curr_step_name.c_str());
  VTR_LOG("Runs\tStatus\tLatest\tForzen\n");
  for (auto run : to_mgr.step_runs(curr_step)) {
    if (!target_run.empty() && target_run != run) {
      continue;
    }
    std::string latest_run = "N";
    if (run == to_mgr.step_latest_run(curr_step)) {
      latest_run = "Y";
    }
    std::string frozen_run = to_mgr.step_run_frozen(curr_step, run) ? "Y" : "N";
    VTR_LOG("%s\t%s\t%s\t%s\n", run.c_str(), to_mgr.step_run_status_str(curr_step, run).c_str(),
            latest_run.c_str(), frozen_run.c_str());
  }
  /* Show substep status  */
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int save_tapeout(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                 const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  std::string config_file_name = to_mgr.file();
  openfpga::CommandOptionId opt_file = cmd.option("config_file");
  if (true == cmd_ctx.option_enable(cmd, opt_file)) {
    VTR_ASSERT(false == cmd_ctx.option_value(cmd, opt_file).empty());
    config_file_name = cmd_ctx.option_value(cmd, opt_file);
  }
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* TODO: Generate MD5 for all the resulting files for all the steps and runs */
  VTR_LOG("Saving results of each step and run...\n");
  for (auto curr_step : to_mgr.dev_steps()) {
    std::string step_name = to_mgr.step_name2str(curr_step);
    VTR_LOG("Saving results of step '%s'...\n", step_name.c_str());
    /* If no run is available, skip it */
    for (auto curr_run : to_mgr.step_runs(curr_step)) {
      VTR_LOG("\tFor run '%s'...\n", curr_run.c_str());
      status = save_results_for_step_run(to_mgr, bm_mgr, pdk_mgr, curr_step, curr_run, verbosity);
      if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
        fflush(stdout);
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  }
  VTR_LOG("Done\n");
  /* Read the configuration file */
  VTR_LOG("Saving configuration file '%s'...\n", config_file_name.c_str());
  // Ensure the directory exists
  {
    std::filesystem::path file_path(config_file_name);
    std::filesystem::path dir_path = file_path.parent_path();
    // Create the directory if it isn't empty
    if (!dir_path.empty()) {
      VTR_LOG("Creating directory: %s\n", dir_path.string().c_str());
      openfpga::create_directory(dir_path.string(), true, verbosity == 1);
    }
  }
  status = write_xml_tapeout_manager(config_file_name.c_str(), to_mgr, true);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
/********************************************************************
 * Core function to load a new project based on a configuration file
 *******************************************************************/
static int load_tapeout_from_config_file(const std::string config_file_name, TapeoutManager& to_mgr,
                                         PdkManager& pdk_mgr, BenchmarkManager& bm_mgr,
                                         const bool& refresh_status) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Read the configuration file */
  VTR_LOG("Reading configuration file '%s'...\n", config_file_name.c_str());
  status = read_xml_tapeout_manager(config_file_name.c_str(), to_mgr);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!to_mgr.validate_pdk_config_file()) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Load PDK configuration */
  std::string pdk_config_file = arkangel::absolute_file_path(to_mgr.pdk_config());
  if (!arkangel::file_is_legal(pdk_config_file)) {
    VTR_LOG_ERROR("Pdk config file '%s' does not exist!\n", pdk_config_file.c_str());
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  pdk_mgr.set_kw_proj_path(arkangel::get_cwd());
  status = read_xml_pdk_manager(pdk_config_file.c_str(), pdk_mgr);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!pdk_mgr.validate()) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Require PDK name to initialize the tapeout manager data */
  to_mgr.set_pdk_name(pdk_mgr.name());
  VTR_LOG("Validating configuration file...\n");
  if (!to_mgr.validate()) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (refresh_status) {
    to_mgr.clear_status();
  }
  /* Load Benchmark configuration */
  bm_mgr.set_kw_proj_path(arkangel::get_cwd());
  for (std::string bm_config_file : to_mgr.benchmark_configs()) {
    std::string bm_config_file_abspath = arkangel::absolute_file_path(bm_config_file);
    if (!arkangel::file_is_legal(bm_config_file_abspath)) {
      VTR_LOG_ERROR("Benchmark config file '%s' does not exist!\n", bm_config_file_abspath.c_str());
      fflush(stdout);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    status = read_xml_benchmark_manager(bm_config_file_abspath.c_str(), bm_mgr, false);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      fflush(stdout);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    if (!bm_mgr.validate()) {
      fflush(stdout);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Sync the disable benchmarks */
  for (std::string disabled_bm : to_mgr.disabled_benchmarks()) {
    status = bm_mgr.try_set_benchmark_disabled(disabled_bm, true);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      fflush(stdout);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Initialize system env for python site packages */
  status = set_system_env_var(PYTHON_PATH_ENV_VAR_NAME, to_mgr.python_site_package_path(), false);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = set_system_env_var(SYSTEM_PATH_ENV_VAR_NAME, to_mgr.python_cocotb_config_path(), true);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = set_system_env_var(LIBPYTHON_PATH_ENV_VAR_NAME, to_mgr.libpython_loc_path(), true);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int create_tapeout(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_ctx) {
#ifdef ARKANGEL_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arkangel::LicenseManager::LicensedFeatureName::AAEE)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_file = cmd.option("config_file");
  VTR_ASSERT(true == cmd_ctx.option_enable(cmd, opt_file));
  VTR_ASSERT(false == cmd_ctx.option_value(cmd, opt_file).empty());
  std::string config_file_name = cmd_ctx.option_value(cmd, opt_file);
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  /* Do not allow multiple creation on project */
  if (arkangel_ctx.is_tapeout_created()) {
    VTR_LOG_ERROR(
      "There is already a tapeout project which is created or loaded! Not allowed to create "
      "another in the same session!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  to_mgr.clear();
  PdkManager& pdk_mgr = arkangel_ctx.mutable_pdk_manager();
  pdk_mgr.clear();
  BenchmarkManager& bm_mgr = arkangel_ctx.mutable_benchmark_manager();
  bm_mgr.clear();
  int status = load_tapeout_from_config_file(config_file_name, to_mgr, pdk_mgr, bm_mgr, true);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Start initializing project directory */
  VTR_LOG("Initializing tapeout project '%s'...\n", to_mgr.name().c_str());
  if (!arkangel::path_is_legal(to_mgr.path())) {
    openfpga::create_directory(to_mgr.path(), true, verbosity == 1);
  }
  arkangel_ctx.set_tapeout_created();
  return status;
}
/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int load_tapeout(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                 const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_file = cmd.option("config_file");
  VTR_ASSERT(true == cmd_ctx.option_enable(cmd, opt_file));
  VTR_ASSERT(false == cmd_ctx.option_value(cmd, opt_file).empty());
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  openfpga::CommandOptionId opt_no_valid = cmd.option("no_validation");
  std::string config_file_name = cmd_ctx.option_value(cmd, opt_file);
  /* Do not allow multiple creation on project */
  if (arkangel_ctx.is_tapeout_created()) {
    VTR_LOG_ERROR(
      "There is already a tapeout project which is created or loaded! Not allowed to create "
      "another in the same session!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  to_mgr.clear();
  PdkManager& pdk_mgr = arkangel_ctx.mutable_pdk_manager();
  pdk_mgr.clear();
  BenchmarkManager& bm_mgr = arkangel_ctx.mutable_benchmark_manager();
  bm_mgr.clear();
  int status = load_tapeout_from_config_file(config_file_name, to_mgr, pdk_mgr, bm_mgr, false);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (cmd_ctx.option_enable(cmd, opt_no_valid)) {
    VTR_LOG_WARN(
      "Validation on existing results is disabled by user. This may cause errors or extra effort "
      "in debugging during your development!\n");
    return openfpga::CMD_EXEC_SUCCESS;
  }
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* TODO: Generate MD5 for all the resulting files for all the steps and runs */
  VTR_LOG("Checking results of each step and run...\n");
  for (auto curr_step : to_mgr.dev_steps()) {
    std::string step_name = to_mgr.step_name2str(curr_step);
    VTR_LOG("Checking results of step '%s'...\n", step_name.c_str());
    /* If no run is available, skip it */
    for (auto curr_run : to_mgr.step_runs(curr_step)) {
      VTR_LOG("\tFor run '%s'...\n", curr_run.c_str());
      status = check_results_for_step_run(to_mgr, bm_mgr, pdk_mgr, curr_step, curr_run, verbosity);
      if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
        fflush(stdout);
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  }
  VTR_LOG("Done\n");
  arkangel_ctx.set_tapeout_created();
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int disable_benchmark(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_name = cmd.option("name");
  VTR_ASSERT(true == cmd_ctx.option_enable(cmd, opt_name));
  VTR_ASSERT(false == cmd_ctx.option_value(cmd, opt_name).empty());
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  /* Do not allow multiple creation on project */
  if (!arkangel_ctx.is_tapeout_created()) {
    VTR_LOG_ERROR("Require a valid tapeout to be created or loaded!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  BenchmarkManager& bm_mgr = arkangel_ctx.mutable_benchmark_manager();
  std::string bm_name = cmd_ctx.option_value(cmd, opt_name);
  VTR_LOGV(verbosity == 1,
           "To disable the benchmark '%s' for architecture evaluation and design verification\n",
           bm_name.c_str());
  int status = bm_mgr.try_set_benchmark_disabled(bm_name, verbosity == 1);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbosity == 1, "Done\n");
  /* Also need to update the tapeout manager here */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  status = to_mgr.add_disabled_benchmark(bm_name);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int disable_corner(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_name = cmd.option("name");
  VTR_ASSERT(true == cmd_ctx.option_enable(cmd, opt_name));
  VTR_ASSERT(false == cmd_ctx.option_value(cmd, opt_name).empty());
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  /* Do not allow multiple creation on project */
  if (!arkangel_ctx.is_tapeout_created()) {
    VTR_LOG_ERROR("Require a valid tapeout to be created or loaded!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  std::string corner_name = cmd_ctx.option_value(cmd, opt_name);
  VTR_LOGV(verbosity == 1, "To disable the corner '%s'\n", corner_name.c_str());
  int status = to_mgr.add_disabled_corner(corner_name, verbosity == 1);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbosity == 1, "Done\n");
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
/********************************************************************
 * Add custom VPR constraints to a benchmark for architecture evaluation
 *******************************************************************/
int add_benchmark_vpr_constraints(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                                  const openfpga::CommandContext& cmd_ctx) {
  // Retrieve command options
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_benchmark = cmd.option("benchmark");
  openfpga::CommandOptionId opt_file = cmd.option("file");
  // Get the verbosity level
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  // Get the benchmark name and validate it
  std::string benchmark_name = "";
  if (cmd_ctx.option_enable(cmd, opt_benchmark)) {
    benchmark_name = cmd_ctx.option_value(cmd, opt_benchmark);
  } else {
    VTR_LOG_ERROR("Benchmark name is required.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  // find benchmark suite id
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  BenchmarkSuiteId st_id = bm_mgr.find_benchmark_suite(benchmark_name);
  BenchmarkId bm_id = bm_mgr.find_benchmark_by_name(benchmark_name);
  if (!bm_mgr.valid_benchmark_id(bm_id) || !bm_mgr.valid_suite_id(st_id)) {
    VTR_LOG_ERROR("Invalid benchmark %s !\n", benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  // Get the VPR constraint file path and validate it
  std::string file = cmd_ctx.option_value(cmd, opt_file);
  // Convert relative path to absolute path before checking existence
  std::filesystem::path file_path(file);
  std::string absolute_file_path = file;
  if (file_path.is_relative()) {
    // Convert relative path to absolute path
    absolute_file_path = std::filesystem::absolute(file_path).string();
  }
  if (!std::filesystem::exists(absolute_file_path)) {
    VTR_LOG_ERROR("VPR constraint file '%s' does not exist.\n", absolute_file_path.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  // Select the proper run and corner
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  int status = try_pick_existing_tapeout_step_run(to_mgr, TapeoutManager::e_step_name::ARCH,
                                                  user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to pick ARCH run. Make sure the specified run exists.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  // Auto-pick corner or use the user-defined corner
  std::string qor_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    qor_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  if (!to_mgr.valid_corner_name(qor_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled.\n",
                  qor_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbosity,
           "Adding VPR constraints for benchmark='%s', run='%s', corner='%s', file='%s'\n",
           benchmark_name.c_str(), run.c_str(), qor_corner.c_str(), absolute_file_path.c_str());
  // Record the constraints in the TapeoutManager so they can be saved to the project file later.
  // The key values (benchmark_name, run, qor_corner) should be preserved as attributes
  int add_status =
    to_mgr.add_vpr_constraints_to_benchmark(benchmark_name, run, qor_corner, absolute_file_path);
  if (add_status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to add VPR constraints to benchmark in TapeoutManager.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  // Log success message if verbosity is enabled
  VTR_LOGV(verbosity,
           "Successfully registered VPR constraints for benchmark '%s' with file '%s'.\n",
           benchmark_name.c_str(), absolute_file_path.c_str());
  // Ensure no conflict with clock buffer constraints
  if (to_mgr.check_clock_buffer_constraints(benchmark_name, run, qor_corner)) {
    VTR_LOG_ERROR("User-defined VPR constraints may conflict with clock buffer constraints!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Add custom VPR constraints to a benchmark for architecture evaluation
 *******************************************************************/
int set_benchmark_custom_pin_utilization(ArkangelContext& arkangel_ctx,
                                         const openfpga::Command& cmd,
                                         const openfpga::CommandContext& cmd_ctx) {
/* Implementation for setting custom pin utilization */
#ifdef ARKANGEL_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arkangel::LicenseManager::LicensedFeatureName::EVAL)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_benchmark = cmd.option("benchmark");
  openfpga::CommandOptionId opt_tile = cmd.option("tile");
  openfpga::CommandOptionId opt_input_pin_util = cmd.option("input_pin_util");
  openfpga::CommandOptionId opt_output_pin_util = cmd.option("output_pin_util");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  /* If the run exists and it succeeds, throw a warning and return, unless user asks to start over
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = autopick_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Get the data */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();

  std::string eval_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    eval_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  /* Check if the corner is valid */
  if (!to_mgr.valid_corner_name(eval_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  eval_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  // Get benchmark name
  if (!cmd_ctx.option_enable(cmd, opt_benchmark)) {
    VTR_LOG_ERROR("Missing required argument: benchmark\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string benchmark_name = cmd_ctx.option_value(cmd, opt_benchmark);
  // Get tile name
  if (!cmd_ctx.option_enable(cmd, opt_tile)) {
    VTR_LOG_ERROR("Missing required argument: tile\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string tile_name = cmd_ctx.option_value(cmd, opt_tile);

  // Get input pin utilization - use -1.0 to indicate not specified
  float input_pin_util = -1.0;  // -1 means not specified
  if (cmd_ctx.option_enable(cmd, opt_input_pin_util)) {
    input_pin_util = std::stof(cmd_ctx.option_value(cmd, opt_input_pin_util));
    if (input_pin_util < 0.0 || input_pin_util > 1.0) {
      VTR_LOG_ERROR("Input pin utilization must be between 0 and 1, got %f\n", input_pin_util);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  // Get output pin utilization - use -1.0 to indicate not specified
  float output_pin_util = -1.0;  // -1 means not specified
  if (cmd_ctx.option_enable(cmd, opt_output_pin_util)) {
    output_pin_util = std::stof(cmd_ctx.option_value(cmd, opt_output_pin_util));
    if (output_pin_util < 0.0 || output_pin_util > 1.0) {
      VTR_LOG_ERROR("Output pin utilization must be between 0 and 1, got %f\n", output_pin_util);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  // At least one utilization should be specified
  if (input_pin_util < 0 && output_pin_util < 0) {
    VTR_LOG_ERROR("At least one of input_pin_util or output_pin_util must be specified\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  // Log the action
  VTR_LOGV(verbosity == 1, "Setting custom pin utilization for benchmark '%s', tile '%s'\n",
           benchmark_name.c_str(), tile_name.c_str());
  if (input_pin_util >= 0) {
    VTR_LOGV(verbosity == 1, "  Input pin utilization: %.1f\n", input_pin_util);
  }
  if (output_pin_util >= 0) {
    VTR_LOGV(verbosity == 1, "  Output pin utilization: %.1f\n", output_pin_util);
  }
  VTR_LOGV(verbosity == 1, "  Run: %s, Corner: %s\n", run.c_str(), eval_corner.c_str());

  // If input or output utilization is not specified, get it from the default value
  float final_input_util = input_pin_util;
  float final_output_util = output_pin_util;

  if (input_pin_util < 0) {
    float default_input_util = to_mgr.get_default_input_pin_utilization(tile_name);
    final_input_util = default_input_util;
  }

  // For output pin utilization - use the new dedicated API as suggested by comment
  if (output_pin_util < 0) {
    float default_output_util = to_mgr.get_default_output_pin_utilization(tile_name);
    final_output_util = default_output_util;
  }
  // Store the custom pin utilization in the project manager
  status = to_mgr.add_custom_pin_utilization_to_benchmark(
    benchmark_name, run, eval_corner, tile_name, final_input_util, final_output_util);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return status;
  }

  VTR_LOGV(verbosity == 1, "Custom pin utilization successfully set\n");

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}
}  // namespace arcxpress
