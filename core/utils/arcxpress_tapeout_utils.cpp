/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arcxpress_tapeout_utils.h"
#include "benchmark_packet_utils.h"
#include "command_exit_codes.h"

/* begin namespace arcxpress */
namespace arcxpress {

/* Get the benchmark packet which is the default choice for most tape-out flow. This is a
 * post-synthesis netlist This is a frequently used for most flow which just need a benchmark as
 * placeholder
 */
int get_tapeout_default_benchmark_packet(const TapeoutManager& to_mgr,
                                         const BenchmarkManager& bm_mgr, BenchmarkPacket& bm_pkt) {
  std::string bm_name = to_mgr.default_benchmark_name();
  bm_pkt = create_benchmark_packet_by_name(bm_mgr, bm_name);
  if (!bm_pkt.valid_for_post_synth()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Get the benchmark packet which is the default choice for clock sweeping flow. */
int get_tapeout_default_benchmark_packet_for_clock_sweep(const TapeoutManager& to_mgr,
                                                         const BenchmarkManager& bm_mgr,
                                                         BenchmarkPacket& bm_pkt) {
  std::string bm_name = to_mgr.default4clk_sweep_benchmark_name();
  bm_pkt = create_benchmark_packet_by_name(bm_mgr, bm_name);
  if (!bm_pkt.valid_for_post_synth()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Get the benchmark packet which is the default choice for clock sweeping flow. */
int get_tapeout_default_benchmark_packet_for_reset_sweep(const TapeoutManager& to_mgr,
                                                         const BenchmarkManager& bm_mgr,
                                                         BenchmarkPacket& bm_pkt) {
  std::string bm_name = to_mgr.default4rst_sweep_benchmark_name();
  bm_pkt = create_benchmark_packet_by_name(bm_mgr, bm_name);
  if (!bm_pkt.valid_for_post_synth()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Automatically pick a proper run for a given step in a tapeout project.
 * - By default, the latest run is picked
 * - If a specific run name is defined by user, use the run as is. Create one if the run does not
 * exist!
 * - If there is no run, create a new run with default name
 * - Error out if the selected run is frozen
 * - Throw warning if the selected run was executed successfully (indicating a risk to invoke
 * results) Return the value through argument selected_run
 */
int autopick_tapeout_step_run(TapeoutManager& to_mgr, const TapeoutManager::e_step_name& step_name,
                              const std::string& user_defined_run, std::string& selected_run) {
  selected_run = to_mgr.step_latest_run(step_name);
  if (!user_defined_run.empty()) {
    selected_run = user_defined_run;
    VTR_LOG("User selected the run '%s'\n", selected_run.c_str());
  } else if (to_mgr.step_runs(step_name).empty()) {
    selected_run = std::string("iter0");
    to_mgr.add_step_run(step_name, selected_run);
    to_mgr.set_step_latest_run(step_name, selected_run);
    VTR_LOG("Automatically create a new run '%s'\n", selected_run.c_str());
  } else {
    VTR_LOG("Automatically use the latest run '%s' as default\n", selected_run.c_str());
  }
  if (!to_mgr.valid_run(step_name, selected_run)) {
    VTR_LOG("The selected run '%s' does not exist! Create a new one\n", selected_run.c_str());
    to_mgr.add_step_run(step_name, selected_run);
    to_mgr.set_step_latest_run(step_name, selected_run);
  }

  /* Nothing can be done for frozen runs */
  if (to_mgr.step_run_frozen(step_name, selected_run)) {
    VTR_LOG_ERROR("The selected run '%s' is frozen which cannot modified. Unlock it and retry.\n",
                  selected_run.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (to_mgr.step_run_status(step_name, selected_run) == TapeoutManager::e_run_status::SUCCESS) {
    VTR_LOG_WARN(
      "The selected run '%s' is succeed. This will invoke previous data! Please double check "
      "before moving forward\n",
      selected_run.c_str());
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

/* Automatically pick a proper run for a given step in a tapeout project.
 * - By default, the latest run is picked
 * - If a specific run name is defined by user, use the run as is. Error out if the run does not
 * exist!
 * - If there is no run, error out
 * Return the value through argument selected_run
 */
int try_pick_mutable_tapeout_step_run(const TapeoutManager& to_mgr,
                                      const TapeoutManager::e_step_name& step_name,
                                      const std::string& user_defined_run,
                                      std::string& selected_run) {
  selected_run = to_mgr.step_latest_run(step_name);
  if (!user_defined_run.empty()) {
    selected_run = user_defined_run;
    VTR_LOG("User selected the run '%s'\n", selected_run.c_str());
  } else if (to_mgr.step_runs(step_name).empty()) {
    VTR_LOG_ERROR("No run is available!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  } else {
    VTR_LOG("Automatically use the latest run '%s' as default\n", selected_run.c_str());
  }
  if (!to_mgr.valid_run(step_name, selected_run)) {
    VTR_LOG_ERROR("The selected run '%s' does not exist!\n", selected_run.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Nothing can be done for frozen runs */
  if (to_mgr.step_run_frozen(step_name, selected_run)) {
    VTR_LOG_ERROR("The selected run '%s' is frozen which cannot modified. Unlock it and retry.\n",
                  selected_run.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (to_mgr.step_run_status(step_name, selected_run) == TapeoutManager::e_run_status::SUCCESS) {
    VTR_LOG_WARN(
      "The selected run '%s' is succeed. This will invoke previous data! Please double check "
      "before moving forward\n",
      selected_run.c_str());
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Automatically pick a proper run for a given step in a tapeout project.
 * - By default, the latest run is picked
 * - If a specific run name is defined by user, use the run as is. Error out if the run does not
 * exist!
 * - If there is no run, error out
 * - Throw warning when the run is not a success
 * Return the value through argument selected_run
 */
int try_pick_existing_tapeout_step_run(const TapeoutManager& to_mgr,
                                       const TapeoutManager::e_step_name& step_name,
                                       const std::string& user_defined_run,
                                       std::string& selected_run) {
  selected_run = to_mgr.step_latest_run(step_name);
  if (!user_defined_run.empty()) {
    selected_run = user_defined_run;
    VTR_LOG("User selected the run '%s'\n", selected_run.c_str());
  } else if (to_mgr.step_runs(step_name).empty()) {
    VTR_LOG_ERROR("No run is available!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  } else {
    VTR_LOG("Automatically use the latest run '%s' as default\n", selected_run.c_str());
  }
  if (!to_mgr.valid_run(step_name, selected_run)) {
    VTR_LOG_ERROR("The selected run '%s' does not exist!\n", selected_run.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (to_mgr.step_run_status(step_name, selected_run) != TapeoutManager::e_run_status::SUCCESS) {
    VTR_LOG_WARN(
      "The selected run '%s' '%s'. The resulting data may be incorrect! Please double check "
      "before moving forward\n",
      selected_run.c_str(), to_mgr.step_run_status_str(step_name, selected_run).c_str());
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

/* Automatically pick an existing run for a given dependent step in a tapeout project.
 * - By default, the latest run is picked
 * - If a specific run name is defined by user, use the run as is. Error out if the run does not
 * exist!
 * - If there is no run, error out
 * - Throw error when the run is not a success
 * Return the value through argument selected_run
 */
int pick_existing_tapeout_step_dep_run(const TapeoutManager& to_mgr,
                                       const TapeoutManager::e_step_name& step_name,
                                       const std::string& user_defined_run,
                                       std::string& selected_run) {
  selected_run = to_mgr.step_latest_run(step_name);
  if (!user_defined_run.empty()) {
    selected_run = user_defined_run;
    if (!to_mgr.valid_run(step_name, selected_run)) {
      VTR_LOG_ERROR(
        "Invalid run '%s' is selected for %s! It does not have a run whose name matches!\n",
        selected_run.c_str(), to_mgr.step_name2str(step_name).c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("User selected the run '%s' of %s\n", selected_run.c_str(),
            to_mgr.step_name2str(step_name).c_str());
  } else if (to_mgr.step_runs(step_name).empty()) {
    VTR_LOG_ERROR("Invalid run '%s' is selected for %s! It has no runs!\n", selected_run.c_str(),
                  to_mgr.step_name2str(step_name).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  } else {
    VTR_LOG("Automatically use the latest run '%s' of %s as default\n", selected_run.c_str(),
            to_mgr.step_name2str(step_name).c_str());
  }
  if (to_mgr.step_run_status(step_name, selected_run) != TapeoutManager::e_run_status::SUCCESS) {
    VTR_LOG_WARN(
      "The selected run of '%s' of %s is not succeed. This will cause failures in downstream "
      "development! Please double check "
      "before moving forward\n",
      selected_run.c_str(), to_mgr.step_name2str(step_name).c_str());
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* For a given selected corner, check if it indicates to sweep all the corners or just 1 corner.
 * Return the list of corners to be considered */
std::vector<std::string> find_corners_to_sweep(const TapeoutManager& to_mgr,
                                               const std::string& eval_corner) {
  std::vector<std::string> corners_to_sweep;
  if (to_mgr.sweep_corner(eval_corner)) {
    VTR_LOG("User choose to sweep all the corners. Expect long runtime.\n");
    corners_to_sweep = to_mgr.corner_tags();
  } else {
    if (to_mgr.is_disabled_corner(eval_corner)) {
      VTR_LOG_ERROR("Specified corner %s is disabled! \n ", eval_corner.c_str());
      return corners_to_sweep;
    }
    corners_to_sweep.push_back(eval_corner);
  }
  return corners_to_sweep;
}

}  // namespace arcxpress
