#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "arcxpress_report_util.h"
#include "openfpga_port.h"
#include "pdk_manager.h"
#include "qorso_strategy.h"
#include "tableprinter.h"
#include "tapeout_manager.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArcXpress benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class QorManager {
 public: /* -- Types -- */
  /* Define all the file types including input and outputs
   * The file type is used for file tracking during various development process/steps
   * IMPORTANT: the first 4 elements must match the e_file_type of TapeoutManager !!!
   */
  enum class e_file_type {
    PDK_CONFIG,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };

 private: /* -- Types for internal use only -- */
 public:  /* -- Constructors -- */
  QorManager(const std::string& root_path, const TapeoutManager& to_mgr, const PdkManager& pdk_mgr);

 public: /* -- Aggregators -- */
 public: /* -- Accessors -- */
  /* Generate the path to input/output files for each step */
  std::string file(const e_file_type& f_type, const bool& verbose) const;
  int arch_timing_files(std::vector<std::string>& files) const;
  int extract_qor(const std::string& qor_bin_path, const bool& dryrun, const size_t& max_jobs,
                  const bool& verbose) const;
  int extract_and_report_static_power(std::string& spwr_str, const bool& verbose) const;
  int extract_and_report_benchmark_power(std::string& spwr_str, const std::string& bm_name,
                                         const bool& verbose) const;
  int extract_and_report_config_power(std::string& spwr_str, const bool& verbose) const;
  std::string qor_scale_report_file(std::string report_file) const;
  int print_qorso_task_scale_factors(TablePrinter& tp, const std::string& corner,
                                     const int& precision, const bool& verbose) const;

 public: /* -- Mutators -- */
  void set_source_netlist_type(const std::string& val);
  void set_source_netlist_dir(const std::string& val);
  QorsoTaskId create_task(const std::string& name, const TapeoutManager::e_qorso_task_type& type);
  bool set_task_config(const QorsoTaskId& qtask_id, const std::string& val);
  void set_corner(const std::string& tag, const std::string& pvt_tag, const std::string& spef_tag,
                  const std::string& ocv_tag);
  void set_tile_reference_file(const std::string& val);
  void set_fabric_hierarchy_file(const std::string& val);
  void set_timing_report_dirpath(const std::string& val);
  void set_ppa_summary_dirpath(const std::string& val);
  void set_sdc_file_base_dirpath(const std::string& val);
  void set_sdc_file_value_dirpath(const std::string& val);
  void set_preconifgured_sim_waveform_dirpath(const std::string& val);
  /* The unit of clock freq is [MHz] */
  int set_gclk_clock_freq(const std::string& val);
  int set_gclk_port(const openfpga::BasicPort& val);
  int set_grst_port(const openfpga::BasicPort& val);
  int set_gclk_sdc_file_value_dirpath(const size_t& idx, const std::string& val);
  int set_grst_sdc_file_value_dirpath(const size_t& idx, const std::string& val);

 public: /* -- Validator -- */
  bool valid_file_type(const e_file_type& f_type) const;
  bool valid_task(const QorsoTaskId& task_id) const;

 public: /* Public utility */
  e_file_type str2file_type(const std::string& str, const bool& verbose = false) const;
  std::string file_type_all2str() const;
  int run_benchmark_dynamic_power_analysis(const std::string& qorso_bin_path,
                                           const std::string& benchmark_name, const bool& dryrun,
                                           const size_t& max_jobs, const bool& verbose) const;
  int run_benchmark_sta(const std::string& qorso_bin_path, const std::string& benchmark_name,
                        const bool& dryrun, const size_t& max_jobs, const bool& verbose) const;
  std::string qorso_custom_sdc_dir_benchmark_sta(const std::string& bm_name) const;

 private: /* Private utility */
  /* A complete process to copy a file from a source to a destination. Support to copy as a symbolic
   * link It will create directories if does not exist Sanity checks will be applied:
   * - Source file does not exist
   * - Destination file does not exist
   */
  int copy_file_with_mkdir(const std::string& src_file, const std::string& des_file,
                           const bool& symlink, const bool& verbose) const;
  int setup_qorso_task(const size_t& task_id, const bool& verbose) const;

  int run_qorso_task(const std::string& qorso_bin_path, const size_t& max_jobs,
                     const size_t& task_id, const bool& dryrun, const bool& verbose) const;

  std::string qorso_config_file(const size_t& task_id) const;
  std::string qorso_custom_sdc_dir(const size_t& task_id) const;
  std::string qorso_gclk_custom_sdc_file(const size_t& task_id) const;
  std::string qorso_gclk_custom_sdc_bitstream_base_file(const size_t& task_id) const;
  std::string qorso_gclk_custom_sdc_bitstream_value_template_path(const size_t& task_id) const;
  std::string qorso_gclk_custom_sdc_bitstream_value_file(const size_t& task_id,
                                                         const size_t& clk_idx) const;
  std::string qorso_gclk_bitstream_value_template_fname() const;
  std::string qorso_gclk_bitstream_value_fname(const size_t& clk_idx) const;
  std::string qorso_grst_custom_sdc_bitstream_value_template_path(const size_t& task_id) const;
  std::string qorso_grst_custom_sdc_bitstream_value_file(const size_t& task_id,
                                                         const size_t& rst_idx) const;
  std::string qorso_grst_bitstream_value_template_fname() const;
  std::string qorso_grst_bitstream_value_fname(const size_t& clk_idx) const;
  std::string qorso_config_file_benchmark_dynamic_power(const std::string& bm_name) const;
  std::string qorso_sdc_dir_benchmark_dynamic_power(const std::string& bm_name) const;

  std::string qorso_waveform_dir_benchmark_dynamic_power(const std::string& bm_name) const;

  std::string qorso_config_file_benchmark_sta(const std::string& bm_name) const;

  std::string qorso_sdc_dir_benchmark_sta(const std::string& bm_name) const;

  int parse_qorso_result(const std::string& qorso_bin_path, const size_t& max_jobs,
                         const size_t& task_id, const bool& verbose) const;
  std::string qorso_task_root(const size_t& task_id) const;
  std::string qorso_task_root_dynamic_power(const std::string& bm_name) const;
  std::string qorso_task_root_sta(const std::string& bm_name) const;

  int setup_benchmark_dynamic_power(const std::string& benchmark_name,
                                    const bool& force_sanity_check, const bool& verbose) const;
  int run_qorso_task_dynamic_power(const std::string& benchmark_name,
                                   const std::string& qorso_bin_path, const size_t& max_jobs,
                                   const bool& dryrun, const bool& verbose) const;
  int parse_qorso_dynamic_power_result(const std::string& qorso_bin_path, const size_t& max_jobs,
                                       const std::string& bm_name, const bool& verbose) const;
  int setup_benchmark_sta(const std::string& benchmark_name, const bool& force_sanity_check,
                          const bool& verbose) const;
  int run_qorso_task_sta(const std::string& benchmark_name, const std::string& qorso_bin_path,
                         const size_t& max_jobs, const bool& dryrun, const bool& verbose) const;
  int parse_qorso_sta_result(const std::string& qorso_bin_path, const size_t& max_jobs,
                             const std::string& bm_name, const bool& verbose) const;
  int generate_gclk_timing_custom_sdc_file(const std::string& cust_sdc_fpath,
                                           const bool& verbose) const;

 private: /* -- Private Validator -- */
  void check_valid_file_type(const e_file_type& f_type) const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::string booting_root_;
  std::string root_path_;
  std::string proj_name_;

  /* Source file lists */
  std::string src_netlist_type_;
  std::string src_netlist_dir_;
  std::vector<std::string> qor_task_names_;
  std::vector<TapeoutManager::e_qorso_task_type> qor_task_types_;
  std::vector<std::string> qor_task_config_;

  std::string src_tile_ref_fpath_;
  std::string src_fabric_hierarchy_fpath_;
  std::string timing_report_dirpath_;
  std::string ppa_summary_dirpath_;
  std::string sdc_file_base_dirpath_;
  std::string sdc_file_value_dirpath_;
  std::string preconifgured_sim_waveform_dirpath_;

  /* Global signal settings */
  float gclk_clock_freq_;
  openfpga::BasicPort gclk_port_;
  openfpga::BasicPort grst_port_;
  std::map<size_t, std::string> gclk_sdc_file_value_paths_;
  std::map<size_t, std::string> grst_sdc_file_value_paths_;
  std::string GCLK_SWEEP_VAR_NAME_;
  std::string GRST_SWEEP_VAR_NAME_;
  std::string GCLK_BITSTREAM_BASE_FILENAME_;
  std::string GCLK_BITSTREAM_VALUE_FILENAME_PREFIX_;
  std::string GRST_BITSTREAM_VALUE_FILENAME_PREFIX_;
  std::string GCLK_BITSTREAM_VALUE_FILENAME_POSTFIX_;
  std::string GRST_BITSTREAM_VALUE_FILENAME_POSTFIX_;

  /* Corner map: name -> [0 -> pvt; 1 -> spef; 2 -> ocv] */
  std::string corner_tag_;
  std::array<std::string, 3> corner_map_;

  /* Constants */
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> FILE_NAMES_;
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> FILE_TAGS_;

  /* Directory names */
  std::string QORSO_DIR_NAME_;
  std::string QORSO_CONFIG_DIR_NAME_;
  std::string QORSO_SDC_DIR_NAME_;
  std::string QORSO_CUSTOM_SDC_DIR_NAME_;
  std::string QORSO_WAVEFORM_DIR_NAME_;
  std::string QORSO_DYNAMIC_POWER_TASK_NAME_;
  std::string QORSO_STA_TASK_NAME_;
  std::string QORSO_CONFIG_FILENAME_;
  std::string PDK_DIR_NAME_;
  std::string NETLIST_DIR_NAME_;
  std::string FABRIC_NETLIST_FILENAME_;
  std::string GCLK_CUSTOM_SDC_FILENAME_;

  std::string PYTHON_COMMAND_;

  /* Local cache */
  const PdkManager* pdk_mgr_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress
