#include "arkangel_simulation.h"

#include <chrono>
#include <ctime>
#include <fstream>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#undef min
#undef max
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef IN
#undef ABSOLUTE
#ifdef isdigit
#undef isdigit
#endif
#endif

#include "arch_eval_manager.h"
#include "arkangel_constants.h"
#include "arkangel_file_os.h"
#include "arkangel_setup_utils.h"
#include "arkangel_string_util.h"
#include "arkangel_tapeout_utils.h"
#include "command_exit_codes.h"
#include "dv_manager.h"
#include "openfpga_basic.h"
#include "openfpga_digest.h"
#include "vtr_time.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Run synthesis
 * - Ensure there is a valid project
 * - Get the architecture file from device data navigator
 * - Get the design source file from project manager
 * - Collect options from strategy manager
 *******************************************************************/

int run_app_hdl_simulation(openfpga::Shell<ArkangelContext>* shell, ArkangelContext& arkangel_ctx,
                           const openfpga::Command& cmd, const openfpga::CommandContext& cmd_ctx) {
  vtr::Timer sim_runtime;
  openfpga::CommandOptionId opt_benchmark_name = cmd.option("benchmark");
  openfpga::CommandOptionId opt_simulator = cmd.option("simulator");
  openfpga::CommandOptionId opt_type = cmd.option("type");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_dw = cmd.option("dump_waveform");

  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Sanity checks on options */
  std::string waveform = "none";
  std::string waveform_option = "";
  if (cmd_ctx.option_enable(cmd, opt_dw)) {
    waveform = cmd_ctx.option_value(cmd, opt_dw);
    if (waveform == "vcd") {
      waveform_option = "DUMP_VCD";
    } else if (waveform == "fsdb") {
      waveform_option = "DUMP_FSDB";
    } else {
      VTR_LOG_ERROR("waveform type %s is invalid! \n", waveform.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* Do not allow multiple creation on project */
  if (!arkangel_ctx.is_tapeout_created()) {
    VTR_LOG_ERROR("Require a valid tapeout to be created or loaded!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string sim_type = cmd_ctx.option_value(cmd, opt_type);
#ifdef ARCXPRESS_LICENSED_BUILD
  if (sim_type == "rtl") {
    if (!arkangel_ctx.license_manager().checkOut(
          arcxpress::LicenseManager::LicensedFeatureName::AAEE)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  } else if (sim_type == "post_implementation") {
    if (!arkangel_ctx.license_manager().checkOut(
          arcxpress::LicenseManager::LicensedFeatureName::AAEE)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  } else if (sim_type == "post_synth") {
    if (!arkangel_ctx.license_manager().checkOut(
          arcxpress::LicenseManager::LicensedFeatureName::AAEE)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  } else {  // unlikely to be licensed
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  /*get benchmark name*/
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  std::string bm_name = cmd_ctx.option_value(cmd, opt_benchmark_name);
  VTR_LOGV(verbosity == 1, "Selected benchmark for simulation is %s \n", bm_name.c_str());
  auto benchmark_id = bm_mgr.find_benchmark_by_name(bm_name);
  std::string top_module = bm_mgr.top_module(benchmark_id);
  BenchmarkSuiteId benchmark_suite_id = bm_mgr.find_benchmark_suite(bm_name);

  /* Sanity checks on options */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  std::string simulator = to_mgr.default_simulator();
  if (cmd_ctx.option_enable(cmd, opt_simulator)) {
    simulator = cmd_ctx.option_value(cmd, opt_simulator);
    TapeoutManager::e_simulator_type simulator_type = to_mgr.str2simulator_type(simulator);
    if (!to_mgr.valid_simulator_type(simulator_type)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOGV(verbosity == 1, "Selected simulator is %s \n", simulator.c_str());
  } else {
    VTR_LOG("Default simulator %s will be used! \n", simulator.c_str());
  }

  /* Setup ArchEvalManager*/
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::BITSTREAM;
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

  ArchEvalManager ae_mgr(to_mgr, run, bm_mgr);

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
  std::vector<std::string> corners_to_sweep = find_corners_to_sweep(to_mgr, eval_corner);

  status = setup_arch_eval_manager(to_mgr, ae_mgr, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string output_file = "output";

  /* get source sim files from bitgen folder */
  if (sim_type == "rtl") {
    /* Add post-synth verilog */
    std::string sim_input_files = "";
    for (auto f_id : bm_mgr.design_sources(benchmark_id)) {
      if (bm_mgr.design_source_type(benchmark_id, f_id) != BenchmarkManager::e_file_type::VERILOG) {
        VTR_LOG_ERROR(
          "Non-Verilog file(s) detected in design sources. These files are not supported for "
          "application simulation.\n");
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      std::string source_file = bm_mgr.design_source_file(benchmark_id, f_id);
      sim_input_files += source_file;
      status = ae_mgr.invoke_cocotb_for_application_simulation(
        to_mgr, benchmark_id, benchmark_suite_id, sim_input_files, simulator, eval_corner, sim_type,
        waveform_option);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        float sim_end_time = sim_runtime.elapsed_sec();
        VTR_LOG(
          "\tRunning rtl simulation failed. Total time spent running rtl simulation: %g "
          "seconds\n",
          sim_end_time);
        return openfpga::CMD_EXEC_FATAL_ERROR;
      } else {
        float sim_end_time = sim_runtime.elapsed_sec();
        VTR_LOG(
          "\tRunning rtl simulation succeed. Total time spent running rtl simulation: %g "
          "seconds\n",
          sim_end_time);
        return openfpga::CMD_EXEC_SUCCESS;
      }
    }
  } else if (sim_type == "post_synth") {
    for (auto corner : corners_to_sweep) {
      std::string sim_input_files = "";
      std::string source_file = ae_mgr.bitgen_file(
        benchmark_suite_id, benchmark_id, ArchEvalManager::e_file_type::OUT_VERILOG, corner, true);
      sim_input_files += source_file;
      status = ae_mgr.invoke_cocotb_for_application_simulation(
        to_mgr, benchmark_id, benchmark_suite_id, sim_input_files, simulator, corner, sim_type,
        waveform_option);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        float sim_end_time = sim_runtime.elapsed_sec();
        VTR_LOG(
          "\tRunning post_synth simulation failed. Total time spent running post_synth simulation: "
          "%g seconds\n",
          sim_end_time);
        return openfpga::CMD_EXEC_FATAL_ERROR;
      } else {
        float sim_end_time = sim_runtime.elapsed_sec();
        VTR_LOG(
          "\tRunning post_synth simulation succeed. Total time spent running post_synth "
          "simulation: %g seconds\n",
          sim_end_time);
        return openfpga::CMD_EXEC_SUCCESS;
      }
    }
  } else if (sim_type == "post_implementation") {
    for (auto corner : corners_to_sweep) {
      std::string sim_input_files = "";
      std::string source_file = ae_mgr.bitgen_file(
        benchmark_suite_id, benchmark_id,
        ArchEvalManager::e_file_type::VPR_POST_IMPLEMENT_VERILOG_FILE, corner, true);
      sim_input_files += source_file;
      status = ae_mgr.invoke_cocotb_for_application_simulation(
        to_mgr, benchmark_id, benchmark_suite_id, sim_input_files, simulator, corner, sim_type,
        waveform_option);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        float sim_end_time = sim_runtime.elapsed_sec();
        VTR_LOG(
          "\tRunning post_implementation simulation failed. Total time spent running "
          "post_implementation simulation: %g seconds\n",
          sim_end_time);
        return openfpga::CMD_EXEC_FATAL_ERROR;
      } else {
        float sim_end_time = sim_runtime.elapsed_sec();
        VTR_LOG(
          "\tRunning post_implementation simulation succeed. Total time spent running "
          "post_implementation simulation: %g seconds\n",
          sim_end_time);
        return openfpga::CMD_EXEC_SUCCESS;
      }
    }
  }
  VTR_LOG_ERROR("Invalid type of simulation '%s'. Expect [rtl|post_synth|post_implementation]\n",
                sim_type.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}
} /* end namespace arcxpress */
