#include "tapeout_manager.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>

#include "command_exit_codes.h"
#include "pugixml.hpp"
#include "side_manager.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

TapeoutManager::TapeoutManager() {
  is_valid_ = false;
  STEP_NAME_STRING_ = {"pre_arch", "arch", "bitstream", "netlist", "pd", "so", "dv"};
  STATUS_STRING_ = {"success", "fail", "need_an_update", "fresh"};
  ARCH_FILE_TYPE_STRING_ = {"vpr_arch",
                            "openfpga_arch",
                            "openfpga_global_network_arch",
                            "openfpga_clock_buffer_location_map",
                            "design_variables",
                            "openfpga_sim_setting",
                            "openfpga_ckbuf_cell_map",
                            "openfpga_bitstream_setting",
                            "tile_config",
                            "gclk_pcf",
                            "vpr_sdc",
                            "pcf_custom_command",
                            "boundary_timing",
                            "pcf"};
  SUBSTEP_NAME_STRING_ = {"arch_creation",
                          "arch_setup",
                          "arch_init_module_names",
                          "arch_init_fabric_key",
                          "arch_io_coordinate",
                          "arch_pin_table",
                          "arch_eval",
                          "netlist_generation",
                          "netlist_optimization",
                          "netlist_coarse_analyze",
                          "netlist_coarse_floorplan",
                          "netlist_qor_extraction",
                          "pre_arch_lut_analyze",
                          "dv_setup",
                          "dv_rtl_level",
                          "dv_gate_level",
                          "dv_post_layout",
                          "dv_gl_benchmark_power_analysis",
                          "dv_gl_benchmark_sta"};
  for (auto type : arch_file_types()) {
    dp_arch_file_cnt_[size_t(type)] = 0;
    ultimate_arch_file_cnt_[size_t(type)] = 0;
  }
  for (auto type : openfpga_setting_file_types()) {
    dp_arch_file_cnt_[size_t(type)] = 0;
    ultimate_arch_file_cnt_[size_t(type)] = 0;
  }
  for (auto step : steps()) {
    step_cnt_[size_t(step)] = 0;
  }
  route_chan_width_ = 100; /* A default value */
  KW_PWD_ = "${PWD}";
  KW_ROOT_ = "${ROOT_PATH}";
  KW_PROJHOME_ = "${PROJ_PATH}";
  reserved_keywords_[KW_PWD_] = std::filesystem::current_path().string();
  reserved_keywords_[KW_ROOT_] = std::string();
  reserved_keywords_[KW_PROJHOME_] = std::string();
  SCRIPT_TYPE_STRING_ = {"generate_bitstream",
                         "generate_gclk_bitstream",
                         "generate_grst_bitstream",
                         "generate_base_bitstream",
                         "generate_device_data",
                         "generate_init_module_names",
                         "generate_init_fabric_key",
                         "generate_io_coordinate",
                         "generate_fabric",
                         "generate_preconfig_testbench",
                         "generate_full_testbench",
                         "synthesis",
                         "rewrite",
                         "sweep_k"};
  IP_FILE_TYPE_STRING_ = {"ip_rtl", "ip_gl"};
  IP_FILE_FORMAT_STRING_ = {"auto", "verilog", "system_verilog"};
  CORNER_TAG_TYPE_STRING_ = {"sclib", "spef", "arch_timing"};
  QORSO_TASK_TYPE_STRING_ = {"leakage", "config_power", "timing",     "dynamic_power",
                             "sta",     "timing_gclk",  "timing_grst"};
  /* TODO: Must be consistency with the cocotb utils, once the benchmark simulation is merged to
   * cocotb utils natively */
  SIMULATOR_TYPE_STRING_ = {"vcs", "icarus", "modelsim", "questa"};
  OPENFPGA_BIN_RELPATH_ = KW_ROOT_ + "/build/libs/OpenFPGA/openfpga/openfpga";
  FKEYASSIST_BIN_RELPATH_ =
    KW_ROOT_ + "/build/libs/OpenFPGA/libs/libfabrickey/fabric_key_assistant";
  PINTABLEGEN_BIN_RELPATH_ = KW_ROOT_ + "/etc/utils/pin_table_gen/pin_table_generator.py";
  NETLIST_MAKEUP_BIN_RELPATH_ = KW_ROOT_ + "/etc/utils/netlist_makeup/netlist_makeup.py";
  CCFF_MAKEUP_BIN_RELPATH_ = KW_ROOT_ + "/etc/utils/netlist_makeup/ccff_makeup.py";
  SYNTH_BIN_RELPATH_ = KW_ROOT_ + "/etc/utils/snps_dc_utils/scripts/run_dc_task.py";
  FRONTEND_BIN_RELPATH_ = KW_ROOT_ + "/build/libs/arcxpress_frontend/yosys/bin/yosys";
  FRONTEND_CELL_LIB_RELPATH_ =
    KW_ROOT_ + "/build/libs/arcxpress_frontend/yosys/share/yosys/rapidflex";
  QORSO_BIN_RELPATH_ = KW_ROOT_ + "/etc/utils/ptpx_utils/scripts/run_ptpx_task.py";
  BGG_BIN_RELPATH_ = KW_ROOT_ + "/etc/utils/bus_group_gen/bus_group_gen.py";
  COCOTB_UTIL_BIN_RELPATH_ = KW_ROOT_ + "/etc/utils/cocotb_utils/scripts/run_cocotb_task.py";
  COCOTB_CONVERT_MAKEFILE_APPLICATION_SIMULATION_ =
    KW_ROOT_ + "/etc/utils/cocotb_utils/scripts/convert_cocotb_makefile_application_simulation.py";
  COCOTB_FORCE_BITSTREAM_BIN_RELPATH_ =
    KW_ROOT_ + "/etc/utils/cocotb_utils/scripts/cocotb_force_bitstream.py";
  COCOTB_MAKEFILE_INC_FILE_RELPATH_ = KW_ROOT_ + "/etc/utils/cocotb_makeinc/makefile_vcs.inc";
  BM_SIM_COCOTB_MAKEFILE_INC_FILE_RELPATH_ =
    KW_ROOT_ + "/etc/utils/cocotb_makeinc/makefile_benchmark_sim.inc";
  PYTHON_BIN_RELPATH_ = KW_ROOT_ + "/build/python3.9.25/bin/python3.9";
  PYTHON_COCOTB_CONFIG_RELPATH_ =
    KW_ROOT_ + "/build/python3.9.25/vendor/site_packages/cocotb/bin/cocotb-config";
  LIBPYTHON_LOC_RELPATH_ = KW_ROOT_ + "/build/python3.9.25/lib/libpython3.so";
  KW_CORNER_ALL_ = "all";
}

std::vector<TapeoutManager::e_step_name> TapeoutManager::dev_steps() const {
  return {TapeoutManager::e_step_name::ARCH,    TapeoutManager::e_step_name::BITSTREAM,
          TapeoutManager::e_step_name::NETLIST, TapeoutManager::e_step_name::PD,
          TapeoutManager::e_step_name::SO,      TapeoutManager::e_step_name::DV};
}

std::array<TapeoutManager::e_step_name, size_t(TapeoutManager::e_step_name::NUM_TYPES)>
TapeoutManager::steps() const {
  return {TapeoutManager::e_step_name::PRE_ARCH,  TapeoutManager::e_step_name::ARCH,
          TapeoutManager::e_step_name::BITSTREAM, TapeoutManager::e_step_name::NETLIST,
          TapeoutManager::e_step_name::PD,        TapeoutManager::e_step_name::SO,
          TapeoutManager::e_step_name::DV};
}

std::vector<std::string> TapeoutManager::step_runs(const e_step_name& step) const {
  check_valid();
  check_valid_step(step);
  std::vector<std::string> ret;
  for (auto result : step_status_[size_t(step)]) {
    ret.push_back(result.first);
  }
  return ret;
}

std::vector<TapeoutManager::e_arch_file_type> TapeoutManager::arch_file_types() const {
  return {TapeoutManager::e_arch_file_type::VPR_ARCH,
          TapeoutManager::e_arch_file_type::OPENFPGA_ARCH,
          TapeoutManager::e_arch_file_type::OPENFPGA_GNTWK_ARCH,
          TapeoutManager::e_arch_file_type::OPENFPGA_CKBUF_LOC_MAP,
          TapeoutManager::e_arch_file_type::DESIGN_VAR};
}

std::vector<TapeoutManager::e_arch_file_type> TapeoutManager::openfpga_setting_file_types() const {
  return {TapeoutManager::e_arch_file_type::OPENFPGA_SIM_SETTING,
          TapeoutManager::e_arch_file_type::OPENFPGA_CKBUF_MAP,
          TapeoutManager::e_arch_file_type::OPENFPGA_BITSTREAM_SETTING,
          TapeoutManager::e_arch_file_type::OPENFPGA_TILE_CONFIG,
          TapeoutManager::e_arch_file_type::OPENFPGA_PCF_XML,
          TapeoutManager::e_arch_file_type::VPR_SDC,
          TapeoutManager::e_arch_file_type::OPENFPGA_PCF_CONFIG,
          TapeoutManager::e_arch_file_type::OPENFPGA_BOUNDARY_TIMING,
          TapeoutManager::e_arch_file_type::OPENFPGA_PCF};
}

std::vector<TapeoutManager::e_script_type> TapeoutManager::openfpga_script_file_types() const {
  return {TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BASEBITGEN,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_DEVICEDATA,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_REFMODULENAMES,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_REFFKEY,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_IOCOORD,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_FABRIC,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_PRECONFIGTB,
          TapeoutManager::e_script_type::OPENFPGA_SCRIPT_FULLTB};
}

std::vector<TapeoutManager::e_script_type> TapeoutManager::yosys_script_file_types() const {
  return {TapeoutManager::e_script_type::YOSYS_SCRIPT_SYNTH,
          TapeoutManager::e_script_type::YOSYS_SCRIPT_REWRITE,
          TapeoutManager::e_script_type::YOSYS_SCRIPT_SWEEPK};
}

std::string TapeoutManager::openfpga_bin_path() const {
  check_valid();
  return replace_reserved_keywords(OPENFPGA_BIN_RELPATH_);
}

std::string TapeoutManager::python_bin_path() const {
  check_valid();
  return replace_reserved_keywords(PYTHON_BIN_RELPATH_);
}

std::string TapeoutManager::python_site_package_path() const {
  check_valid();
  return replace_reserved_keywords(PYTHON_SITE_PACKAGES_RELPATH_);
}

std::string TapeoutManager::python_cocotb_config_path() const {
  check_valid();
  return replace_reserved_keywords(PYTHON_COCOTB_CONFIG_RELPATH_);
}

std::string TapeoutManager::libpython_loc_path() const {
  check_valid();
  return replace_reserved_keywords(LIBPYTHON_LOC_RELPATH_);
}

std::string TapeoutManager::fabric_key_assistant_bin_path() const {
  check_valid();
  return replace_reserved_keywords(FKEYASSIST_BIN_RELPATH_);
}

std::string TapeoutManager::pin_table_gen_bin_path() const {
  check_valid();
  return replace_reserved_keywords(PINTABLEGEN_BIN_RELPATH_);
}

std::string TapeoutManager::netlist_makeup_bin_path() const {
  check_valid();
  return replace_reserved_keywords(NETLIST_MAKEUP_BIN_RELPATH_);
}

std::string TapeoutManager::ccff_makeup_bin_path() const {
  check_valid();
  return replace_reserved_keywords(CCFF_MAKEUP_BIN_RELPATH_);
}

std::string TapeoutManager::synth_bin_path() const {
  check_valid();
  return replace_reserved_keywords(SYNTH_BIN_RELPATH_);
}

std::string TapeoutManager::frontend_bin_path() const {
  check_valid();
  return replace_reserved_keywords(FRONTEND_BIN_RELPATH_);
}

std::string TapeoutManager::frontend_cell_lib_path() const {
  check_valid();
  std::filesystem::path rpath = replace_reserved_keywords(FRONTEND_CELL_LIB_RELPATH_);
  std::filesystem::path fpath = rpath / std::filesystem::path(family_);
  return fpath.string();
}

std::string TapeoutManager::qorso_bin_path() const {
  check_valid();
  return replace_reserved_keywords(QORSO_BIN_RELPATH_);
}

std::string TapeoutManager::bus_group_gen_bin_path() const {
  check_valid();
  return replace_reserved_keywords(BGG_BIN_RELPATH_);
}

std::string TapeoutManager::cocotb_util_bin_path() const {
  check_valid();
  return replace_reserved_keywords(COCOTB_UTIL_BIN_RELPATH_);
}

std::string TapeoutManager::cocotb_convert_makefile_application_simulation() const {
  check_valid();
  return replace_reserved_keywords(COCOTB_CONVERT_MAKEFILE_APPLICATION_SIMULATION_);
}

std::string TapeoutManager::cocotb_force_bitstream_bin_path() const {
  check_valid();
  return replace_reserved_keywords(COCOTB_FORCE_BITSTREAM_BIN_RELPATH_);
}

std::string TapeoutManager::cocotb_makefile_include_file_path() const {
  check_valid();
  return replace_reserved_keywords(COCOTB_MAKEFILE_INC_FILE_RELPATH_);
}

std::string TapeoutManager::benchmark_simulation_cocotb_makefile_include_file_path() const {
  check_valid();
  return replace_reserved_keywords(BM_SIM_COCOTB_MAKEFILE_INC_FILE_RELPATH_);
}

std::string TapeoutManager::file() const {
  check_valid();
  return file_;
}

std::string TapeoutManager::name() const {
  check_valid();
  return name_;
}

std::string TapeoutManager::family() const {
  check_valid();
  return family_;
}

std::string TapeoutManager::booting_root() const {
  check_valid();
  return reserved_keywords_.at(KW_ROOT_);
}

std::string TapeoutManager::path() const {
  check_valid();
  return replace_reserved_keywords(path_);
}

std::string TapeoutManager::author() const {
  check_valid();
  return author_;
}

std::string TapeoutManager::date_created() const {
  check_valid();
  return date_created_;
}

std::string TapeoutManager::date_last_modified() const {
  check_valid();
  return date_last_modified_;
}

std::string TapeoutManager::tool_version() const {
  check_valid();
  return tool_version_;
}

std::string TapeoutManager::pdk_config(const bool& keep_reserved_word) const {
  check_valid_pdk_config_file();
  if (keep_reserved_word) {
    return pdk_config_;
  }
  return replace_reserved_keywords(pdk_config_);
}

std::vector<std::string> TapeoutManager::boundary_timing_corners() const {
  std::vector<std::string> ret;
  for (auto pair : boundary_timing_files_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::string TapeoutManager::boundary_timing_file(const std::string& corner,
                                                 const bool& keep_reserved_word) const {
  check_valid_boundary_timing_file();
  auto result = boundary_timing_files_.find(corner);
  if (result != boundary_timing_files_.end()) {
    if (keep_reserved_word) {
      return result->second;
    }
    return replace_reserved_keywords(result->second);
  }
  /* Use the default timing file from arch templates */
  return std::string();
}

std::string TapeoutManager::dp_arch_file(const e_arch_file_type& f_type,
                                         const bool& keep_reserved_word) const {
  check_valid();
  check_valid_arch_file_type(f_type);
  if (keep_reserved_word) {
    return dp_arch_files_[size_t(f_type)];
  }
  return replace_reserved_keywords(dp_arch_files_[size_t(f_type)]);
}

std::vector<std::string> TapeoutManager::dp_arch_files(const bool& keep_reserved_word) const {
  check_valid();
  std::vector<std::string> ret;
  for (e_arch_file_type ftype : arch_file_types()) {
    std::string fname = dp_arch_file(ftype, keep_reserved_word);
    ret.push_back(fname);
  }
  return ret;
}

std::string TapeoutManager::ultimate_arch_file(const e_arch_file_type& f_type,
                                               const bool& keep_reserved_word) const {
  check_valid();
  check_valid_arch_file_type(f_type);
  if (keep_reserved_word) {
    return ultimate_arch_files_[size_t(f_type)];
  }
  return replace_reserved_keywords(ultimate_arch_files_[size_t(f_type)]);
}

std::vector<std::string> TapeoutManager::ultimate_arch_files(const bool& keep_reserved_word) const {
  check_valid();
  std::vector<std::string> ret;
  for (e_arch_file_type ftype : arch_file_types()) {
    std::string fname = ultimate_arch_file(ftype, keep_reserved_word);
    ret.push_back(fname);
  }
  return ret;
}

std::string TapeoutManager::openfpga_setting_file(const e_arch_file_type& f_type,
                                                  const bool& keep_reserved_word) const {
  check_valid();
  check_valid_openfpga_setting_file_type(f_type);
  if (keep_reserved_word) {
    return ultimate_arch_files_[size_t(f_type)];
  }
  return replace_reserved_keywords(ultimate_arch_files_[size_t(f_type)]);
}

std::string TapeoutManager::openfpga_script_file(const e_script_type& f_type,
                                                 const bool& keep_reserved_word) const {
  check_valid();
  check_valid_openfpga_script_type(f_type);
  if (keep_reserved_word) {
    return script_files_[size_t(f_type)];
  }
  return replace_reserved_keywords(script_files_[size_t(f_type)]);
}

std::string TapeoutManager::yosys_script_file(const e_script_type& f_type,
                                              const bool& keep_reserved_word) const {
  check_valid();
  check_valid_yosys_script_type(f_type);
  if (keep_reserved_word) {
    return script_files_[size_t(f_type)];
  }
  return replace_reserved_keywords(script_files_[size_t(f_type)]);
}

std::vector<std::string> TapeoutManager::benchmark_configs(const bool& keep_reserved_word) const {
  check_valid();
  std::vector<std::string> bm;
  bm.reserve(benchmark_config_.size());
  for (auto result : benchmark_config_) {
    if (keep_reserved_word) {
      bm.push_back(result.first);
    } else {
      bm.push_back(replace_reserved_keywords(result.first));
    }
  }
  return bm;
}

std::vector<std::string> TapeoutManager::netlist_flags_pdk_names() const {
  std::vector<std::string> ret;
  for (auto result : netlist_flags_) {
    ret.push_back(result.first);
  }
  return ret;
}

std::vector<std::string> TapeoutManager::netlist_flags(const std::string& pdk_name) const {
  std::vector<std::string> ret;
  auto pdk_result = netlist_flags_.find(pdk_name);
  if (pdk_result == netlist_flags_.end()) {
    VTR_LOG_ERROR("The pdk name '%s' is not defined in any netlist flag!\n", pdk_name.c_str());
    return ret;
  }
  for (auto result : pdk_result->second) {
    ret.push_back(result.first);
  }
  return ret;
}

std::string TapeoutManager::flag_value(const std::string& pdk_name, const std::string& name) const {
  auto pdk_result = netlist_flags_.find(pdk_name);
  if (pdk_result == netlist_flags_.end()) {
    VTR_LOG_ERROR("The pdk name '%s' is not defined in any netlist flag!\n", pdk_name.c_str());
    return std::string();
  }
  auto result = pdk_result->second.find(name);
  if (result == pdk_result->second.end()) {
    VTR_LOG_ERROR("Invalid flag name '%s' under pdk name '%s' which is not defined!\n",
                  name.c_str(), pdk_name.c_str());
  }
  return result->second;
}

std::vector<std::string> TapeoutManager::qorso_tasks() const {
  std::vector<std::string> ret;
  for (auto pair : qorso_task_name2ids_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<std::string> TapeoutManager::qorso_task_pdk_names(const std::string& t_name) const {
  check_valid();
  VTR_ASSERT(valid_qorso_task(t_name));
  std::vector<std::string> ret;
  for (auto pair : qorso_task_config_[qorso_task_name2ids_.at(t_name)]) {
    ret.push_back(pair.first);
  }
  return ret;
}

TapeoutManager::e_qorso_task_type TapeoutManager::qorso_task_type(const std::string& t_name) const {
  check_valid();
  VTR_ASSERT(valid_qorso_task(t_name));
  return qorso_task_types_[qorso_task_name2ids_.at(t_name)];
}

std::string TapeoutManager::qorso_task_type_str(const std::string& t_name) const {
  return qorso_task_type2str(qorso_task_type(t_name));
}

std::string TapeoutManager::qorso_task_config(const std::string& t_name,
                                              const std::string& pdk_name,
                                              const bool& keep_reserved_word) const {
  check_valid();
  VTR_ASSERT(valid_qorso_task(t_name));
  auto pdk_result = qorso_task_config_[qorso_task_name2ids_.at(t_name)].find(pdk_name);
  if (pdk_result == qorso_task_config_[qorso_task_name2ids_.at(t_name)].end()) {
    VTR_LOG_ERROR("Invalid pdk name '%s' which is not supported by the Qorso task '%s'!\n",
                  pdk_name.c_str(), t_name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (keep_reserved_word) {
    return pdk_result->second;
  }
  return replace_reserved_keywords(pdk_result->second);
}

std::vector<IpFileId> TapeoutManager::ip_files() const {
  std::vector<IpFileId> ret;
  for (size_t f_id = 0; f_id < ip_files_.size(); f_id++) {
    ret.push_back(IpFileId(f_id));
  }
  return ret;
}

TapeoutManager::e_ip_file_type TapeoutManager::ip_file_type(const IpFileId& f_id) const {
  check_valid();
  VTR_ASSERT(valid_ip_file_id(f_id));
  return ip_file_types_[size_t(f_id)];
}

TapeoutManager::e_ip_file_format TapeoutManager::ip_file_format(const IpFileId& f_id) const {
  check_valid();
  VTR_ASSERT(valid_ip_file_id(f_id));
  return ip_file_formats_[size_t(f_id)];
}

std::vector<IpFileId> TapeoutManager::ip_files_by_type(const e_ip_file_type& f_type) const {
  std::vector<IpFileId> ret;
  for (size_t f_id = 0; f_id < ip_files_.size(); f_id++) {
    if (ip_file_types_[size_t(f_id)] == f_type) {
      ret.push_back(IpFileId(f_id));
    }
  }
  return ret;
}

std::string TapeoutManager::ip_file_type_str(const IpFileId& f_id) const {
  check_valid();
  VTR_ASSERT(valid_ip_file_id(f_id));
  return ip_file_type2str(ip_file_types_[size_t(f_id)]);
}

std::string TapeoutManager::ip_file_format_str(const IpFileId& f_id) const {
  check_valid();
  VTR_ASSERT(valid_ip_file_id(f_id));
  return ip_file_format2str(ip_file_formats_[size_t(f_id)]);
}

std::string TapeoutManager::ip_file(const IpFileId& f_id, const bool& keep_reserved_word) const {
  check_valid();
  VTR_ASSERT(valid_ip_file_id(f_id));
  if (keep_reserved_word) {
    return ip_files_[size_t(f_id)];
  }
  return replace_reserved_keywords(ip_files_[size_t(f_id)]);
}

std::string TapeoutManager::default_benchmark_name() const { return default_benchmark_name_; }
std::string TapeoutManager::default4clk_sweep_benchmark_name() const {
  return default4clk_sweep_benchmark_name_;
}
std::string TapeoutManager::default4rst_sweep_benchmark_name() const {
  return default4rst_sweep_benchmark_name_;
}

std::string TapeoutManager::step_latest_run(const e_step_name& step) const {
  check_valid();
  check_valid_step(step);
  return step_latest_runs_[size_t(step)];
}

TapeoutManager::e_run_status TapeoutManager::step_run_status(
  const TapeoutManager::e_step_name& step, const std::string& run) const {
  check_valid();
  check_valid_run(step, run);
  return step_status_[size_t(step)].at(run).status;
}

std::string TapeoutManager::step_run_status_str(const TapeoutManager::e_step_name& step,
                                                const std::string& run) const {
  return status2str(step_run_status(step, run));
}

bool TapeoutManager::step_run_frozen(const TapeoutManager::e_step_name& step,
                                     const std::string& run) const {
  check_valid();
  check_valid_run(step, run);
  return step_status_[size_t(step)].at(run).frozen;
}

std::vector<TapeoutManager::e_step_name> TapeoutManager::step_run_deps(
  const TapeoutManager::e_step_name& step, const std::string& run) const {
  check_valid();
  check_valid_run(step, run);
  std::vector<e_step_name> ret;
  for (auto curr_dep : steps()) {
    auto result = step_deps_[size_t(step)].find(run);
    if (result == step_deps_[size_t(step)].end()) {
      continue;
    }
    if (!result->second[size_t(curr_dep)].empty()) {
      ret.push_back(curr_dep);
    }
  }
  return ret;
}

std::vector<TapeoutManager::e_substep_name> TapeoutManager::substeps(const e_step_name& step,
                                                                     const std::string& run) const {
  check_valid();
  check_valid_run(step, run);
  std::vector<e_substep_name> ret;
  for (auto result : substep_status_[size_t(step)].at(run).status) {
    ret.push_back(result.first);
  }
  return ret;
}

TapeoutManager::e_run_status TapeoutManager::substep_status(const e_step_name& step,
                                                            const std::string& run,
                                                            const e_substep_name& substep) const {
  check_valid();
  check_valid_run(step, run);
  auto result = substep_status_[size_t(step)].at(run).status.find(substep);
  if (result != substep_status_[size_t(step)].at(run).status.end()) {
    return result->second;
  }
  return e_run_status::FRESH;
}

std::string TapeoutManager::substep_status_str(const e_step_name& step, const std::string& run,
                                               const e_substep_name& substep) const {
  return status2str(substep_status(step, run, substep));
}

std::string TapeoutManager::step_run_dep_run(const TapeoutManager::e_step_name& step,
                                             const std::string& run,
                                             const TapeoutManager::e_step_name& dep) const {
  check_valid();
  check_valid_run(step, run);
  return step_deps_[size_t(step)].at(run)[size_t(dep)];
}

size_t TapeoutManager::route_chan_width() const { return route_chan_width_; }

std::vector<std::string> TapeoutManager::synth_pdk_names() const {
  std::vector<std::string> ret;
  for (auto pair : synth_sdc_root_) {
    ret.push_back(pair.first);
  }
  return ret;
}

size_t TapeoutManager::ccff_max_fanout(const std::string& pdk_name) const {
  auto result = ccff_max_fanout_.find(pdk_name);
  if (result == ccff_max_fanout_.end()) {
    VTR_LOG_ERROR("The provided pdk '%s' is not supported\n", pdk_name.c_str());
    return 0; /* TODO: Should be a show stopper value! */
  }
  return result->second;
}

std::string TapeoutManager::synth_sdc_root_path(const std::string& pdk_name,
                                                const bool& keep_reserved_word) const {
  auto result = synth_sdc_root_.find(pdk_name);
  if (result == synth_sdc_root_.end()) {
    VTR_LOG_ERROR("The provided pdk '%s' is not supported\n", pdk_name.c_str());
    return std::string();
  }
  if (keep_reserved_word) {
    return result->second;
  }
  return replace_reserved_keywords(result->second);
}
std::string TapeoutManager::synth_strategy_file(const std::string& pdk_name,
                                                const bool& keep_reserved_word) const {
  auto result = synth_strategy_file_.find(pdk_name);
  if (result == synth_strategy_file_.end()) {
    VTR_LOG_ERROR("The provided pdk '%s' is not supported\n", pdk_name.c_str());
    return std::string();
  }
  if (keep_reserved_word) {
    return result->second;
  }
  return replace_reserved_keywords(result->second);
}

std::vector<std::string> TapeoutManager::floorplan_pdk_names() const {
  std::vector<std::string> ret;
  for (auto pair : floorplan_strategy_file_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::string TapeoutManager::floorplan_strategy_file(const std::string& pdk_name,
                                                    const bool& keep_reserved_word) const {
  auto result = floorplan_strategy_file_.find(pdk_name);
  if (result == floorplan_strategy_file_.end()) {
    VTR_LOG_ERROR("The provided pdk '%s' is not supported\n", pdk_name.c_str());
    return std::string();
  }
  if (keep_reserved_word) {
    return result->second;
  }
  return replace_reserved_keywords(result->second);
}

std::string TapeoutManager::netlist_makeup_key_root_path(const bool& keep_reserved_word) const {
  if (keep_reserved_word) {
    return netlist_makeup_key_root_path_;
  }
  return replace_reserved_keywords(netlist_makeup_key_root_path_);
}

void TapeoutManager::set_route_chan_width(const size_t& val) { route_chan_width_ = val; }

bool TapeoutManager::set_ccff_max_fanout(const std::string& pdk_name, const size_t& val) {
  auto result = ccff_max_fanout_.find(pdk_name);
  if (result != ccff_max_fanout_.end() && result->second != val) {
    VTR_LOG_WARN("Attempt to redefine ccff max fanout for pdk '%s' from val '%lu' to val '%lu'!\n",
                 pdk_name.c_str(), result->second, val);
    return false;
  }
  ccff_max_fanout_[pdk_name] = val;
  return true;
}

bool TapeoutManager::set_synth_sdc_root_path(const std::string& pdk_name, const std::string& val) {
  auto result = synth_sdc_root_.find(pdk_name);
  if (result != synth_sdc_root_.end() && result->second != val) {
    VTR_LOG_WARN("Attempt to redefine synth sdc root for pdk '%s' from val '%s' to val '%s'!\n",
                 pdk_name.c_str(), result->second.c_str(), val.c_str());
    return false;
  }
  synth_sdc_root_[pdk_name] = val;
  return true;
}

bool TapeoutManager::set_synth_strategy_file(const std::string& pdk_name, const std::string& val) {
  auto result = synth_strategy_file_.find(pdk_name);
  if (result != synth_strategy_file_.end() && result->second != val) {
    VTR_LOG_WARN("Attempt to redefine synth strategy for pdk '%s' from val '%s' to val '%s'!\n",
                 pdk_name.c_str(), result->second.c_str(), val.c_str());
    return false;
  }
  synth_strategy_file_[pdk_name] = val;
  return true;
}

bool TapeoutManager::set_floorplan_strategy_file(const std::string& pdk_name,
                                                 const std::string& val) {
  auto result = floorplan_strategy_file_.find(pdk_name);
  if (result != floorplan_strategy_file_.end() && result->second != val) {
    VTR_LOG_WARN("Attempt to redefine floorplan strategy for pdk '%s' from val '%s' to val '%s'!\n",
                 pdk_name.c_str(), result->second.c_str(), val.c_str());
    return false;
  }
  floorplan_strategy_file_[pdk_name] = val;
  return true;
}

void TapeoutManager::set_netlist_makeup_key_root_path(const std::string& val) {
  netlist_makeup_key_root_path_ = val;
}

void TapeoutManager::set_file(const std::string& val) { file_ = val; }

void TapeoutManager::set_name(const std::string& val) { name_ = val; }

void TapeoutManager::set_family(const std::string& val) { family_ = val; }

bool TapeoutManager::set_path(const std::string& val) {
  size_t start_pos = val.find(KW_PROJHOME_);
  if (start_pos != std::string::npos) {
    VTR_LOG_ERROR("Project path '%s' cannot contain reserved keyword '%s'!\n", val.c_str(),
                  KW_PROJHOME_.c_str());
    return false;
  }
  path_ = val;
  reserved_keywords_[KW_PROJHOME_] = replace_reserved_keywords(path_);
  return true;
}

void TapeoutManager::set_author(const std::string& val) { author_ = val; }

void TapeoutManager::set_date_created(const std::string& val) { date_created_ = val; }

void TapeoutManager::set_date_last_modified(const std::string& val) { date_last_modified_ = val; }

void TapeoutManager::set_tool_version(const std::string& val) { tool_version_ = val; }

void TapeoutManager::set_default_benchmark_name(const std::string& val) {
  default_benchmark_name_ = val;
}
void TapeoutManager::set_default4clk_sweep_benchmark_name(const std::string& val) {
  default4clk_sweep_benchmark_name_ = val;
}
void TapeoutManager::set_default4rst_sweep_benchmark_name(const std::string& val) {
  default4rst_sweep_benchmark_name_ = val;
}

void TapeoutManager::set_kw_root_path(const std::string& val) {
  reserved_keywords_[KW_ROOT_] = val;
}

void TapeoutManager::set_pdk_config(const std::string& val) { pdk_config_ = val; }
void TapeoutManager::add_boundary_timing_file(const std::string& corner, const std::string& val) {
  boundary_timing_files_[corner] = val;
}

bool TapeoutManager::add_benchmark_config(const std::string& val) {
  auto result = benchmark_config_.find(val);
  if (result == benchmark_config_.end()) {
    benchmark_config_[val] = 1;
    return true;
  } else {
    benchmark_config_[val]++;
  }
  return false;
}

bool TapeoutManager::add_dp_arch_file(const std::string& ftype_str, const std::string& fname) {
  e_arch_file_type ftype = str2arch_file_type(ftype_str, true);
  if (!valid_arch_file_type(ftype)) {
    return false;
  }
  dp_arch_file_cnt_[size_t(ftype)]++;
  dp_arch_files_[size_t(ftype)] = fname;
  return true;
}

bool TapeoutManager::add_ultimate_arch_file(const std::string& ftype_str,
                                            const std::string& fname) {
  e_arch_file_type ftype = str2arch_file_type(ftype_str, true);
  if (!valid_arch_file_type(ftype)) {
    return false;
  }
  ultimate_arch_file_cnt_[size_t(ftype)]++;
  ultimate_arch_files_[size_t(ftype)] = fname;
  return true;
}

bool TapeoutManager::add_openfpga_setting_file(const std::string& ftype_str,
                                               const std::string& fname) {
  e_arch_file_type ftype = str2arch_file_type(ftype_str, true);
  if (!valid_openfpga_setting_file_type(ftype)) {
    return false;
  }
  ultimate_arch_file_cnt_[size_t(ftype)]++;
  ultimate_arch_files_[size_t(ftype)] = fname;
  return true;
}

bool TapeoutManager::add_openfpga_script_file(const std::string& ftype_str,
                                              const std::string& fname) {
  e_script_type ftype = str2script_type(ftype_str, true);
  if (!valid_openfpga_script_type(ftype)) {
    return false;
  }
  script_file_cnt_[size_t(ftype)]++;
  script_files_[size_t(ftype)] = fname;
  return true;
}

bool TapeoutManager::add_yosys_script_file(const std::string& ftype_str, const std::string& fname) {
  e_script_type ftype = str2script_type(ftype_str, true);
  if (!valid_yosys_script_type(ftype)) {
    return false;
  }
  script_file_cnt_[size_t(ftype)]++;
  script_files_[size_t(ftype)] = fname;
  return true;
}

bool TapeoutManager::add_ip_file(const std::string& ftype_str, const std::string& fname,
                                 const std::string& ffmt_str) {
  e_ip_file_type ftype = str2ip_file_type(ftype_str, true);
  if (!valid_ip_file_type(ftype)) {
    return false;
  }
  e_ip_file_format ffmt = str2ip_file_format(ffmt_str, true);
  if (!valid_ip_file_format(ffmt)) {
    return false;
  }
  ip_file_types_.push_back(ftype);
  ip_file_formats_.push_back(ffmt);
  ip_files_.push_back(fname);
  return true;
}

bool TapeoutManager::add_qorso_task(const std::string& name, const std::string& type) {
  auto result = qorso_task_name2ids_.find(name);
  if (result != qorso_task_name2ids_.end()) {
    VTR_LOG_ERROR("Attempt to redefine the Qorso task '%s'\n", name.c_str());
    return false;
  }
  size_t new_id = qorso_task_types_.size();
  qorso_task_name2ids_[name] = new_id;
  qorso_task_types_.push_back(str2qorso_task_type(type, true));
  qorso_task_config_.emplace_back();
  return true;
}

bool TapeoutManager::add_qorso_task_config_by_pdk(const std::string& name,
                                                  const std::string& pdk_name,
                                                  const std::string& config_file) {
  auto result = qorso_task_name2ids_.find(name);
  if (result == qorso_task_name2ids_.end()) {
    VTR_LOG_ERROR("Unable to add config file to an undefine Qorso task '%s'\n", name.c_str());
    return false;
  }
  size_t int_id = result->second;
  auto pdk_result = qorso_task_config_[int_id].find(pdk_name);
  if (pdk_result != qorso_task_config_[int_id].end()) {
    VTR_LOG_ERROR(
      "Attempt to redefine the config file for the Qorso task '%s' under pdk name '%s'\n",
      name.c_str(), pdk_name.c_str());
    return false;
  }
  qorso_task_config_[int_id][pdk_name] = config_file;
  return true;
}

bool TapeoutManager::add_netlist_flag(const std::string& pdk_name, const std::string& name,
                                      const std::string& val) {
  auto result = netlist_flags_[pdk_name].find(name);
  if (result != netlist_flags_[pdk_name].end() && result->second != val) {
    VTR_LOG_WARN("Attempt to redefine netlist flag '%s' from val '%s' to val '%s'!\n", name.c_str(),
                 result->second.c_str(), val.c_str());
    return false;
  }
  netlist_flags_[pdk_name][name] = val;
  return true;
}

bool TapeoutManager::add_step_run(const e_step_name& step, const std::string& run) {
  if (!valid_step_name(step)) {
    return false;
  }
  /* The run name should be unique! */
  auto result = step_status_[size_t(step)].find(run);
  if (result == step_status_[size_t(step)].end()) {
    step_status_[size_t(step)][run] = TapeoutManager::RunInfo();
    return true;
  }
  /* Throw warning */
  VTR_LOG_ERROR("Run '%s' under step '%s' is already defined. Cannot add a new run!\n", run.c_str(),
                step_name2str(step).c_str());
  return false;
}

bool TapeoutManager::add_step_run(const std::string& step_str, const std::string& run) {
  e_step_name step = str2step_name(step_str, true);
  return add_step_run(step, run);
}

bool TapeoutManager::set_step_latest_run(const e_step_name& step, const std::string& run) {
  if (!valid_step_name(step)) {
    return false;
  }
  step_cnt_[size_t(step)]++;
  step_latest_runs_[size_t(step)] = run;
  return true;
}

bool TapeoutManager::set_step_latest_run(const std::string& step_str, const std::string& run) {
  e_step_name step = str2step_name(step_str, true);
  return set_step_latest_run(step, run);
}

bool TapeoutManager::set_step_run_status(const e_step_name& step, const std::string& run,
                                         const e_run_status& status) {
  if (!valid_step_name(step)) {
    return false;
  }
  check_valid_run(step, run);
  step_status_[size_t(step)][run].status = status;
  return true;
}

bool TapeoutManager::set_step_run_status(const std::string& step_str, const std::string& run,
                                         const std::string& status) {
  e_step_name step = str2step_name(step_str, true);
  return set_step_run_status(step, run, str2status(status));
}

bool TapeoutManager::set_step_run_frozen(const std::string& step_str, const std::string& run,
                                         const bool& val) {
  e_step_name step = str2step_name(step_str, true);
  if (!valid_step_name(step)) {
    return false;
  }
  check_valid_run(step, run);
  step_status_[size_t(step)][run].frozen = val;
  return true;
}

bool TapeoutManager::add_step_run_dep(const e_step_name& step, const std::string& run,
                                      const e_step_name& dep, const std::string& dep_run) {
  if (!valid_step_name(step)) {
    return false;
  }
  check_valid_run(step, run);
  if (!valid_step_name(dep)) {
    return false;
  }
  check_valid_dep(step, dep);
  step_deps_[size_t(step)][run][size_t(dep)] = dep_run;
  return true;
}

bool TapeoutManager::add_step_run_dep(const std::string& step_str, const std::string& run,
                                      const std::string& dep_str, const std::string& dep_run) {
  TapeoutManager::e_step_name step = str2step_name(step_str, true);
  TapeoutManager::e_step_name dep = str2step_name(dep_str, true);
  return add_step_run_dep(step, run, dep, dep_run);
}

bool TapeoutManager::set_substep_status(const e_step_name& step, const std::string& run,
                                        const e_substep_name& substep, const e_run_status& status) {
  if (!valid_step_name(step)) {
    return false;
  }
  check_valid_run(step, run);
  if (!valid_substep_name(substep)) {
    return false;
  }
  substep_status_[size_t(step)][run].status[substep] = status;
  return true;
}

bool TapeoutManager::set_substep_status(const std::string& step_str, const std::string& run,
                                        const std::string& substep_str,
                                        const std::string& status_str) {
  e_step_name step = str2step_name(step_str, true);
  e_substep_name substep = str2substep_name(substep_str, true);
  e_run_status status = str2status(status_str, true);
  return set_substep_status(step, run, substep, status);
}

void TapeoutManager::clear() {
  name_.clear();
  family_.clear();
  path_.clear();
  author_.clear();
  date_created_.clear();
  date_last_modified_.clear();
  tool_version_.clear();
  pdk_config_.clear();
  boundary_timing_files_.clear();
  benchmark_config_.clear();
  default_benchmark_name_.clear();
  default4clk_sweep_benchmark_name_.clear();
  default4rst_sweep_benchmark_name_.clear();
  disabled_benchmarks_.clear();
  disabled_corners_.clear();
  for (auto type : arch_file_types()) {
    dp_arch_file_cnt_[size_t(type)] = 0;
    dp_arch_files_[size_t(type)].clear();
    ultimate_arch_file_cnt_[size_t(type)] = 0;
    ultimate_arch_files_[size_t(type)].clear();
  }
  for (auto type : openfpga_setting_file_types()) {
    dp_arch_file_cnt_[size_t(type)] = 0;
    dp_arch_files_[size_t(type)].clear();
    ultimate_arch_file_cnt_[size_t(type)] = 0;
    ultimate_arch_files_[size_t(type)].clear();
  }
  for (auto type : openfpga_script_file_types()) {
    script_file_cnt_[size_t(type)] = 0;
    script_files_[size_t(type)].clear();
  }
  clear_status();
  ip_file_types_.clear();
  ip_file_formats_.clear();
  ip_files_.clear();
  route_chan_width_ = 100; /* TODO: Use a constant as default value */
  ccff_max_fanout_.clear();
  netlist_flags_.clear();
  netlist_makeup_key_root_path_.clear();
  synth_strategy_file_.clear();
  floorplan_strategy_file_.clear();
  qorso_task_name2ids_.clear();
  qorso_task_types_.clear();
  qorso_task_config_.clear();
  corner_map_.clear();
  default_corner_.clear();
  pdk2builtin_corner_ids_.clear();
  pdk_name_.clear();
  builtin_corner_map_.clear();
  builtin_default_corners_.clear();
  openfpga_stats_conf_file_.clear();
  openfpga_error_conf_file_.clear();
  yosys_error_conf_file_.clear();
  arch_template_file_.clear();
  arch_timing_files_.clear();
  arch_timing_template_file_.clear();
  arch_timing_template_pdk_.clear();
  netlist_makeup_config_file_.clear();
  ccff_makeup_buf_config_file_.clear();
  core_dimension_.clear();
  sync_dp_dimension_.clear();
  num_clocks_.clear();
  num_pclocks_.clear();
  num_config_regions_.clear();
  num_resets_.clear();
  num_ios_.clear();
  is_valid_ = false;
}

void TapeoutManager::clear_status() {
  for (auto step : steps()) {
    step_cnt_[size_t(step)] = 0;
    step_latest_runs_[size_t(step)].clear();
    step_status_[size_t(step)].clear();
    step_deps_[size_t(step)].clear();
  }
}

bool TapeoutManager::valid_run(const TapeoutManager::e_step_name& step,
                               const std::string& run) const {
  return valid_step_name(step) && run != invalid_run() &&
         step_status_[size_t(step)].find(run) != step_status_[size_t(step)].end();
}

bool TapeoutManager::valid_dep(const TapeoutManager::e_step_name& step,
                               const TapeoutManager::e_step_name& dep) const {
  return valid_step_name(step) && valid_step_name(dep) && step != dep;
}

bool TapeoutManager::validate_boundary_timing_file() const {
  is_valid_boundary_timing_file_ = false;
  if (boundary_timing_files_.empty()) {
    VTR_LOG_WARN(
      "Boundary timing file is not set. A default one will be considered. This may cause "
      "performance loss\n");
    is_valid_boundary_timing_file_ = true;
    return true;
  }
  /* Validate if all the corners are valid. First check the custom corners defined if not,   */
  for (auto btf_pair : boundary_timing_files_) {
    std::string curr_corner = btf_pair.first;
    auto corner_map_result = corner_map_.find(curr_corner);
    if (corner_map_result == corner_map_.end()) {
      auto pdk_corner_result = pdk2builtin_corner_ids_.find(pdk_name_);
      if (pdk_corner_result == pdk2builtin_corner_ids_.end()) {
        VTR_LOG_ERROR(
          "Corner '%s' is not a valid one. Neither defined in project or a built-in one under pdk "
          "'%s'\n",
          curr_corner.c_str(), pdk_name_.c_str()); /* Corner name is not valid */
        return false;
      }
      auto curr_corner_result = builtin_corner_map_[pdk_corner_result->second].find(curr_corner);
      if (curr_corner_result == builtin_corner_map_[pdk_corner_result->second].end()) {
        VTR_LOG_ERROR(
          "Corner '%s' is not a valid one. Neither defined in project or a built-in one under pdk "
          "'%s'\n",
          curr_corner.c_str(), pdk_name_.c_str()); /* Corner name is not valid */
        return false;
      }
    }
  }
  is_valid_boundary_timing_file_ = true;
  return true;
}

bool TapeoutManager::validate_pdk_config_file() const {
  is_valid_pdk_config_file_ = false;
  size_t num_err = 0;
  if (pdk_config_.empty()) {
    VTR_LOG_ERROR("Tapeout has no associated PDK\n");
    num_err++;
  }
  if (num_err == 0) {
    is_valid_pdk_config_file_ = true;
  }
  return num_err == 0;
}

bool TapeoutManager::validate() const {
  is_valid_ = false;
  /* Must have at least 1 time stamp */
  size_t num_err = 0;
  if (name_.empty()) {
    VTR_LOG_ERROR("Tapeout has no name!\n");
    num_err++;
  }
  if (family_.empty()) {
    VTR_LOG_ERROR("Tapeout has no family!\n");
    num_err++;
  }
  if (path_.empty()) {
    VTR_LOG_ERROR("Tapeout has no path!\n");
    num_err++;
  }
  if (author_.empty()) {
    VTR_LOG_ERROR("Tapeout has no author!\n");
    num_err++;
  }
  if (date_created_.empty()) {
    VTR_LOG_ERROR("Tapeout has no date creation!\n");
    num_err++;
  }
  if (date_last_modified_.empty()) {
    VTR_LOG_ERROR("Tapeout has no date last modified!\n");
    num_err++;
  }
  if (tool_version_.empty()) {
    VTR_LOG_ERROR("Tapeout has no tool version!\n");
    num_err++;
  }
  if (!validate_pdk_config_file()) {
    num_err++;
  }
  if (benchmark_config_.empty()) {
    VTR_LOG_ERROR("Tapeout has no associated benchmark suites\n");
    num_err++;
  }
  if (default_benchmark_name_.empty()) {
    VTR_LOG_ERROR("Tapeout has no default benchmark name\n");
    num_err++;
  }
  if (default4clk_sweep_benchmark_name_.empty()) {
    VTR_LOG_ERROR("Tapeout has no default benchmark name required for clock sweeping\n");
    num_err++;
  }
  if (default4rst_sweep_benchmark_name_.empty()) {
    VTR_LOG_ERROR("Tapeout has no default benchmark name required for reset sweeping\n");
    num_err++;
  }
  if (!BootConfig().valid_boot_type(boot_type_)) {
    VTR_LOG_ERROR("Boot type is not valid!");
    num_err++;
  }
  /* Unique benchmark suite */
  for (const auto& result : benchmark_config_) {
    if (result.second > 1) {
      VTR_LOG_ERROR("Benchmark file '%s' is defined more than once. Please double check!\n",
                    result.first.c_str());
      num_err++;
    }
  }
  /* Check default corner:
   * - There is only one corner, infer the default corner
   * - For multiple corner, there must be a specific default corner */
  if (pdk_name_.empty()) {
    VTR_LOG_ERROR(
      "No valid PDK name is defined, which is required by corner map. Please double check!\n");
    num_err++;
  }
  /* If both built-in corners and user defined corners are empty, a fatal error is seen. Also
   * default corner should be validated, it exists either in built-in corners or user defined
   * corners */
  size_t num_corners = corner_map_.size();
  auto corner_result = pdk2builtin_corner_ids_.find(pdk_name_);
  if (corner_result != pdk2builtin_corner_ids_.end()) {
    num_corners += builtin_corner_map_[corner_result->second].size();
  }
  std::string actual_default_corner = default_corner_;
  if (corner_result != pdk2builtin_corner_ids_.end() && actual_default_corner.empty()) {
    actual_default_corner = builtin_default_corners_[corner_result->second];
  }
  if (num_corners == 0) {
    VTR_LOG_ERROR(
      "There must be at least one corner defined in the corner map. Please double check!\n");
    num_err++;
  }
  if (num_corners == 1 && actual_default_corner.empty()) {
    /* If there is no user-defined corner, pick the one from built-in. If there is, just pick the
     * one */
    if (corner_map_.empty()) {
      VTR_ASSERT_SAFE(corner_result != pdk2builtin_corner_ids_.end());
      default_corner_ = builtin_corner_map_[corner_result->second].begin()->first;
    } else {
      default_corner_ = corner_map_.begin()->first;
    }
    VTR_LOG("Auto infer the default corner '%s' as there is only 1 corner defined!\n",
            default_corner_.c_str());
    if (is_disabled_corner(default_corner_)) {
      VTR_LOG_ERROR("The only 1 corner is disabled! \n ");
      num_err++;
    }
  }

  /*If there are two corners and one is disabled. Then pick the enabled one as default_corner_*/
  if (num_corners == 2 && actual_default_corner.empty()) {
    /* two corners are defined in builtin_corner, check whether one is disabled */
    if (corner_map_.empty()) {
      VTR_ASSERT_SAFE(corner_result != pdk2builtin_corner_ids_.end());
      int num_disabled_corner = 0;
      for (auto corner_info : builtin_corner_map_[corner_result->second]) {
        if (!is_disabled_corner(corner_info.first)) {
          default_corner_ = corner_info.first;
        } else {
          num_disabled_corner++;
        }
      }
      if (default_corner_.empty()) {
        VTR_LOG_ERROR(
          "There are no availble corners in built-in corner map. Two of them are all disabled! \n");
        num_err++;
      } else if (num_disabled_corner == 0) {
        VTR_LOG_ERROR(
          "Missing the default corner There are more than two corners defined in the built-in "
          "corner map. "
          "Please double check!\n");
        num_err++;
      }
    } else if (builtin_corner_map_[corner_result->second].empty()) {
      /* corner_map has two corners. check whether one is disabled */
      int num_disabled_corner = 0;
      for (auto corner_info : corner_map_) {
        if (!is_disabled_corner(corner_info.first)) {
          default_corner_ = corner_info.first;
        } else {
          num_disabled_corner++;
        }
      }
      if (default_corner_.empty()) {
        VTR_LOG_ERROR(
          "There are no availble corners in user-defined corner map. Two of them are all disabled! "
          "\n");
        num_err++;
      } else if (num_disabled_corner == 0) {
        VTR_LOG_ERROR(
          "Missing the default corner There are more than two corners defined in the uder-defined "
          "corner map. "
          "Please double check!\n");
        num_err++;
      }
    } else {
      int num_disabled_corner = 0;
      if (!is_disabled_corner(corner_map_.begin()->first)) {
        default_corner_ = corner_map_.begin()->first;
      } else {
        num_disabled_corner++;
      }

      if (!is_disabled_corner(builtin_corner_map_[corner_result->second].begin()->first)) {
        default_corner_ = builtin_corner_map_[corner_result->second].begin()->first;
      } else {
        num_disabled_corner++;
      }
      if (default_corner_.empty()) {
        VTR_LOG_ERROR(
          "There are no availble corners in built-in corner map and user-defined corner map. Two "
          "of them are all disabled! \n");
        num_err++;
      } else if (num_disabled_corner == 0) {
        VTR_LOG_ERROR(
          "Missing the default corner There are more than two corners defined. "
          "Please double check!\n");
        num_err++;
      }
    }
  }

  if (num_corners > 1 && default_corner_.empty()) {
    VTR_LOG_ERROR(
      "Missing the default corner There are more than two corners defined in the corner map. "
      "Please double check!\n");
    num_err++;
  }
  /* Ensure the default corner is a valid one, either in user-defined or built-in */
  bool valid_default_corner = false;
  auto default_corner_result = corner_map_.find(default_corner_);
  if (default_corner_result != corner_map_.end() && !is_disabled_corner(default_corner_)) {
    valid_default_corner = true;
  } else {
    if (corner_result != pdk2builtin_corner_ids_.end()) {
      auto default_corner_builtin_result =
        builtin_corner_map_[corner_result->second].find(default_corner_);
      if (default_corner_builtin_result != builtin_corner_map_[corner_result->second].end() &&
          !is_disabled_corner(default_corner_)) {
        valid_default_corner = true;
      }
    }
  }
  if (!valid_default_corner) {
    VTR_LOG_ERROR(
      "Invalid default corner '%s' which does not exist in user-defined corners or built-in "
      "corners! Please double check!\n",
      default_corner_.c_str());
    num_err++;
  }
  /* Suggest to do the boundary timing file check after all the corner check is done */
  if (!validate_boundary_timing_file()) {
    num_err++;
  }
  if (openfpga_stats_conf_file_.empty()) {
    VTR_LOG_ERROR("Missing OpenFPGA statistics configuration file!\n");
    num_err++;
  }
  if (openfpga_error_conf_file_.empty()) {
    VTR_LOG_ERROR("Missing OpenFPGA error check configuration file!\n");
    num_err++;
  }
  if (yosys_error_conf_file_.empty()) {
    VTR_LOG_ERROR("Missing Yosys error check configuration file!\n");
    num_err++;
  }
  if (arch_template_file_.empty()) {
    VTR_LOG_ERROR("Missing architecture template file!\n");
    num_err++;
  }
  /* Arch timing template file should be specified */
  if (arch_timing_template_file_.empty()) {
    VTR_LOG_ERROR("Missing architecture timing template file!\n");
    num_err++;
  }
  if (arch_timing_template_pdk_.empty()) {
    VTR_LOG_ERROR("Missing pdk name required for default architecture timing template!\n");
    num_err++;
  }
  /* Validate the default template */
  auto tmpl_pdk = arch_timing_files_.find(arch_timing_template_pdk_);
  if (tmpl_pdk == arch_timing_files_.end()) {
    VTR_LOG_ERROR(
      "The pdk '%s' required for default architecture timing template is not defined!\n",
      arch_timing_template_pdk_.c_str());
    num_err++;
  } else {
    auto tmpl_tfile = tmpl_pdk->second.find(arch_timing_template_file_);
    if (tmpl_tfile == tmpl_pdk->second.end()) {
      VTR_LOG_ERROR(
        "The template tag '%s' on pdk '%s' required for default architecture timing template is "
        "not defined!\n",
        arch_timing_template_file_.c_str(), arch_timing_template_pdk_.c_str());
      num_err++;
    }
  }
  /* For built-in corners, all the arch timing files should be valid
   * For user-defined corners, arch timing file may not be valid. In this case, a template file is
   * needed
   */
  for (auto pdk_pair : pdk2builtin_corner_ids_) {
    size_t pdk_idx = pdk_pair.second;
    for (auto corner_pair : builtin_corner_map_[pdk_idx]) {
      std::string corner_atiming_ftype =
        corner_pair.second[size_t(TapeoutManager::e_corner_tag_type::ARCH_TIMING)];
      auto result = arch_timing_files_.find(pdk_pair.first);
      if (result == arch_timing_files_.end()) {
        VTR_LOG_ERROR(
          "No valid arch timing files are defined for pdk '%s' which is required by built-in "
          "corners\n",
          pdk_pair.first.c_str());
        num_err++;
        continue;
      }
      auto atiming_result = result->second.find(corner_pair.first);
      if (atiming_result == result->second.end()) {
        VTR_LOG_ERROR(
          "The built-in corner '%s' for pdk '%s' requires a specific arch timing file tag '%s' "
          "does not exist in arch "
          "timing file list!\n",
          corner_pair.first.c_str(), pdk_pair.first.c_str(), corner_atiming_ftype.c_str());
        num_err++;
        continue;
      }
    }
  }
  if (!arch_io_constr_.validate(true)) {
    num_err++;
  }
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err == 0;
}

void TapeoutManager::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Tapeout data is invalid!\n");
    VTR_LOG("Check the data sources where it is loaded from!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_pdk_config_file() const {
  if (!is_valid_pdk_config_file_) {
    VTR_LOG_ERROR("PDK configuration file of tapeout data is invalid!\n");
    VTR_LOG("Check the data sources where it is loaded from!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_boundary_timing_file() const {
  if (!is_valid_boundary_timing_file_) {
    VTR_LOG_ERROR("Boundary timing file of tapeout data is invalid!\n");
    VTR_LOG("Check the data sources where it is loaded from!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_arch_file_type(
  const TapeoutManager::e_arch_file_type& type) const {
  if (!valid_arch_file_type(type)) {
    VTR_LOG_ERROR("Invalid type of architecture file. Expect %s!",
                  arch_file_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_openfpga_setting_file_type(
  const TapeoutManager::e_arch_file_type& type) const {
  if (!valid_openfpga_setting_file_type(type)) {
    VTR_LOG_ERROR("Invalid type of openfpga setting file. Expect %s!",
                  arch_file_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_script_type(const TapeoutManager::e_script_type& type) const {
  if (!valid_script_type(type)) {
    VTR_LOG_ERROR("Invalid type of openfpga/yosys shell script file. Expect %s!",
                  script_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_openfpga_script_type(
  const TapeoutManager::e_script_type& type) const {
  if (!valid_openfpga_script_type(type)) {
    VTR_LOG_ERROR("Invalid type of openfpga shell script file. Expect %s!",
                  openfpga_script_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_yosys_script_type(
  const TapeoutManager::e_script_type& type) const {
  if (!valid_yosys_script_type(type)) {
    VTR_LOG_ERROR("Invalid type of yosys script file. Expect %s!",
                  yosys_script_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_step(const TapeoutManager::e_step_name& step) const {
  if (!valid_step_name(step)) {
    VTR_LOG_ERROR("Invalid step name. Expect %s!", step_name_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_run(const TapeoutManager::e_step_name& step,
                                     const std::string& run) const {
  if (!valid_run(step, run)) {
    VTR_LOG_ERROR("Run '%s' of step '%ld' does not exist!", run.c_str(),
                  step_name2str(step).c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TapeoutManager::check_valid_dep(const TapeoutManager::e_step_name& step,
                                     const e_step_name& dep) const {
  if (!valid_dep(step, dep)) {
    VTR_LOG_ERROR("Step '%s' cannot depend on '%s'. No self-dependency is allowed!",
                  step_name2str(dep).c_str(), step_name2str(step).c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

TapeoutManager::e_step_name TapeoutManager::str2step_name(const std::string& str,
                                                          const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_step_name::PRE_ARCH);
       itype != size_t(TapeoutManager::e_step_name::NUM_TYPES); ++itype) {
    if (str == std::string(STEP_NAME_STRING_[itype])) {
      return static_cast<TapeoutManager::e_step_name>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid step! Expect %s\n", step_name_all2str().c_str());
  return TapeoutManager::e_step_name::NUM_TYPES;
}

std::string TapeoutManager::step_name2str(const TapeoutManager::e_step_name& type,
                                          const bool& verbose) const {
  if (!valid_step_name(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid step! Expect %s\n", step_name_all2str().c_str());
    return std::string();
  }
  return std::string(STEP_NAME_STRING_[size_t(type)]);
}

size_t TapeoutManager::step_name_str_max_len() const {
  size_t max_len = 0;
  for (auto curr_str : STEP_NAME_STRING_) {
    max_len = std::max(std::string(curr_str).size(), max_len);
  }
  return max_len;
}

std::string TapeoutManager::step_name_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_step_name::PRE_ARCH);
       itype != size_t(TapeoutManager::e_step_name::NUM_TYPES); ++itype) {
    full_types += std::string(STEP_NAME_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_step_name(const TapeoutManager::e_step_name& step) const {
  return step != TapeoutManager::e_step_name::NUM_TYPES;
}

TapeoutManager::e_substep_name TapeoutManager::str2substep_name(const std::string& str,
                                                                const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_substep_name::ARCH_INIT);
       itype != size_t(TapeoutManager::e_substep_name::NUM_TYPES); ++itype) {
    if (str == std::string(SUBSTEP_NAME_STRING_[itype])) {
      return static_cast<TapeoutManager::e_substep_name>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid substep! Expect %s\n", substep_name_all2str().c_str());
  return TapeoutManager::e_substep_name::NUM_TYPES;
}

std::string TapeoutManager::substep_name2str(const TapeoutManager::e_substep_name& type,
                                             const bool& verbose) const {
  if (!valid_substep_name(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid substep! Expect %s\n", substep_name_all2str().c_str());
    return std::string();
  }
  return std::string(SUBSTEP_NAME_STRING_[size_t(type)]);
}

size_t TapeoutManager::substep_name_str_max_len() const {
  size_t max_len = 0;
  for (auto curr_str : SUBSTEP_NAME_STRING_) {
    max_len = std::max(max_len, std::string(curr_str).size());
  }
  return max_len;
}

std::string TapeoutManager::substep_name_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_substep_name::ARCH_INIT);
       itype != size_t(TapeoutManager::e_substep_name::NUM_TYPES); ++itype) {
    full_types += std::string(SUBSTEP_NAME_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_substep_name(const TapeoutManager::e_substep_name& substep) const {
  return substep != TapeoutManager::e_substep_name::NUM_TYPES;
}

TapeoutManager::e_run_status TapeoutManager::str2status(const std::string& str,
                                                        const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_run_status::SUCCESS);
       itype != size_t(TapeoutManager::e_run_status::NUM_TYPES); ++itype) {
    if (str == std::string(STATUS_STRING_[itype])) {
      return static_cast<TapeoutManager::e_run_status>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid status! Expect %s\n", status_all2str().c_str());
  return TapeoutManager::e_run_status::NUM_TYPES;
}

std::string TapeoutManager::status2str(const TapeoutManager::e_run_status& type,
                                       const bool& verbose) const {
  if (!valid_run_status(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid step! Expect %s\n", status_all2str().c_str());
    return std::string();
  }
  return std::string(STATUS_STRING_[size_t(type)]);
}

size_t TapeoutManager::status_str_max_len() const {
  size_t max_len = 0;
  for (auto curr_str : STATUS_STRING_) {
    max_len = std::max(std::string(curr_str).size(), max_len);
  }
  return max_len;
}

std::string TapeoutManager::status_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_run_status::SUCCESS);
       itype != size_t(TapeoutManager::e_run_status::NUM_TYPES); ++itype) {
    full_types += std::string(STATUS_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_run_status(const TapeoutManager::e_run_status& status) const {
  return status != TapeoutManager::e_run_status::NUM_TYPES;
}

TapeoutManager::e_arch_file_type TapeoutManager::str2arch_file_type(const std::string& str,
                                                                    const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_arch_file_type::VPR_ARCH);
       itype != size_t(TapeoutManager::e_arch_file_type::NUM_TYPES); ++itype) {
    if (str == std::string(ARCH_FILE_TYPE_STRING_[itype])) {
      return static_cast<TapeoutManager::e_arch_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of architecture file! Expect %s\n",
                 arch_file_type_all2str().c_str());
  return TapeoutManager::e_arch_file_type::NUM_TYPES;
}

std::string TapeoutManager::arch_file_type2str(const TapeoutManager::e_arch_file_type& type,
                                               const bool& verbose) const {
  if (!valid_arch_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of architecture file! Expect %s\n",
                   arch_file_type_all2str().c_str());
    return std::string();
  }
  return std::string(ARCH_FILE_TYPE_STRING_[size_t(type)]);
}

std::string TapeoutManager::openfpga_setting_file_type2str(
  const TapeoutManager::e_arch_file_type& type, const bool& verbose) const {
  if (!valid_openfpga_setting_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of openfpga_setting file! Expect %s\n",
                   arch_file_type_all2str().c_str());
    return std::string();
  }
  return std::string(ARCH_FILE_TYPE_STRING_[size_t(type)]);
}

std::string TapeoutManager::arch_file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_arch_file_type::VPR_ARCH);
       itype != size_t(TapeoutManager::e_arch_file_type::NUM_TYPES); ++itype) {
    full_types += std::string(ARCH_FILE_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_arch_file_type(
  const TapeoutManager::e_arch_file_type& arch_file_type) const {
  return size_t(arch_file_type) <= size_t(TapeoutManager::e_arch_file_type::DESIGN_VAR);
}

bool TapeoutManager::valid_openfpga_setting_file_type(
  const TapeoutManager::e_arch_file_type& arch_file_type) const {
  return size_t(TapeoutManager::e_arch_file_type::OPENFPGA_SIM_SETTING) <= size_t(arch_file_type) <
         size_t(TapeoutManager::e_arch_file_type::NUM_TYPES);
}

TapeoutManager::e_script_type TapeoutManager::str2script_type(const std::string& str,
                                                              const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN);
       itype != size_t(TapeoutManager::e_script_type::NUM_TYPES); ++itype) {
    if (str == std::string(SCRIPT_TYPE_STRING_[itype])) {
      return static_cast<TapeoutManager::e_script_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of openfpga script file! Expect %s\n",
                 script_type_all2str().c_str());
  return TapeoutManager::e_script_type::NUM_TYPES;
}

std::string TapeoutManager::script_type2str(const TapeoutManager::e_script_type& type,
                                            const bool& verbose) const {
  if (!valid_script_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of openfpga script file! Expect %s\n",
                   script_type_all2str().c_str());
    return std::string();
  }
  return std::string(SCRIPT_TYPE_STRING_[size_t(type)]);
}

std::string TapeoutManager::script_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN);
       itype != size_t(TapeoutManager::e_script_type::NUM_TYPES); ++itype) {
    full_types += std::string(SCRIPT_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::string TapeoutManager::openfpga_script_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN);
       itype <= size_t(TapeoutManager::e_script_type::OPENFPGA_SCRIPT_FULLTB); ++itype) {
    full_types += std::string(SCRIPT_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::string TapeoutManager::yosys_script_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_script_type::YOSYS_SCRIPT_SYNTH);
       itype <= size_t(TapeoutManager::e_script_type::YOSYS_SCRIPT_SWEEPK); ++itype) {
    full_types += std::string(SCRIPT_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_script_type(const TapeoutManager::e_script_type& script_type) const {
  return script_type != TapeoutManager::e_script_type::NUM_TYPES;
}

bool TapeoutManager::valid_openfpga_script_type(
  const TapeoutManager::e_script_type& script_type) const {
  return (size_t(TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN) <= size_t(script_type)) &&
         (size_t(script_type) <= size_t(TapeoutManager::e_script_type::OPENFPGA_SCRIPT_FULLTB));
}

bool TapeoutManager::valid_yosys_script_type(
  const TapeoutManager::e_script_type& script_type) const {
  return (size_t(TapeoutManager::e_script_type::YOSYS_SCRIPT_SYNTH) <= size_t(script_type)) &&
         (size_t(script_type) <= size_t(TapeoutManager::e_script_type::YOSYS_SCRIPT_SWEEPK));
}

std::string TapeoutManager::replace_reserved_keywords(const std::string& val) const {
  std::string ret = val;
  for (auto kw : reserved_keywords_) {
    size_t start_pos = ret.find(kw.first);
    if (start_pos == std::string::npos) {
      continue;
    }
    ret.replace(start_pos, kw.first.length(), kw.second);
  }
  return ret;
}

bool TapeoutManager::check_and_update_step_status(const e_step_name& step, const std::string& run,
                                                  const bool& verbose) {
  /* Update timestamp */
  update_date_last_modified();
  /* Check all the substep status and update the status of the step */
  bool all_success = true;
  for (auto substep : substeps(step, run)) {
    e_run_status curr_substep_status = substep_status(step, run, substep);
    if (curr_substep_status != e_run_status::SUCCESS) {
      VTR_LOGV(verbose, "Substep '%s' is not %s!\n", substep_name2str(substep).c_str(),
               status2str(curr_substep_status, false).c_str());
      all_success = false;
      break;
    }
  }
  if (all_success) {
    VTR_LOGV(verbose, "Auto-update step '%s' status to %s!\n", step_name2str(step).c_str(),
             status2str(e_run_status::SUCCESS, false).c_str());
    if (!set_step_run_status(step, run, e_run_status::SUCCESS)) {
      return false;
    }
  }
  /* Check all the downstream steps if the substep failed. */
  e_step_name curr_step = step;
  e_step_name nxt_step = next_step(step);
  while (valid_step_name(nxt_step)) {
    std::string nxt_run = find_downstream_run_on_step_run(curr_step, run, nxt_step);
    VTR_LOGV(verbose, "Found run '%s' for step '%s'\n", nxt_run.c_str(),
             step_name2str(nxt_step).c_str());
    if (!valid_run(nxt_step, nxt_run)) {
      /* The next step does not have any valid run, early finish here. No more downstream is
       * impacted */
      VTR_LOGV(verbose, "Found invalid run '%s' for step '%s'. break now\n", nxt_run.c_str(),
               step_name2str(nxt_step).c_str());
      break;
    }
    VTR_LOGV(verbose, "Found valid run '%s' for step '%s'\n", nxt_run.c_str(),
             step_name2str(nxt_step).c_str());
    /* Update the step run status and also all the substep status should be reset */
    if (!set_step_run_status_including_substeps(nxt_step, nxt_run, e_run_status::REQ_REFRESH)) {
      return false;
    }
    /* Move to next step */
    e_step_name tmp_step = nxt_step;
    nxt_step = next_step(curr_step);
    curr_step = tmp_step;
    VTR_LOGV(verbose, "Jump from current step '%s' to the next '%s'\n",
             step_name2str(curr_step).c_str(), step_name2str(nxt_step).c_str());
  }

  return true;
}

std::string TapeoutManager::invalid_run() const { return std::string(); }

TapeoutManager::e_ip_file_type TapeoutManager::str2ip_file_type(const std::string& str,
                                                                const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_ip_file_type::RTL);
       itype != size_t(TapeoutManager::e_ip_file_type::NUM_TYPES); ++itype) {
    if (str == std::string(IP_FILE_TYPE_STRING_[itype])) {
      return static_cast<TapeoutManager::e_ip_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of IP design source file! Expect %s\n",
                 ip_file_type_all2str().c_str());
  return TapeoutManager::e_ip_file_type::NUM_TYPES;
}

std::string TapeoutManager::ip_file_type2str(const TapeoutManager::e_ip_file_type& type,
                                             const bool& verbose) const {
  if (!valid_ip_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of IP design source file! Expect %s\n",
                   ip_file_type_all2str().c_str());
    return std::string();
  }
  return std::string(IP_FILE_TYPE_STRING_[size_t(type)]);
}

std::string TapeoutManager::ip_file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_ip_file_type::RTL);
       itype != size_t(TapeoutManager::e_ip_file_type::NUM_TYPES); ++itype) {
    full_types += std::string(IP_FILE_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_ip_file_type(const TapeoutManager::e_ip_file_type& ip_file_type) const {
  return ip_file_type != TapeoutManager::e_ip_file_type::NUM_TYPES;
}

void TapeoutManager::check_valid_ip_file_type(const TapeoutManager::e_ip_file_type& type) const {
  if (!valid_ip_file_type(type)) {
    VTR_LOG_ERROR("Invalid type of IP design source file. Expect %s!",
                  ip_file_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

TapeoutManager::e_ip_file_format TapeoutManager::str2ip_file_format(const std::string& str,
                                                                    const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_ip_file_format::AUTO);
       itype != size_t(TapeoutManager::e_ip_file_format::NUM_TYPES); ++itype) {
    if (str == std::string(IP_FILE_FORMAT_STRING_[itype])) {
      return static_cast<TapeoutManager::e_ip_file_format>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid format of IP design source file! Expect %s\n",
                 ip_file_format_all2str().c_str());
  return TapeoutManager::e_ip_file_format::NUM_TYPES;
}

std::string TapeoutManager::ip_file_format2str(const TapeoutManager::e_ip_file_format& fmt,
                                               const bool& verbose) const {
  if (!valid_ip_file_format(fmt)) {
    VTR_LOGV_ERROR(verbose, "Invalid format of IP design source file! Expect %s\n",
                   ip_file_format_all2str().c_str());
    return std::string();
  }
  return std::string(IP_FILE_FORMAT_STRING_[size_t(fmt)]);
}

std::string TapeoutManager::ip_file_format_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_ip_file_format::AUTO);
       itype != size_t(TapeoutManager::e_ip_file_format::NUM_TYPES); ++itype) {
    full_types += std::string(IP_FILE_FORMAT_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_ip_file_format(
  const TapeoutManager::e_ip_file_format& ip_file_fmt) const {
  return ip_file_fmt != TapeoutManager::e_ip_file_format::NUM_TYPES;
}

void TapeoutManager::check_valid_ip_file_format(const TapeoutManager::e_ip_file_format& fmt) const {
  if (!valid_ip_file_format(fmt)) {
    VTR_LOG_ERROR("Invalid format of IP design source file. Expect %s!",
                  ip_file_format_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

bool TapeoutManager::valid_ip_file_id(const IpFileId& f_id) const {
  return size_t(f_id) < ip_files_.size();
}

bool TapeoutManager::valid_qorso_task(const std::string& t_name) const {
  auto result = qorso_task_name2ids_.find(t_name);
  return result != qorso_task_name2ids_.end();
}

TapeoutManager::e_corner_tag_type TapeoutManager::str2corner_tag_type(const std::string& str,
                                                                      const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_corner_tag_type::SCLIB);
       itype != size_t(TapeoutManager::e_corner_tag_type::NUM_TYPES); ++itype) {
    if (str == std::string(CORNER_TAG_TYPE_STRING_[itype])) {
      return static_cast<TapeoutManager::e_corner_tag_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of corner tags! Expect %s\n",
                 corner_tag_type_all2str().c_str());
  return TapeoutManager::e_corner_tag_type::NUM_TYPES;
}

std::string TapeoutManager::corner_tag_type2str(const TapeoutManager::e_corner_tag_type& type,
                                                const bool& verbose) const {
  if (!valid_corner_tag_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of corner tags! Expect %s\n",
                   corner_tag_type_all2str().c_str());
    return std::string();
  }
  return std::string(CORNER_TAG_TYPE_STRING_[size_t(type)]);
}

std::string TapeoutManager::corner_tag_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_corner_tag_type::SCLIB);
       itype != size_t(TapeoutManager::e_corner_tag_type::NUM_TYPES); ++itype) {
    full_types += std::string(CORNER_TAG_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_corner_tag_type(
  const TapeoutManager::e_corner_tag_type& corner_tag_type) const {
  return corner_tag_type != TapeoutManager::e_corner_tag_type::NUM_TYPES;
}

void TapeoutManager::check_valid_corner_tag_type(
  const TapeoutManager::e_corner_tag_type& type) const {
  if (!valid_corner_tag_type(type)) {
    VTR_LOG_ERROR("Invalid type of corner tags. Expect %s!", corner_tag_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

bool TapeoutManager::add_corner_tag(const std::string& name, const std::string& sclib_tag,
                                    const std::string& spef_tag) {
  /* Sanity checks: reserved keyword should NOT be used */
  if (name == KW_CORNER_ALL_) {
    VTR_LOG_ERROR(
      "Reserved keyword '%s' cannot be used for the corner name, which indicates to sweep all the "
      "corners!\n",
      KW_CORNER_ALL_.c_str());
    return false;
  }
  auto result = corner_map_.find(name);
  if (result != corner_map_.end()) {
    VTR_LOG_WARN("Duplicated the corner tag '%s' with sclib tag '%s' and spef tag '%s'!\n",
                 name.c_str(), sclib_tag.c_str(), spef_tag.c_str());
    return false;
  }

  corner_map_[name][size_t(TapeoutManager::e_corner_tag_type::SCLIB)] = sclib_tag;
  corner_map_[name][size_t(TapeoutManager::e_corner_tag_type::SPEF)] = spef_tag;
  corner_map_[name][size_t(TapeoutManager::e_corner_tag_type::ARCH_TIMING)] = name;

  return true;
}

bool TapeoutManager::add_builtin_corner_tag(const std::string& pdk_name, const std::string& name,
                                            const std::string& sclib_tag,
                                            const std::string& spef_tag) {
  /* Sanity checks: reserved keyword should NOT be used */
  if (name == KW_CORNER_ALL_) {
    VTR_LOG_ERROR(
      "Reserved keyword '%s' cannot be used for the corner name, which indicates to sweep all the "
      "corners!\n",
      KW_CORNER_ALL_.c_str());
    return false;
  }
  auto pdk_result = pdk2builtin_corner_ids_.find(pdk_name);
  size_t entry_id = builtin_corner_map_.size();
  if (pdk_result != pdk2builtin_corner_ids_.end()) {
    entry_id = pdk_result->second;
  } else {
    builtin_corner_map_.emplace_back();
    builtin_default_corners_.emplace_back();
    pdk2builtin_corner_ids_[pdk_name] = entry_id;
  }
  auto result = builtin_corner_map_[entry_id].find(name);
  if (result != builtin_corner_map_[entry_id].end()) {
    VTR_LOG_WARN("Duplicated the built-in corner tag '%s' with sclib tag '%s' and spef tag '%s'!\n",
                 name.c_str(), sclib_tag.c_str(), spef_tag.c_str());
    return false;
  }

  builtin_corner_map_[entry_id][name][size_t(TapeoutManager::e_corner_tag_type::SCLIB)] = sclib_tag;
  builtin_corner_map_[entry_id][name][size_t(TapeoutManager::e_corner_tag_type::SPEF)] = spef_tag;
  builtin_corner_map_[entry_id][name][size_t(TapeoutManager::e_corner_tag_type::ARCH_TIMING)] =
    name;
  return true;
}

bool TapeoutManager::is_user_defined_corner(const std::string& name) const {
  check_valid();
  auto result = corner_map_.find(name);
  if (is_disabled_corner(name)) {
    VTR_LOG("corner %s is disabled! \n", name.c_str());
  }
  return result != corner_map_.end();
}

std::vector<std::string> TapeoutManager::user_defined_corner_tags() const {
  check_valid();
  std::vector<std::string> ret;
  for (auto pair : corner_map_) {
    if (is_disabled_corner(pair.first)) {
      continue;
    }
    ret.push_back(pair.first);
  }
  return ret;
}

bool TapeoutManager::is_builtin_corner(const std::string& name) const {
  check_valid();
  auto result = pdk2builtin_corner_ids_.find(pdk_name_);
  if (result == pdk2builtin_corner_ids_.end()) {
    return false;
  }
  auto c_result = builtin_corner_map_[result->second].find(name);
  if (is_disabled_corner(name)) {
    VTR_LOG("corner %s is disabled! \n", name.c_str());
  }
  return c_result != builtin_corner_map_[result->second].end();
}

bool TapeoutManager::is_disabled_corner(const std::string& corner_name, const bool& verbose) const {
  auto result = std::find(disabled_corners_.begin(), disabled_corners_.end(), corner_name);
  if (result != disabled_corners_.end()) {
    VTR_LOGV(verbose == 1, "Corner %s is disabled. \n");
    return true;
  }
  VTR_LOGV(verbose == 1, "Corner %s is not disabled. \n");
  return false;
}

std::vector<std::string> TapeoutManager::builtin_corner_tags() const {
  check_valid();
  std::vector<std::string> ret;
  auto result = pdk2builtin_corner_ids_.find(pdk_name_);
  if (result != pdk2builtin_corner_ids_.end()) {
    for (const auto& builtin_pair : builtin_corner_map_[result->second]) {
      if (is_disabled_corner(builtin_pair.first)) {
        continue;
      }
      ret.push_back(builtin_pair.first);
    }
  }
  return ret;
}

std::vector<std::string> TapeoutManager::corner_tags() const {
  check_valid();
  std::vector<std::string> ret;
  for (auto pair : corner_map_) {
    if (is_disabled_corner(pair.first)) {
      continue;
    }
    ret.push_back(pair.first);
  }
  auto result = pdk2builtin_corner_ids_.find(pdk_name_);
  if (result != pdk2builtin_corner_ids_.end()) {
    for (const auto& builtin_pair : builtin_corner_map_[result->second]) {
      if (std::find(ret.begin(), ret.end(), builtin_pair.first) == ret.end()) {
        if (is_disabled_corner(builtin_pair.first)) {
          continue;
        }
        ret.push_back(builtin_pair.first);
      }
    }
  }
  return ret;
}

std::string TapeoutManager::user_defined_corner_tag(const std::string& name,
                                                    const TapeoutManager::e_corner_tag_type& type,
                                                    const bool& keep_reserved_word) const {
  check_valid();
  check_valid_corner_tag_type(type);
  auto result = corner_map_.find(name);
  if (result == corner_map_.end() || is_disabled_corner(name)) {
    VTR_LOG_ERROR("The corner name '%s' is not defined or disabled!\n", name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  /* For arch timing, use the default template */
  if (keep_reserved_word) {
    return result->second[size_t(type)];
  }
  return replace_reserved_keywords(result->second[size_t(type)]);
}

std::string TapeoutManager::builtin_corner_tag(const std::string& name,
                                               const TapeoutManager::e_corner_tag_type& type,
                                               const bool& keep_reserved_word) const {
  check_valid();
  check_valid_corner_tag_type(type);
  auto result = corner_map_.find(name);
  std::string ret;
  auto bid_result = pdk2builtin_corner_ids_.find(pdk_name_);
  if (bid_result == pdk2builtin_corner_ids_.end()) {
    VTR_LOG_ERROR("The pdk name '%s' is not defined! \n", pdk_name_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }

  auto bin_result = builtin_corner_map_[bid_result->second].find(name);
  if (bin_result == builtin_corner_map_[bid_result->second].end() || is_disabled_corner(name)) {
    VTR_LOG_ERROR("The corner name '%s' is not defined or disabled!\n", name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  ret = bin_result->second[size_t(type)];
  if (keep_reserved_word) {
    return ret;
  }
  return replace_reserved_keywords(ret);
}

std::string TapeoutManager::corner_tag(const std::string& name,
                                       const TapeoutManager::e_corner_tag_type& type,
                                       const bool& keep_reserved_word) const {
  check_valid();
  check_valid_corner_tag_type(type);
  auto result = corner_map_.find(name);
  std::string ret;
  if (result == corner_map_.end()) {
    return builtin_corner_tag(name, type, keep_reserved_word);
  }
  return user_defined_corner_tag(name, type, keep_reserved_word);
}

int TapeoutManager::load_boot_config(const BootConfig& boot_cfg) {
  boot_type_ = boot_cfg.boot_type();
  for (auto bin_type : boot_cfg.bin_types()) {
    switch (bin_type) {
      case BootConfig::e_bin_type::OPENFPGA:
        OPENFPGA_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::FKEY_ASSIST:
        FKEYASSIST_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::PIN_TABLE_GEN:
        PINTABLEGEN_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::NETLIST_MAKEUP:
        NETLIST_MAKEUP_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::CCFF_MAKEUP:
        CCFF_MAKEUP_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::NETLIST_SYNTH:
        SYNTH_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::FRONTEND:
        FRONTEND_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::FRONTEND_CELL_LIB:
        FRONTEND_CELL_LIB_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::QORSO:
        QORSO_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::BUS_GROUP_GEN:
        BGG_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::COCOTB_UTIL:
        COCOTB_UTIL_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::COCOTB_FORCE_BIT:
        COCOTB_FORCE_BITSTREAM_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::COCOTB_APPLICATION_SIM_UTIL:
        COCOTB_CONVERT_MAKEFILE_APPLICATION_SIMULATION_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::COCOTB_MAKEFILE_INC:
        COCOTB_MAKEFILE_INC_FILE_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::BENCHMARK_SIM_COCOTB_MAKEFILE_INC:
        BM_SIM_COCOTB_MAKEFILE_INC_FILE_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::PYTHON:
        PYTHON_BIN_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::PYTHON_SITE_PACKAGES:
        PYTHON_SITE_PACKAGES_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::PYTHON_COCOTB_CONFIG:
        PYTHON_COCOTB_CONFIG_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      case BootConfig::e_bin_type::LIBPYTHON_LOC:
        LIBPYTHON_LOC_RELPATH_ = boot_cfg.bin_relpath(bin_type);
        break;
      default:
        return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

void TapeoutManager::update_date_last_modified() {
  auto end = std::chrono::system_clock::now();
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  date_last_modified_ = std::ctime(&end_time);
  /* Chop the last new line. This happens to some OS */
  if (date_last_modified_.empty() && date_last_modified_.back() == '\n') {
    date_last_modified_.pop_back();
  }
}

std::string TapeoutManager::default_corner() const { return default_corner_; }

void TapeoutManager::set_default_corner(const std::string& val) { default_corner_ = val; }
void TapeoutManager::set_pdk_name(const std::string& val) { pdk_name_ = val; }

void TapeoutManager::set_default_builtin_corner(const std::string& pdk_name,
                                                const std::string& val) {
  /* Find if the pdk name exists, if not, create an entry */

  auto result = pdk2builtin_corner_ids_.find(pdk_name);
  if (result != pdk2builtin_corner_ids_.end()) {
    builtin_default_corners_[result->second] = val;
  } else {
    size_t new_id = builtin_default_corners_.size();
    pdk2builtin_corner_ids_[pdk_name] = new_id;
    builtin_default_corners_.push_back(val);
    builtin_corner_map_.emplace_back();
  }
}

TapeoutManager::e_qorso_task_type TapeoutManager::str2qorso_task_type(const std::string& str,
                                                                      const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_qorso_task_type::LEAKAGE);
       itype != size_t(TapeoutManager::e_qorso_task_type::NUM_TYPES); ++itype) {
    if (str == std::string(QORSO_TASK_TYPE_STRING_[itype])) {
      return static_cast<TapeoutManager::e_qorso_task_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of Qorso task! Expect %s\n",
                 qorso_task_type_all2str().c_str());
  return TapeoutManager::e_qorso_task_type::NUM_TYPES;
}

std::string TapeoutManager::qorso_task_type2str(const TapeoutManager::e_qorso_task_type& type,
                                                const bool& verbose) const {
  if (!valid_qorso_task_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of Qorso task! Expect %s\n",
                   qorso_task_type_all2str().c_str());
    return std::string();
  }
  return std::string(QORSO_TASK_TYPE_STRING_[size_t(type)]);
}

std::string TapeoutManager::qorso_task_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_qorso_task_type::LEAKAGE);
       itype != size_t(TapeoutManager::e_qorso_task_type::NUM_TYPES); ++itype) {
    full_types += std::string(QORSO_TASK_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_qorso_task_type(
  const TapeoutManager::e_qorso_task_type& qorso_task_type) const {
  return qorso_task_type != TapeoutManager::e_qorso_task_type::NUM_TYPES;
}

void TapeoutManager::check_valid_qorso_task_type(
  const TapeoutManager::e_qorso_task_type& type) const {
  if (!valid_qorso_task_type(type)) {
    VTR_LOG_ERROR("Invalid type of Qorso task. Expect %s!", qorso_task_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

bool TapeoutManager::valid_corner_name(const std::string& val) const {
  /* The reserved corner is all which indicates to sweep all the available corners */
  if (KW_CORNER_ALL_ == val) {
    return true;
  }
  if (is_disabled_corner(val)) {
    VTR_LOG("The corner '%s' is disabled!\n", val.c_str());
    return false;
  }
  auto result = corner_map_.find(val);
  if (result == corner_map_.end()) {
    auto bid_result = pdk2builtin_corner_ids_.find(pdk_name_);
    if (bid_result == pdk2builtin_corner_ids_.end()) {
      return false;
    }
    auto bin_result = builtin_corner_map_[bid_result->second].find(val);
    return bin_result != builtin_corner_map_[bid_result->second].end();
  }
  return true;
}

void TapeoutManager::show_corners() const {
  VTR_LOG("Valid corners are\n");
  for (const auto& pair : corner_map_) {
    VTR_LOG("\t[User-defined] %s\n", pair.first.c_str());
  }
  auto result = pdk2builtin_corner_ids_.find(pdk_name_);
  if (result != pdk2builtin_corner_ids_.end()) {
    for (const auto& builtin_pair : builtin_corner_map_[result->second]) {
      auto dup_result = corner_map_.find(builtin_pair.first);
      if (dup_result != corner_map_.end()) {
        VTR_LOG("\t[Built-in]     %s\n", builtin_pair.first.c_str());
      }
    }
  }
  VTR_LOG("Disabled corners are\n");
  for (const auto& corner_name : disabled_corners_) {
    VTR_LOG("\t[Disabled-corners] %s\n", corner_name.c_str());
  }
}

bool TapeoutManager::sweep_corner(const std::string& val) const { return KW_CORNER_ALL_ == val; }

std::string TapeoutManager::openfpga_stats_conf_file(const bool& keep_reserved_word) const {
  check_valid();
  if (keep_reserved_word) {
    return openfpga_stats_conf_file_;
  }
  return replace_reserved_keywords(openfpga_stats_conf_file_);
}

void TapeoutManager::set_openfpga_stats_conf_file(const std::string& val) {
  openfpga_stats_conf_file_ = val;
}

std::string TapeoutManager::openfpga_error_conf_file(const bool& keep_reserved_word) const {
  check_valid();
  if (keep_reserved_word) {
    return openfpga_error_conf_file_;
  }
  return replace_reserved_keywords(openfpga_error_conf_file_);
}

void TapeoutManager::set_openfpga_error_conf_file(const std::string& val) {
  openfpga_error_conf_file_ = val;
}

std::string TapeoutManager::yosys_error_conf_file(const bool& keep_reserved_word) const {
  check_valid();
  if (keep_reserved_word) {
    return yosys_error_conf_file_;
  }
  return replace_reserved_keywords(yosys_error_conf_file_);
}

void TapeoutManager::set_yosys_error_conf_file(const std::string& val) {
  yosys_error_conf_file_ = val;
}

TapeoutManager::e_step_name TapeoutManager::next_step(
  const TapeoutManager::e_step_name& curr_step) const {
  if (curr_step == e_step_name::NUM_TYPES) {
    return curr_step; /* Already the end.*/
  }
  return static_cast<e_step_name>(size_t(curr_step) + 1);
}

std::string TapeoutManager::find_downstream_run_on_step_run(
  const TapeoutManager::e_step_name& step, const std::string& run,
  const TapeoutManager::e_step_name& n_step) const {
  check_valid_run(step, run);
  check_valid_step(n_step);
  for (auto pair : step_deps_[size_t(n_step)]) {
    if (pair.second[size_t(step)] == run) {
      return pair.first;
    }
  }
  return invalid_run();
}

bool TapeoutManager::set_step_run_status_including_substeps(
  const TapeoutManager::e_step_name& step, const std::string& run,
  const TapeoutManager::e_run_status& status) {
  bool set_status = set_step_run_status(step, run, status);
  if (!set_status) {
    return false;
  }
  for (e_substep_name substep : substeps(step, run)) {
    set_status = set_substep_status(step, run, substep, status);
    if (!set_status) {
      return false;
    }
  }
  return true;
}

std::string TapeoutManager::arch_template_file(const bool& keep_reserved_word,
                                               const bool& req_check_valid) const {
  if (req_check_valid) {
    check_valid();
  }
  if (keep_reserved_word) {
    return arch_template_file_;
  }
  return replace_reserved_keywords(arch_template_file_);
}
void TapeoutManager::set_arch_template_file(const std::string& val) { arch_template_file_ = val; }

bool TapeoutManager::set_arch_timing_template_pdk(const std::string& val) {
  arch_timing_template_pdk_ = val;
  return true;
}

bool TapeoutManager::set_arch_timing_template_file(const std::string& val) {
  arch_timing_template_file_ = val;
  return true;
}

bool TapeoutManager::add_arch_timing_file(const std::string& pdk_name, const std::string& type,
                                          const std::string& fname) {
  auto pdk_result = arch_timing_files_.find(pdk_name);
  if (pdk_result == arch_timing_files_.end()) {
    arch_timing_files_[pdk_name][type] = fname;
    return true;
  }
  auto result = arch_timing_files_[pdk_name].find(type);
  if (result != arch_timing_files_[pdk_name].end()) {
    VTR_LOG_ERROR(
      "Duplicated arch timing file for the corner definition '%s' for pdk '%s'! Please double "
      "check and ensure "
      "one corner only has 1 unique arch timing file.\n",
      pdk_name.c_str(), type.c_str());
    return false;
  }
  arch_timing_files_[pdk_name][type] = fname;
  return true;
}

std::string TapeoutManager::arch_timing_file(const std::string& name,
                                             const bool& keep_reserved_word) const {
  std::string ftype =
    corner_tag(name, TapeoutManager::e_corner_tag_type::ARCH_TIMING, keep_reserved_word);
  auto pdk_result = arch_timing_files_.find(pdk_name_);
  /* Deposit the default one */
  std::string ret = arch_timing_files_.at(arch_timing_template_pdk_).at(arch_timing_template_file_);
  if (pdk_result != arch_timing_files_.end()) {
    auto atiming_result = pdk_result->second.find(ftype);
    if (atiming_result != pdk_result->second.end()) {
      ret = pdk_result->second.at(ftype);
    }
  }
  if (keep_reserved_word) {
    return ret;
  }
  return replace_reserved_keywords(ret);
}

std::vector<std::string> TapeoutManager::arch_timing_file_pdks() const {
  check_valid();
  std::vector<std::string> ret;
  for (auto pair : arch_timing_files_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::string TapeoutManager::arch_timing_template_pdk() const {
  check_valid();
  return arch_timing_template_pdk_;
}

std::string TapeoutManager::arch_timing_template_file() const {
  check_valid();
  return arch_timing_template_file_;
}

std::vector<std::string> TapeoutManager::arch_timing_file_types(const std::string& pdk_name) const {
  check_valid();
  auto pdk_result = arch_timing_files_.find(pdk_name);
  if (pdk_result == arch_timing_files_.end()) {
    VTR_LOG_ERROR("Invalid pdk for arch timing file!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::vector<std::string> ret;
  for (auto pair : arch_timing_files_.at(pdk_name)) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::string TapeoutManager::arch_timing_file_by_type(const std::string& pdk_name,
                                                     const std::string& ftype,
                                                     const bool& keep_reserved_word) const {
  check_valid();
  auto pdk_result = arch_timing_files_.find(pdk_name);
  if (pdk_result == arch_timing_files_.end()) {
    VTR_LOG_ERROR("Invalid pdk for arch timing file!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  auto result = arch_timing_files_.at(pdk_name).find(ftype);
  if (result == arch_timing_files_.at(pdk_name).end()) {
    VTR_LOG_ERROR("Invalid type of arch timing file!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (keep_reserved_word) {
    return arch_timing_files_.at(pdk_name).at(ftype);
  }
  return replace_reserved_keywords(arch_timing_files_.at(pdk_name).at(ftype));
}

bool TapeoutManager::add_num_io_per_tile_setting(const std::string& side_str,
                                                 const std::string& type_str,
                                                 const std::string& var_name,
                                                 const std::string& msb_var_name,
                                                 const size_t& max) {
  return arch_io_constr_.add_num_io_per_tile_setting(side_str, type_str, var_name, msb_var_name,
                                                     max);
}

bool TapeoutManager::add_io_type_setting(const std::string& side_str, const std::string& var_name) {
  return arch_io_constr_.add_io_type_setting(side_str, var_name);
}

const ArchIoConstraints& TapeoutManager::arch_io_constraints() const { return arch_io_constr_; }

bool TapeoutManager::is_pdk_natively_supported_in_synth(const std::string& pdk_name) const {
  auto result = synth_strategy_file_.find(pdk_name);
  return result != synth_strategy_file_.end();
}

bool TapeoutManager::is_pdk_natively_supported_in_floorplan(const std::string& pdk_name) const {
  auto result = floorplan_strategy_file_.find(pdk_name);
  return result != floorplan_strategy_file_.end();
}

bool TapeoutManager::add_netlist_makeup_config_file(const std::string& pdk_name,
                                                    const std::string& val) {
  auto result = netlist_makeup_config_file_.find(pdk_name);
  if (result != netlist_makeup_config_file_.end() && result->second != val) {
    VTR_LOG_WARN(
      "Attempt to redefine netlist makeup configuration file for pdk '%s' from val '%lu' to "
      "val '%lu'!\n",
      pdk_name.c_str(), result->second, val);
    return false;
  }
  netlist_makeup_config_file_[pdk_name] = val;
  return true;
}

bool TapeoutManager::add_ccff_makeup_buf_config_file(const std::string& pdk_name,
                                                     const std::string& val) {
  auto result = ccff_makeup_buf_config_file_.find(pdk_name);
  if (result != ccff_makeup_buf_config_file_.end() && result->second != val) {
    VTR_LOG_WARN(
      "Attempt to redefine CCFF makeup buffer configuration file for pdk '%s' from val '%lu' to "
      "val '%lu'!\n",
      pdk_name.c_str(), result->second, val);
    return false;
  }
  ccff_makeup_buf_config_file_[pdk_name] = val;
  return true;
}

std::vector<std::string> TapeoutManager::ccff_makeup_pdk_names() const {
  std::vector<std::string> ret;
  for (auto result : ccff_makeup_buf_config_file_) {
    ret.push_back(result.first);
  }
  return ret;
}

bool TapeoutManager::pdk_require_netlist_makeup_config(const std::string& pdk_name) const {
  auto pdk_result = netlist_makeup_config_file_.find(pdk_name);
  return pdk_result != netlist_makeup_config_file_.end();
}

std::string TapeoutManager::netlist_makeup_config_file(const std::string& pdk_name,
                                                       const bool& full_path) const {
  auto pdk_result = netlist_makeup_config_file_.find(pdk_name);
  if (pdk_result == netlist_makeup_config_file_.end()) {
    VTR_LOG_ERROR("The pdk name '%s' is not defined in any netlist flag!\n", pdk_name.c_str());
    return std::string();
  }
  if (full_path) {
    std::filesystem::path root_path = netlist_makeup_key_root_path(false);
    std::filesystem::path f_path = root_path / std::filesystem::path(pdk_result->second);
    return f_path.string();
  }
  return pdk_result->second;
}

std::string TapeoutManager::ccff_makeup_buf_config_file(const std::string& pdk_name,
                                                        const bool& full_path) const {
  auto pdk_result = ccff_makeup_buf_config_file_.find(pdk_name);
  if (pdk_result == ccff_makeup_buf_config_file_.end()) {
    VTR_LOG_ERROR("The pdk name '%s' is not defined in any netlist flag!\n", pdk_name.c_str());
    return std::string();
  }
  if (full_path) {
    std::filesystem::path root_path = netlist_makeup_key_root_path(false);
    std::filesystem::path f_path = root_path / std::filesystem::path(pdk_result->second);
    return f_path.string();
  }
  return pdk_result->second;
}

std::vector<std::string> TapeoutManager::disabled_benchmarks() const {
  return disabled_benchmarks_;
}

std::vector<std::string> TapeoutManager::disabled_corners() const { return disabled_corners_; }

int TapeoutManager::add_disabled_benchmark(const std::string& val) {
  /* Check if the benchmark already exists  */
  auto result = std::find(disabled_benchmarks_.begin(), disabled_benchmarks_.end(), val);
  if (result == disabled_benchmarks_.end()) {
    disabled_benchmarks_.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  /* Error out */
  VTR_LOG_WARN("Duplicated definition for benchmark '%s' on disable list!\n", val.c_str());
  return openfpga::CMD_EXEC_MINOR_ERROR;
}

int TapeoutManager::add_disabled_corner(const std::string& val, const bool& verbose) {
  /* Check if the benchmark already exists  */
  if (val == default_corner_) {
    VTR_LOG_WARN(
      "Cannot disable default corner %s, please check available corners or change the default "
      "corner in the project file! \n",
      val.c_str());
    VTR_LOG("The available corners are shown below:\n");
    show_corners();
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  auto result = std::find(disabled_corners_.begin(), disabled_corners_.end(), val);
  if (result == disabled_corners_.end()) {
    VTR_LOGV(verbose == 1, "Set corner %s to be disabled \n", val.c_str());
    disabled_corners_.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  /* Error out */
  VTR_LOG_WARN("Duplicated definition for corner '%s' on disable list!\n", val.c_str());
  return openfpga::CMD_EXEC_SUCCESS;
}

std::string TapeoutManager::arch_gen_bin_path(const bool& keep_reserved_word) const {
  check_valid();
  if (keep_reserved_word) {
    return arch_gen_bin_path_.at(boot_type_);
  }
  return replace_reserved_keywords(arch_gen_bin_path_.at(boot_type_));
}

bool TapeoutManager::set_arch_gen_bin_path(const std::string& type_str, const std::string& val) {
  BootConfig boot_cfg;
  BootConfig::e_boot_type b_type = boot_cfg.str2boot_type(type_str, true);
  if (!boot_cfg.valid_boot_type(b_type)) {
    return false;
  }
  arch_gen_bin_path_[b_type] = val;
  return true;
}

int TapeoutManager::io_num(const std::string& run, const e_side& curr_side,
                           const ArchIoConstraints::e_io_direction& io_type) const {
  check_valid();
  auto result = num_ios_.find(run);
  if (result == num_ios_.end()) {
    /* Return default value */
    return -1;
  }
  if (!arch_io_constr_.valid_side(curr_side)) {
    VTR_LOG_ERROR("Invalid side '%s' for I/O number setting. Expect %s!\n",
                  arch_io_constr_.side2str(curr_side).c_str(),
                  arch_io_constr_.side_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (!arch_io_constr_.valid_io_direction(io_type)) {
    VTR_LOG_ERROR("Invalid type '%s' for I/O number setting. Expect %s!\n",
                  arch_io_constr_.io_direction2str(io_type).c_str(),
                  arch_io_constr_.io_direction_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return result->second[size_t(io_type)].at(curr_side);
}

bool TapeoutManager::sync_dp_dimension(const std::string& run) const {
  check_valid();
  auto result = sync_dp_dimension_.find(run);
  if (result == sync_dp_dimension_.end()) {
    /* Return default value */
    return false;
  }
  return result->second;
}

size_t TapeoutManager::core_height(const std::string& run) const {
  check_valid();
  auto result = core_dimension_.find(run);
  if (result == core_dimension_.end()) {
    /* Return default value */
    return 0;
  }
  return result->second.y();
}

size_t TapeoutManager::core_width(const std::string& run) const {
  check_valid();
  auto result = core_dimension_.find(run);
  if (result == core_dimension_.end()) {
    /* Return default value */
    return 0;
  }
  return result->second.x();
}

size_t TapeoutManager::num_clocks(const std::string& run) const {
  check_valid();
  auto result = num_clocks_.find(run);
  if (result == num_clocks_.end()) {
    /* Return default value */
    return 0;
  }
  return result->second;
}

size_t TapeoutManager::num_prog_clocks(const std::string& run) const {
  check_valid();
  auto result = num_pclocks_.find(run);
  if (result == num_pclocks_.end()) {
    /* Return default value */
    return 0;
  }
  return result->second;
}

size_t TapeoutManager::num_config_regions(const std::string& run) const {
  check_valid();
  auto result = num_config_regions_.find(run);
  if (result == num_config_regions_.end()) {
    /* Return default value */
    return 0;
  }
  return result->second;
}
size_t TapeoutManager::num_resets(const std::string& run) const {
  check_valid();
  auto result = num_resets_.find(run);
  if (result == num_resets_.end()) {
    /* Return default value */
    return 0;
  }
  return result->second;
}

int TapeoutManager::remove_io(const std::string& run, const std::string& side_str,
                              const bool& verbose) {
  check_valid();
  check_valid_run(e_step_name::ARCH, run);
  for (auto io_dir : arch_io_constr_.io_directions()) {
    /* This API is not used by parser, so validation is enforced */
    int status =
      set_io_num(run, side_str, arch_io_constr_.io_direction2str(io_dir), 0, false, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int TapeoutManager::set_sync_dp_dimension(const std::string& run, const bool& val,
                                          const bool& no_validation, const bool& verbose) {
  if (!no_validation) {
    check_valid();
    check_valid_run(e_step_name::ARCH, run);
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    if (val) {
      VTR_LOGV(verbose,
               "User chooses to apply same dimension on dp and ultimate fabric. This ensures "
               "consistency between the two fabrics\n");
    } else {
      VTR_LOGV(verbose,
               "User chooses to apply different dimension on dp and ultimate fabric. A small dp "
               "fabric reduces design planning effort\n");
    }
  }
  sync_dp_dimension_[run] = val;

  return openfpga::CMD_EXEC_SUCCESS;
}

int TapeoutManager::set_core_dimension(const std::string& run, const size_t& height,
                                       const size_t& width, const bool& no_validation,
                                       const bool& verbose) {
  if (!no_validation) {
    check_valid();
    check_valid_run(e_step_name::ARCH, run);
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Strongly suggest to set the height and width in even number\n");
    VTR_LOGV(
      verbose,
      "The min. core size is typically 2x2. Refer to device datasheet for detailed requirements\n");
    if (height != 0 && width != 0) {
      VTR_LOG("Set eFPGA core fabric size to %lu (H) x %lu (W)\n", height, width);
    }
    VTR_LOG("This excludes the I/O rows and columns\n");
    VTR_LOG("Final check will be applied when running architecture generation\n");
  }
  core_dimension_.insert(std::make_pair(run, vtr::Point<size_t>(width, height)));

  return openfpga::CMD_EXEC_SUCCESS;
}

int TapeoutManager::set_num_clocks(const std::string& run, const size_t& num_clks,
                                   const bool& no_validation, const bool& verbose) {
  if (!no_validation) {
    check_valid();
    check_valid_run(e_step_name::ARCH, run);
  }
  if (num_clks % 2 == 1) {
    VTR_LOG_ERROR("The number of clocks must be an even number, e.g., 2\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (num_clks == 0) {
    VTR_LOG("Will use the default number of clocks for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of clocks of eFPGA core fabric size to %lu\n", num_clks);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_clocks_[run] = num_clks;
  return openfpga::CMD_EXEC_SUCCESS;
}

int TapeoutManager::set_num_resets(const std::string& run, const size_t& num_rsts,
                                   const bool& no_validation, const bool& verbose) {
  if (!no_validation) {
    check_valid();
    check_valid_run(e_step_name::ARCH, run);
  }
  if (num_rsts % 2 == 1) {
    VTR_LOG_ERROR("The number of resets must be an even number, e.g., 2\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (num_rsts == 0) {
    VTR_LOG("Will use the default number of resets for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of resets of eFPGA core fabric size to %lu\n", num_rsts);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_resets_[run] = num_rsts;
  return openfpga::CMD_EXEC_SUCCESS;
}

int TapeoutManager::set_io_num(const std::string& run, const std::string& side_str,
                               const std::string& type_str, const size_t& io_num,
                               const bool& no_validation, const bool& verbose) {
  if (!no_validation) {
    check_valid();
    check_valid_run(e_step_name::ARCH, run);
  }
  /* TODO: Check the side, type are valid or not */
  e_side curr_side = arch_io_constr_.str2side(side_str, verbose);
  if (!arch_io_constr_.valid_side(curr_side)) {
    VTR_LOG_ERROR("Invalid side '%s' for I/O number setting. Expect %s!\n", side_str.c_str(),
                  arch_io_constr_.side_all2str().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  ArchIoConstraints::e_io_direction io_type = arch_io_constr_.str2io_direction(type_str, verbose);
  if (!arch_io_constr_.valid_io_direction(io_type)) {
    VTR_LOG_ERROR("Invalid type '%s' for I/O number setting. Expect %s!\n", type_str.c_str(),
                  arch_io_constr_.io_direction_all2str().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbose, "Set the number of %s on %s side of eFPGA to %lu\n",
           arch_io_constr_.io_direction2str(io_type).c_str(),
           arch_io_constr_.side2str(curr_side).c_str(), io_num);
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_ios_[run][size_t(io_type)][curr_side] = io_num;

  return openfpga::CMD_EXEC_SUCCESS;
}

int TapeoutManager::set_num_prog_clocks(const std::string& run, const size_t& num_pclks,
                                        const bool& no_validation, const bool& verbose) {
  if (!no_validation) {
    check_valid();
    check_valid_run(e_step_name::ARCH, run);
  }
  if (num_pclks == 0) {
    VTR_LOG("Will use the default number of programming clocks for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of programming clocks of eFPGA core fabric size to %lu\n",
             num_pclks);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_pclocks_[run] = num_pclks;
  return openfpga::CMD_EXEC_SUCCESS;
}

int TapeoutManager::set_num_config_regions(const std::string& run, const size_t& num_regions,
                                           const bool& no_validation, const bool& verbose) {
  if (!no_validation) {
    check_valid();
    check_valid_run(e_step_name::ARCH, run);
  }
  if (num_regions == 0) {
    VTR_LOG("Will use the default number of configurable regions for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of configurable regions of eFPGA core fabric size to %lu\n",
             num_regions);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_config_regions_[run] = num_regions;
  return openfpga::CMD_EXEC_SUCCESS;
}

std::vector<std::string> TapeoutManager::arch_gen_file_types() const {
  std::vector<std::string> ret;
  for (auto pair : arch_gen_bin_path_) {
    ret.push_back(BootConfig().boot_type2str(pair.first));
  }
  return ret;
}
std::string TapeoutManager::raw_arch_gen_bin_path(const std::string& type_str) const {
  check_valid();
  return arch_gen_bin_path_.at(BootConfig().str2boot_type(type_str));
}

TapeoutManager::e_simulator_type TapeoutManager::str2simulator_type(const std::string& str,
                                                                    const bool& verbose) const {
  for (int itype = size_t(TapeoutManager::e_simulator_type::VCS);
       itype != size_t(TapeoutManager::e_simulator_type::NUM_TYPES); ++itype) {
    if (str == std::string(SIMULATOR_TYPE_STRING_[itype])) {
      return static_cast<TapeoutManager::e_simulator_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of simulator! Expect %s\n",
                 simulator_type_all2str().c_str());
  return TapeoutManager::e_simulator_type::NUM_TYPES;
}

std::string TapeoutManager::simulator_type2str(const TapeoutManager::e_simulator_type& type,
                                               const bool& verbose) const {
  if (!valid_simulator_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of simulator! Expect %s\n",
                   simulator_type_all2str().c_str());
    return std::string();
  }
  return std::string(SIMULATOR_TYPE_STRING_[size_t(type)]);
}

std::string TapeoutManager::simulator_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(TapeoutManager::e_simulator_type::VCS);
       itype != size_t(TapeoutManager::e_simulator_type::NUM_TYPES); ++itype) {
    full_types += std::string(SIMULATOR_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool TapeoutManager::valid_simulator_type(
  const TapeoutManager::e_simulator_type& simulator_type) const {
  return simulator_type != TapeoutManager::e_simulator_type::NUM_TYPES;
}

void TapeoutManager::check_valid_simulator_type(
  const TapeoutManager::e_simulator_type& type) const {
  if (!valid_simulator_type(type)) {
    VTR_LOG_ERROR("Invalid type of simulator. Expect %s!", simulator_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::string TapeoutManager::default_simulator() const { return SIMULATOR_TYPE_STRING_[0]; }

TapeoutManager::BenchmarkStatus& TapeoutManager::find_benchmark_status(
  const std::string& benchmark_name, const std::string& run, const std::string& corner) {
  BenchmarkStatusKey key{benchmark_name, run, corner};
  auto it = benchmark_status_info_.find(key);
  if (it == benchmark_status_info_.end()) {
    // If not found, create a new one
    return benchmark_status_info_[key];
  }
  return it->second;
}

const TapeoutManager::BenchmarkStatus& TapeoutManager::find_benchmark_status(
  const std::string& benchmark_name, const std::string& run, const std::string& corner) const {
  BenchmarkStatusKey key{benchmark_name, run, corner};
  auto it = benchmark_status_info_.find(key);
  if (it == benchmark_status_info_.end()) {
    VTR_LOG_ERROR("Benchmark status not found for benchmark: %s, run: %s, corner: %s\n",
                  benchmark_name.c_str(), run.c_str(), corner.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return it->second;
}

std::vector<TapeoutManager::BenchmarkStatusKey> TapeoutManager::get_all_constraint_keys() const {
  std::vector<BenchmarkStatusKey> result;
  for (const auto& [key, _] : benchmark_status_info_) {
    result.push_back(key);
  }
  return result;
}

int TapeoutManager::add_vpr_constraints_to_benchmark(const std::string& benchmark_name,
                                                     const std::string& run,
                                                     const std::string& corner,
                                                     const std::string& file) {
  BenchmarkStatusKey key{benchmark_name, run, corner};
  benchmark_status_info_[key].constraint_files.push_back(file);
  return openfpga::CMD_EXEC_SUCCESS;
}

std::vector<std::string> TapeoutManager::get_vpr_constraint_files(const std::string& benchmark_name,
                                                                  const std::string& run,
                                                                  const std::string& corner) const {
  BenchmarkStatusKey key{benchmark_name, run, corner};
  auto it = benchmark_status_info_.find(key);
  if (it == benchmark_status_info_.end()) {
    // Return empty vector if the benchmark status doesn't exist
    return std::vector<std::string>();
  }
  return it->second.constraint_files;
}

std::vector<std::string> TapeoutManager::get_aggregated_constraint_files(
  const std::string& benchmark_name, const std::string& run, const std::string& corner,
  const std::string& internal_clock_constraint_file) const {
  std::vector<std::string> ret = get_vpr_constraint_files(benchmark_name, run, corner);
  if (!internal_clock_constraint_file.empty()) {
    ret.push_back(internal_clock_constraint_file);
  }
  return ret;
}

bool TapeoutManager::check_clock_buffer_constraints(const std::string& benchmark_name,
                                                    const std::string& run,
                                                    const std::string& corner) const {
  // Basic implementation: scaffold and avoid link errors.
  // Future: detect real conflicts between user-defined VPR constraints and clock buffer
  // constraints.
  BenchmarkStatusKey key{benchmark_name, run, corner};
  auto it = benchmark_status_info_.find(key);
  if (it == benchmark_status_info_.end()) {
    // Return false (no conflict) if benchmark status doesn't exist
    return false;
  }
  // No check currently performed, assume no conflict.
  return false;
}

int TapeoutManager::add_custom_pin_utilization_to_benchmark(
  const std::string& benchmark_name, const std::string& run, const std::string& corner,
  const std::string& tile_name, double input_pin_util, double output_pin_util) {
  // Store the custom pin utilization data
  custom_pin_utilization_nested_[benchmark_name][run][corner].tile_utilizations[tile_name] = {
    input_pin_util, output_pin_util};
  return openfpga::CMD_EXEC_SUCCESS;
}

std::pair<bool, TapeoutManager::CustomPinUtilization>
TapeoutManager::get_custom_pin_utilization_for_tile(const std::string& benchmark_name,
                                                    const std::string& run,
                                                    const std::string& corner,
                                                    const std::string& tile_name) const {
  auto benchmark_it = custom_pin_utilization_nested_.find(benchmark_name);
  if (benchmark_it != custom_pin_utilization_nested_.end()) {
    auto run_it = benchmark_it->second.find(run);
    if (run_it != benchmark_it->second.end()) {
      auto corner_it = run_it->second.find(corner);
      if (corner_it != run_it->second.end()) {
        auto tile_it = corner_it->second.tile_utilizations.find(tile_name);
        if (tile_it != corner_it->second.tile_utilizations.end()) {
          return std::make_pair(true, tile_it->second);
        }
      }
    }
  }
  // Return false and default values if not found
  return std::make_pair(false, CustomPinUtilization{0.0, 0.0});
}

std::vector<std::pair<std::string, TapeoutManager::CustomPinUtilization>>
TapeoutManager::get_custom_pin_utilization_for_benchmark_run_corner(
  const std::string& benchmark_name, const std::string& run, const std::string& corner) const {
  std::vector<std::pair<std::string, CustomPinUtilization>> result;

  auto benchmark_it = custom_pin_utilization_nested_.find(benchmark_name);
  if (benchmark_it != custom_pin_utilization_nested_.end()) {
    auto run_it = benchmark_it->second.find(run);
    if (run_it != benchmark_it->second.end()) {
      auto corner_it = run_it->second.find(corner);
      if (corner_it != run_it->second.end()) {
        for (const auto& [tile_name, util_data] : corner_it->second.tile_utilizations) {
          result.push_back(std::make_pair(tile_name, util_data));
        }
      }
    }
  }

  return result;
}

std::string TapeoutManager::get_target_ext_pin_utilization_str(const std::string& benchmark_name,
                                                               const std::string& run,
                                                               const std::string& corner) const {
  std::string result = "";

  // Get all custom pin utilization data for this benchmark/run/corner
  auto util_data = get_custom_pin_utilization_for_benchmark_run_corner(benchmark_name, run, corner);

  if (util_data.empty()) {
    return result;  // Return empty string if no custom pin utilization data
  }

  // Build the string in the expected format: clb:0.800000,1.000000 io:0.600000,0.800000
  for (size_t i = 0; i < util_data.size(); ++i) {
    const auto& [tile_name, util_values] = util_data[i];
    if (i > 0) {
      result += " ";  // Separate tiles with space
    }
    result += tile_name + ":" + std::to_string(util_values.input_pin_util) + "," +
              std::to_string(util_values.output_pin_util);
  }

  return result;
}

std::map<std::string, std::pair<float, float>> TapeoutManager::get_all_default_pin_utilization_map()
  const {
  std::map<std::string, std::pair<float, float>> result;

  std::string arch_file = arch_template_file();
  if (arch_file.empty()) {
    return result;
  }

  pugi::xml_document doc;
  pugi::xml_parse_result res = doc.load_file(arch_file.c_str());
  if (!res) {
    return result;
  }

  pugi::xml_node default_pack_pin_utilization =
    doc.child("efpga_arch_template").child("arch_template").child("default_pack_pin_utilization");
  if (!default_pack_pin_utilization) {
    // Fallback to the old location in case of legacy files
    default_pack_pin_utilization =
      doc.child("efpga_arch_template").child("default_pack_pin_utilization");
  }

  if (!default_pack_pin_utilization) {
    return result;
  }

  for (pugi::xml_node tile : default_pack_pin_utilization.children("tile")) {
    std::string name = tile.attribute("name").value();
    float input_util = tile.child("input_pin_util").text().as_float(-1.0f);
    float output_util = tile.child("output_pin_util").text().as_float(-1.0f);

    if (input_util >= 0.0 && input_util <= 1.0 && output_util >= 0.0 && output_util <= 1.0) {
      result[name] = std::make_pair(input_util, output_util);
    }
  }

  return result;
}

std::pair<float, float> TapeoutManager::get_default_pin_utilization(
  const std::string& tile_name) const {
  auto util_map = get_all_default_pin_utilization_map();
  auto it = util_map.find(tile_name);
  if (it != util_map.end()) {
    return it->second;
  }
  return std::make_pair(-1.0f, -1.0f);
}

float TapeoutManager::get_default_output_pin_utilization(const std::string& tile_name) const {
  auto default_util = get_default_pin_utilization(tile_name);
  return default_util.second;
}

float TapeoutManager::get_default_input_pin_utilization(const std::string& tile_name) const {
  auto default_util = get_default_pin_utilization(tile_name);
  return default_util.first;
}

std::string TapeoutManager::get_all_default_pin_utilization_str() const {
  std::string result;
  auto util_map = get_all_default_pin_utilization_map();

  bool first = true;
  for (const auto& [name, util] : util_map) {
    if (!first) {
      result += " ";
    }
    result += name + ":" + std::to_string(util.first) + "," + std::to_string(util.second);
    first = false;
  }

  return result;
}

void TapeoutManager::clear_constraint_data() {
  benchmark_status_info_.clear();
  custom_pin_utilization_nested_.clear();
}
}  // End namespace arcxpress
