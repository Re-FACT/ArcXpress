/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_dev_manager.h"
#include "arch_eval_manager.h"
#include "arcxpress_export.h"
#include "arcxpress_file_os.h"
#include "arcxpress_graphviz_utils.h"
#include "arcxpress_tapeout_utils.h"
#include "command_exit_codes.h"
#include "netlist_dev_manager.h"
#include "openfpga_digest.h"
/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int export_netlist(const ArcxpressContext& arcxpress_ctx, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_file = cmd.option("file");
  openfpga::CommandOptionId opt_fow = cmd.option("force_overwrite");
  VTR_ASSERT(true == cmd_ctx.option_enable(cmd, opt_file));
  VTR_ASSERT(false == cmd_ctx.option_value(cmd, opt_file).empty());
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_re2w = cmd.option("reduce_error_to_warning");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  bool re2w = false;
  if (cmd_ctx.option_enable(cmd, opt_re2w)) {
    re2w = true;
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arcxpress_ctx.tapeout_manager();
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
    if (re2w) {
      VTR_LOG_WARN(
        "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
        "failed!\n",
        to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    } else {
      VTR_LOG_ERROR(
        "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
        "failed!\n",
        to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
      return openfpga::CMD_EXEC_MINOR_ERROR;
    }
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    if (re2w) {
      VTR_LOG_WARN(
        "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
        "fresh!\n",
        to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    } else {
      VTR_LOG_ERROR(
        "Gate-level netlists are not available due to required substep '%s' of netlist run '%s' is "
        "fresh!\n",
        to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
      return openfpga::CMD_EXEC_MINOR_ERROR;
    }
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Gate-level netlists may not be correct as required substep '%s' of netlist run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Setup */
  const PdkManager& pdk_mgr = arcxpress_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);
  VTR_LOG("Export netlist package for the netlist run '%s' of tapeout project: %s\n", run.c_str(),
          to_mgr.name().c_str());
  bool fow = cmd_ctx.option_enable(cmd, opt_fow);
  std::string packaged_fname = cmd_ctx.option_value(cmd, opt_file);
  status = nd_mgr.export_netlist(packaged_fname, fow, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int export_arch(const ArcxpressContext& arcxpress_ctx, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_file = cmd.option("file");
  openfpga::CommandOptionId opt_fow = cmd.option("force_overwrite");
  VTR_ASSERT(true == cmd_ctx.option_enable(cmd, opt_file));
  VTR_ASSERT(false == cmd_ctx.option_value(cmd, opt_file).empty());
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Select the proper run */
  const TapeoutManager& to_mgr = arcxpress_ctx.tapeout_manager();
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
      "Arch data are not available due to required substep '%s' of arch run '%s' is "
      "failed!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) == TapeoutManager::e_run_status::FRESH) {
    VTR_LOG_ERROR(
      "Arch data are not available due to required substep '%s' of arch run '%s' is "
      "fresh!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
    return openfpga::CMD_EXEC_MINOR_ERROR;
  }
  if (to_mgr.substep_status(step_name, run, s_step_name) ==
      TapeoutManager::e_run_status::REQ_REFRESH) {
    VTR_LOG_WARN(
      "Arch data may not be correct as required substep '%s' of netlist run '%s' is "
      "out-of-date!\n",
      to_mgr.substep_name2str(s_step_name).c_str(), run.c_str());
  }

  /* Setup */
  ArchDevManager ad_mgr(to_mgr, run);
  VTR_LOG("Export architecture package for the arch run '%s' of tapeout project: %s\n", run.c_str(),
          to_mgr.name().c_str());
  bool fow = cmd_ctx.option_enable(cmd, opt_fow);
  std::string packaged_fname = cmd_ctx.option_value(cmd, opt_file);
  status = ad_mgr.export_arch(packaged_fname, fow, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

// If Graphviz is found, enable the following functionality
// This is a temporay fix due to the graphviz package issue on CentOS 7.9
#ifdef USE_GRAPHVIZ
int export_benchmark_schematic(ArcxpressContext& arcxpress_ctx, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_ctx) {
  /* Must have a valid tapeout first */
  if (!arcxpress_ctx.is_tapeout_created()) {
    VTR_LOG_ERROR("A valid tapeout must be created or loaded!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Get tapeout manager early */
  TapeoutManager& to_mgr = arcxpress_ctx.mutable_tapeout_manager();

  /* Define step name early */
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;

  /* Run handling */
  openfpga::CommandOptionId opt_run = cmd.option("run");
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

  /* Check if the eval_arch step has been successfully completed */
  /* Since eval_arch runs under BITSTREAM step with GOLDEN_BITSTREAM substep, we need to check that
   */
  TapeoutManager::e_step_name bitstream_step_name = TapeoutManager::e_step_name::BITSTREAM;
  TapeoutManager::e_substep_name eval_arch_step = TapeoutManager::e_substep_name::GOLDEN_BITSTREAM;
  std::string bitstream_run = to_mgr.invalid_run();
  std::string user_defined_bitstream_run = to_mgr.invalid_run();

  // Try to find a corresponding bitstream run that would contain the eval_arch results
  status = autopick_tapeout_step_run(to_mgr, bitstream_step_name, user_defined_bitstream_run,
                                     bitstream_run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    // If no bitstream run exists yet, we can't proceed with benchmark schematic export
    VTR_LOG_ERROR(
      "Bitstream run is required to execute eval_arch before exporting benchmark schematic. No "
      "bitstream run found!\n");
    return openfpga::CMD_EXEC_MINOR_ERROR;
  } else {
    // Check the actual eval_arch results (stored in GOLDEN_BITSTREAM substep of BITSTREAM step)
    if (to_mgr.substep_status(bitstream_step_name, bitstream_run, eval_arch_step) ==
        TapeoutManager::e_run_status::FAILURE) {
      VTR_LOG_ERROR(
        "Benchmark schematic export failed because required substep '%s' of bitstream run '%s' has "
        "failed!\n",
        to_mgr.substep_name2str(eval_arch_step).c_str(), bitstream_run.c_str());
      return openfpga::CMD_EXEC_MINOR_ERROR;
    }
    if (to_mgr.substep_status(bitstream_step_name, bitstream_run, eval_arch_step) ==
        TapeoutManager::e_run_status::FRESH) {
      VTR_LOG_ERROR(
        "Benchmark schematic export failed because required substep '%s' of bitstream run '%s' has "
        "not been executed!\n",
        to_mgr.substep_name2str(eval_arch_step).c_str(), bitstream_run.c_str());
      return openfpga::CMD_EXEC_MINOR_ERROR;
    }
    if (to_mgr.substep_status(bitstream_step_name, bitstream_run, eval_arch_step) ==
        TapeoutManager::e_run_status::REQ_REFRESH) {
      VTR_LOG_WARN(
        "Benchmark schematic export may use outdated data because required substep '%s' of "
        "bitstream run '%s' is out-of-date!\n",
        to_mgr.substep_name2str(eval_arch_step).c_str(), bitstream_run.c_str());
    }
  }

  /* Corner handling */
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  std::string eval_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    eval_corner = cmd_ctx.option_value(cmd, opt_corner);
  }

  if (!to_mgr.valid_corner_name(eval_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  eval_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Output file */
  openfpga::CommandOptionId opt_file = cmd.option("file");

  std::string file_path = cmd_ctx.option_value(cmd, opt_file);

  /* Benchmark name */
  openfpga::CommandOptionId benchmark = cmd.option("benchmark");
  std::string benchmark_name;
  if (cmd_ctx.option_enable(cmd, benchmark)) {
    benchmark_name = cmd_ctx.option_value(cmd, benchmark);
  }

  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Validate extension */
  std::string extension = file_path.substr(file_path.find_last_of('.') + 1);
  if (!is_graphviz_supported_format(extension)) {
    VTR_LOG_ERROR("Unsupported file format '%s'.\n", extension.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  } else {
    VTR_LOGV(verbosity, "Converting Graphviz to %s...\n", extension.c_str());
  }
  /* Get benchmark data */
  const BenchmarkManager& bm_mgr = arcxpress_ctx.benchmark_manager();
  ArchEvalManager ae_mgr(to_mgr, run, bm_mgr);

  BenchmarkSuiteId st_id = bm_mgr.find_benchmark_suite(benchmark_name);
  BenchmarkId bm_id = bm_mgr.find_benchmark_by_name(benchmark_name);

  std::string input_file_path = ae_mgr.bitgen_file(
    st_id, bm_id, ArchEvalManager::e_file_type::OUT_BLOCK_DIAGRAM, eval_corner, false);

  if (input_file_path.empty()) {
    VTR_LOG_ERROR("Failed to generate block diagram file path.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Convert Graphviz to image */
  status =
    arcxpress::convert_graphviz_to_image(input_file_path, file_path, extension, verbosity = 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    fflush(stdout);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  fflush(stdout);
  return status;
}
#endif

}  // namespace arcxpress
