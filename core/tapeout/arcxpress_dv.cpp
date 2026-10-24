/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_dev_manager.h"
#include "arch_eval_manager.h"
#include "arcxpress_dv.h"
#include "arcxpress_file_os.h"
#include "arcxpress_report_util.h"
#include "arcxpress_setup_utils.h"
#include "arcxpress_tapeout_utils.h"
#include "benchmark_packet_utils.h"
#include "command_exit_codes.h"
#include "dv_manager.h"
#include "netlist_dev_manager.h"
#include "openfpga_digest.h"
/* begin namespace arcxpress */
namespace arcxpress {

static int setup_dv_manager(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                            const ArchEvalManager& ae_mgr, DvManager& dv_mgr, const bool& verbose) {
  for (auto f_type : to_mgr.openfpga_setting_file_types()) {
    /* File type does not matter here, assign any should work */
    if (!dv_mgr.required_file_type(f_type)) {
      continue;
    }
    auto dv_f_type = dv_mgr.convert_file_type(f_type);
    bool l_status = dv_mgr.add_file(dv_f_type, to_mgr.openfpga_setting_file(f_type));
    if (!l_status) {
      VTR_LOG_ERROR("Failed to add the openfpga setting file '%s'\n",
                    to_mgr.openfpga_setting_file(f_type).c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  for (auto f_type : to_mgr.yosys_script_file_types()) {
    /* Bypass the script which is not needed by evaluation */
    if (!dv_mgr.required_script_type(f_type)) {
      continue;
    }
    auto dv_f_type = dv_mgr.convert_script_type(f_type);
    bool l_status = dv_mgr.add_yosys_script(dv_f_type, to_mgr.yosys_script_file(f_type));
    if (!l_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  for (auto f_type : to_mgr.openfpga_script_file_types()) {
    /* Bypass the script which is not needed by evaluation */
    if (!dv_mgr.required_script_type(f_type)) {
      continue;
    }
    auto dv_f_type = dv_mgr.convert_script_type(f_type);
    /* File tyoe does not matter here, assign any should work */
    bool l_status = dv_mgr.add_openfpga_script(dv_f_type, to_mgr.openfpga_script_file(f_type));
    if (!l_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  dv_mgr.set_route_chan_width(to_mgr.route_chan_width());
  /* A virtual arch dev manager to get the constants */
  ArchDevManager ad_mgr(to_mgr, to_mgr.invalid_run());
  dv_mgr.set_fabric_name(ad_mgr.release_fabric_name());
  /* Add base bitstream file */
  dv_mgr.add_file(DvManager::e_file_type::BASE_BITSTREAM_XML_FILE,
                  nd_mgr.file(NetlistDevManager::e_fabric_type::ULTIMATE,
                              NetlistDevManager::e_file_type::BASE_BITSTREAM_FILE, verbose));
  /* Load netlist source dir */
  bool status = dv_mgr.init_netlist_source_dir(verbose);
  if (!status) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int setup_dv(ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
             const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_bitstream_run = cmd.option("bitstream_run");
  openfpga::CommandOptionId opt_netlist_run = cmd.option("netlist_run");
  openfpga::CommandOptionId opt_j = cmd.option("jobs");
  openfpga::CommandOptionId opt_ntwt = cmd.option("new_thread_wait_time");
  openfpga::CommandOptionId opt_sswt = cmd.option("sim_start_wait_time");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arcxpress_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::DV;
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

  /* Sanity checks on the bitstream run */
  TapeoutManager::e_step_name bitgen_step_name = TapeoutManager::e_step_name::BITSTREAM;
  std::string bitgen_run = to_mgr.invalid_run();
  std::string user_defined_bitgen_run = to_mgr.invalid_run();
  status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_bitstream_run)) {
    user_defined_bitgen_run = cmd_ctx.option_value(cmd, opt_bitstream_run);
  }
  status = pick_existing_tapeout_step_dep_run(to_mgr, bitgen_step_name, user_defined_bitgen_run,
                                              bitgen_run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Sanity checks on the netlist run */
  TapeoutManager::e_step_name nlist_step_name = TapeoutManager::e_step_name::NETLIST;
  std::string nlist_run = to_mgr.invalid_run();
  std::string user_defined_nlist_run = to_mgr.invalid_run();
  status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_netlist_run)) {
    user_defined_nlist_run = cmd_ctx.option_value(cmd, opt_netlist_run);
  }
  status =
    pick_existing_tapeout_step_dep_run(to_mgr, nlist_step_name, user_defined_nlist_run, nlist_run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Backtrace to the arch run which the bitstream generation is based on */
  TapeoutManager::e_step_name arch_step_name = TapeoutManager::e_step_name::ARCH;
  std::string bitgen_dep_arch_run =
    to_mgr.step_run_dep_run(bitgen_step_name, bitgen_run, arch_step_name);
  std::string nlist_dep_arch_run =
    to_mgr.step_run_dep_run(nlist_step_name, bitgen_run, arch_step_name);
  if (bitgen_dep_arch_run != nlist_dep_arch_run) {
    VTR_LOG_ERROR(
      "Bitstream and netlist come from different version of architecture '%s' and '%s' "
      "respectively! This will cause fatal errors in downstream flow!\n",
      bitgen_dep_arch_run.c_str(), nlist_dep_arch_run.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  to_mgr.check_valid_run(arch_step_name, bitgen_dep_arch_run);
  to_mgr.check_valid_run(arch_step_name, nlist_dep_arch_run);

  /* TODO: Require previous steps of arch development to be successful!!! */
  to_mgr.add_step_run_dep(step_name, run, bitgen_step_name, bitgen_run);
  to_mgr.add_step_run_dep(step_name, run, nlist_step_name, nlist_run);
  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::DV_SETUP,
                            TapeoutManager::e_run_status::FAILURE);

  /* Initialize */
  const BenchmarkManager& bm_mgr = arcxpress_ctx.benchmark_manager();
  const PdkManager& pdk_mgr = arcxpress_ctx.pdk_manager();
  ArchEvalManager ae_mgr(to_mgr, bitgen_run, bm_mgr);
  /* Load corners to arch eval manager */
  for (std::string corner : to_mgr.corner_tags()) {
    std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
    ae_mgr.add_arch_timing_file(corner, arch_timing_file);
  }

  NetlistDevManager nd_mgr(to_mgr, nlist_run, pdk_mgr);
  DvManager dv_mgr(to_mgr, run, ae_mgr, nd_mgr);

  /* TODO: Setup, loading W, openfpga setting files etc. */
  status = setup_dv_manager(to_mgr, nd_mgr, ae_mgr, dv_mgr, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    corner = cmd_ctx.option_value(cmd, opt_corner);
  }

  if (!to_mgr.valid_corner_name(corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Generating testbenches...\n");
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  status = dv_mgr.generate_testbench(to_mgr.openfpga_bin_path(), to_mgr.bus_group_gen_bin_path(),
                                     corner, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Setup simulation environment...\n");
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  size_t max_jobs = 1;
  if (cmd_ctx.option_enable(cmd, opt_j)) {
    max_jobs = std::stoi(cmd_ctx.option_value(cmd, opt_j));
  }
  size_t new_thread_wait_time = 1;
  if (cmd_ctx.option_enable(cmd, opt_ntwt)) {
    new_thread_wait_time = std::stoi(cmd_ctx.option_value(cmd, opt_ntwt));
  }
  size_t sim_start_wait_time = 1;
  if (cmd_ctx.option_enable(cmd, opt_sswt)) {
    sim_start_wait_time = std::stoi(cmd_ctx.option_value(cmd, opt_sswt));
  }

  status = dv_mgr.setup_cocotb(to_mgr.cocotb_util_bin_path(),
                               to_mgr.cocotb_force_bitstream_bin_path(), max_jobs,
                               new_thread_wait_time, sim_start_wait_time, corner, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::DV_SETUP,
                            TapeoutManager::e_run_status::SUCCESS);

  /* Update status */
  if (!to_mgr.check_and_update_step_status(step_name, run, verbosity == 1)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  fflush(stdout);
  return status;
}

int run_preconfig_dv(ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_benchmark = cmd.option("benchmark");
  openfpga::CommandOptionId opt_j = cmd.option("jobs");
  openfpga::CommandOptionId opt_ntwt = cmd.option("new_thread_wait_time");
  openfpga::CommandOptionId opt_sswt = cmd.option("sim_start_wait_time");
  openfpga::CommandOptionId opt_lcwt = cmd.option("log_check_wait_time");
  openfpga::CommandOptionId opt_sim = cmd.option("simulator");
  openfpga::CommandOptionId opt_nt = cmd.option("netlist_type");
  openfpga::CommandOptionId opt_dw = cmd.option("dump_waveform");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arcxpress_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::DV;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_existing_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Backtrace to the arch run which the bitstream generation is based on */
  TapeoutManager::e_step_name bitgen_step_name = TapeoutManager::e_step_name::BITSTREAM;
  std::string bitgen_run = to_mgr.step_run_dep_run(step_name, run, bitgen_step_name);
  to_mgr.check_valid_run(bitgen_step_name, bitgen_run);
  TapeoutManager::e_step_name nlist_step_name = TapeoutManager::e_step_name::NETLIST;
  std::string nlist_run = to_mgr.step_run_dep_run(step_name, run, nlist_step_name);
  to_mgr.check_valid_run(nlist_step_name, nlist_run);

  /* Initialize */
  const BenchmarkManager& bm_mgr = arcxpress_ctx.benchmark_manager();
  const PdkManager& pdk_mgr = arcxpress_ctx.pdk_manager();
  ArchEvalManager ae_mgr(to_mgr, bitgen_run, bm_mgr);
  /* Load corners to arch eval manager */
  for (std::string corner : to_mgr.corner_tags()) {
    std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
    ae_mgr.add_arch_timing_file(corner, arch_timing_file);
  }

  NetlistDevManager nd_mgr(to_mgr, nlist_run, pdk_mgr);
  DvManager dv_mgr(to_mgr, run, ae_mgr, nd_mgr);

  /* TODO: Setup, loading W, openfpga setting files etc. */
  status = setup_dv_manager(to_mgr, nd_mgr, ae_mgr, dv_mgr, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Run preconfigured simulation...\n");
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  size_t max_jobs = 1;
  if (cmd_ctx.option_enable(cmd, opt_j)) {
    max_jobs = std::stoi(cmd_ctx.option_value(cmd, opt_j));
  }
  size_t new_thread_wait_time = 1;
  if (cmd_ctx.option_enable(cmd, opt_ntwt)) {
    new_thread_wait_time = std::stoi(cmd_ctx.option_value(cmd, opt_ntwt));
  }
  size_t sim_start_wait_time = 1;
  if (cmd_ctx.option_enable(cmd, opt_sswt)) {
    sim_start_wait_time = std::stoi(cmd_ctx.option_value(cmd, opt_sswt));
  }
  size_t log_check_wait_time = 1;
  if (cmd_ctx.option_enable(cmd, opt_lcwt)) {
    log_check_wait_time = std::stoi(cmd_ctx.option_value(cmd, opt_lcwt));
  }
  DvManager::e_netlist_type nlist_type = DvManager::e_netlist_type::RTL;
  if (cmd_ctx.option_enable(cmd, opt_nt)) {
    nlist_type = dv_mgr.str2netlist_type(cmd_ctx.option_value(cmd, opt_nt), true);
  }
  DvManager::e_simulator_type sim_type = DvManager::e_simulator_type::VCS;
  if (cmd_ctx.option_enable(cmd, opt_sim)) {
    sim_type = dv_mgr.str2simulator_type(cmd_ctx.option_value(cmd, opt_sim), true);
  }
  DvManager::e_waveform_type waveform_type = DvManager::e_waveform_type::NONE;
  if (cmd_ctx.option_enable(cmd, opt_dw)) {
    waveform_type = dv_mgr.str2waveform_type(cmd_ctx.option_value(cmd, opt_dw), true);
  }

  /* Depends on the netlist type, set substep status */
  TapeoutManager::e_substep_name dv_substep =
    dv_mgr.convert_netlist_type_to_substep_name(nlist_type);
  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, dv_substep, TapeoutManager::e_run_status::FAILURE);

  std::string corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    corner = cmd_ctx.option_value(cmd, opt_corner);
  }

  if (!to_mgr.valid_corner_name(corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::vector<std::string> corners_to_sweep = find_corners_to_sweep(to_mgr, corner);
  std::string benchmark_name = "";
  if (cmd_ctx.option_enable(cmd, opt_benchmark)) {
    benchmark_name = cmd_ctx.option_value(cmd, opt_benchmark);
  }

  for (auto corner_to_sweep : corners_to_sweep) {
    status = dv_mgr.run_preconfig_cocotb(
      to_mgr.cocotb_util_bin_path(), to_mgr.cocotb_makefile_include_file_path(), nlist_type,
      sim_type, waveform_type, max_jobs, new_thread_wait_time, sim_start_wait_time,
      log_check_wait_time, corner_to_sweep, benchmark_name, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  to_mgr.set_substep_status(step_name, run, dv_substep, TapeoutManager::e_run_status::SUCCESS);

  /* Update status */
  if (!to_mgr.check_and_update_step_status(step_name, run, verbosity == 1)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  fflush(stdout);
  return status;
}

/***********************************************************************************
 * Top-level function to report dynamic power for a benchmark of a tapeout flow/step
 **********************************************************************************/
int run_benchmark_power_analysis(ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                                 const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_dryrun = cmd.option("dryrun");
  openfpga::CommandOptionId opt_benchmark = cmd.option("benchmark");
  openfpga::CommandOptionId opt_j = cmd.option("jobs");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  bool dryrun = false;
  if (cmd_ctx.option_enable(cmd, opt_dryrun)) {
    dryrun = true;
  }

  std::string benchmark_name = "";
  if (cmd_ctx.option_enable(cmd, opt_benchmark)) {
    benchmark_name = cmd_ctx.option_value(cmd, opt_benchmark);
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  TapeoutManager& to_mgr = arcxpress_ctx.mutable_tapeout_manager();
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

  /* Backtrace to the arch run which the bitstream generation is based on */
  TapeoutManager::e_step_name bitgen_step_name = TapeoutManager::e_step_name::BITSTREAM;
  std::string bitgen_run = to_mgr.step_run_dep_run(step_name, run, bitgen_step_name);
  to_mgr.check_valid_run(bitgen_step_name, bitgen_run);

  TapeoutManager::e_step_name nlist_step_name = TapeoutManager::e_step_name::NETLIST;
  std::string nlist_run = to_mgr.step_run_dep_run(step_name, run, nlist_step_name);
  to_mgr.check_valid_run(nlist_step_name, nlist_run);

  /* Setup */
  const PdkManager& pdk_mgr = arcxpress_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, nlist_run, pdk_mgr);

  /* Setup Qor types */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::DYNAMIC_POWER;

  /* Setup */
  const BenchmarkManager& bm_mgr = arcxpress_ctx.benchmark_manager();
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

  /*find benchmark suite id*/
  BenchmarkSuiteId st_id = bm_mgr.find_benchmark_suite(benchmark_name);
  BenchmarkId bm_id = bm_mgr.find_benchmark_by_name(benchmark_name);

  /*run preconfigured simulation for selected benchmark */
  status = openfpga::CMD_EXEC_FATAL_ERROR;

  /* Start with a deposit of fail status */
  TapeoutManager::e_substep_name dv_substep = TapeoutManager::e_substep_name::DV_GL_POWER;
  to_mgr.set_substep_status(step_name, run, dv_substep, TapeoutManager::e_run_status::FAILURE);

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

  QorManager qor_mgr(dv_mgr.project_home_dir().string(), to_mgr, pdk_mgr);

  size_t max_jobs = 1;
  if (cmd_ctx.option_enable(cmd, opt_j)) {
    max_jobs = std::stoi(cmd_ctx.option_value(cmd, opt_j));
  }

  for (auto corner_to_sweep : corners_to_sweep) {
    status = setup_qor_manager_dynamic_power(to_mgr, nd_mgr, st_id, bm_id, dv_mgr, qor_mgr,
                                             corner_to_sweep, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    VTR_LOGV(verbosity == 1, "Run dynamic power analysis for benchmark '%s' under corner '%s'\n",
             benchmark_name.c_str(), corner_to_sweep.c_str());
    status = qor_mgr.run_benchmark_dynamic_power_analysis(to_mgr.qorso_bin_path(), benchmark_name,
                                                          dryrun, max_jobs, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  fflush(stdout);

  to_mgr.set_substep_status(step_name, run, dv_substep, TapeoutManager::e_run_status::SUCCESS);

  /* Update status */
  if (!to_mgr.check_and_update_step_status(step_name, run, verbosity == 1)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

/***********************************************************************************
 * Top-level function to report sta for a benchmark of a tapeout flow/step
 **********************************************************************************/
int run_benchmark_sta(ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_dryrun = cmd.option("dryrun");
  openfpga::CommandOptionId opt_benchmark = cmd.option("benchmark");
  openfpga::CommandOptionId opt_clock_freq = cmd.option("clock_freq");
  openfpga::CommandOptionId opt_netlist_type = cmd.option("netlist_type ");
  openfpga::CommandOptionId opt_j = cmd.option("jobs");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  bool dryrun = false;
  if (cmd_ctx.option_enable(cmd, opt_dryrun)) {
    dryrun = true;
  }

  std::string benchmark_name = "";
  if (cmd_ctx.option_enable(cmd, opt_benchmark)) {
    benchmark_name = cmd_ctx.option_value(cmd, opt_benchmark);
  }

  double clock_freq = 100;  // 100 MHz
  if (cmd_ctx.option_enable(cmd, opt_clock_freq)) {
    clock_freq = std::stod(cmd_ctx.option_value(cmd, opt_clock_freq));
  }
  std::string clock_period = to_string_with_precision(1 / clock_freq * 1000.00, 6);  // unit is ns
  std::string netlist_type = "gl";
  if (cmd_ctx.option_enable(cmd, opt_netlist_type)) {
    netlist_type = cmd_ctx.option_value(cmd, opt_netlist_type);
  }
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  TapeoutManager& to_mgr = arcxpress_ctx.mutable_tapeout_manager();
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

  /* Backtrace to the arch run which the bitstream generation is based on */
  TapeoutManager::e_step_name bitgen_step_name = TapeoutManager::e_step_name::BITSTREAM;
  std::string bitgen_run = to_mgr.step_run_dep_run(step_name, run, bitgen_step_name);
  to_mgr.check_valid_run(bitgen_step_name, bitgen_run);

  TapeoutManager::e_step_name nlist_step_name = TapeoutManager::e_step_name::NETLIST;
  std::string nlist_run = to_mgr.step_run_dep_run(step_name, run, nlist_step_name);
  to_mgr.check_valid_run(nlist_step_name, nlist_run);

  /* Setup */
  const PdkManager& pdk_mgr = arcxpress_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, nlist_run, pdk_mgr);

  /* Setup Qor types */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::STA;

  /* Setup */
  const BenchmarkManager& bm_mgr = arcxpress_ctx.benchmark_manager();
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

  /*find benchmark suite id*/
  BenchmarkSuiteId st_id = bm_mgr.find_benchmark_suite(benchmark_name);
  BenchmarkId bm_id = bm_mgr.find_benchmark_by_name(benchmark_name);

  if (!bm_mgr.valid_benchmark_id(bm_id) || !bm_mgr.valid_suite_id(st_id)) {
    VTR_LOG_ERROR("Invalid benchmark %s !\n", benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Start with a deposit of fail status */
  TapeoutManager::e_substep_name dv_substep = TapeoutManager::e_substep_name::DV_GL_STA;
  to_mgr.set_substep_status(step_name, run, dv_substep, TapeoutManager::e_run_status::FAILURE);

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

  QorManager qor_mgr(dv_mgr.project_home_dir().string(), to_mgr, pdk_mgr);

  size_t max_jobs = 1;
  if (cmd_ctx.option_enable(cmd, opt_j)) {
    max_jobs = std::stoi(cmd_ctx.option_value(cmd, opt_j));
  }

  for (auto corner_to_sweep : corners_to_sweep) {
    status = setup_qor_manager_sta(to_mgr, nd_mgr, bm_mgr, st_id, bm_id, dv_mgr, qor_mgr,
                                   corner_to_sweep, clock_period, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    VTR_LOGV(verbosity == 1, "Run STA analysis for benchmark '%s' under corner '%s'\n",
             benchmark_name.c_str(), corner_to_sweep.c_str());
    status = qor_mgr.run_benchmark_sta(to_mgr.qorso_bin_path(), benchmark_name, dryrun, max_jobs,
                                       verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  fflush(stdout);

  to_mgr.set_substep_status(step_name, run, dv_substep, TapeoutManager::e_run_status::SUCCESS);

  /* Update status */
  if (!to_mgr.check_and_update_step_status(step_name, run, verbosity == 1)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arcxpress