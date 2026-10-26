#include "qor_manager.h"

#include "arcxpress_constants.h"
#include "arcxpress_file_os.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "rapidcsv.h"
#include "read_xml_qorso_strategy.h"
#include "read_yaml_tile_ref.h"
#include "read_yaml_tile_types.h"
#include "tableprinter.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "write_xml_pdk_manager.h"

namespace arcxpress {  // Begin namespace arcxpress

QorManager::QorManager(const std::string& root_path, const TapeoutManager& to_mgr,
                       const PdkManager& pdk_mgr) {
  pdk_mgr_ = &pdk_mgr;
  booting_root_ = to_mgr.booting_root();
  root_path_ = root_path;
  proj_name_ = to_mgr.name();
  is_valid_ = false;

  PYTHON_COMMAND_ = to_mgr.python_bin_path();

  QORSO_DIR_NAME_ = "qor_signoff";
  QORSO_CONFIG_DIR_NAME_ = "config";
  QORSO_SDC_DIR_NAME_ = "sdc";
  QORSO_CUSTOM_SDC_DIR_NAME_ = "custom_sdc";
  QORSO_WAVEFORM_DIR_NAME_ = "waveform";
  QORSO_DYNAMIC_POWER_TASK_NAME_ = "dynamic_power";
  QORSO_STA_TASK_NAME_ = "sta";
  QORSO_CONFIG_FILENAME_ = "qorso_task.yaml";
  PDK_DIR_NAME_ = "pdk";
  NETLIST_DIR_NAME_ = "netlist";
  FABRIC_NETLIST_FILENAME_ = "fabric_netlists.v";
  GCLK_CUSTOM_SDC_FILENAME_ = "gclk.tcl";

  GCLK_SWEEP_VAR_NAME_ = "clk_idx";
  GRST_SWEEP_VAR_NAME_ = "rst_idx";
  gclk_clock_freq_ = 100e6;  // [Hz]
  GCLK_BITSTREAM_BASE_FILENAME_ = "bitstream_base.xml";
  GCLK_BITSTREAM_VALUE_FILENAME_PREFIX_ = "bitstream_clk";
  GRST_BITSTREAM_VALUE_FILENAME_PREFIX_ = "bitstream_rst";
  GCLK_BITSTREAM_VALUE_FILENAME_POSTFIX_ = "_on.xml";
  GRST_BITSTREAM_VALUE_FILENAME_POSTFIX_ = "_on.xml";

  FILE_NAMES_ = {"qor_pdk_config.xml"};
  FILE_TAGS_ = {"pdk config for qorso"};
}

std::string QorManager::file(const e_file_type& f_type, const bool& verbose) const {
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = root_path_;
  std::filesystem::path ret;
  switch (f_type) {
    case e_file_type::PDK_CONFIG:
      ret = proj_home / std::filesystem::path(QORSO_DIR_NAME_) /
            std::filesystem::path(PDK_DIR_NAME_) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    default: {
      VTR_LOG_ERROR("Invalid file type!\n");
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
  }
  return ret.string();
}

std::string QorManager::qor_scale_report_file(std::string report_file) const {
  if (report_file.empty()) {
    return "";
  }
  std::filesystem::path proj_home = root_path_;
  std::filesystem::path ret =
    proj_home / std::filesystem::path(QORSO_DIR_NAME_) / std::filesystem::path(report_file);
  return ret.string();
}
void QorManager::set_source_netlist_type(const std::string& val) { src_netlist_type_ = val; }
void QorManager::set_source_netlist_dir(const std::string& val) { src_netlist_dir_ = val; }
void QorManager::set_tile_reference_file(const std::string& val) { src_tile_ref_fpath_ = val; }
void QorManager::set_fabric_hierarchy_file(const std::string& val) {
  src_fabric_hierarchy_fpath_ = val;
}
void QorManager::set_timing_report_dirpath(const std::string& val) { timing_report_dirpath_ = val; }
void QorManager::set_ppa_summary_dirpath(const std::string& val) { ppa_summary_dirpath_ = val; }
void QorManager::set_sdc_file_base_dirpath(const std::string& val) { sdc_file_base_dirpath_ = val; }
void QorManager::set_sdc_file_value_dirpath(const std::string& val) {
  sdc_file_value_dirpath_ = val;
}
void QorManager::set_preconifgured_sim_waveform_dirpath(const std::string& val) {
  preconifgured_sim_waveform_dirpath_ = val;
}

QorsoTaskId QorManager::create_task(const std::string& name,
                                    const TapeoutManager::e_qorso_task_type& type) {
  QorsoTaskId task_id = QorsoTaskId(qor_task_names_.size());
  qor_task_names_.push_back(name);
  qor_task_types_.push_back(type);
  qor_task_config_.emplace_back();
  return task_id;
}

bool QorManager::set_task_config(const QorsoTaskId& qtask_id, const std::string& val) {
  if (!valid_task(qtask_id)) {
    return false;
  }
  qor_task_config_[size_t(qtask_id)] = val;
  return true;
}

bool QorManager::valid_file_type(const QorManager::e_file_type& f_type) const {
  return f_type != QorManager::e_file_type::NUM_TYPES;
}

bool QorManager::valid_task(const QorsoTaskId& qtask_id) const {
  return size_t(qtask_id) < qor_task_names_.size();
}

QorManager::e_file_type QorManager::str2file_type(const std::string& str,
                                                  const bool& verbose) const {
  for (int itype = size_t(QorManager::e_file_type::PDK_CONFIG);
       itype != size_t(QorManager::e_file_type::NUM_TYPES); ++itype) {
    if (str == std::string(FILE_TAGS_[itype])) {
      return static_cast<QorManager::e_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of file! Expect %s\n", file_type_all2str().c_str());
  return QorManager::e_file_type::NUM_TYPES;
}

std::string QorManager::file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(QorManager::e_file_type::PDK_CONFIG);
       itype != size_t(QorManager::e_file_type::NUM_TYPES); ++itype) {
    full_types += std::string(FILE_TAGS_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

void QorManager::check_valid_file_type(const QorManager::e_file_type& f_type) const {
  if (!valid_file_type(f_type)) {
    VTR_LOG_ERROR("Invalid file type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

int QorManager::copy_file_with_mkdir(const std::string& src_file, const std::string& des_file,
                                     const bool& sym_link, const bool& verbose) const {
  int num_err = 0;
  if (!std::filesystem::exists(src_file)) {
    VTR_LOG_ERROR("Source file '%s' does not exist!\n", src_file.c_str());
    num_err++;
  }
  std::string src_file_fix = src_file;
  if (std::filesystem::is_symlink(src_file)) {
    std::filesystem::path src_path(src_file);
    std::filesystem::path target = std::filesystem::read_symlink(src_path);
    if (target.is_relative()) {
      target = src_path.parent_path() / target;
    }
    src_file_fix = std::filesystem::absolute(target).string();
  }
  VTR_LOGV(verbose, "Copying/link file\n");
  VTR_LOGV(verbose, "\tFrom: %s\n", src_file_fix.c_str());
  VTR_LOGV(verbose, "\tTo:   %s\n", des_file.c_str());

  /* Create directory for des */
  std::filesystem::path des_dirpath = des_file;
  openfpga::create_directory(des_dirpath.parent_path().string(), true, verbose);
  if (!sym_link) {
    std::filesystem::copy(src_file_fix, des_file,
                          std::filesystem::copy_options::overwrite_existing);
  } else {
    if (std::filesystem::exists(des_file)) {
      std::filesystem::remove(des_file);
    }
    num_err += universal_create_symlink(src_file_fix, des_file) ? 0 : 1;
  }
  if (!std::filesystem::exists(des_file)) {
    VTR_LOG_ERROR("Destination file '%s' does not exist!\n", des_file.c_str());
    num_err++;
  }
  return num_err;
}

std::string QorManager::qorso_task_root(const size_t& task_id) const {
  std::filesystem::path ret =
    std::filesystem::path(root_path_) / std::filesystem::path(QORSO_DIR_NAME_) /
    std::filesystem::path(corner_tag_) / std::filesystem::path(qor_task_names_[task_id]);
  return ret.string();
}

std::string QorManager::qorso_task_root_dynamic_power(const std::string& bm_name) const {
  std::filesystem::path ret =
    std::filesystem::path(root_path_) / std::filesystem::path(QORSO_DIR_NAME_) /
    std::filesystem::path(corner_tag_) / std::filesystem::path(QORSO_DYNAMIC_POWER_TASK_NAME_) /
    std::filesystem::path(bm_name);
  return ret.string();
}

std::string QorManager::qorso_task_root_sta(const std::string& bm_name) const {
  std::filesystem::path ret =
    std::filesystem::path(root_path_) / std::filesystem::path(QORSO_DIR_NAME_) /
    std::filesystem::path(corner_tag_) / std::filesystem::path(QORSO_STA_TASK_NAME_) /
    std::filesystem::path(bm_name);
  return ret.string();
}

std::string QorManager::qorso_custom_sdc_dir(const size_t& task_id) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_))
    .string();
}

std::string QorManager::qorso_gclk_custom_sdc_file(const size_t& task_id) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_) /
                               std::filesystem::path(GCLK_CUSTOM_SDC_FILENAME_))
    .string();
}

std::string QorManager::qorso_gclk_custom_sdc_bitstream_base_file(const size_t& task_id) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_) /
                               std::filesystem::path(GCLK_BITSTREAM_BASE_FILENAME_))
    .string();
}

std::string QorManager::qorso_gclk_bitstream_value_template_fname() const {
  return GCLK_BITSTREAM_VALUE_FILENAME_PREFIX_ + std::string("${") + GCLK_SWEEP_VAR_NAME_ +
         std::string("}") + GCLK_BITSTREAM_VALUE_FILENAME_POSTFIX_;
}

std::string QorManager::qorso_gclk_bitstream_value_fname(const size_t& clk_idx) const {
  return GCLK_BITSTREAM_VALUE_FILENAME_PREFIX_ + std::to_string(clk_idx) +
         GCLK_BITSTREAM_VALUE_FILENAME_POSTFIX_;
}

std::string QorManager::qorso_grst_bitstream_value_template_fname() const {
  return GRST_BITSTREAM_VALUE_FILENAME_PREFIX_ + std::string("${") + GRST_SWEEP_VAR_NAME_ +
         std::string("}") + GRST_BITSTREAM_VALUE_FILENAME_POSTFIX_;
}

std::string QorManager::qorso_grst_bitstream_value_fname(const size_t& rst_idx) const {
  return GRST_BITSTREAM_VALUE_FILENAME_PREFIX_ + std::to_string(rst_idx) +
         GRST_BITSTREAM_VALUE_FILENAME_POSTFIX_;
}

std::string QorManager::qorso_gclk_custom_sdc_bitstream_value_template_path(
  const size_t& task_id) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_) /
                               std::filesystem::path(qorso_gclk_bitstream_value_template_fname()))
    .string();
}

std::string QorManager::qorso_gclk_custom_sdc_bitstream_value_file(const size_t& task_id,
                                                                   const size_t& clk_idx) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_) /
                               std::filesystem::path(qorso_gclk_bitstream_value_fname(clk_idx)))
    .string();
}

std::string QorManager::qorso_grst_custom_sdc_bitstream_value_template_path(
  const size_t& task_id) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_) /
                               std::filesystem::path(qorso_grst_bitstream_value_template_fname()))
    .string();
}

std::string QorManager::qorso_grst_custom_sdc_bitstream_value_file(const size_t& task_id,
                                                                   const size_t& rst_idx) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_) /
                               std::filesystem::path(qorso_grst_bitstream_value_fname(rst_idx)))
    .string();
}

std::string QorManager::qorso_config_file(const size_t& task_id) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CONFIG_DIR_NAME_) /
                               std::filesystem::path(QORSO_CONFIG_FILENAME_))
    .string();
}

std::string QorManager::qorso_config_file_benchmark_dynamic_power(
  const std::string& bm_name) const {
  std::filesystem::path task_root_dir = qorso_task_root_dynamic_power(bm_name);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CONFIG_DIR_NAME_) /
                               std::filesystem::path(QORSO_CONFIG_FILENAME_))
    .string();
}

std::string QorManager::qorso_sdc_dir_benchmark_dynamic_power(const std::string& bm_name) const {
  std::filesystem::path task_root_dir = qorso_task_root_dynamic_power(bm_name);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_SDC_DIR_NAME_)).string();
}

std::string QorManager::qorso_waveform_dir_benchmark_dynamic_power(
  const std::string& bm_name) const {
  std::filesystem::path task_root_dir = qorso_task_root_dynamic_power(bm_name);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_WAVEFORM_DIR_NAME_))
    .string();
}

std::string QorManager::qorso_config_file_benchmark_sta(const std::string& bm_name) const {
  std::filesystem::path task_root_dir = qorso_task_root_sta(bm_name);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CONFIG_DIR_NAME_) /
                               std::filesystem::path(QORSO_CONFIG_FILENAME_))
    .string();
}

std::string QorManager::qorso_sdc_dir_benchmark_sta(const std::string& bm_name) const {
  std::filesystem::path task_root_dir = qorso_task_root_sta(bm_name);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_SDC_DIR_NAME_)).string();
}

std::string QorManager::qorso_custom_sdc_dir_benchmark_sta(const std::string& bm_name) const {
  std::filesystem::path task_root_dir = qorso_task_root_sta(bm_name);
  return std::filesystem::path(task_root_dir / std::filesystem::path(QORSO_CUSTOM_SDC_DIR_NAME_))
    .string();
}

int QorManager::setup_qorso_task(const size_t& task_id, const bool& verbose) const {
  std::filesystem::path task_root_dir = qorso_task_root(task_id);
  openfpga::create_directory(task_root_dir.string(), true, verbose);
  VTR_LOGV(verbose, "Create runtime directory '%s'\n", task_root_dir.string().c_str());
  /* Generate the config file */
  std::string src_cfg_fpath = std::filesystem::path(qor_task_config_[task_id]).string();
  std::string des_cfg_fpath = qorso_config_file(task_id);

  /* TODO: Initialize */
  std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                            corner_map_[2]};
  QorsoStrategy qs_strtg(qor_task_names_[task_id], booting_root_, pdk_mgr_->name(), corner_info);
  qs_strtg.set_netlist_input(std::string("./") + NETLIST_DIR_NAME_);
  qs_strtg.set_netlist_type(src_netlist_type_);
  qs_strtg.set_task_report_dirpath(timing_report_dirpath_);
  qs_strtg.set_summary_report_dirpath(ppa_summary_dirpath_);
  /* TODO: Set a path related to netlist dev manager and PDK files */
  qs_strtg.add_netlist_exclude(FABRIC_NETLIST_FILENAME_);
  for (std::string fpath_str : pdk_mgr_->sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    std::filesystem::path fpath = fpath_str;
    qs_strtg.add_netlist_exclude(fpath.filename().string());
  }

  int status = read_xml_qorso_strategy(src_cfg_fpath.c_str(), qs_strtg, false, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* TODO: Should have dedicated method to cache this data structure. Avoid any repeated I/O time */
  TileRef tref;
  status = read_yaml_tile_ref(src_tile_ref_fpath_.c_str(), tref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = read_yaml_tile_types(src_fabric_hierarchy_fpath_.c_str(), tref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!qs_strtg.validate(tref, verbose)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Gclk task require clock constraints to be generated at runtime and internally update the
   * strategy */
  if (qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::TIMING_GCLK ||
      qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::TIMING_GRST) {
    /* Generate timing sdc files for global signals */
    VTR_LOGV(verbose, "Write the custom SDC files for global signal timing task...\n");
    std::string cust_sdc_fpath = qorso_gclk_custom_sdc_file(task_id);
    status = generate_gclk_timing_custom_sdc_file(cust_sdc_fpath, verbose);
    VTR_LOGV(verbose, "Done\n");
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOGV(verbose, "Add the custom SDC file to Qorso strategy\n");
    for (auto curr_strtg : qs_strtg.strategies()) {
      for (auto curr_strtg_task : qs_strtg.timing_tasks(curr_strtg)) {
        VTR_LOGV(verbose, "Added the custom SDC file to Qorso task '%s'\n",
                 curr_strtg_task.c_str());
        status = qs_strtg.add_timing_task_sdc_file(curr_strtg, curr_strtg_task, cust_sdc_fpath);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
      }
    }
    VTR_LOGV(verbose, "Done\n");
    VTR_LOGV(verbose,
             "Create symbolic links to base bitstream file required by global timing task\n");
    std::string src_bit_sdc_fpath = sdc_file_base_dirpath_;
    std::string des_bit_sdc_fpath = qorso_gclk_custom_sdc_bitstream_base_file(task_id);
    status = copy_file_with_mkdir(src_bit_sdc_fpath, des_bit_sdc_fpath, true, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOGV(verbose, "Done\n");
  }
  if (qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::TIMING_GCLK) {
    VTR_LOGV(verbose,
             "Overwrite clock sweep variable in Qorso strategy with the architectural settings\n");
    status = qs_strtg.set_strategy_sweep_variable(GCLK_SWEEP_VAR_NAME_, gclk_port_.get_lsb(),
                                                  gclk_port_.get_msb(), 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOGV(verbose, "Done\n");
    VTR_LOGV(verbose, "Create symbolic links to bitstream files required by global timing task\n");
    for (auto pair : gclk_sdc_file_value_paths_) {
      std::string src_fpath = pair.second;
      std::string des_fpath = qorso_gclk_custom_sdc_bitstream_value_file(task_id, pair.first);
      status = copy_file_with_mkdir(src_fpath, des_fpath, true, verbose);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
    VTR_LOGV(verbose, "Done\n");
    VTR_LOGV(verbose, "Add the global clock bitstream to Qorso strategy\n");
    for (auto curr_strtg : qs_strtg.strategies()) {
      for (auto curr_strtg_task : qs_strtg.timing_tasks(curr_strtg)) {
        std::string bitstrm_base_sdc_fpath = qorso_gclk_custom_sdc_bitstream_base_file(task_id);
        status = qs_strtg.set_timing_task_bitstream_base_sdc_file(curr_strtg, curr_strtg_task,
                                                                  bitstrm_base_sdc_fpath);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
        std::string bitstrm_val_sdc_fpath =
          qorso_gclk_custom_sdc_bitstream_value_template_path(task_id);
        status = qs_strtg.set_timing_task_bitstream_value_sdc_file(curr_strtg, curr_strtg_task,
                                                                   bitstrm_val_sdc_fpath);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
      }
    }
    VTR_LOGV(verbose, "Done\n");
  }
  if (qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::TIMING_GRST) {
    VTR_LOGV(verbose,
             "Overwrite reset sweep variable in Qorso strategy with the architectural settings\n");
    status = qs_strtg.set_strategy_sweep_variable(GRST_SWEEP_VAR_NAME_, grst_port_.get_lsb(),
                                                  grst_port_.get_msb(), 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOGV(verbose, "Done\n");
    VTR_LOGV(verbose, "Create symbolic links to bitstream files required by global timing task\n");
    for (auto pair : grst_sdc_file_value_paths_) {
      std::string src_fpath = pair.second;
      std::string des_fpath = qorso_grst_custom_sdc_bitstream_value_file(task_id, pair.first);
      status = copy_file_with_mkdir(src_fpath, des_fpath, true, verbose);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
    VTR_LOGV(verbose, "Add the global reset bitstream to Qorso strategy\n");
    for (auto curr_strtg : qs_strtg.strategies()) {
      for (auto curr_strtg_task : qs_strtg.timing_tasks(curr_strtg)) {
        std::string bitstrm_base_sdc_fpath = qorso_gclk_custom_sdc_bitstream_base_file(task_id);
        status = qs_strtg.set_timing_task_bitstream_base_sdc_file(curr_strtg, curr_strtg_task,
                                                                  bitstrm_base_sdc_fpath);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
        std::string bitstrm_val_sdc_fpath =
          qorso_grst_custom_sdc_bitstream_value_template_path(task_id);
        status = qs_strtg.set_timing_task_bitstream_value_sdc_file(curr_strtg, curr_strtg_task,
                                                                   bitstrm_val_sdc_fpath);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
      }
    }
    VTR_LOGV(verbose, "Done\n");
  }

  /* TODO: Set a path related to netlist dev manager */
  if (qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::TIMING) {
    VTR_LOGV(verbose, "Write the Qorso timing task to an YAML file: %s.\n", des_cfg_fpath.c_str());
    status = qs_strtg.write_yaml_qorso_timing_task(des_cfg_fpath, verbose);
  } else if (qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::TIMING_GCLK ||
             qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::TIMING_GRST) {
    VTR_LOGV(verbose, "Write the Qorso timing task to an YAML file: %s.\n", des_cfg_fpath.c_str());
    status = qs_strtg.write_yaml_qorso_gclk_timing_task(des_cfg_fpath, verbose);
  } else {
    VTR_ASSERT(qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::LEAKAGE ||
               qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::DYNAMIC_POWER ||
               qor_task_types_[task_id] == TapeoutManager::e_qorso_task_type::CONFIG_POWER);
    VTR_LOGV(verbose, "Write the Qorso power task to an YAML file: %s.\n", des_cfg_fpath.c_str());
    status = qs_strtg.write_yaml_qorso_power_task(des_cfg_fpath, verbose);
  }
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Create link to netlist dir */
  std::string src_nlist_dirpath = std::filesystem::path(src_netlist_dir_).string();
  std::string des_nlist_dirpath =
    std::filesystem::path(task_root_dir / std::filesystem::path(NETLIST_DIR_NAME_)).string();
  if (std::filesystem::exists(des_nlist_dirpath)) {
    std::filesystem::remove(des_nlist_dirpath);
  }
  VTR_LOGV(verbose, "Create netlist link from '%s' to '%s'\n", src_nlist_dirpath.c_str(),
           des_nlist_dirpath.c_str());
  bool link_status = universal_create_symlink(std::filesystem::path(src_nlist_dirpath),
                                              std::filesystem::path(des_nlist_dirpath));
  if (!link_status) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::setup_benchmark_dynamic_power(const std::string& benchmark_name,
                                              const bool& force_sanity_check,
                                              const bool& verbose) const {
  /* Add corner map */
  std::filesystem::path task_root_dir = qorso_task_root_dynamic_power(benchmark_name);
  openfpga::create_directory(task_root_dir.string(), true, verbose);
  VTR_LOGV(verbose, "Create runtime directory '%s'\n", task_root_dir.string().c_str());

  std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                            corner_map_[2]};
  QorsoStrategy qs_strtg(benchmark_name, booting_root_, pdk_mgr_->name(), corner_info);
  qs_strtg.set_netlist_input(std::string("./") + NETLIST_DIR_NAME_);
  qs_strtg.set_netlist_type(src_netlist_type_);
  qs_strtg.set_task_report_dirpath(timing_report_dirpath_);
  qs_strtg.set_summary_report_dirpath(ppa_summary_dirpath_);
  /* TODO: Set a path related to netlist dev manager and PDK files */
  qs_strtg.add_netlist_exclude(FABRIC_NETLIST_FILENAME_);
  for (std::string fpath_str : pdk_mgr_->sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    std::filesystem::path fpath = fpath_str;
    qs_strtg.add_netlist_exclude(fpath.filename().string());
  }

  /* Create link to netlist dir */
  std::string src_nlist_dirpath = std::filesystem::path(src_netlist_dir_).string();
  std::string des_nlist_dirpath =
    std::filesystem::path(task_root_dir / std::filesystem::path(NETLIST_DIR_NAME_)).string();
  if (std::filesystem::exists(des_nlist_dirpath)) {
    std::filesystem::remove(des_nlist_dirpath);
  }
  VTR_LOGV(verbose, "Create netlist link from '%s' to '%s'\n", src_nlist_dirpath.c_str(),
           des_nlist_dirpath.c_str());
  bool link_status = universal_create_symlink(src_nlist_dirpath, des_nlist_dirpath);
  if (!link_status) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /*create link to sdc dir*/
  if (std::filesystem::exists(qorso_sdc_dir_benchmark_dynamic_power(benchmark_name))) {
    std::filesystem::remove_all(qorso_sdc_dir_benchmark_dynamic_power(benchmark_name));
  }
  std::string src_sdc_base_dirpath = std::filesystem::path(sdc_file_base_dirpath_).string();
  std::string des_sdc_base_dirpath =
    qorso_sdc_dir_benchmark_dynamic_power(benchmark_name) + "/fabric_bitstream_base.xml";
  VTR_LOGV(verbose, "Create sdc link from '%s' to '%s'\n", src_sdc_base_dirpath.c_str(),
           des_sdc_base_dirpath.c_str());
  copy_file_with_mkdir(src_sdc_base_dirpath, des_sdc_base_dirpath, true, verbose);

  std::string src_sdc_value_dirpath = std::filesystem::path(sdc_file_value_dirpath_).string();
  std::string des_sdc_value_dirpath =
    qorso_sdc_dir_benchmark_dynamic_power(benchmark_name) + "/fabric_bitstream.xml";

  VTR_LOGV(verbose, "Create sdc link from '%s' to '%s'\n", src_sdc_value_dirpath.c_str(),
           des_sdc_value_dirpath.c_str());
  copy_file_with_mkdir(src_sdc_value_dirpath, des_sdc_value_dirpath, true, verbose);

  /*create link to waveform dir*/
  if (std::filesystem::exists(qorso_waveform_dir_benchmark_dynamic_power(benchmark_name))) {
    std::filesystem::remove_all(qorso_waveform_dir_benchmark_dynamic_power(benchmark_name));
  }
  std::string src_waveform_dirpath =
    std::filesystem::path(preconifgured_sim_waveform_dirpath_).string();
  if (force_sanity_check) {
    if (!std::filesystem::exists(std::filesystem::path(src_waveform_dirpath))) {
      VTR_LOG_ERROR(
        "Required waveform file:\n\t%s\ndoes not exist! Please run preconfigured simulation with "
        "waveform enabled!\n",
        src_waveform_dirpath.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  std::string des_waveform_dirpath =
    qorso_waveform_dir_benchmark_dynamic_power(benchmark_name) + '/' + benchmark_name + ".fsdb";

  VTR_LOGV(verbose, "Create waveform link from '%s' to '%s'\n", src_waveform_dirpath.c_str(),
           des_waveform_dirpath.c_str());
  int num_err = copy_file_with_mkdir(src_waveform_dirpath, des_waveform_dirpath, true, verbose);
  std::string des_cfg_fpath = qorso_config_file_benchmark_dynamic_power(benchmark_name);
  VTR_LOGV(verbose, "Generating qorso task config '%s'...\n", des_cfg_fpath.c_str());
  num_err += qs_strtg.write_yaml_qorso_benchmark_power_task(des_cfg_fpath, benchmark_name, verbose);
  if (num_err != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in generating qorso task config '%s'\n", des_cfg_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  /* Corner map for spef */
  std::map<std::string, std::string> rc_corner_map;
  rc_corner_map[corner_tag_] = corner_map_[1];

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  /* write PDK config file: file writer is not yet ready */
  VTR_LOG("Generating PDK configuration to '%s'...\n", pdk_config_fpath.c_str());
  num_err +=
    write_xml_pdk_config_for_qor_extraction(pdk_config_fpath.c_str(), (*pdk_mgr_), rc_corner_map);
  if (num_err != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in generating PDK configuration '%s'\n", pdk_config_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::setup_benchmark_sta(const std::string& benchmark_name,
                                    const bool& force_sanity_check, const bool& verbose) const {
  /* Add corner map */
  std::filesystem::path task_root_dir = qorso_task_root_sta(benchmark_name);
  openfpga::create_directory(task_root_dir.string(), true, verbose);
  VTR_LOGV(verbose, "Create runtime directory '%s'\n", task_root_dir.string().c_str());

  std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                            corner_map_[2]};
  QorsoStrategy qs_strtg(benchmark_name, booting_root_, pdk_mgr_->name(), corner_info);
  qs_strtg.set_netlist_input(std::string("./") + NETLIST_DIR_NAME_);
  qs_strtg.set_netlist_type(src_netlist_type_);
  qs_strtg.set_task_report_dirpath(timing_report_dirpath_);
  qs_strtg.set_summary_report_dirpath(ppa_summary_dirpath_);
  /* TODO: Set a path related to netlist dev manager and PDK files */
  qs_strtg.add_netlist_exclude(FABRIC_NETLIST_FILENAME_);
  for (std::string fpath_str : pdk_mgr_->sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    std::filesystem::path fpath = fpath_str;
    qs_strtg.add_netlist_exclude(fpath.filename().string());
  }

  /* Create link to netlist dir */
  std::string src_nlist_dirpath = std::filesystem::path(src_netlist_dir_).string();
  std::string des_nlist_dirpath =
    std::filesystem::path(task_root_dir / std::filesystem::path(NETLIST_DIR_NAME_)).string();
  if (std::filesystem::exists(des_nlist_dirpath)) {
    std::filesystem::remove(des_nlist_dirpath);
  }
  VTR_LOGV(verbose, "Create netlist link from '%s' to '%s'\n", src_nlist_dirpath.c_str(),
           des_nlist_dirpath.c_str());
  bool link_status = universal_create_symlink(src_nlist_dirpath, des_nlist_dirpath);
  if (!link_status) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /*create link to sdc dir*/
  if (std::filesystem::exists(qorso_sdc_dir_benchmark_sta(benchmark_name))) {
    std::filesystem::remove_all(qorso_sdc_dir_benchmark_sta(benchmark_name));
  }
  std::string src_sdc_base_dirpath = std::filesystem::path(sdc_file_base_dirpath_).string();
  std::string des_sdc_base_dirpath =
    qorso_sdc_dir_benchmark_sta(benchmark_name) + "/fabric_bitstream_base.xml";
  VTR_LOGV(verbose, "Create sdc link from '%s' to '%s'\n", src_sdc_base_dirpath.c_str(),
           des_sdc_base_dirpath.c_str());
  copy_file_with_mkdir(src_sdc_base_dirpath, des_sdc_base_dirpath, true, verbose);

  std::string src_sdc_value_dirpath = std::filesystem::path(sdc_file_value_dirpath_).string();
  std::string des_sdc_value_dirpath =
    qorso_sdc_dir_benchmark_sta(benchmark_name) + "/fabric_bitstream.xml";

  VTR_LOGV(verbose, "Create sdc link from '%s' to '%s'\n", src_sdc_value_dirpath.c_str(),
           des_sdc_value_dirpath.c_str());
  copy_file_with_mkdir(src_sdc_value_dirpath, des_sdc_value_dirpath, true, verbose);

  int num_err = 0;
  std::string des_cfg_fpath = qorso_config_file_benchmark_sta(benchmark_name);
  VTR_LOGV(verbose, "Generating qorso task config '%s'...\n", des_cfg_fpath.c_str());
  num_err += qs_strtg.write_yaml_qorso_benchmark_sta_task(des_cfg_fpath, benchmark_name, verbose);
  if (num_err != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed generating qorso task config '%s'\n", des_cfg_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  /* Corner map for spef */
  std::map<std::string, std::string> rc_corner_map;
  rc_corner_map[corner_tag_] = corner_map_[1];

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  /* write PDK config file: file writer is not yet ready */
  VTR_LOG("Generating PDK configuration to '%s'...\n", pdk_config_fpath.c_str());
  num_err +=
    write_xml_pdk_config_for_qor_extraction(pdk_config_fpath.c_str(), (*pdk_mgr_), rc_corner_map);
  if (num_err != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed generating PDK configuration '%s'\n", pdk_config_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::run_benchmark_dynamic_power_analysis(const std::string& qorso_bin_path,
                                                     const std::string& benchmark_name,
                                                     const bool& dryrun, const size_t& max_jobs,
                                                     const bool& verbose) const {
  /* Generate task config file */
  int status = setup_benchmark_dynamic_power(benchmark_name, !dryrun, verbose);
  VTR_LOGV(verbose, "Generate Qorso configuration for dynamic power analysis on benchmark '%s'\n",
           benchmark_name.c_str());
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR(
      "Failed generating Qorso configuration for dynamic power analysis on benchmark '%s'\n",
      benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Run qorso */
  status = run_qorso_task_dynamic_power(benchmark_name, qorso_bin_path, max_jobs, dryrun, verbose);
  VTR_LOGV(verbose, "Run Qorso for dynamic power analysis on benchmark '%s'\n",
           benchmark_name.c_str());
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed running Qorso for dynamic power analysis on benchmark '%s'\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (dryrun) {
    return status;
  }
  /* Run qorso reporting one by one */
  VTR_LOGV(verbose, "Generate Qorso report for dynamic power analysis on benchmark '%s'\n",
           benchmark_name.c_str());
  status = parse_qorso_dynamic_power_result(qorso_bin_path, max_jobs, benchmark_name, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed generating Qorso report for dynamic power analysis on benchmark '%s'\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");
  return status;
}

int QorManager::run_benchmark_sta(const std::string& qorso_bin_path,
                                  const std::string& benchmark_name, const bool& dryrun,
                                  const size_t& max_jobs, const bool& verbose) const {
  /* Generate task config file */
  VTR_LOGV(verbose, "Generate Qorso configuratin for STA on benchmark '%s'\n",
           benchmark_name.c_str());
  int status = setup_benchmark_sta(benchmark_name, !dryrun, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed generate Qorso configuratin for STA on benchmark '%s'\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Run qorso */
  VTR_LOGV(verbose, "Run Qorso for STA on benchmark '%s'\n", benchmark_name.c_str());
  status = run_qorso_task_sta(benchmark_name, qorso_bin_path, max_jobs, dryrun, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed running Qorso for STA on benchmark '%s'\n", benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (dryrun) {
    return status;
  }
  /* Run qorso reporting one by one */
  VTR_LOGV(verbose, "Generate Qorso report for STA on benchmark '%s'\n", benchmark_name.c_str());
  status = parse_qorso_sta_result(qorso_bin_path, max_jobs, benchmark_name, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed generating Qorso report for STA on benchmark '%s'\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");
  return status;
}

int QorManager::run_qorso_task(const std::string& qorso_bin_path, const size_t& max_jobs,
                               const size_t& task_id, const bool& dryrun,
                               const bool& verbose) const {
  std::string qorso_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + qorso_bin_path);
  qorso_cmd += std::string(" --config ") + qorso_config_file(task_id);
  qorso_cmd += std::string(" --jobs ") + std::to_string(max_jobs);

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  qorso_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;

  if (dryrun) {
    qorso_cmd += std::string(" --file_generation_only ");
  }

  VTR_LOGV(verbose, "Running command:\n\t%s\n", qorso_cmd.c_str());

  if (dryrun) {
    VTR_LOG_WARN("User choose to dryrun Qor extraction for task '%s'\n",
                 qor_task_names_[task_id].c_str());
  }
  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = qorso_task_root(task_id);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run Qor extraction for task '%s'!\n",
                  qor_task_names_[task_id].c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(qorso_cmd.c_str())) {
    VTR_LOG_ERROR("Qor extraction run failed with errors for task '%s'!\n",
                  qor_task_names_[task_id].c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::run_qorso_task_dynamic_power(const std::string& benchmark_name,
                                             const std::string& qorso_bin_path,
                                             const size_t& max_jobs, const bool& dryrun,
                                             const bool& verbose) const {
  std::string qorso_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + qorso_bin_path);
  qorso_cmd +=
    std::string(" --config ") + qorso_config_file_benchmark_dynamic_power(benchmark_name);
  qorso_cmd += std::string(" --jobs ") + std::to_string(max_jobs);

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  qorso_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;

  if (dryrun) {
    qorso_cmd += std::string(" --file_generation_only ");
  }

  VTR_LOGV(verbose, "Running command:\n\t%s\n", qorso_cmd.c_str());

  if (dryrun) {
    VTR_LOG_WARN("User choose to dryrun dynamic power analysis for benchmark '%s'\n",
                 benchmark_name.c_str());
  }
  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = qorso_task_root_dynamic_power(benchmark_name);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run dynamic power analysis for benchmark '%s'!\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(qorso_cmd.c_str())) {
    VTR_LOG_ERROR("Dynamic power analysis run failed with errors for benchmark '%s'!\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::run_qorso_task_sta(const std::string& benchmark_name,
                                   const std::string& qorso_bin_path, const size_t& max_jobs,
                                   const bool& dryrun, const bool& verbose) const {
  std::string qorso_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + qorso_bin_path);
  qorso_cmd += std::string(" --config ") + qorso_config_file_benchmark_sta(benchmark_name);
  qorso_cmd += std::string(" --jobs ") + std::to_string(max_jobs);

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  qorso_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;

  if (dryrun) {
    qorso_cmd += std::string(" --file_generation_only ");
  }

  VTR_LOGV(verbose, "Running command:\n\t%s\n", qorso_cmd.c_str());

  if (dryrun) {
    VTR_LOG_WARN("User choose to dryrun timing analysis for benchmark '%s'\n",
                 benchmark_name.c_str());
  }
  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = qorso_task_root_sta(benchmark_name);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run timing analysis for benchmark '%s'!\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(qorso_cmd.c_str())) {
    VTR_LOG_ERROR("Timing analysis run failed with errors for benchmark '%s'!\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::parse_qorso_dynamic_power_result(const std::string& qorso_bin_path,
                                                 const size_t& max_jobs, const std::string& bm_name,
                                                 const bool& verbose) const {
  std::string qorso_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + qorso_bin_path);
  qorso_cmd += std::string(" --config ") + qorso_config_file_benchmark_dynamic_power(bm_name);
  qorso_cmd += std::string(" --jobs ") + std::to_string(max_jobs);

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  qorso_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;

  qorso_cmd += std::string(" --parse_report_only ");

  VTR_LOGV(verbose, "Running command:\n\t%s\n", qorso_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = qorso_task_root_dynamic_power(bm_name);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR(
      "No process is available to parse Qor result for dynamic power analysis on benchmark '%s'!\n",
      bm_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(qorso_cmd.c_str())) {
    VTR_LOG_ERROR(
      "Parse Qor result failed with errors for dynamic power analysis on benchmark '%s'!\n",
      bm_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::parse_qorso_sta_result(const std::string& qorso_bin_path, const size_t& max_jobs,
                                       const std::string& bm_name, const bool& verbose) const {
  std::string qorso_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + qorso_bin_path);
  qorso_cmd += std::string(" --config ") + qorso_config_file_benchmark_sta(bm_name);
  qorso_cmd += std::string(" --jobs ") + std::to_string(max_jobs);

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  qorso_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;

  qorso_cmd += std::string(" --parse_report_only ");

  VTR_LOGV(verbose, "Running command:\n\t%s\n", qorso_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = qorso_task_root_sta(bm_name);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to parse Qor result for STA on benchmark '%s'!\n",
                  bm_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(qorso_cmd.c_str())) {
    VTR_LOG_ERROR("Parse Qor result failed with errors for STA on benchmark '%s'!\n",
                  bm_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::parse_qorso_result(const std::string& qorso_bin_path, const size_t& max_jobs,
                                   const size_t& task_id, const bool& verbose) const {
  std::string qorso_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + qorso_bin_path);
  qorso_cmd += std::string(" --config ") + qorso_config_file(task_id);
  qorso_cmd += std::string(" --jobs ") + std::to_string(max_jobs);

  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  qorso_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;

  qorso_cmd += std::string(" --parse_report_only ");

  VTR_LOGV(verbose, "Running command:\n\t%s\n", qorso_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = qorso_task_root(task_id);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to parse Qor result for task '%s'!\n",
                  qor_task_names_[task_id].c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(qorso_cmd.c_str())) {
    VTR_LOG_ERROR("Parse Qor result failed with errors for task '%s'!\n",
                  qor_task_names_[task_id].c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::extract_qor(const std::string& qorso_bin_path, const bool& dryrun,
                            const size_t& max_jobs, const bool& verbose) const {
  int num_err = 0;
  /* Corner map for spef */
  std::map<std::string, std::string> rc_corner_map;
  rc_corner_map[corner_tag_] = corner_map_[1];
  /* write PDK config file: file writer is not yet ready */
  std::string pdk_config_fpath = file(e_file_type::PDK_CONFIG, verbose);
  VTR_LOG("Generating PDK configuration to '%s'\n", pdk_config_fpath.c_str());
  num_err +=
    write_xml_pdk_config_for_qor_extraction(pdk_config_fpath.c_str(), (*pdk_mgr_), rc_corner_map);
  if (num_err != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  /* Setup runtime for all the tasks  */
  for (size_t itask = 0; itask < qor_task_names_.size(); ++itask) {
    VTR_LOGV(verbose, "Setting up Qorso task '%s'\n", qor_task_names_[itask].c_str());
    num_err += setup_qorso_task(itask, verbose);
    if (num_err != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Failed setting up Qorso task '%s'\n", qor_task_names_[itask].c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
  }

  /* Run qorso one by one */
  for (size_t itask = 0; itask < qor_task_names_.size(); ++itask) {
    VTR_LOGV(verbose, "Run Qorso task '%s'\n", qor_task_names_[itask].c_str());
    num_err += run_qorso_task(qorso_bin_path, max_jobs, itask, dryrun, verbose);
    if (num_err != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Failed running Qorso task '%s'\n", qor_task_names_[itask].c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
  }
  if (dryrun) {
    return openfpga::CMD_EXEC_SUCCESS;
  }

  /* Run qorso reporting one by one */
  for (size_t itask = 0; itask < qor_task_names_.size(); ++itask) {
    VTR_LOGV(verbose, "Generate Qorso report for task '%s'\n", qor_task_names_[itask].c_str());
    num_err += parse_qorso_result(qorso_bin_path, max_jobs, itask, verbose);
    if (num_err != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Failed generating Qorso report for task '%s'\n",
                    qor_task_names_[itask].c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::print_qorso_task_scale_factors(TablePrinter& tp, const std::string& corner,
                                               const int& precision, const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  for (size_t itask = 0; itask < qor_task_names_.size(); ++itask) {
    VTR_LOGV("Setting up Qorso task '%s'\n", qor_task_names_[itask].c_str());
    std::string src_cfg_fpath = std::filesystem::path(qor_task_config_[itask]).string();
    /* TODO: Initialize */
    std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                              corner_map_[2]};
    QorsoStrategy qs_strtg =
      QorsoStrategy(qor_task_names_[itask], booting_root_, pdk_mgr_->name(), corner_info);

    status = read_xml_qorso_strategy(src_cfg_fpath.c_str(), qs_strtg, false, verbose);

    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Failed to read qorso strategy xml file\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::string general_task_name = qor_task_names_[itask];
    status = qs_strtg.scale_factors(general_task_name, tp, corner, precision);

    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Failed to get scale factors for subtasks\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  VTR_LOG("Done\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

void QorManager::set_corner(const std::string& tag, const std::string& pvt_tag,
                            const std::string& spef_tag, const std::string& ocv_tag) {
  corner_tag_ = tag;
  corner_map_[0] = pvt_tag;
  corner_map_[1] = spef_tag;
  corner_map_[2] = ocv_tag;
}

int QorManager::arch_timing_files(std::vector<std::string>& files) const {
  files.clear();
  /* TODO: Initialize */
  std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                            corner_map_[2]};
  for (size_t itask = 0; itask < qor_task_names_.size(); ++itask) {
    QorsoStrategy qs_strtg(qor_task_names_[itask], booting_root_, pdk_mgr_->name(), corner_info);
    qs_strtg.set_netlist_input(std::string("./") + NETLIST_DIR_NAME_);
    qs_strtg.set_netlist_type(src_netlist_type_);
    qs_strtg.set_task_report_dirpath(timing_report_dirpath_);
    qs_strtg.set_summary_report_dirpath(ppa_summary_dirpath_);
    /* TODO: Set a path related to netlist dev manager and PDK files */
    qs_strtg.add_netlist_exclude(FABRIC_NETLIST_FILENAME_);
    for (std::string fpath_str : pdk_mgr_->sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
      std::filesystem::path fpath = fpath_str;
      qs_strtg.add_netlist_exclude(fpath.filename().string());
    }

    std::string src_cfg_fpath = std::filesystem::path(qor_task_config_[itask]).string();
    int status = read_xml_qorso_strategy(src_cfg_fpath.c_str(), qs_strtg, true, false);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::vector<std::string> curr_files = qs_strtg.summary_yaml_files();
    files.insert(files.end(), curr_files.begin(), curr_files.end());
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::extract_and_report_benchmark_power(std::string& spwr_str,
                                                   const std::string& bm_name,
                                                   const bool& verbose) const {
  /* Add corner map */
  std::filesystem::path task_root_dir = qorso_task_root_dynamic_power(bm_name);
  openfpga::create_directory(task_root_dir.string(), true, verbose);
  VTR_LOGV(verbose, "Create runtime directory '%s'\n", task_root_dir.string().c_str());

  std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                            corner_map_[2]};
  /* Give an empty name as it does not matter the report file name here */
  QorsoStrategy qs_strtg(std::string(), booting_root_, pdk_mgr_->name(), corner_info);
  qs_strtg.set_netlist_input(std::string("./") + NETLIST_DIR_NAME_);
  qs_strtg.set_netlist_type(src_netlist_type_);
  qs_strtg.set_task_report_dirpath(timing_report_dirpath_);
  qs_strtg.set_summary_report_dirpath(ppa_summary_dirpath_);
  /* TODO: Set a path related to netlist dev manager and PDK files */
  qs_strtg.add_netlist_exclude(FABRIC_NETLIST_FILENAME_);
  for (std::string fpath_str : pdk_mgr_->sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    std::filesystem::path fpath = fpath_str;
    qs_strtg.add_netlist_exclude(fpath.filename().string());
  }

  /* FIXME: Make it a constant */
  std::string pcsv_file = qs_strtg.report_power_summary_csv_file("dynamic_power");

  /* Read resulting CSV file */
  if (!std::filesystem::exists(pcsv_file)) {
    VTR_LOG_ERROR("Expected benchmark power report does not exist at\n\t%s\n", pcsv_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Parsing the power report at \n\t%s\n", pcsv_file.c_str());
  rapidcsv::Document doc(pcsv_file.c_str(), rapidcsv::LabelParams(-1, -1),
                         rapidcsv::SeparatorParams(','));
  int num_rows = doc.GetRowCount();
  size_t cnt = 0;
  float sum_pwr = 0.;
  for (int irow = 1; irow < num_rows; irow++) {
    std::vector<std::string> row_vec = doc.GetRow<std::string>(irow);
    /* 1st column is the name and the 2nd is the number */
    sum_pwr += std::stof(row_vec[1]);
    cnt++;
  }
  /* Print out the power
     Currently, the default unit for leakage power is W
     TODO: Check the unit and apply scaling! */
  VTR_LOG("\nTotal power is %g %s\n", sum_pwr * 1.0e3, "mW");
  std::stringstream ss;
  ss << sum_pwr * 1.0e3 << "mW";
  spwr_str = ss.str();

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::extract_and_report_static_power(std::string& spwr_str, const bool& verbose) const {
  /* Read Qorso strategy files */
  std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                            corner_map_[2]};
  /* Give an empty name as it does not matter the report file name here */
  QorsoStrategy qs_strtg(std::string(), booting_root_, pdk_mgr_->name(), corner_info);
  qs_strtg.set_netlist_input(std::string("./") + NETLIST_DIR_NAME_);
  qs_strtg.set_netlist_type(src_netlist_type_);
  qs_strtg.set_task_report_dirpath(timing_report_dirpath_);
  qs_strtg.set_summary_report_dirpath(ppa_summary_dirpath_);
  /* TODO: Set a path related to netlist dev manager and PDK files */
  qs_strtg.add_netlist_exclude(FABRIC_NETLIST_FILENAME_);
  for (std::string fpath_str : pdk_mgr_->sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    std::filesystem::path fpath = fpath_str;
    qs_strtg.add_netlist_exclude(fpath.filename().string());
  }

  for (size_t itask = 0; itask < qor_task_names_.size(); ++itask) {
    std::string src_cfg_fpath = std::filesystem::path(qor_task_config_[itask]).string();
    int status = read_xml_qorso_strategy(src_cfg_fpath.c_str(), qs_strtg, true, false);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* TODO: Should have dedicated method to cache this data structure. Avoid any repeated I/O time */
  TileRef tref;
  int status = read_yaml_tile_ref(src_tile_ref_fpath_.c_str(), tref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = read_yaml_tile_types(src_fabric_hierarchy_fpath_.c_str(), tref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!qs_strtg.validate(tref, verbose)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Sanity checks: Only 1 strategy is allowed */
  std::vector<std::string> strtg_names = qs_strtg.strategies();
  if (strtg_names.empty()) {
    VTR_LOG_ERROR("No valid strategy defined for static power extraction!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (strtg_names.size() > 1) {
    VTR_LOG_ERROR(
      "Only expect 1 strategy defined for static power extraction! Currently there are %ld\n",
      strtg_names.size());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string pcsv_file = qs_strtg.report_power_summary_csv_file(strtg_names[0]);

  /* Read resulting CSV file */
  if (!std::filesystem::exists(pcsv_file)) {
    VTR_LOG_ERROR("Expected static power report does not exist at\n\t%s\n", pcsv_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Parsing the power report at \n\t%s\n", pcsv_file.c_str());
  rapidcsv::Document doc(pcsv_file.c_str(), rapidcsv::LabelParams(-1, -1),
                         rapidcsv::SeparatorParams(','));
  int num_rows = doc.GetRowCount();
  size_t cnt = 0;
  float sum_pwr = 0.;
  for (int irow = 1; irow < num_rows; irow++) {
    std::vector<std::string> row_vec = doc.GetRow<std::string>(irow);
    /* 1st column is the name and the 2nd is the number */
    sum_pwr += std::stof(row_vec[1]);
    cnt++;
  }
  /* Print out the power
     Currently, the default unit for leakage power is W
     TODO: Check the unit and apply scaling! */
  VTR_LOG("\nStatic power is %g %s\n", sum_pwr * 1.0e3, "mW");
  std::stringstream ss;
  ss << sum_pwr * 1.0e3 << "mW";
  spwr_str = ss.str();

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::extract_and_report_config_power(std::string& spwr_str, const bool& verbose) const {
  /* Read Qorso strategy files */
  std::array<std::string, 4> corner_info = {corner_tag_, corner_map_[0], corner_map_[1],
                                            corner_map_[2]};
  /* Give an empty name as it does not matter the report file name here */
  QorsoStrategy qs_strtg(std::string(), booting_root_, pdk_mgr_->name(), corner_info);
  qs_strtg.set_netlist_input(std::string("./") + NETLIST_DIR_NAME_);
  qs_strtg.set_netlist_type(src_netlist_type_);
  qs_strtg.set_task_report_dirpath(timing_report_dirpath_);
  qs_strtg.set_summary_report_dirpath(ppa_summary_dirpath_);
  /* TODO: Set a path related to netlist dev manager and PDK files */
  qs_strtg.add_netlist_exclude(FABRIC_NETLIST_FILENAME_);
  for (std::string fpath_str : pdk_mgr_->sclib_files_by_type(PdkManager::e_file_type::VERILOG)) {
    std::filesystem::path fpath = fpath_str;
    qs_strtg.add_netlist_exclude(fpath.filename().string());
  }

  for (size_t itask = 0; itask < qor_task_names_.size(); ++itask) {
    std::string src_cfg_fpath = std::filesystem::path(qor_task_config_[itask]).string();
    int status = read_xml_qorso_strategy(src_cfg_fpath.c_str(), qs_strtg, true, false);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* TODO: Should have dedicated method to cache this data structure. Avoid any repeated I/O time */
  TileRef tref;
  int status = read_yaml_tile_ref(src_tile_ref_fpath_.c_str(), tref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = read_yaml_tile_types(src_fabric_hierarchy_fpath_.c_str(), tref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!qs_strtg.validate(tref, verbose)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Sanity checks: Only 1 strategy is allowed */
  std::vector<std::string> strtg_names = qs_strtg.strategies();
  if (strtg_names.empty()) {
    VTR_LOG_ERROR("No valid strategy defined for configuration power extraction!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (strtg_names.size() > 1) {
    VTR_LOG_ERROR(
      "Only expect 1 strategy defined for configuration power extraction! Currently there are "
      "%ld\n",
      strtg_names.size());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string pcsv_file = qs_strtg.report_power_summary_csv_file(strtg_names[0]);

  /* Read resulting CSV file */
  if (!std::filesystem::exists(pcsv_file)) {
    VTR_LOG_ERROR("Expected configuration power report does not exist at\n\t%s\n",
                  pcsv_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Parsing the power report at \n\t%s\n", pcsv_file.c_str());
  rapidcsv::Document doc(pcsv_file.c_str(), rapidcsv::LabelParams(-1, -1),
                         rapidcsv::SeparatorParams(','));
  int num_rows = doc.GetRowCount();
  size_t cnt = 0;
  float sum_pwr = 0.;
  for (int irow = 1; irow < num_rows; irow++) {
    std::vector<std::string> row_vec = doc.GetRow<std::string>(irow);
    /* 1st column is the name and the 2nd is the number */
    sum_pwr += std::stof(row_vec[1]);
    cnt++;
  }
  /* Print out the power
     Currently, the default unit for leakage power is W
     TODO: Check the unit and apply scaling! */
  VTR_LOG("\nConfiguration power is %g %s\n", sum_pwr * 1.0e3, "mW");
  std::stringstream ss;
  ss << sum_pwr * 1.0e3 << "mW";
  spwr_str = ss.str();

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::generate_gclk_timing_custom_sdc_file(const std::string& cust_sdc_fpath,
                                                     const bool& verbose) const {
  /* Derive target directory from qorso_task_config */
  std::filesystem::path sdc_fpath = std::filesystem::path(cust_sdc_fpath);
  std::string cust_sdc_dir = sdc_fpath.parent_path().string();

  /* Create directory if it doesn't exist */
  openfpga::create_directory(cust_sdc_dir, true, verbose);

  /* Open output file */
  std::ofstream sdc_ofs(sdc_fpath, std::ios::out | std::ios::trunc);
  if (!sdc_ofs.is_open()) {
    VTR_LOG_ERROR("Failed to open output SDC file '%s'\n", sdc_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbose, "Generating timing_gclk SDC file at '%s'\n", sdc_fpath.c_str());
  /* Convert to the clock period in [ns] */
  float clk_period = (1. / gclk_clock_freq_) / 1e-9;
  float waveform_offset = clk_period / 2.0f;

  /* Generate SDC constraints for each clock and reset port */
  for (auto ipin : gclk_port_.pins()) {
    openfpga::BasicPort curr_port(gclk_port_.get_name(), ipin, ipin);
    std::string actual_clock_port = curr_port.to_simple_verilog_string();
    /* Write create_clock constraint */
    sdc_ofs << "create_clock -name CLK_" << ipin << " -period " << clk_period << " -waveform {0 "
            << waveform_offset << "} [get_ports \"" << actual_clock_port << "\"]\n";
    VTR_LOGV(verbose, "\tCreated clock '%s' with period '%g' [ns] \n", actual_clock_port.c_str(),
             clk_period);
  }

  sdc_ofs.close();

  VTR_LOGV(verbose, "Successfully generated SDC file for %ld global clocks\n",
           gclk_port_.get_width());

  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::set_gclk_clock_freq(const std::string& val) {
  try {
    gclk_clock_freq_ = std::stof(val) * 1e6;
  } catch (const std::invalid_argument& e) {
    VTR_LOG_ERROR("Invalid global clock frequency %s [MHz]. Expect to be a float number\n",
                  e.what());
  } catch (const std::out_of_range& e) {
    VTR_LOG_ERROR("Global clock frequency %s [MHz] is out of range as a float number\n", e.what());
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::set_gclk_port(const openfpga::BasicPort& val) {
  gclk_port_ = val;
  if (!gclk_port_.is_valid()) {
    VTR_LOG_ERROR("Invalid global clock port '%s'\n", gclk_port_.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::set_grst_port(const openfpga::BasicPort& val) {
  grst_port_ = val;
  if (!grst_port_.is_valid()) {
    VTR_LOG_ERROR("Invalid global reset port '%s'\n", grst_port_.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorManager::set_gclk_sdc_file_value_dirpath(const size_t& idx, const std::string& val) {
  if (!gclk_port_.is_valid()) {
    VTR_LOG_ERROR("Invalid global clock port '%s'\n", gclk_port_.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort curr_port(gclk_port_.get_name(), idx, idx);
  if (!gclk_port_.contained(curr_port)) {
    VTR_LOG_ERROR(
      "Invalid global clock port '%s' for indexing sdc value file, which is out of the range "
      "'%s'\n",
      curr_port.to_verilog_string().c_str(), gclk_port_.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  gclk_sdc_file_value_paths_[idx] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}
int QorManager::set_grst_sdc_file_value_dirpath(const size_t& idx, const std::string& val) {
  if (!grst_port_.is_valid()) {
    VTR_LOG_ERROR("Invalid global reset port '%s'\n", grst_port_.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::BasicPort curr_port(grst_port_.get_name(), idx, idx);
  if (!grst_port_.contained(curr_port)) {
    VTR_LOG_ERROR(
      "Invalid global reset port '%s' for indexing sdc value file, which is out of the range "
      "'%s'\n",
      curr_port.to_verilog_string().c_str(), grst_port_.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  grst_sdc_file_value_paths_[idx] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arcxpress
