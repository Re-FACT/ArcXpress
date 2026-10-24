/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_dev_manager.h"
#include "arch_eval_manager.h"
#include "arkangel_arch_dev.h"
#include "arkangel_file_os.h"
#include "arkangel_setup_utils.h"
#include "arkangel_tapeout_utils.h"
#include "benchmark_packet_utils.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "report_arch_eval_qor_utils.h"
#include "task_scheduler.h"
/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int create_arch(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_ctx) {
#ifdef ARKANGEL_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arkangel::LicenseManager::LicensedFeatureName::EVAL)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_run = cmd.option("run");
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

  /* Deposit fail status */
  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());
  ArchDevManager ad_mgr(to_mgr, run);

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  BenchmarkPacket bm_pkt;
  status = setup_arch_dev_manager(to_mgr, bm_mgr, ad_mgr, bm_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_INIT,
                            TapeoutManager::e_run_status::FAILURE);
  /* Prepare the runtime directory */
  VTR_LOG("Initializing arch directory...\n");
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  status = ad_mgr.init_arch_dir_all(verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_INIT,
                            TapeoutManager::e_run_status::SUCCESS);

  VTR_LOG("Done\n");

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to create a new project based on a configuration file
 *******************************************************************/
int setup_arch(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
               const openfpga::CommandContext& cmd_ctx) {
#ifdef ARKANGEL_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arkangel::LicenseManager::LicensedFeatureName::EVAL)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_run = cmd.option("run");
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

  /* Deposit fail status */
  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());
  ArchDevManager ad_mgr(to_mgr, run);

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  BenchmarkPacket bm_pkt;
  status = setup_arch_dev_manager(to_mgr, bm_mgr, ad_mgr, bm_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_DATA,
                            TapeoutManager::e_run_status::FAILURE);
  VTR_LOGV(verbosity == 1, "Running architecture generator\n");
  status = ad_mgr.exec_arch_gen(verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbosity == 1, "Generating architecture device data\n");
  status = ad_mgr.generate_device_data_all(bm_pkt, to_mgr.openfpga_bin_path(), verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_DATA,
                            TapeoutManager::e_run_status::SUCCESS);

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_INIT_MODULE_NAMES,
                            TapeoutManager::e_run_status::FAILURE);
  VTR_LOGV(verbosity == 1, "Generating initial module names\n");
  status =
    ad_mgr.generate_reference_module_names_all(bm_pkt, to_mgr.openfpga_bin_path(), verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbosity == 1, "Checking architecture consistency between fabrics\n");
  status = ad_mgr.check_fabric_consistency(verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_INIT_MODULE_NAMES,
                            TapeoutManager::e_run_status::SUCCESS);

  VTR_LOG("Done\n");

  fflush(stdout);
  return openfpga::CMD_EXEC_SUCCESS;
}

int show_tapeout_arch(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_type = cmd.option("type");
  openfpga::CommandOptionId opt_fabric = cmd.option("fabric");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  const TapeoutManager& to_mgr = arkangel_ctx.tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
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

  /* Get the data */
  ArchDevManager ad_mgr(to_mgr, run);
  ArchDevManager::e_fabric_type fabric = ArchDevManager::e_fabric_type::ULTIMATE;
  if (cmd_ctx.option_enable(cmd, opt_fabric)) {
    fabric = ad_mgr.str2fabric_type(cmd_ctx.option_value(cmd, opt_fabric), true);
  }
  ArchDevManager::e_file_type data_type =
    ad_mgr.str2file_type(cmd_ctx.option_value(cmd, opt_type), true);
  status = ad_mgr.show_file(fabric, data_type, verbosity == 1);

  fflush(stdout);
  return status;
}

int set_arch_io_num(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                    const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_side = cmd.option("side");
  openfpga::CommandOptionId opt_type = cmd.option("type");
  openfpga::CommandOptionId opt_num = cmd.option("num");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_side));
  std::string side_str = cmd_ctx.option_value(cmd, opt_side);
  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_type));
  std::string type_str = cmd_ctx.option_value(cmd, opt_type);
  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_num));
  int io_num = std::stoi(cmd_ctx.option_value(cmd, opt_num));
  /* sanity checks */
  if (io_num < 0) {
    VTR_LOG_ERROR("Invalid I/O number '%s' which must be a positive number!\n", io_num);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = to_mgr.set_io_num(run, side_str, type_str, io_num, true, verbosity == 1);

  return status;
}

int set_arch_sync_dp_dimension(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  status = to_mgr.set_sync_dp_dimension(run, true, true, verbosity == 1);

  return status;
}

int set_arch_core_dimension(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                            const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_height = cmd.option("height");
  openfpga::CommandOptionId opt_width = cmd.option("width");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_height));
  int core_height = std::stoi(cmd_ctx.option_value(cmd, opt_height));
  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_width));
  int core_width = std::stoi(cmd_ctx.option_value(cmd, opt_width));
  /* sanity checks */
  if (core_height < 2) {
    VTR_LOG_ERROR("Invalid core height '%d' which must be at least 2!\n", core_height);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (core_width < 2) {
    VTR_LOG_ERROR("Invalid I/O number '%d' which must be at least 2!\n", core_width);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = to_mgr.set_core_dimension(run, core_height, core_width, true, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return status;
}

int set_arch_num_clk(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_num = cmd.option("num");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_num));
  int num_clk = std::stoi(cmd_ctx.option_value(cmd, opt_num));
  /* sanity checks */
  if (num_clk < 1) {
    VTR_LOG_ERROR("Invalid number of clock '%d' which must be at least 1!\n", num_clk);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = to_mgr.set_num_clocks(run, num_clk, true, verbosity == 1);

  return status;
}

int set_arch_num_rst(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_num = cmd.option("num");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_num));
  int num_rst = std::stoi(cmd_ctx.option_value(cmd, opt_num));
  /* sanity checks */
  if (num_rst < 1) {
    VTR_LOG_ERROR("Invalid number of resets '%d' which must be at least 1!\n", num_rst);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = to_mgr.set_num_resets(run, num_rst, true, verbosity == 1);

  return status;
}

int set_arch_num_pclk(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_num = cmd.option("num");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_num));
  int num_pclk = std::stoi(cmd_ctx.option_value(cmd, opt_num));
  /* sanity checks */
  if (num_pclk < 1) {
    VTR_LOG_ERROR("Invalid number of programming clock '%d' which must be at least 1!\n", num_pclk);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = to_mgr.set_num_prog_clocks(run, num_pclk, true, verbosity == 1);

  return status;
}

int set_arch_num_config_region(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_num = cmd.option("num");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_num));
  int num_config_region = std::stoi(cmd_ctx.option_value(cmd, opt_num));
  /* sanity checks */
  if (num_config_region < 1) {
    VTR_LOG_ERROR("Invalid number of configurable regions '%d' which must be at least 1!\n",
                  num_config_region);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = to_mgr.set_num_config_regions(run, num_config_region, true, verbosity == 1);

  return status;
}

int remove_arch_io(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_side = cmd.option("side");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_ASSERT(cmd_ctx.option_enable(cmd, opt_side));
  std::string side_str = cmd_ctx.option_value(cmd, opt_side);
  status = to_mgr.remove_io(run, side_str, verbosity == 1);

  return status;
}

int set_tapeout_arch(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_type = cmd.option("type");
  openfpga::CommandOptionId opt_fabric = cmd.option("fabric");
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_file = cmd.option("file");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::ARCH;
  std::string run = to_mgr.invalid_run();
  std::string user_defined_run = to_mgr.invalid_run();
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_run)) {
    user_defined_run = cmd_ctx.option_value(cmd, opt_run);
  }
  status = try_pick_mutable_tapeout_step_run(to_mgr, step_name, user_defined_run, run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Get the data */
  ArchDevManager ad_mgr(to_mgr, run);

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  BenchmarkPacket bm_pkt;
  status = setup_arch_dev_manager(to_mgr, bm_mgr, ad_mgr, bm_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  ArchDevManager::e_fabric_type fabric = ArchDevManager::e_fabric_type::ULTIMATE;
  if (cmd_ctx.option_enable(cmd, opt_fabric)) {
    fabric = ad_mgr.str2fabric_type(cmd_ctx.option_value(cmd, opt_fabric), true);
  }
  ArchDevManager::e_file_type data_type =
    ad_mgr.str2file_type(cmd_ctx.option_value(cmd, opt_type), true);

  /* Use default has a higher priority than the handcrafted file */
  std::string fpath;
  if (cmd_ctx.option_enable(cmd, opt_file)) {
    fpath = cmd_ctx.option_value(cmd, opt_file);
  }

  /* Deposit a failure */
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Different data leads to various decisions
   * - module names has to be same for all the fabric
   */
  if (data_type == ArchDevManager::e_file_type::CRAFTED_MODULE_NAME) {
    to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_INIT_FABRIC_KEY,
                              TapeoutManager::e_run_status::FAILURE);
    for (ArchDevManager::e_fabric_type curr_fb_type : ad_mgr.fabrics()) {
      status = ad_mgr.add_crafted_file(curr_fb_type, data_type, fpath, verbosity == 1);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
    /* TODO: Run fabric key generation */
    VTR_LOGV(verbosity == 1, "Generating initial fabric keys\n");
    status =
      ad_mgr.generate_reference_fabric_key_all(bm_pkt, to_mgr.openfpga_bin_path(), verbosity == 1);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_INIT_FABRIC_KEY,
                              TapeoutManager::e_run_status::SUCCESS);
  } else if (data_type == ArchDevManager::e_file_type::CRAFTED_FABRIC_KEY) {
    /* Deposit a failure */
    to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_IO_COORD,
                              TapeoutManager::e_run_status::FAILURE);
    /* Only allowed for ultimate fabric */
    if (fabric != ArchDevManager::e_fabric_type::ULTIMATE) {
      VTR_LOG_ERROR("Handcrafted fabric key file is only allowed for '%s' fabric!\n",
                    ad_mgr.fabric_type2str(ArchDevManager::e_fabric_type::ULTIMATE).c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    status = ad_mgr.add_crafted_file(fabric, data_type, fpath, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Run io coordinate generation */
    VTR_LOGV(verbosity == 1, "Generating I/O coordinate files\n");
    status = ad_mgr.generate_io_coordinate_all(
      bm_pkt, to_mgr.openfpga_bin_path(), to_mgr.fabric_key_assistant_bin_path(), verbosity == 1);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_IO_COORD,
                              TapeoutManager::e_run_status::SUCCESS);
  } else if (data_type == ArchDevManager::e_file_type::PIN_TABLE_NAMING_RULE) {
    /* Deposit a failure */
    to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_PIN_TABLE,
                              TapeoutManager::e_run_status::FAILURE);
    status = ad_mgr.add_crafted_file(fabric, data_type, fpath, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Run io coordinate generation */
    VTR_LOGV(verbosity == 1, "Generating I/O pin table files\n");
    status = ad_mgr.generate_pin_table(fabric, to_mgr.pin_table_gen_bin_path(), verbosity == 1);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::ARCH_PIN_TABLE,
                              TapeoutManager::e_run_status::SUCCESS);
  }
  /* Update status */
  if (!to_mgr.check_and_update_step_status(step_name, run, verbosity == 1)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  fflush(stdout);
  return status;
}

int eval_tapeout_arch(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_ctx) {
#ifdef ARKANGEL_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arkangel::LicenseManager::LicensedFeatureName::EVAL)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_rpt_qor = cmd.option("report_qor");
  openfpga::CommandOptionId opt_arch_run = cmd.option("arch_run");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_force_copy_arch_data = cmd.option("force_copy_arch_data");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  openfpga::CommandOptionId opt_max_processor = cmd.option("jobs");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }
  int max_processor = 1;
  if (cmd_ctx.option_enable(cmd, opt_max_processor)) {
    max_processor = std::stoi(cmd_ctx.option_value(cmd, opt_max_processor));
  }
  bool force_copy_arch_data = false;
  if (cmd_ctx.option_enable(cmd, opt_force_copy_arch_data)) {
    force_copy_arch_data = true;
  }

  /* Get the run to show any stats
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
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

  /* Sanity checks on the arch run */
  TapeoutManager::e_step_name arch_step_name = TapeoutManager::e_step_name::ARCH;
  std::string arch_run = to_mgr.invalid_run();
  std::string user_defined_arch_run = to_mgr.invalid_run();
  status = openfpga::CMD_EXEC_FATAL_ERROR;

  if (cmd_ctx.option_enable(cmd, opt_arch_run)) {
    user_defined_arch_run = cmd_ctx.option_value(cmd, opt_arch_run);
  }
  status =
    pick_existing_tapeout_step_dep_run(to_mgr, arch_step_name, user_defined_arch_run, arch_run);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Get the data */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
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

  /* TODO: Require previous steps of arch development to be successful!!! */
  to_mgr.add_step_run_dep(step_name, run, arch_step_name, arch_run);
  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::GOLDEN_BITSTREAM,
                            TapeoutManager::e_run_status::FAILURE);

  VTR_LOG("Generating golden bitstream and reports...\n");
  /* Fallback to the classical bitgen run when single-thread is specified. The ArchEval manager will
   * run this way. The reason is that the Windows version of the task scheduler is not working. Need
   * further investigation before enabling it. */
#ifdef _WIN32
  VTR_LOG("Force single-job run during bitstream generation on Windows system\n");
  /* Tasks are independent of each other; therefore, we can use multiple processors to accelerate
   * execution. */
  ParallelTaskScheduler task_scheduler(1, verbosity == 1);
  for (auto corner : corners_to_sweep) {
    status =
      ae_mgr.generate_bitstream(to_mgr.frontend_bin_path(), to_mgr.openfpga_bin_path(), corner,
                                task_scheduler, force_copy_arch_data, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
#else
  auto t0 = std::chrono::steady_clock::now();
  ParallelTaskScheduler task_scheduler(max_processor, verbosity == 1);
  /* Tasks are independent of each other; therefore, we can use multiple processors to accelerate
   * execution. */
  for (auto corner : corners_to_sweep) {
    status =
      ae_mgr.generate_bitstream(to_mgr.frontend_bin_path(), to_mgr.openfpga_bin_path(), corner,
                                task_scheduler, force_copy_arch_data, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  status = task_scheduler.run_tasks();
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  auto t1 = std::chrono::steady_clock::now();
  VTR_LOGV(max_processor > 1,
           "Bitgen runs parallelly using %d processors and takes time %d seconds\n", max_processor,
           std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count());
#endif

  /* Analyze results if specified */
  if (cmd_ctx.option_enable(cmd, opt_rpt_qor)) {
    std::string rpt_fname = cmd_ctx.option_value(cmd, opt_rpt_qor);
    status = report_arch_eval_qor_to_csv_file(rpt_fname, ae_mgr, corners_to_sweep, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  } else {
    status = ae_mgr.gen_builtin_report_bitgen_qor(corners_to_sweep);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::GOLDEN_BITSTREAM,
                            TapeoutManager::e_run_status::SUCCESS);

  /* Update status */
  if (!to_mgr.check_and_update_step_status(step_name, run, verbosity == 1)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  fflush(stdout);
  return status;
}

}  // namespace arcxpress
