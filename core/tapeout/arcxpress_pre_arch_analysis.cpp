/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_eval_manager.h"
#include "arkangel_pre_arch_analysis.h"
#include "arkangel_tapeout_utils.h"
#include "command_exit_codes.h"
#include "report_arch_eval_qor_utils.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int analyze_logic_resource(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                           const openfpga::CommandContext& cmd_ctx) {
#ifdef ARCXPRESS_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arcxpress::LicenseManager::LicensedFeatureName::EVAL)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_min_lut_size = cmd.option("min_lut_size");
  openfpga::CommandOptionId opt_max_lut_size = cmd.option("max_lut_size");
  openfpga::CommandOptionId opt_rpt_qor = cmd.option("report_qor");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* If the run exists and it succeeds, throw a warning and return, unless user asks to start over
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::PRE_ARCH;
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

  /* Deposit fail status */
  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  ArchEvalManager ae_mgr(to_mgr, run, bm_mgr);

  if (cmd_ctx.option_enable(cmd, opt_min_lut_size)) {
    ae_mgr.set_min_k(std::stoi(cmd_ctx.option_value(cmd, opt_min_lut_size)));
  }
  if (cmd_ctx.option_enable(cmd, opt_max_lut_size)) {
    ae_mgr.set_max_k(std::stoi(cmd_ctx.option_value(cmd, opt_max_lut_size)));
  }
  for (auto f_type : to_mgr.yosys_script_file_types()) {
    auto ae_f_type = ae_mgr.convert_script_type(f_type);
    /* File tyoe does not matter here, assign any should work */
    bool l_status = ae_mgr.add_yosys_script(ae_f_type, to_mgr.yosys_script_file(f_type));
    if (!l_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::PRE_ARCH_LUT_ANALYZE,
                            TapeoutManager::e_run_status::FAILURE);
  /* Prepare the runtime directory */
  VTR_LOG("Initializing pre-arch analysis directory...\n");
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  status = ae_mgr.sweep_lut_size(to_mgr.frontend_bin_path(), verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Analyze results if specified */
  if (cmd_ctx.option_enable(cmd, opt_rpt_qor)) {
    std::string rpt_fname = cmd_ctx.option_value(cmd, opt_rpt_qor);
    status = report_pre_arch_qor_to_csv_file(rpt_fname, ae_mgr);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::PRE_ARCH_LUT_ANALYZE,
                            TapeoutManager::e_run_status::SUCCESS);

  VTR_LOG("Done\n");

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arcxpress
