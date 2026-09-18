/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_dev_manager.h"
#include "arkangel_export.h"
#include "arkangel_file_os.h"
#include "arkangel_setup_utils.h"
#include "arkangel_tapeout_utils.h"
#include "command_exit_codes.h"
#include "netlist_dev_manager.h"
#include "openfpga_digest.h"
#include "qor_manager.h"

/* begin namespace arkangel */
namespace arkangel {

/********************************************************************
 * Top-level function to back-annotate timing
 *******************************************************************/
int backannotate(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
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
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
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

  /* TODO: Backtrace the run of architecture development */
  TapeoutManager::e_step_name dep_step_name = TapeoutManager::e_step_name::ARCH;
  std::string dep_run = to_mgr.step_run_dep_run(step_name, run, dep_step_name);
  if (dep_run == to_mgr.invalid_run()) {
    VTR_LOG_ERROR(
      "Invalid run is found for the step of architecture development which the netlist development "
      "run '%s' depends on!\n",
      run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  VTR_LOG(
    "Back-annotation will be applied the architecture run '%s' which the netlist development run "
    "'%s' depends on\n",
    dep_run.c_str(), run.c_str());

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

  /* Setup */
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  BenchmarkPacket bm_pkt;
  ArchDevManager ad_mgr(to_mgr, dep_run);
  status = setup_arch_dev_manager(to_mgr, bm_mgr, ad_mgr, bm_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Get the architecture YAML files that contains timing results from the Qor Manager */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::TIMING;
  for (auto corner_to_sweep : corners_to_sweep) {
    QorManager qor_mgr(nd_mgr.project_home_dir().string(), to_mgr, pdk_mgr);
    status = setup_qor_manager_post_synth(to_mgr, nd_mgr, pdk_mgr, qor_mgr, qor_type, std::string(),
                                          corner_to_sweep, std::string("100"), verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::vector<std::string> post_qor_arch_timing_files;
    status = qor_mgr.arch_timing_files(post_qor_arch_timing_files);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    /* Merge the arch timing file into the architecture database */
    VTR_LOG(
      "Back-annotate the timing results on corner '%s' of netlist run '%s' of tapeout project '%s' "
      "to the "
      "architecture "
      "whose run is '%s'\n",
      corner_to_sweep.c_str(), run.c_str(), to_mgr.name().c_str(), dep_run.c_str());
    status = ad_mgr.merge_arch_timing(corner_to_sweep, post_qor_arch_timing_files, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    fflush(stdout);
  }
  /* TODO: Flip the status for arch dev as the timing data is now refreshed */
  VTR_LOG(
    "Remember to setup your arch as the timing data is now updated, before running any "
    "architecture evaluation\n");

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arkangel
