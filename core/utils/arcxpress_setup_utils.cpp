/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */

#include "arch_gen_ptpx_sta_files.h"
#include "arcxpress_setup_utils.h"
#include "arcxpress_tapeout_utils.h"
#include "circuit_types.h"
#include "command_exit_codes.h"
#include "global_port_loc_map.h"
#include "netlist_dev_manager.h"
#include "openfpga_digest.h"
#include "read_xml_clk_loc_map.h"
#include "read_xml_pin_constraints.h"

/* begin namespace arcxpress */
namespace arcxpress {

int setup_qor_manager_post_synth(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                                 const PdkManager& pdk_mgr, QorManager& qor_mgr,
                                 const TapeoutManager::e_qorso_task_type& qor_type,
                                 const std::string& req_qtask_name, const std::string& qor_corner,
                                 const std::string& clk_freq, const bool& verbose) {
  /* Add corner map */
  /* TODO: Support ocv corner from project file */
  std::string tag = qor_corner;
  qor_mgr.set_corner(tag, to_mgr.corner_tag(tag, TapeoutManager::e_corner_tag_type::SCLIB),
                     to_mgr.corner_tag(tag, TapeoutManager::e_corner_tag_type::SPEF),
                     std::string("tt"));

  /* Add tasks - Keep original logic unchanged */
  size_t added_num_tasks = 0;
  for (auto qtask_name : to_mgr.qorso_tasks()) {
    if (to_mgr.valid_qorso_task_type(qor_type)) {
      auto task_type = to_mgr.qorso_task_type(qtask_name);
      /* Select tasks strictly by type */
      if (qor_type != task_type) {
        continue;
      }
    }
    /* Check if there is a filter on the qor task name */
    bool add_curr_qtask = true;
    if (!req_qtask_name.empty() && qtask_name != req_qtask_name) {
      add_curr_qtask = false;
    }
    if (!add_curr_qtask) {
      continue;
    }
    VTR_LOGV(verbose && !req_qtask_name.empty() && add_curr_qtask,
             "User selects to run single task '%s' in this qor extraction\n", qtask_name.c_str());
    QorsoTaskId qor_task_id = qor_mgr.create_task(qtask_name, to_mgr.qorso_task_type(qtask_name));

    qor_mgr.set_task_config(qor_task_id, to_mgr.qorso_task_config(qtask_name, pdk_mgr.name()));

    added_num_tasks++;
  }
  if (added_num_tasks == 0) {
    VTR_LOG_ERROR(
      "No qor extraction task is selected. This may be due to an invalid task name or the task "
      "name does not fall in the selected type\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Add netlist source directory */
  NetlistDevManager::e_netlist_type nlist_type = NetlistDevManager::e_netlist_type::GL;
  qor_mgr.set_source_netlist_dir(nd_mgr.netlist_release_dir(nlist_type));
  qor_mgr.set_source_netlist_type(nd_mgr.netlist_type2str(nlist_type));

  /* Get tile ref file and fabric hierarchy file */
  NetlistDevManager::e_fabric_type fb_type = NetlistDevManager::e_fabric_type::ULTIMATE;
  std::string tref_fpath = nd_mgr.file(fb_type, NetlistDevManager::e_file_type::TILE_REF, verbose);
  std::string fb_hie_fpath =
    nd_mgr.file(fb_type, NetlistDevManager::e_file_type::FABRIC_HIERARCHY_FILE, verbose);
  qor_mgr.set_tile_reference_file(tref_fpath);
  qor_mgr.set_fabric_hierarchy_file(fb_hie_fpath);

  /* Set global clock period */
  int status = qor_mgr.set_gclk_port(nd_mgr.gclk_port());
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = qor_mgr.set_grst_port(nd_mgr.grst_port());
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = qor_mgr.set_gclk_clock_freq(clk_freq);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Set sdc file path */
  std::string sdc_file_base_path =
    nd_mgr.file(NetlistDevManager::e_fabric_type::ULTIMATE,
                NetlistDevManager::e_file_type::BASE_BITSTREAM_FILE, verbose);
  qor_mgr.set_sdc_file_base_dirpath(sdc_file_base_path);
  /* For each clock and reset, add bitstream value as sdc file */
  for (auto clk_sweep_id : nd_mgr.gclock_sweep_ids()) {
    std::string sdc_file_value_path = nd_mgr.gclock_bitstream_value_file(clk_sweep_id, verbose);
    qor_mgr.set_gclk_sdc_file_value_dirpath(clk_sweep_id, sdc_file_value_path);
  }
  for (auto rst_sweep_id : nd_mgr.greset_sweep_ids()) {
    std::string sdc_file_value_path = nd_mgr.greset_bitstream_value_file(rst_sweep_id, verbose);
    qor_mgr.set_grst_sdc_file_value_dirpath(rst_sweep_id, sdc_file_value_path);
  }

  /* Set report file path */
  std::string rpt_timing_dirpath = nd_mgr.report_timing_dirpath(fb_type, nlist_type);
  qor_mgr.set_timing_report_dirpath(rpt_timing_dirpath);

  std::string ppa_smry_dirpath = nd_mgr.ppa_summary_dirpath(fb_type, nlist_type);
  qor_mgr.set_ppa_summary_dirpath(ppa_smry_dirpath);

  return openfpga::CMD_EXEC_SUCCESS;
}

int setup_qor_manager_dynamic_power(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                                    const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                    const DvManager& dv_mgr, QorManager& qor_mgr,
                                    const std::string& qor_corner, const bool& verbose) {
  /* Add corner map */
  /* TODO: Support ocv corner from project file */
  std::string tag = qor_corner;
  qor_mgr.set_corner(tag, to_mgr.corner_tag(tag, TapeoutManager::e_corner_tag_type::SCLIB),
                     to_mgr.corner_tag(tag, TapeoutManager::e_corner_tag_type::SPEF),
                     std::string("tt"));
  /* Add netlist source directory */
  NetlistDevManager::e_netlist_type nlist_type = NetlistDevManager::e_netlist_type::GL;
  qor_mgr.set_source_netlist_dir(nd_mgr.netlist_release_dir(nlist_type));
  qor_mgr.set_source_netlist_type(nd_mgr.netlist_type2str(nlist_type));

  /* Set report file path */
  std::string rpt_timing_dirpath = dv_mgr.report_timing_dirpath(DvManager::e_netlist_type::GL);
  qor_mgr.set_timing_report_dirpath(rpt_timing_dirpath);

  std::string ppa_smry_dirpath = dv_mgr.ppa_summary_dirpath(DvManager::e_netlist_type::GL);
  qor_mgr.set_ppa_summary_dirpath(ppa_smry_dirpath);

  /* Set sdc file path */
  std::string sdc_file_base_path =
    dv_mgr.file(st_id, bm_id, DvManager::e_file_type::BASE_BITSTREAM_XML_FILE, qor_corner, verbose);
  std::string sdc_file_value_path =
    dv_mgr.file(st_id, bm_id, DvManager::e_file_type::BITSTREAM_XML_FILE, qor_corner, verbose);
  std::string sim_file_runtime_dir =
    dv_mgr.file(st_id, bm_id, DvManager::e_file_type::WAVEFORM_FSDB, qor_corner, verbose);
  qor_mgr.set_sdc_file_base_dirpath(sdc_file_base_path);
  qor_mgr.set_sdc_file_value_dirpath(sdc_file_value_path);
  qor_mgr.set_preconifgured_sim_waveform_dirpath(sim_file_runtime_dir);
  return openfpga::CMD_EXEC_SUCCESS;
}

int setup_qor_manager_sta(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                          const BenchmarkManager& bm_mgr, const BenchmarkSuiteId& st_id,
                          const BenchmarkId& bm_id, const DvManager& dv_mgr, QorManager& qor_mgr,
                          const std::string& qor_corner, const std::string& clock_period,
                          const bool& verbose) {
  /* Add corner map */
  /* TODO: Support ocv corner from project file */
  std::string tag = qor_corner;
  qor_mgr.set_corner(tag, to_mgr.corner_tag(tag, TapeoutManager::e_corner_tag_type::SCLIB),
                     to_mgr.corner_tag(tag, TapeoutManager::e_corner_tag_type::SPEF),
                     std::string("tt"));
  /* Add netlist source directory */
  NetlistDevManager::e_netlist_type nlist_type = NetlistDevManager::e_netlist_type::GL;
  qor_mgr.set_source_netlist_dir(nd_mgr.netlist_release_dir(nlist_type));
  qor_mgr.set_source_netlist_type(nd_mgr.netlist_type2str(nlist_type));

  /* Set report file path */
  std::string rpt_timing_dirpath = dv_mgr.report_timing_dirpath(DvManager::e_netlist_type::GL);
  qor_mgr.set_timing_report_dirpath(rpt_timing_dirpath);

  std::string ppa_smry_dirpath = dv_mgr.ppa_summary_dirpath(DvManager::e_netlist_type::GL);
  qor_mgr.set_ppa_summary_dirpath(ppa_smry_dirpath);

  /* Set sdc file path */
  std::string sdc_file_base_path =
    dv_mgr.file(st_id, bm_id, DvManager::e_file_type::BASE_BITSTREAM_XML_FILE, qor_corner, verbose);
  std::string sdc_file_value_path =
    dv_mgr.file(st_id, bm_id, DvManager::e_file_type::BITSTREAM_XML_FILE, qor_corner, verbose);
  qor_mgr.set_sdc_file_base_dirpath(sdc_file_base_path);
  qor_mgr.set_sdc_file_value_dirpath(sdc_file_value_path);

  /*create custom_sdc folder*/
  std::string custom_sdc_dir_path =
    qor_mgr.qorso_custom_sdc_dir_benchmark_sta(bm_mgr.benchmark_name(bm_id));
  if (std::filesystem::exists(custom_sdc_dir_path)) {
    std::filesystem::remove_all(custom_sdc_dir_path);
  }
  openfpga::create_directory(custom_sdc_dir_path, true, verbose);
  std::string constrain_io_file_path =
    std::filesystem::path(custom_sdc_dir_path / std::filesystem::path("constrain_io.sdc")).string();
  std::string disable_config_file_path =
    std::filesystem::path(custom_sdc_dir_path / std::filesystem::path("disable_config.tcl"))
      .string();

  std::string final_gclk_pcf =
    dv_mgr.file(st_id, bm_id, DvManager::e_file_type::OPENFPGA_FINAL_PCF_XML, qor_corner, verbose);

  std::string clock_loc_map_file =
    dv_mgr.file(st_id, bm_id, DvManager::e_file_type::OPENFPGA_CKBUF_LOC_MAP, qor_corner, verbose);
  int status = openfpga::CMD_EXEC_SUCCESS;
  status = generate_constrain_io_file(constrain_io_file_path, final_gclk_pcf, clock_loc_map_file,
                                      clock_period, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Generate constrain_io.sdc failed! \n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = generate_disable_config(disable_config_file_path);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Generate disable_config.tcl failed! \n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Load essential files to arch development manager */
int setup_arch_dev_manager(const TapeoutManager& to_mgr, const BenchmarkManager& bm_mgr,
                           ArchDevManager& ad_mgr, BenchmarkPacket& bm_pkt) {
  /* Setup */
  for (auto f_type : to_mgr.arch_file_types()) {
    /* for DP fabric */
    auto ad_f_type = ad_mgr.convert_file_type(f_type);
    ad_mgr.add_file(ArchDevManager::e_fabric_type::DP, ad_f_type, to_mgr.dp_arch_file(f_type));
    ad_mgr.add_file(ArchDevManager::e_fabric_type::ULTIMATE, ad_f_type,
                    to_mgr.ultimate_arch_file(f_type));
  }
  for (auto f_type : to_mgr.openfpga_setting_file_types()) {
    if (!ad_mgr.required_file_type(f_type)) {
      continue;
    }
    auto ad_f_type = ad_mgr.convert_file_type(f_type);
    /* File tyoe does not matter here, assign any should work */
    bool l_status = ad_mgr.add_file(ArchDevManager::e_fabric_type::ULTIMATE, ad_f_type,
                                    to_mgr.openfpga_setting_file(f_type));
    if (!l_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  for (auto f_type : to_mgr.openfpga_script_file_types()) {
    if (!ad_mgr.required_script_type(f_type)) {
      continue;
    }
    auto ad_f_type = ad_mgr.convert_script_type(f_type);
    ad_mgr.add_openfpga_script(ad_f_type, to_mgr.openfpga_script_file(f_type));
  }
  ad_mgr.set_route_chan_width(to_mgr.route_chan_width());
  int status = get_tapeout_default_benchmark_packet(to_mgr, bm_mgr, bm_pkt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Arch timing files should be in */
  for (std::string corner : to_mgr.corner_tags()) {
    std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
    ad_mgr.add_arch_timing_file(corner, arch_timing_file);
  }
  if (!ad_mgr.set_default_corner(to_mgr.default_corner())) {
    VTR_LOG_ERROR("Invalid default corner '%s' which is not a valid one!\n",
                  to_mgr.default_corner().c_str());
    to_mgr.show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int setup_arch_eval_manager(const TapeoutManager& to_mgr, ArchEvalManager& ae_mgr,
                            const std::string& arch_run) {
  ArchDevManager ad_mgr(to_mgr, arch_run);
  /* Setup the source arch directory where netlist development will be based on */
  ae_mgr.set_source_arch_dir(ad_mgr.arch_dir_by_fabric(ArchDevManager::e_fabric_type::ULTIMATE));
  ae_mgr.set_source_pin_table_dir(
    ad_mgr.pin_table_dir_by_fabric(ArchDevManager::e_fabric_type::ULTIMATE));

  ae_mgr.set_fabric_name(ad_mgr.release_fabric_name());
  ae_mgr.set_route_chan_width(to_mgr.route_chan_width());

  for (auto f_type : to_mgr.yosys_script_file_types()) {
    /* Bypass the script which is not needed by evaluation */
    if (!ae_mgr.required_script_type(f_type)) {
      continue;
    }
    auto ae_f_type = ae_mgr.convert_script_type(f_type);
    bool l_status = ae_mgr.add_yosys_script(ae_f_type, to_mgr.yosys_script_file(f_type));
    if (!l_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  for (auto f_type : to_mgr.openfpga_script_file_types()) {
    /* Bypass the script which is not needed by evaluation */
    if (!ae_mgr.required_script_type(f_type)) {
      continue;
    }
    auto ae_f_type = ae_mgr.convert_script_type(f_type);
    /* File tyoe does not matter here, assign any should work */
    bool l_status = ae_mgr.add_openfpga_script(ae_f_type, to_mgr.openfpga_script_file(f_type));
    if (!l_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  for (auto f_type : to_mgr.openfpga_setting_file_types()) {
    if (!ae_mgr.required_file_type(f_type)) {
      continue;
    }
    auto ae_f_type = ae_mgr.convert_file_type(f_type);
    /* File tyoe does not matter here, assign any should work */
    bool l_status = ae_mgr.add_file(ae_f_type, to_mgr.openfpga_setting_file(f_type));
    if (!l_status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Boundary timing file overwrite */
  for (std::string corner : to_mgr.corner_tags()) {
    std::string bdry_timing_file = to_mgr.boundary_timing_file(corner, false);
    ae_mgr.add_boundary_timing_file(corner, bdry_timing_file);
  }
  /* Arch timing files should be in */
  for (std::string corner : to_mgr.corner_tags()) {
    std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
    ae_mgr.add_arch_timing_file(corner, arch_timing_file);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int setup_dv_manager_for_qor_analysis(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                                      const ArchEvalManager& ae_mgr, DvManager& dv_mgr,
                                      const bool& verbose) {
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

}  // namespace arcxpress
