/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_dev_manager.h"
#include "arkangel_file_os.h"
#include "arkangel_report.h"
#include "arkangel_setup_utils.h"
#include "arkangel_tapeout_utils.h"
#include "command_exit_codes.h"
#include "dv_manager.h"
#include "netlist_dev_manager.h"
#include "openfpga_digest.h"
#include "qor_manager.h"
#include "tableprinter.h"
/* begin namespace arcxpress */
namespace arcxpress {
/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_corners(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_file = cmd.option("file");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  std::string report_file = "";
  if (cmd_ctx.option_enable(cmd, opt_file)) {
    report_file = cmd_ctx.option_value(cmd, opt_file);
  }

  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  VTR_LOG("Corners of tapeout project '%s'\n", to_mgr.name().c_str());

  TablePrinter tp({"Corner Type", "Corner Name"});
  for (std::string corner_tag : to_mgr.corner_tags()) {
    std::string corner_type = "";
    std::string corner_name = "";
    if (corner_tag == to_mgr.default_corner()) {
      corner_type += "*  ";
    } else {
      corner_type += "   ";
    }
    if (to_mgr.is_user_defined_corner(corner_tag) && to_mgr.is_builtin_corner(corner_tag)) {
      corner_type += "(User-defined^)";
    } else if (to_mgr.is_user_defined_corner(corner_tag)) {
      corner_type += "(User-defined)";
    } else if (to_mgr.is_disabled_corner(corner_tag)) {
      corner_type += "(Disabled)";
    } else {
      VTR_ASSERT(to_mgr.is_builtin_corner(corner_tag));
      corner_type += "(Built-in)";
    }
    corner_name = corner_tag;
    tp.add_row({corner_type, corner_name});
  }
  tp.print(report_file);
  VTR_LOG("* is the default corner\n");
  VTR_LOG("^ is both user defined and built-in but user defined settings are considered\n");

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_module_names(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name = TapeoutManager::e_substep_name::ARCH_IO_COORD;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "Module names are not available due to required substep '%s' of arch run '%s' is failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Module names are not available due to required substep '%s' of arch run '%s' is fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Module names may not be correct as required substep '%s' of arch run '%s' is out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Create arch dev manager and module names */
  ArchDevManager ad_mgr(to_mgr, run);
  VTR_LOG("Module names for architecture run '%s' of tapeout project: %s\n", run.c_str(),
          to_mgr.name().c_str());
  status = ad_mgr.show_module_name_status(ArchDevManager::e_fabric_type::ULTIMATE, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_ref_module_names(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                            const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name =
    TapeoutManager::e_substep_name::ARCH_INIT_MODULE_NAMES;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "Module names are not available due to required substep '%s' of arch run '%s' is failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Module names are not available due to required substep '%s' of arch run '%s' is fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Module names may not be correct as required substep '%s' of arch run '%s' is out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Create arch dev manager and module names */
  ArchDevManager ad_mgr(to_mgr, run);
  VTR_LOG("Module names for architecture run '%s' of tapeout project: %s\n", run.c_str(),
          to_mgr.name().c_str());
  status =
    ad_mgr.show_init_module_name_status(ArchDevManager::e_fabric_type::ULTIMATE, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_io(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
              const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_report_file = cmd.option("file");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_report_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_report_file);
  }
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name = TapeoutManager::e_substep_name::ARCH_PIN_TABLE;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "I/O pin table is not available due to required substep '%s' of arch run '%s' is failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "I/O pin table is not available due to required substep '%s' of arch run '%s' is fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "I/O pin table may not be correct as required substep '%s' of arch run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Create arch dev manager and module names */
  ArchDevManager ad_mgr(to_mgr, run);
  VTR_LOG("Detailed I/O information for architecture run '%s' of tapeout project: %s\n",
          run.c_str(), to_mgr.name().c_str());
  status = ad_mgr.show_io_status(ArchDevManager::e_fabric_type::ULTIMATE, report_file_path,
                                 verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_static_power(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_report_file = cmd.option("file");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_report_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_report_file);
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name = TapeoutManager::e_substep_name::NETLIST_QOR;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Gate-level netlists may not be correct as required substep '%s' of netlist run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Setup */
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  /* Setup Qor types */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::LEAKAGE;

  /* Auto-pick corner */
  std::string qor_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    qor_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  if (!to_mgr.valid_corner_name(qor_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  qor_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<std::string> corners_to_sweep = find_corners_to_sweep(to_mgr, qor_corner);

  std::map<std::string, std::string> spwr_map;
  for (auto corner_to_sweep : corners_to_sweep) {
    QorManager qor_mgr(nd_mgr.project_home_dir().string(), to_mgr, pdk_mgr);
    status = setup_qor_manager_post_synth(to_mgr, nd_mgr, pdk_mgr, qor_mgr, qor_type, std::string(),
                                          corner_to_sweep, std::string("100"), verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    /* Prepare the runtime directory */
    std::string spwr_val;
    VTR_LOGV(verbosity == 1, "Static power summary for corner '%s'\n", corner_to_sweep.c_str());
    status = qor_mgr.extract_and_report_static_power(spwr_val, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    spwr_map[corner_to_sweep] = spwr_val;
  }
  /* Show a report */

  TablePrinter tp({"Corner", "Static Power"});
  for (auto pair : spwr_map) {
    tp.add_row({pair.first, pair.second});
  }
  tp.print(report_file_path);
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_benchmark_power(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                           const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_benchmark = cmd.option("benchmark");
  openfpga::CommandOptionId opt_report_file = cmd.option("file");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_report_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_report_file);
  }
  std::string benchmark_name = "";
  if (cmd_ctx.option_enable(cmd, opt_benchmark)) {
    benchmark_name = cmd_ctx.option_value(cmd, opt_benchmark);
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::DV;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name = TapeoutManager::e_substep_name::DV_GL_POWER;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "Gate-level benchmark power reports are not available due to required substep '%s' of "
      "netlist run '%s' is "
      "failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Gate-level benchmark power reports are not available due to required substep '%s' of "
      "netlist run '%s' is "
      "fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Gate-level benchmark power reports may not be correct as required substep '%s' of netlist "
      "run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Backtrace to the arch run which the bitstream generation is based on */
  TapeoutManager::e_step_name bitgen_step_name = TapeoutManager::e_step_name::BITSTREAM;
  std::string bitgen_run = to_mgr.step_run_dep_run(step_name, run, bitgen_step_name);
  to_mgr.check_valid_run(bitgen_step_name, bitgen_run);

  TapeoutManager::e_step_name nlist_step_name = TapeoutManager::e_step_name::NETLIST;
  std::string nlist_run = to_mgr.step_run_dep_run(step_name, run, nlist_step_name);
  to_mgr.check_valid_run(nlist_step_name, nlist_run);

  /* Setup */
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, nlist_run, pdk_mgr);

  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  ArchEvalManager ae_mgr(to_mgr, bitgen_run, bm_mgr);

  /* Load corners to arch eval manager */
  for (std::string corner : to_mgr.corner_tags()) {
    std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
    ae_mgr.add_arch_timing_file(corner, arch_timing_file);
  }

  DvManager dv_mgr(to_mgr, run, ae_mgr, nd_mgr);

  /* TODO: Setup, loading W, openfpga setting files etc. */
  status = setup_dv_manager_for_qor_analysis(to_mgr, nd_mgr, ae_mgr, dv_mgr, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Setup Qor types */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::DYNAMIC_POWER;

  /* Auto-pick corner */
  std::string qor_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    qor_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  if (!to_mgr.valid_corner_name(qor_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  qor_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<std::string> corners_to_sweep = find_corners_to_sweep(to_mgr, qor_corner);

  BenchmarkSuiteId st_id = bm_mgr.find_benchmark_suite(benchmark_name);
  BenchmarkId bm_id = bm_mgr.find_benchmark_by_name(benchmark_name);

  std::map<std::string, std::string> spwr_map;
  for (auto corner_to_sweep : corners_to_sweep) {
    QorManager qor_mgr(nd_mgr.project_home_dir().string(), to_mgr, pdk_mgr);
    status = setup_qor_manager_dynamic_power(to_mgr, nd_mgr, st_id, bm_id, dv_mgr, qor_mgr,
                                             corner_to_sweep, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    /* Prepare the runtime directory */
    std::string spwr_val;
    VTR_LOGV(verbosity == 1, "Benchmark power summary for corner '%s'\n", corner_to_sweep.c_str());
    status = qor_mgr.extract_and_report_benchmark_power(spwr_val, benchmark_name, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    spwr_map[corner_to_sweep] = spwr_val;
  }
  /* Show a report */
  TablePrinter tp({"Benchmark", "Corner", "Total Power"});
  for (auto pair : spwr_map) {
    tp.add_row({benchmark_name, pair.first, pair.second});
  }
  tp.print(report_file_path);
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_config_power(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_report_file = cmd.option("file");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_report_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_report_file);
  }
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name = TapeoutManager::e_substep_name::NETLIST_QOR;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Gate-level netlists may not be correct as required substep '%s' of netlist run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Setup */
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  /* Setup Qor types */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::CONFIG_POWER;

  /* Auto-pick corner */
  std::string qor_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    qor_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  if (!to_mgr.valid_corner_name(qor_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  qor_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<std::string> corners_to_sweep = find_corners_to_sweep(to_mgr, qor_corner);

  std::map<std::string, std::string> spwr_map;
  for (auto corner_to_sweep : corners_to_sweep) {
    QorManager qor_mgr(nd_mgr.project_home_dir().string(), to_mgr, pdk_mgr);
    status = setup_qor_manager_post_synth(to_mgr, nd_mgr, pdk_mgr, qor_mgr, qor_type, std::string(),
                                          corner_to_sweep, std::string("100"), verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    /* Prepare the runtime directory */
    std::string spwr_val;
    VTR_LOGV(verbosity == 1, "Configuration power summary for corner '%s'\n",
             corner_to_sweep.c_str());
    status = qor_mgr.extract_and_report_config_power(spwr_val, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    spwr_map[corner_to_sweep] = spwr_val;
  }
  /* Show a report */
  VTR_LOG("A summary of configuration power:\n");
  TablePrinter tp({"Corner", "Configuration Power"});
  for (auto pair : spwr_map) {
    tp.add_row({pair.first, pair.second});
  }
  tp.print(report_file_path);
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int report_config_stats(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_pclk_freq = cmd.option("pclk_freq");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_precision = cmd.option("precision");

  openfpga::CommandOptionId opt_report_file = cmd.option("file");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  int precision = 6;
  if (cmd_ctx.option_enable(cmd, opt_precision)) {
    precision = std::stoi(cmd_ctx.option_value(cmd, opt_precision));
  }

  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_report_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_report_file);
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name = TapeoutManager::e_substep_name::NETLIST_GEN;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Gate-level netlists may not be correct as required substep '%s' of netlist run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Setup */
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  float pclk_freq = std::stof(cmd_ctx.option_value(cmd, opt_pclk_freq));
  status = nd_mgr.show_config_stats(pclk_freq, report_file_path, precision, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Report qor scale
 *******************************************************************/
int report_qor_scale(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_type = cmd.option("type");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_report_file = cmd.option("file");
  openfpga::CommandOptionId opt_precision = cmd.option("precision");

  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  int precision = 2;  // default to have precision 2
  if (cmd_ctx.option_enable(cmd, opt_precision)) {
    precision = std::stoi(cmd_ctx.option_value(cmd, opt_precision));
    VTR_LOGV(verbosity == 1, "Scale factor will have precision: %d\n", precision);
  }
  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_report_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_report_file);
  }

  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);

  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Require the substep of selected arch run to be a success */
  TapeoutManager::e_substep_name s_step_name = TapeoutManager::e_substep_name::NETLIST_QOR;
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FAILURE) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
      "fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Gate-level netlists may not be correct as required substep '%s' of netlist run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());

  /* Setup */
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  /* Setup Qor types */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::TIMING;
  if (cmd_ctx.option_enable(cmd, opt_type)) {
    qor_type = to_mgr.str2qorso_task_type(cmd_ctx.option_value(cmd, opt_type), true);
    if (!to_mgr.valid_qorso_task_type(qor_type)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* Auto-pick corner */
  std::string qor_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    qor_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  if (!to_mgr.valid_corner_name(qor_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  qor_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<std::string> corners_to_sweep = find_corners_to_sweep(to_mgr, qor_corner);

  QorManager qor_mgr(nd_mgr.project_home_dir().string(), to_mgr, pdk_mgr);

  arkangel::TablePrinter tp({"Qor Tasks", "Variable", "Scale Factor", "Corner"});
  for (auto corner_to_sweep : corners_to_sweep) {
    status = openfpga::CMD_EXEC_FATAL_ERROR;
    status = setup_qor_manager_post_synth(to_mgr, nd_mgr, pdk_mgr, qor_mgr, qor_type, std::string(),
                                          corner_to_sweep, std::string("100"), verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Prepare the runtime directory */
    VTR_LOGV(verbosity == 1, "Quality-of-Result Scale factor check\n");
    VTR_LOGV(verbosity == 1, "Corner: %s\n", corner_to_sweep.c_str());

    status = qor_mgr.print_qorso_task_scale_factors(tp, corner_to_sweep, precision, verbosity);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  if (!report_file_path.empty()) {
    VTR_LOGV(verbosity == 1, "Write quality-of-Result Scale factors to %s \n",
             report_file_path.c_str());
  } else {
    VTR_LOGV(verbosity == 1, "Print quality-of-Result Scale factors on screen\n");
  }
  tp.print(report_file_path);
  fflush(stdout);

  return status;
}

}  // namespace arcxpress
