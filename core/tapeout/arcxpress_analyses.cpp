/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_eval_manager.h"
#include "arcxpress_analyses.h"
#include "arcxpress_file_os.h"
#include "arcxpress_setup_utils.h"
#include "arcxpress_tapeout_utils.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int analyze_pre_arch_qor(const ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                         const openfpga::CommandContext& cmd_ctx) {
  const TapeoutManager& to_mgr = arcxpress_ctx.tapeout_manager();
  VTR_LOG("Under construction for tapeout project '%s'\n", to_mgr.name().c_str());

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to show status of a tapeout flow/step
 *******************************************************************/
int analyze_arch_eval_qor(const ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                          const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_file = cmd.option("file");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_precision = cmd.option("precision");

  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  int precision = 6;
  if (cmd_ctx.option_enable(cmd, opt_precision)) {
    precision = std::stoi(cmd_ctx.option_value(cmd, opt_precision));
  }

  std::string report_file_path = "";
  if (cmd_ctx.option_enable(cmd, opt_file)) {
    report_file_path = cmd_ctx.option_value(cmd, opt_file);
  }

  /* Get the run to show any stats
   */
  const TapeoutManager& to_mgr = arcxpress_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::BITSTREAM;
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
  /* Sanity checks on the arch run */
  TapeoutManager::e_step_name arch_step_name = TapeoutManager::e_step_name::ARCH;
  std::string arch_run = to_mgr.step_run_dep_run(step_name, run, arch_step_name);
  to_mgr.check_valid_run(arch_step_name, arch_run);

  /* Get the data */
  const BenchmarkManager& bm_mgr = arcxpress_ctx.benchmark_manager();
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

  status = setup_arch_eval_manager(to_mgr, ae_mgr, arch_run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string tag_name = "Benchmark";
  std::string tag_fmax = "Fmax (MHz)";
  std::string corner_hearder = "Corner";
  TablePrinter tp({tag_name, tag_fmax, corner_hearder});
  VTR_LOG("Parsing golden QoR reports...\n");
  for (auto corner_to_sweep : corners_to_sweep) {
    status = openfpga::CMD_EXEC_FATAL_ERROR;
    VTR_LOG("Corner: %s\n", corner_to_sweep.c_str());
    status = ae_mgr.extract_arch_eval_qor(corner_to_sweep, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    status = ae_mgr.show_arch_eval_qor(corner_to_sweep, tp, precision, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  if (!report_file_path.empty()) {
    VTR_LOGV(verbosity, "Arch eval qor will be saved to the file path %s. \n",
             report_file_path.c_str());
  }
  tp.print(report_file_path);
  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arcxpress
