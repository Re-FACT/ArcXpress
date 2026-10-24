/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "arch_dev_manager.h"
#include "arch_gen_ptpx_sta_files.h"
#include "arkangel_file_os.h"
#include "arkangel_netlist_dev.h"
#include "arkangel_setup_utils.h"
#include "arkangel_tapeout_utils.h"
#include "benchmark_packet_utils.h"
#include "command_exit_codes.h"
#include "global_port_loc_map.h"
#include "netlist_dev_manager.h"
#include "openfpga_digest.h"
#include "qor_manager.h"
#include "read_xml_clk_loc_map.h"

/* begin namespace arcxpress */
namespace arcxpress {

static int setup_netlist_dev_manager_synth_aspect(
  const TapeoutManager& to_mgr, const PdkManager& pdk_mgr, NetlistDevManager& nd_mgr,
  const std::string& selected_corner, const std::string& cust_synth_strtg = std::string()) {
  nd_mgr.set_source_netlist_makeup_dir(to_mgr.netlist_makeup_key_root_path());
  nd_mgr.set_source_synth_sdc_dir(to_mgr.synth_sdc_root_path(pdk_mgr.name()));
  if (cust_synth_strtg.empty()) {
    nd_mgr.set_source_synth_strategy_file(to_mgr.synth_strategy_file(pdk_mgr.name()));
  } else {
    nd_mgr.set_source_synth_strategy_file(cust_synth_strtg);
  }
  nd_mgr.set_source_floorplan_strategy_file(to_mgr.floorplan_strategy_file(pdk_mgr.name()));
  nd_mgr.set_ccff_max_fanout(to_mgr.ccff_max_fanout(pdk_mgr.name()));
  if (to_mgr.pdk_require_netlist_makeup_config(pdk_mgr.name())) {
    nd_mgr.set_netlist_makeup_config_file(to_mgr.netlist_makeup_config_file(pdk_mgr.name()));
  }
  nd_mgr.set_ccff_makeup_buf_config_file(to_mgr.ccff_makeup_buf_config_file(pdk_mgr.name()));

  /* Setup PDK files */
  nd_mgr.set_source_pdk_dir(pdk_mgr.root());
  /* Add ip files that require special format */
  for (auto f_id : to_mgr.ip_files_by_type(TapeoutManager::e_ip_file_type::RTL)) {
    nd_mgr.add_ip_file(to_mgr.ip_file(f_id), to_mgr.ip_file_format(f_id));
  }
  /* Add ip files */
  for (auto f_id : to_mgr.ip_files_by_type(TapeoutManager::e_ip_file_type::GL)) {
    nd_mgr.add_ip_gl_file(to_mgr.ip_file(f_id));
  }
  for (std::string fpath : pdk_mgr.sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    nd_mgr.add_pdk_verilog(fpath);
  }
  /* Default corner: Consider the SCLIB corner tag here as synthesis only requires this one */
  nd_mgr.set_default_corner(
    to_mgr.corner_tag(selected_corner, TapeoutManager::e_corner_tag_type::SCLIB));
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Load essential files to arch development manager */
static int setup_netlist_dev_manager(const TapeoutManager& to_mgr, const BenchmarkManager& bm_mgr,
                                     const PdkManager& pdk_mgr, const std::string& arch_run,
                                     NetlistDevManager& nd_mgr, BenchmarkPacket& bm_pkt,
                                     BenchmarkPacket& bm4clk_sweep_pkt,
                                     BenchmarkPacket& bm4rst_sweep_pkt) {
  ArchDevManager ad_mgr(to_mgr, arch_run);
  /* Setup the source arch directory where netlist development will be based on */
  nd_mgr.set_source_arch_dir(ad_mgr.arch_dir());
  for (auto f_type : to_mgr.openfpga_setting_file_types()) {
    if (!nd_mgr.required_file_type(f_type)) {
      continue;
    }
    auto nd_f_type = nd_mgr.convert_file_type(f_type);
    /* File type does not matter here, assign any should work */
    bool l_status = nd_mgr.add_file(NetlistDevManager::e_fabric_type::ULTIMATE, nd_f_type,
                                    to_mgr.openfpga_setting_file(f_type));
    if (!l_status) {
      VTR_LOG_ERROR("Failed to add the openfpga setting file '%s'\n",
                    to_mgr.openfpga_setting_file(f_type).c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  for (auto f_type : to_mgr.openfpga_script_file_types()) {
    auto nd_f_type = nd_mgr.convert_script_type(f_type);
    if (nd_mgr.valid_script_type(nd_f_type)) {
      nd_mgr.add_openfpga_script(nd_f_type, to_mgr.openfpga_script_file(f_type));
    }
  }
  nd_mgr.set_route_chan_width(to_mgr.route_chan_width());
  nd_mgr.set_ccff_max_fanout(to_mgr.ccff_max_fanout(pdk_mgr.name()));
  nd_mgr.set_source_netlist_makeup_dir(to_mgr.netlist_makeup_key_root_path());
  /* Netlist flags */
  for (auto f_name : to_mgr.netlist_flags(pdk_mgr.name())) {
    bool nflag_status = nd_mgr.add_netlist_flag(f_name, to_mgr.flag_value(pdk_mgr.name(), f_name));
    if (!nflag_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Add ip files */
  for (auto f_id : to_mgr.ip_files_by_type(TapeoutManager::e_ip_file_type::RTL)) {
    nd_mgr.add_ip_file(to_mgr.ip_file(f_id), to_mgr.ip_file_format(f_id));
  }
  /* Setup PDK files */
  nd_mgr.set_source_pdk_dir(pdk_mgr.root());
  for (std::string fpath : pdk_mgr.sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    nd_mgr.add_pdk_verilog(fpath);
  }
  /* Get benchmarks */
  int status = get_tapeout_default_benchmark_packet(to_mgr, bm_mgr, bm_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = get_tapeout_default_benchmark_packet_for_clock_sweep(to_mgr, bm_mgr, bm4clk_sweep_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = get_tapeout_default_benchmark_packet_for_reset_sweep(to_mgr, bm_mgr, bm4rst_sweep_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  if (to_mgr.pdk_require_netlist_makeup_config(pdk_mgr.name())) {
    nd_mgr.set_netlist_makeup_config_file(to_mgr.netlist_makeup_config_file(pdk_mgr.name()));
  }
  /* Default corner */
  nd_mgr.set_default_corner(to_mgr.default_corner());

  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Top-level function to generate RTL netlist
 *******************************************************************/
int generate_netlist(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_nlist_makeup = cmd.option("netlist_makeup");
  openfpga::CommandOptionId opt_arch_run = cmd.option("arch_run");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* If the run exists and it succeeds, throw a warning and return, unless user asks to start over
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
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

  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);
  BenchmarkPacket bm_pkt;
  BenchmarkPacket bm4clk_sweep_pkt;
  BenchmarkPacket bm4rst_sweep_pkt;
  status = setup_netlist_dev_manager(to_mgr, bm_mgr, pdk_mgr, arch_run, nd_mgr, bm_pkt,
                                     bm4clk_sweep_pkt, bm4rst_sweep_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Start with a deposit of fail status */
  to_mgr.add_step_run_dep(step_name, run, arch_step_name, arch_run);
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_GEN,
                            TapeoutManager::e_run_status::FAILURE);
  /* Prepare the runtime directory */
  VTR_LOG("Initializing netlist directory...\n");
  status = openfpga::CMD_EXEC_FATAL_ERROR;
  status = nd_mgr.init_netlist_dir_all(verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Generating RTL netlists\n");

  bool enable_nlist_makeup = cmd_ctx.option_enable(cmd, opt_nlist_makeup);
  status = nd_mgr.generate_netlist_all(bm_pkt, to_mgr.openfpga_bin_path(),
                                       to_mgr.netlist_makeup_bin_path(), enable_nlist_makeup,
                                       verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Generating base bitstream\n");
  status = nd_mgr.generate_base_bitstream(bm_pkt, to_mgr.openfpga_bin_path(), verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Generating bitstream for clock sweeping\n");
  status = nd_mgr.generate_clock_sweep_bitstream(bm4clk_sweep_pkt, to_mgr.openfpga_bin_path(),
                                                 verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Generating bitstream for reset sweeping\n");
  status = nd_mgr.generate_reset_sweep_bitstream(bm4rst_sweep_pkt, to_mgr.openfpga_bin_path(),
                                                 verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_GEN,
                            TapeoutManager::e_run_status::SUCCESS);

  fflush(stdout);
  return status;
}

/********************************************************************
 * Top-level function to optimize netlists
 *******************************************************************/
int netlist_opt(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_ctx) {
#ifdef ARCXPRESS_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arcxpress::LicenseManager::LicensedFeatureName::PROTOTYPE)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_stage = cmd.option("stage");
  openfpga::CommandOptionId opt_dryrun = cmd.option("dryrun");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_strtg = cmd.option("strategy");
  openfpga::CommandOptionId opt_j = cmd.option("jobs");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* If the run exists and it succeeds, throw a warning and return, unless user asks to start over
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
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

  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  /* Error out if the pdk is not supported */
  if (!to_mgr.is_pdk_natively_supported_in_synth(pdk_mgr.name()) &&
      !cmd_ctx.option_enable(cmd, opt_strtg)) {
    VTR_LOG_ERROR(
      "The pdk '%s' is not natively supported for netlist optmization. Please provide a custom "
      "synthesis strategy!\n",
      pdk_mgr.name().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_OPT,
                            TapeoutManager::e_run_status::FAILURE);

  /* Select corner, if not specified, use the default */
  std::string syn_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    syn_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  if (!to_mgr.valid_corner_name(syn_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  syn_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string cust_synth_strtg;
  if (cmd_ctx.option_enable(cmd, opt_strtg)) {
    cust_synth_strtg = cmd_ctx.option_value(cmd, opt_strtg);
  }
  status =
    setup_netlist_dev_manager_synth_aspect(to_mgr, pdk_mgr, nd_mgr, syn_corner, cust_synth_strtg);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string stage_sel = nd_mgr.opt_stage_all_str();
  if (cmd_ctx.option_enable(cmd, opt_stage)) {
    stage_sel = cmd_ctx.option_value(cmd, opt_stage);
  }
  VTR_LOGV("User selected to run stage '%s'\n", stage_sel.c_str());

  /* Prepare the runtime directory */
  VTR_LOGV(verbosity == 1, "Optimizing RTL netlists\n");
  bool dryrun = cmd_ctx.option_enable(cmd, opt_dryrun);

  size_t max_jobs = 1;
  if (cmd_ctx.option_enable(cmd, opt_j)) {
    max_jobs = std::stoi(cmd_ctx.option_value(cmd, opt_j));
  }
  status = nd_mgr.optimize_netlist(to_mgr.ccff_makeup_bin_path(), to_mgr.synth_bin_path(),
                                   stage_sel, dryrun, max_jobs, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_OPT,
                            TapeoutManager::e_run_status::SUCCESS);

  fflush(stdout);
  return status;
}

/********************************************************************
 * Top-level function to optimize netlists
 *******************************************************************/
int analyze_tile_area(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_dryrun = cmd.option("dryrun");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_j = cmd.option("jobs");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* If the run exists and it succeeds, throw a warning and return, unless user asks to start over
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
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

  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_COARSE_ANALYZE,
                            TapeoutManager::e_run_status::FAILURE);

  /* Select corner, if not specified, use the default */
  std::string syn_corner = to_mgr.default_corner();
  if (cmd_ctx.option_enable(cmd, opt_corner)) {
    syn_corner = cmd_ctx.option_value(cmd, opt_corner);
  }
  if (!to_mgr.valid_corner_name(syn_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  syn_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  status = setup_netlist_dev_manager_synth_aspect(to_mgr, pdk_mgr, nd_mgr, syn_corner);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Prepare the runtime directory */
  VTR_LOGV(verbosity == 1, "Coarse analyze netlists\n");
  bool dryrun = cmd_ctx.option_enable(cmd, opt_dryrun);
  size_t max_jobs = 1;
  if (cmd_ctx.option_enable(cmd, opt_j)) {
    max_jobs = std::stoi(cmd_ctx.option_value(cmd, opt_j));
  }
  status = nd_mgr.coarse_analyze_netlist(to_mgr.synth_bin_path(), dryrun, max_jobs, verbosity == 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_COARSE_ANALYZE,
                            TapeoutManager::e_run_status::SUCCESS);

  fflush(stdout);
  return status;
}

/********************************************************************
 * Top-level function to optimize netlists
 *******************************************************************/
int coarse_floorplan(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx) {
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_rptf = cmd.option("report_file");
  openfpga::CommandOptionId opt_rptf_shaping = cmd.option("shaping_report_file");

  openfpga::CommandOptionId opt_strategy = cmd.option("strategy");

  std::string rpt_fpath;
  std::string strategy_file;
  TapeoutManager::e_substep_name substep_name =
    TapeoutManager::e_substep_name::NETLIST_COARSE_FLOORPLAN;

  if (cmd_ctx.option_enable(cmd, opt_rptf)) {
    rpt_fpath = cmd_ctx.option_value(cmd, opt_rptf);
  }

  openfpga::CommandOptionId opt_precision = cmd.option("precision");
  int precision = 2;
  if (cmd_ctx.option_enable(cmd, opt_precision)) {
    precision = std::stoi(cmd_ctx.option_value(cmd, opt_precision));
  }
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* If the run exists and it succeeds, throw a warning and return, unless user asks to start over
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
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

  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());

  /* Setup */
  const BenchmarkManager& bm_mgr = arkangel_ctx.benchmark_manager();
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  std::string shaping_rpt = "";
  if (cmd_ctx.option_enable(cmd, opt_rptf_shaping)) {
    shaping_rpt = cmd_ctx.option_value(cmd, opt_rptf_shaping);
  }
  /* Error out if the pdk is not supported */
  if (!to_mgr.is_pdk_natively_supported_in_floorplan(pdk_mgr.name())) {
    VTR_LOG_ERROR(
      "The pdk '%s' is not natively supported for floorplanning. Please provide a custom synthesis "
      "strategy!\n",
      pdk_mgr.name().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  if (cmd_ctx.option_enable(cmd, opt_strategy)) {
    strategy_file = cmd_ctx.option_value(cmd, opt_strategy);
  }
  if (!strategy_file.empty()) {
    if (!std::filesystem::exists(strategy_file)) {
      VTR_LOG_ERROR("Strategy file '%s' does not exist!\n", strategy_file.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    nd_mgr.set_source_floorplan_strategy_file(strategy_file);
  }

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, substep_name, TapeoutManager::e_run_status::FAILURE);

  /* Select corner, if not specified, use the default */
  std::string syn_corner = to_mgr.default_corner();

  if (!to_mgr.valid_corner_name(syn_corner)) {
    VTR_LOG_ERROR("Invalid corner name '%s' which is not defined or being disabled \n",
                  syn_corner.c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  status = setup_netlist_dev_manager_synth_aspect(to_mgr, pdk_mgr, nd_mgr, syn_corner);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Prepare the runtime directory */

  VTR_LOGV(verbosity == 1, "Coarse floorplanning for DP fabric\n");
  /* Set rpt_fpath to an empty string to avoid generating an output file. */
  status = nd_mgr.coarse_floorplan("", shaping_rpt, precision, verbosity == 1,
                                   NetlistDevManager::e_fabric_type::DP, strategy_file);

  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbosity == 1, "Coarse floorplanning for Ultimate fabric\n");
  status = nd_mgr.coarse_floorplan(rpt_fpath, shaping_rpt, precision, verbosity == 1,
                                   NetlistDevManager::e_fabric_type::ULTIMATE, strategy_file);

  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  to_mgr.set_substep_status(step_name, run, substep_name, TapeoutManager::e_run_status::SUCCESS);

  fflush(stdout);
  return status;
}

/********************************************************************
 * Top-level function to optimize netlists
 *******************************************************************/
int extract_netlist_qor(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx) {
#ifdef ARCXPRESS_LICENSED_BUILD
  if (!arkangel_ctx.license_manager().checkOut(
        arcxpress::LicenseManager::LicensedFeatureName::PROTOTYPE)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  openfpga::CommandOptionId opt_run = cmd.option("run");
  openfpga::CommandOptionId opt_dryrun = cmd.option("dryrun");
  openfpga::CommandOptionId opt_task_name = cmd.option("task_name");
  openfpga::CommandOptionId opt_type = cmd.option("type");
  openfpga::CommandOptionId opt_corner = cmd.option("corner");
  openfpga::CommandOptionId opt_clock_freq = cmd.option("clock_freq");
  openfpga::CommandOptionId opt_j = cmd.option("jobs");
  openfpga::CommandOptionId opt_verbosity = cmd.option("verbosity");
  std::string clk_freq = "100";  // [MHz]
  if (cmd_ctx.option_enable(cmd, opt_clock_freq)) {
    clk_freq = cmd_ctx.option_value(cmd, opt_clock_freq);
  }

  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* If the run exists and it succeeds, throw a warning and return, unless user asks to start over
   */
  TapeoutManager& to_mgr = arkangel_ctx.mutable_tapeout_manager();
  TapeoutManager::e_step_name step_name = TapeoutManager::e_step_name::NETLIST;
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

  /* Launch the arch dev manager */
  VTR_LOGV(verbosity == 1, "Project path: %s\n", to_mgr.path().c_str());
  VTR_LOGV(verbosity == 1, "Project name: %s\n", to_mgr.name().c_str());

  /* Setup */
  const PdkManager& pdk_mgr = arkangel_ctx.pdk_manager();
  NetlistDevManager nd_mgr(to_mgr, run, pdk_mgr);

  /* Setup Qor types */
  TapeoutManager::e_qorso_task_type qor_type = TapeoutManager::e_qorso_task_type::NUM_TYPES;
  if (cmd_ctx.option_enable(cmd, opt_type)) {
    qor_type = to_mgr.str2qorso_task_type(cmd_ctx.option_value(cmd, opt_type), true);
    if (!to_mgr.valid_qorso_task_type(qor_type)) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* Start with a deposit of fail status */
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_QOR,
                            TapeoutManager::e_run_status::FAILURE);
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

  std::string req_qor_task_name = "";
  if (cmd_ctx.option_enable(cmd, opt_task_name)) {
    req_qor_task_name = cmd_ctx.option_value(cmd, opt_task_name);
  }

  for (auto corner_to_sweep : corners_to_sweep) {
    /* Each time the QoR manager is refreshed. Otherwise, tasks are accumulated*/
    QorManager qor_mgr(nd_mgr.project_home_dir().string(), to_mgr, pdk_mgr);

    /* Setup QOR manager - handles all tasks including timing_gclk uniformly */
    status =
      setup_qor_manager_post_synth(to_mgr, nd_mgr, pdk_mgr, qor_mgr, qor_type, req_qor_task_name,
                                   corner_to_sweep, clk_freq, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Failed to setup QOR manager\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    /* Prepare the runtime directory */
    VTR_LOGV(verbosity == 1, "Quality-of-Result extraction on netlists\n");
    VTR_LOGV(verbosity == 1, "Corner: %s\n", corner_to_sweep.c_str());
    bool dryrun = cmd_ctx.option_enable(cmd, opt_dryrun);
    size_t max_jobs = 1;
    if (cmd_ctx.option_enable(cmd, opt_j)) {
      max_jobs = std::stoi(cmd_ctx.option_value(cmd, opt_j));
    }
    status = qor_mgr.extract_qor(to_mgr.qorso_bin_path(), dryrun, max_jobs, verbosity == 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    fflush(stdout);
  }
  to_mgr.set_substep_status(step_name, run, TapeoutManager::e_substep_name::NETLIST_QOR,
                            TapeoutManager::e_run_status::SUCCESS);
  fflush(stdout);

  return status;
}

}  // namespace arcxpress