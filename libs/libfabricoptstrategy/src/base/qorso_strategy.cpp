#include "qorso_strategy.h"

#include <filesystem>
#include <regex>

#include "arkangel_report_util.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "verilog_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

QorsoStrategy::QorsoStrategy(const std::string& name, const std::string& root_path,
                             const std::string& tech_name,
                             const std::array<std::string, 4>& corner_tags) {
  name_ = name;
  root_path_ = root_path;
  tech_name_ = tech_name;
  corner_tag_ = corner_tags[0];
  pvt_corner_ = corner_tags[1];
  rc_corner_ = corner_tags[2];
  ocv_corner_ = corner_tags[3];
  is_valid_strategies_ = false;
  is_valid_gsettings_ = false;

  YAML_DEVICE_NODE_ = "device";
  YAML_DEVICE_NAME_ = "name";
  YAML_DEVICE_VERSION_ = "version";
  YAML_TECH_NODE_ = "technology";
  YAML_TECH_ROOT_ = "root";
  YAML_TECH_CORNER_ = "corner";
  YAML_TECH_CORNER_PVT_ = "pvt";
  YAML_TECH_CORNER_RC_ = "rc";
  YAML_TECH_CORNER_OCV_ = "ocv";
  YAML_NETLIST_NODE_ = "netlist";
  YAML_NETLIST_ROOT_ = "root";
  YAML_NETLIST_EXCLUDE_ = "exclude";
  YAML_NETLIST_TYPE_ = "type";
  YAML_NETLIST_OCV_ = "on_chip_variation";
  YAML_SPEF_NODE_ = "rc_parasitics";
  YAML_SPEF_ROOT_ = "root";
  YAML_SPEF_INCLUDE_ = "include";

  YAML_POWER_TASK_NODE_ = "report_power";
  YAML_POWER_TASK_NAME_ = "name";
  YAML_POWER_TASK_DESIGN_ = "design";
  YAML_POWER_TASK_METHOD_ = "method";
  YAML_POWER_TASK_VARIABLES_ = "variables";
  YAML_POWER_TASK_APPVARS_ = "appvars";
  YAML_POWER_TASK_SDC_ = "sdc";
  YAML_POWER_TASK_REPORTS_ = "reports";
  YAML_POWER_TASK_REPORT_NAME_ = "name";
  YAML_POWER_TASK_REPORT_PATH_ = "path";
  YAML_POWER_TASK_REPORT_COMMAND_ = "command";
  YAML_POWER_TASK_REPORT_REMOVE_TIME_STAMP_ = "remove_time_stamp";
  /*for benchmark dynamic power*/
  YAML_POWER_TASK_BENCHMARK_ = "benchmark";
  YAML_POWER_TASK_BITSTREAM_ = "bitstream";
  YAML_POWER_TASK_BITSTREAM_BASE_ = "base";
  YAML_POWER_TASK_BITSTREAM_VALUE_ = "value";
  YAML_POWER_TASK_ACTIVITY_ = "activity";
  YAML_POWER_TASK_ACTIVITY_TYPE_ = "type";
  YAML_POWER_TASK_ACTIVITY_FILE_ = "file";
  YAML_POWER_TASK_ACTIVITY_SRIP_PATH_ = "strip_path";

  YAML_POWER_SUMMARY_NODE_ = "power_summary";
  YAML_POWER_SUMMARY_NAME_ = "name";
  YAML_POWER_SUMMARY_HISTO_ = "histogram";
  YAML_POWER_SUMMARY_DISTR_ = "distribution";
  YAML_POWER_SUMMARY_CSV_ = "csv";
  YAML_POWER_SUMMARY_FILE_ = "file";
  YAML_POWER_SUMMARY_CSV_NAME_ = "name";
  YAML_POWER_SUMMARY_DATA_TYPE_ = "data_type";
  YAML_POWER_SUMMARY_TASKS_ = "power_tasks";
  YAML_POWER_SUMMARY_HIERARCHY_LEVEL_ = "hierarchy_level";

  YAML_TIMING_TASK_NODE_ = "report_timing";
  YAML_TIMING_TASK_NAME_ = "name";
  YAML_TIMING_TASK_DESIGN_ = "current_design";
  YAML_TIMING_TASK_REPORT_ = "report_file";
  YAML_TIMING_TASK_PATH_ = "path";
  YAML_TIMING_TASK_FROM_PIN_ = "from_pin";
  YAML_TIMING_TASK_TO_PIN_ = "to_pin";
  YAML_TIMING_TASK_THRU_PIN_ = "through_pin";
  YAML_TIMING_TASK_MAX_DELAY_ = "max_delay";
  YAML_TIMING_TASK_MIN_DELAY_ = "min_delay";
  YAML_TIMING_TASK_DELAY_TYPE_ = "delay_type";
  YAML_TIMING_TASK_CLOCK_GROUP_ = "clock_group";
  YAML_TIMING_TASK_NWORST_ = "nworst";
  YAML_TIMING_TASK_MAX_PATHS_ = "max_paths";
  YAML_TIMING_TASK_SDC_ = "sdc";

  /*for benchmark timing*/
  YAML_BENCHMARK_TIMING_TASK_NODE_ = "report_benchmark_timing";
  YAML_TIMING_TASK_REPORT_NAME_ = "name";
  YAML_TIMING_TASK_BENCHMARK_DESIGN_ = "design";
  YAML_TIMING_TASK_BENCHMARK_ = "benchmark";
  YAML_TIMING_TASK_BITSTREAM_ = "bitstream";
  YAML_TIMING_TASK_BITSTREAM_BASE_ = "base";
  YAML_TIMING_TASK_BITSTREAM_VALUE_ = "value";
  YAML_TIMING_TASK_REPORT_PATH_ = "path";
  YAML_TIMING_TASK_REPORT_COMMAND_ = "command";
  YAML_TIMING_TASK_REPORT_REMOVE_TIME_STAMP_ = "remove_time_stamp";
  YAML_TIMING_TASK_REPORTS_ = "reports";
  YAML_BENCHMARK_TIMING_SUMMARY_TASKS_ = "benchmark_timing_tasks";

  YAML_TIMING_SUMMARY_DISTR_BARCHART_ = "barchart";
  YAML_TIMING_SUMMARY_REPORT_PARSER_NODE_ = "report_parser";
  YAML_TIMING_SUMMARY_REPORT_PARSER_ALLOW_UNCONSTRAINED_PATH_ = "allow_unconstrained_path";

  YAML_TIMING_SUMMARY_NODE_ = "timing_summary";
  YAML_TIMING_SUMMARY_NAME_ = "name";
  YAML_TIMING_SUMMARY_DISTR_NODE_ = "distribution";
  YAML_TIMING_SUMMARY_DISTR_TABLE_ = "table";
  YAML_TIMING_SUMMARY_DISTR_HISTO_ = "histogram";
  YAML_TIMING_SUMMARY_YAML_NODE_ = "yaml";
  YAML_TIMING_SUMMARY_YAML_NAME_ = "name";
  YAML_TIMING_SUMMARY_YAML_APPEND_ = "append";
  YAML_TIMING_SUMMARY_YAML_VAR_ = "variable";
  YAML_TIMING_SUMMARY_YAML_DATA_ = "timing_data";
  YAML_TIMING_SUMMARY_YAML_DATA_TYPE_ = "data_type";
  YAML_TIMING_SUMMARY_YAML_SCALE_FACTOR_ = "scale_factor";
  YAML_TIMING_SUMMARY_CSV_NODE_ = "csv";
  YAML_TIMING_SUMMARY_CSV_NAME_ = "name";
  YAML_TIMING_SUMMARY_CSV_APPEND_ = "append";
  YAML_TIMING_SUMMARY_CSV_VAR_ = "variable";
  YAML_TIMING_SUMMARY_CSV_DATA_ = "timing_data";
  YAML_TIMING_SUMMARY_TASKS_ = "timing_tasks";

  YAML_TOTAL_POWER_VALUE_ = "total power";

  TIMING_SUMMARY_FILENAME_PREFIX_ = "timing_";

  KW_PWD_ = "${PWD}";
  KW_ROOT_ = "${ROOT_PATH}";
  KW_PROJHOME_ = "${PROJ_PATH}";
  TASK_KW_CURRENT_DESIGN_ = "[current_design]";
  TASK_KW_CURRENT_BENCHMARK_ = "[current_benchmark]";
  reserved_keywords_[KW_PWD_] = std::filesystem::current_path().string();
  reserved_keywords_[KW_ROOT_] = root_path_;
  reserved_keywords_[KW_PROJHOME_] = std::string();
  NETLIST_TYPE_STRING_ = {"rtl", "gl", "pl"};
  STRATEGY_TYPE_STRING_ = {"timing", "power", "timing_gclk", "timing_grst"};
}

void QorsoStrategy::check_valid() const {
  check_valid_strategies();
  check_valid_general_settings();
}

void QorsoStrategy::check_valid_strategies() const {
  if (!is_valid_strategies_) {
    VTR_LOG_ERROR("Qorso strategy data is invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void QorsoStrategy::check_valid_general_settings() const {
  if (!is_valid_gsettings_) {
    VTR_LOG_ERROR("Qorso strategy general settings are invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

int QorsoStrategy::scale_factors(const std::string& general_task_name, TablePrinter& tp,
                                 const std::string& corner, const int& precision) const {
  for (const auto& [task_name, task_id] : name2ids_) {
    auto subtasks = sub_tasks_[task_id];
    std::vector<std::string> subtask_names;
    for (const auto& [key, value] : subtasks) {
      subtask_names.push_back(key);
    }
    for (auto i : subtask_names) {
      tp.add_row({general_task_name, i,
                  to_string_with_precision(scale_factors_[task_id], precision), corner});
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

void QorsoStrategy::clear_general_settings() {
  root_path_.clear();
  tech_name_.clear();
  pvt_corner_.clear();
  rc_corner_.clear();
  ocv_corner_.clear();
  netlist_input_.clear();
  netlist_exclude_.clear();
  netlist_type_ = e_netlist_type::NUM_TYPES;
  spef_root_.clear();
  gvars_.clear();
  task_rpt_dirpath_.clear();
  summary_rpt_dirpath_.clear();
  is_valid_gsettings_ = false;
}

void QorsoStrategy::clear_strategies() {
  name2ids_.clear();
  strategy_types_.clear();
  summary_data_.clear();
  scale_factors_.clear();
  summary_data_types_.clear();
  arch_variables_.clear();
  timing_tasks_.clear();
  power_tasks_.clear();
  is_valid_strategies_ = false;
}

void QorsoStrategy::clear() {
  clear_strategies();
  clear_general_settings();
}

int QorsoStrategy::add_strategy(const std::string& type, const std::string& val) {
  /* Check if there is an collision in names */
  auto result = name2ids_.find(val);
  if (result != name2ids_.end()) {
    VTR_LOG_ERROR("Duplicated name '%s' when qorso strategy!\n", val.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t new_id = strategy_types_.size();
  name2ids_[val] = new_id;
  strategy_types_.push_back(str2strategy_type(type, true));
  summary_data_.emplace_back();
  scale_factors_.push_back(1.0);
  summary_data_types_.emplace_back();
  arch_variables_.emplace_back();
  sub_tasks_.emplace_back();
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_summary_data(const std::string& name, const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  summary_data_[name2ids_.at(name)] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_summary_data_type(const std::string& name, const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  summary_data_types_[name2ids_.at(name)] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_summary_data_scale_factor(const std::string& name, const float& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  scale_factors_[name2ids_.at(name)] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_arch_variable_pair(const std::string& name, const std::string& arch_name,
                                          const std::string& disp_name) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  auto result = arch_variables_[name2ids_.at(name)].find(arch_name);
  if (result == arch_variables_[name2ids_.at(name)].end()) {
    arch_variables_[name2ids_.at(name)][arch_name] = disp_name;
    return openfpga::CMD_EXEC_SUCCESS;
  }
  /* Do not allow overwrite, error out */
  VTR_LOG_ERROR("Intend to redefine the existing variable '%s' which is not allowed!\n",
                arch_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::add_timing_task(const std::string& strtg_name, const std::string& task_name) {
  if (!valid_strategy(strtg_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  auto result = sub_tasks_[name2ids_.at(strtg_name)].find(task_name);
  if (result != sub_tasks_[name2ids_.at(strtg_name)].end()) {
    VTR_LOG_ERROR("Duplicated timing task name '%s' when adding it to the qorso strategy '%s'!\n",
                  task_name.c_str(), strtg_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Add a new task */
  size_t new_id = timing_tasks_.size();
  sub_tasks_[name2ids_.at(strtg_name)][task_name] = new_id;
  timing_tasks_.emplace_back();
  timing_tasks_[new_id].name = task_name;
  /* For Backtrace: fast lookup */
  timing_task_parent_strategy_.emplace_back(strtg_name);
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_timing_task_design_auto_infer_from_tile_type(const std::string& strtg_name,
                                                                    const std::string& task_name,
                                                                    const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].design_infer_types = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_timing_task_sdc_file(const std::string& strtg_name,
                                            const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  /* Avoid duplicated SDC */
  auto result = std::find(timing_tasks_[timing_task_id].sdc_files.begin(),
                          timing_tasks_[timing_task_id].sdc_files.end(), val);
  if (result == timing_tasks_[timing_task_id].sdc_files.end()) {
    timing_tasks_[timing_task_id].sdc_files.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR("Duplicated SDC file '%s' for timing task '%s' under strategy '%s'!\n", val.c_str(),
                task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::set_timing_task_bitstream_base_sdc_file(const std::string& strtg_name,
                                                           const std::string& task_name,
                                                           const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].bitstream_base_sdc_file = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_timing_task_bitstream_value_sdc_file(const std::string& strtg_name,
                                                            const std::string& task_name,
                                                            const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].bitstream_value_sdc_file = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_timing_task_design(const std::string& strtg_name,
                                          const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  auto result = std::find(timing_tasks_[timing_task_id].designs.begin(),
                          timing_tasks_[timing_task_id].designs.end(), val);
  if (result == timing_tasks_[timing_task_id].designs.end()) {
    timing_tasks_[timing_task_id].designs.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR("Duplicated design '%s' for timing task '%s' under strategy '%s'!\n", val.c_str(),
                task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::set_timing_task_max_delay(const std::string& strtg_name,
                                             const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].max_delay = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_timing_task_min_delay(const std::string& strtg_name,
                                             const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].min_delay = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_timing_task_delay_type(const std::string& strtg_name,
                                              const std::string& task_name,
                                              const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].delay_type = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_timing_task_clock_group(const std::string& strtg_name,
                                               const std::string& task_name,
                                               const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].clock_group = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_timing_task_nworst(const std::string& strtg_name,
                                          const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].nworst = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_timing_task_max_paths(const std::string& strtg_name,
                                             const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  timing_tasks_[timing_task_id].max_paths = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_timing_task_from_pin(const std::string& strtg_name,
                                            const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  auto result = std::find(timing_tasks_[timing_task_id].from_pins.begin(),
                          timing_tasks_[timing_task_id].from_pins.end(), val);
  if (result == timing_tasks_[timing_task_id].from_pins.end()) {
    timing_tasks_[timing_task_id].from_pins.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR("Duplicated from_pin '%s' for timing task '%s' under strategy '%s'!\n", val.c_str(),
                task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::add_timing_task_to_pin(const std::string& strtg_name,
                                          const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  auto result = std::find(timing_tasks_[timing_task_id].to_pins.begin(),
                          timing_tasks_[timing_task_id].to_pins.end(), val);
  if (result == timing_tasks_[timing_task_id].to_pins.end()) {
    timing_tasks_[timing_task_id].to_pins.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR("Duplicated to_pin '%s' for timing task '%s' under strategy '%s'!\n", val.c_str(),
                task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::add_timing_task_thru_pin(const std::string& strtg_name,
                                            const std::string& task_name, const std::string& val) {
  if (!valid_timing_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t timing_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  auto result = std::find(timing_tasks_[timing_task_id].thru_pins.begin(),
                          timing_tasks_[timing_task_id].thru_pins.end(), val);
  if (result == timing_tasks_[timing_task_id].thru_pins.end()) {
    timing_tasks_[timing_task_id].thru_pins.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR("Duplicated through_pin '%s' for timing task '%s' under strategy '%s'!\n",
                val.c_str(), task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::add_power_task(const std::string& strtg_name, const std::string& task_name) {
  if (!valid_strategy(strtg_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  auto result = sub_tasks_[name2ids_.at(strtg_name)].find(task_name);
  if (result != sub_tasks_[name2ids_.at(strtg_name)].end()) {
    VTR_LOG_ERROR("Duplicated power task name '%s' when adding it to the qorso strategy '%s'!\n",
                  task_name.c_str(), strtg_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Add a new task */
  size_t new_id = power_tasks_.size();
  sub_tasks_[name2ids_.at(strtg_name)][task_name] = new_id;
  power_tasks_.emplace_back();
  power_tasks_[new_id].name = task_name;
  /* For Backtrace: fast lookup */
  power_task_parent_strategy_.emplace_back(strtg_name);
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_power_task_design(const std::string& strtg_name,
                                         const std::string& task_name, const std::string& val) {
  if (!valid_power_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t power_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  auto result = std::find(power_tasks_[power_task_id].designs.begin(),
                          power_tasks_[power_task_id].designs.end(), val);
  if (result == power_tasks_[power_task_id].designs.end()) {
    power_tasks_[power_task_id].designs.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR("Duplicated design '%s' for power task '%s' under strategy '%s'!\n", val.c_str(),
                task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::set_power_task_method(const std::string& strtg_name,
                                         const std::string& task_name, const std::string& val) {
  if (!valid_power_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t power_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  power_tasks_[power_task_id].method = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_power_task_app_variable(const std::string& strtg_name,
                                               const std::string& task_name,
                                               const std::string& var_name,
                                               const std::string& var_val) {
  if (!valid_power_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t power_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  auto result = power_tasks_[power_task_id].appvars.find(var_name);
  if (result == power_tasks_[power_task_id].appvars.end()) {
    power_tasks_[power_task_id].appvars[var_name] = var_val;
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR(
    "Duplicated definition on app variable '%s' for power task '%s' under strategy '%s'!\n",
    var_name.c_str(), task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::add_power_task_sdc_variable(const std::string& strtg_name,
                                               const std::string& task_name,
                                               const std::string& var_name,
                                               const std::string& var_val) {
  if (!valid_power_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t power_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  auto result = power_tasks_[power_task_id].sdc_vars.find(var_name);
  if (result == power_tasks_[power_task_id].sdc_vars.end()) {
    power_tasks_[power_task_id].sdc_vars[var_name] = var_val;
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR(
    "Duplicated definition on SDC variable '%s' for power task '%s' under strategy '%s'!\n",
    var_name.c_str(), task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::add_power_task_sdc_file(const std::string& strtg_name,
                                           const std::string& task_name, const std::string& val) {
  if (!valid_power_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t power_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  /* Avoid duplicated SDC */
  auto result = std::find(power_tasks_[power_task_id].sdc_files.begin(),
                          power_tasks_[power_task_id].sdc_files.end(), val);
  if (result == power_tasks_[power_task_id].sdc_files.end()) {
    power_tasks_[power_task_id].sdc_files.push_back(val);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR("Duplicated SDC file '%s' for power task '%s' under strategy '%s'!\n", val.c_str(),
                task_name.c_str(), strtg_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int QorsoStrategy::set_power_task_report_command(const std::string& strtg_name,
                                                 const std::string& task_name,
                                                 const std::string& val) {
  if (!valid_power_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t power_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  power_tasks_[power_task_id].report_command = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::set_power_task_remove_time_stamp(const std::string& strtg_name,
                                                    const std::string& task_name, const bool& val) {
  if (!valid_power_task(strtg_name, task_name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t power_task_id = sub_tasks_[name2ids_.at(strtg_name)].at(task_name);
  power_tasks_[power_task_id].report_no_time_stamp = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_strategy_sweep_variable(const std::string& var_name, const int& start,
                                               const int& end, const int& step) {
  auto result = gvars_.find(var_name);
  if (result != gvars_.end()) {
    VTR_LOG_ERROR("Intend to redefine the strategy variable '%s', which is not allowed!\n",
                  var_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* TODO: Sanity checks on the sweep settings, if start < end, step > 0; if start > end, step < 0 ?
   * step cannot be zero */
  // Normalize: if start == end, treat as constant and set step to 0
  gvars_[var_name][0] = start;
  gvars_[var_name][1] = end;
  if (start == end) {
    gvars_[var_name][2] = 0;
  } else {
    gvars_[var_name][2] = step;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_strategy_constant_variable(const std::string& var_name, const int& val) {
  auto result = gvars_.find(var_name);
  if (result != gvars_.end()) {
    VTR_LOG_ERROR("Intend to redefine the strategy variable '%s', which is not allowed!\n",
                  var_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  gvars_[var_name][0] = val;
  gvars_[var_name][1] = val;
  gvars_[var_name][2] = 0;
  return openfpga::CMD_EXEC_SUCCESS;
}

void QorsoStrategy::set_netlist_input(const std::string& val) { netlist_input_ = val; }
void QorsoStrategy::set_task_report_dirpath(const std::string& val) { task_rpt_dirpath_ = val; }
void QorsoStrategy::set_summary_report_dirpath(const std::string& val) {
  summary_rpt_dirpath_ = val;
}

void QorsoStrategy::set_spef_root(const std::string& val) { spef_root_ = val; }

int QorsoStrategy::set_netlist_type(const std::string& val) {
  e_netlist_type nlist_type = str2netlist_type(val, true);
  if (!valid_netlist_type(nlist_type)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  netlist_type_ = nlist_type;
  return openfpga::CMD_EXEC_SUCCESS;
}

void QorsoStrategy::add_netlist_exclude(const std::string& val) { netlist_exclude_.push_back(val); }

std::string QorsoStrategy::replace_reserved_keywords(const std::string& val) const {
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

bool QorsoStrategy::validate(const TileRef& tref, const bool& verbose) {
  VTR_LOGV(verbose, "Validating general settings...\n");
  if (!validate_general_settings()) {
    return false;
  }
  VTR_LOGV(verbose, "Done...\n");
  /* load tile name to tasks that require auto inference */
  if (!auto_infer_timing_task_designs(tref, verbose)) {
    return false;
  }
  if (!auto_infer_power_task_designs(tref, verbose)) {
    return false;
  }
  VTR_LOGV(verbose, "Validating detailed strategies...\n");
  if (!validate_strategies()) {
    return false;
  }
  VTR_LOGV(verbose, "Done...\n");
  return true;
}

bool QorsoStrategy::validate_strategies() const {
  /* Each should have valid type, summary datas and at least 1 sub-task */
  for (auto pair : name2ids_) {
    size_t istrtg = pair.second;
    if (!valid_strategy_type(strategy_types_[istrtg])) {
      VTR_LOG_ERROR("Invalid type for QoR sign-off strategy '%s'!\n", pair.first.c_str());
      return false;
    }
    if (strategy_types_[istrtg] == e_strategy_type::TIMING ||
        strategy_types_[istrtg] == e_strategy_type::TIMING_GRST ||
        strategy_types_[istrtg] == e_strategy_type::TIMING_GCLK) {
      if (!valid_timing_summary_data(summary_data_[istrtg], true)) {
        VTR_LOG_ERROR("Invalid timing summary data for QoR sign-off strategy '%s'!\n",
                      pair.first.c_str());
        return false;
      }
      if (!valid_timing_summary_data_type(summary_data_types_[istrtg], true)) {
        VTR_LOG_ERROR("Invalid timing summary data type for QoR sign-off strategy '%s'!\n",
                      pair.first.c_str());
        return false;
      }
      if (arch_variables_[istrtg].empty()) {
        VTR_LOG_ERROR(
          "At least one YAML variable should be defined for QoR sign-off strategy '%s'!\n",
          pair.first.c_str());
        return false;
      }
      for (auto var_pair : arch_variables_[istrtg]) {
        if (var_pair.second.empty()) {
          VTR_LOG_ERROR(
            "YAML variable '%s' does not pair with any CSV variable, which is expected for QoR "
            "sign-off strategy '%s'!\n",
            var_pair.first.c_str(), pair.first.c_str());
          return false;
        }
      }
    }
    if (strategy_types_[istrtg] == e_strategy_type::POWER) {
      if (!valid_power_summary_data(summary_data_[istrtg], true)) {
        VTR_LOG_ERROR("Invalid power summary data for QoR sign-off strategy '%s'!\n",
                      pair.first.c_str());
        return false;
      }
    }
  }
  is_valid_strategies_ = true;
  return true;
}

bool QorsoStrategy::validate_general_settings() const {
  if (root_path_.empty()) {
    VTR_LOG_ERROR("Root path is not specified\n");
    return false;
  }
  if (tech_name_.empty()) {
    VTR_LOG_ERROR("Technology name is not specified\n");
    return false;
  }
  if (pvt_corner_.empty()) {
    VTR_LOG_ERROR("Technology Process-Voltage-Temperature corner is not specified\n");
    return false;
  }
  if (rc_corner_.empty()) {
    VTR_LOG_ERROR("Technology RC corner is not specified\n");
    return false;
  }
  if (ocv_corner_.empty()) {
    VTR_LOG_ERROR("Technology On-Chip-Variation corner is not specified\n");
    return false;
  }
  if (netlist_input_.empty()) {
    VTR_LOG_ERROR("Input netlist path is not specified\n");
    return false;
  }
  if (task_rpt_dirpath_.empty()) {
    VTR_LOG_ERROR("The report directory path for QoR tasks is not specified\n");
    return false;
  }
  if (summary_rpt_dirpath_.empty()) {
    VTR_LOG_ERROR("The summary directory path for QoR tasks is not specified\n");
    return false;
  }
  if (!valid_netlist_type(netlist_type_)) {
    VTR_LOG_ERROR("Invalid netlist type\n");
    return false;
  }
  if (netlist_type_ == e_netlist_type::PL && spef_root_.empty()) {
    VTR_LOG_ERROR("For post-layout QoR sign-off, the root directory of SPEF should be provided\n");
    return false;
  }
  /* Sanity checks on the sweep settings, if start < end, step > 0; if start > end, step < 0 ? step
   * cannot be zero */
  for (auto pair : gvars_) {
    if (pair.second[0] == pair.second[1] && pair.second[2] != 0) {
      VTR_LOG_ERROR(
        "Strategy variable '%s' is defined as a contant '%d' but it has a non-zero step '%d'!\n",
        pair.first.c_str(), pair.second[0], pair.second[2]);
      return false;
    }
    if (pair.second[0] < pair.second[1] && pair.second[2] < 0) {
      VTR_LOG_ERROR(
        "Strategy variable '%s' is defined as a sweep from '%d' to '%d' but it has a negative step "
        "'%d'! This will cause an infinite loop\n",
        pair.first.c_str(), pair.second[0], pair.second[1], pair.second[2]);
      return false;
    }
    if (pair.second[0] > pair.second[1] && pair.second[2] > 0) {
      VTR_LOG_ERROR(
        "Strategy variable '%s' is defined as a sweep from '%d' downto '%d' but it has a positive "
        "step '%d'! This will cause an infinite loop\n",
        pair.first.c_str(), pair.second[0], pair.second[1], pair.second[2]);
      return false;
    }
  }
  is_valid_gsettings_ = true;
  return true;
}

QorsoStrategy::e_netlist_type QorsoStrategy::str2netlist_type(const std::string& str,
                                                              const bool& verbose) const {
  for (int itype = size_t(QorsoStrategy::e_netlist_type::RTL);
       itype != size_t(QorsoStrategy::e_netlist_type::NUM_TYPES); ++itype) {
    if (str == std::string(NETLIST_TYPE_STRING_[itype])) {
      return static_cast<QorsoStrategy::e_netlist_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of netlist! Expect %s\n", netlist_type_all2str().c_str());
  return QorsoStrategy::e_netlist_type::NUM_TYPES;
}

std::string QorsoStrategy::netlist_type2str(const QorsoStrategy::e_netlist_type& type,
                                            const bool& verbose) const {
  if (!valid_netlist_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of netlist! Expect %s\n", netlist_type_all2str().c_str());
    return std::string();
  }
  return std::string(NETLIST_TYPE_STRING_[size_t(type)]);
}

std::string QorsoStrategy::netlist_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(QorsoStrategy::e_netlist_type::RTL);
       itype != size_t(QorsoStrategy::e_netlist_type::NUM_TYPES); ++itype) {
    full_types += std::string(NETLIST_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool QorsoStrategy::valid_netlist_type(const QorsoStrategy::e_netlist_type& netlist_type) const {
  return netlist_type != QorsoStrategy::e_netlist_type::NUM_TYPES;
}

QorsoStrategy::e_strategy_type QorsoStrategy::str2strategy_type(const std::string& str,
                                                                const bool& verbose) const {
  for (int itype = size_t(QorsoStrategy::e_strategy_type::TIMING);
       itype < size_t(QorsoStrategy::e_strategy_type::NUM_TYPES); ++itype) {
    if (str == std::string(STRATEGY_TYPE_STRING_[itype])) {
      return static_cast<QorsoStrategy::e_strategy_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of Qor Sign-off strategy! Expect %s\n",
                 strategy_type_all2str().c_str());
  return QorsoStrategy::e_strategy_type::NUM_TYPES;
}

std::string QorsoStrategy::strategy_type2str(const QorsoStrategy::e_strategy_type& type,
                                             const bool& verbose) const {
  if (!valid_strategy_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of Qor Sign-off strategy! Expect %s\n",
                   strategy_type_all2str().c_str());
    return std::string();
  }
  return std::string(STRATEGY_TYPE_STRING_[size_t(type)]);
}

std::string QorsoStrategy::strategy_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(QorsoStrategy::e_strategy_type::TIMING);
       itype != size_t(QorsoStrategy::e_strategy_type::NUM_TYPES); ++itype) {
    full_types += std::string(STRATEGY_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool QorsoStrategy::valid_strategy_type(const QorsoStrategy::e_strategy_type& strategy_type) const {
  return strategy_type != QorsoStrategy::e_strategy_type::NUM_TYPES;
}

bool QorsoStrategy::valid_timing_summary_data(const std::string& val, const bool& verbose) const {
  if (val != std::string("arrival time") && val != std::string("required time") &&
      val != std::string("slack")) {
    VTR_LOGV_ERROR(
      verbose, "Invalid timing summary data '%s', expect [arrival time | required time | slack]!\n",
      val.c_str());
    return false;
  }
  return true;
}

bool QorsoStrategy::valid_timing_summary_data_type(const std::string& val,
                                                   const bool& verbose) const {
  if (val != std::string("Max") && val != std::string("Min") && val != std::string("Average")) {
    VTR_LOGV_ERROR(verbose,
                   "Invalid type of timing summary data '%s', expect [Max | Min | Average]!\n",
                   val.c_str());
    return false;
  }
  return true;
}

bool QorsoStrategy::valid_power_summary_data(const std::string& val, const bool& verbose) const {
  if (val != std::string("leak power") && val != std::string("switch power") &&
      val != std::string("int power") && val != std::string("total power")) {
    VTR_LOGV_ERROR(verbose,
                   "Invalid type of power summary data '%s', expect [leak power | switch power | "
                   "int power | total power]!\n",
                   val.c_str());
    return false;
  }
  return true;
}

bool QorsoStrategy::valid_strategy(const std::string& name) const {
  auto result = name2ids_.find(name);
  return result != name2ids_.end();
}

bool QorsoStrategy::valid_timing_task(const std::string& strtg_name,
                                      const std::string& task_name) const {
  if (!valid_strategy(strtg_name)) {
    return false;
  }
  auto result = sub_tasks_[name2ids_.at(strtg_name)].find(task_name);
  if (result == sub_tasks_[name2ids_.at(strtg_name)].end()) {
    return false;
  }
  return result->second < timing_tasks_.size();
}

bool QorsoStrategy::valid_power_task(const std::string& strtg_name,
                                     const std::string& task_name) const {
  if (!valid_strategy(strtg_name)) {
    return false;
  }
  auto result = sub_tasks_[name2ids_.at(strtg_name)].find(task_name);
  if (result == sub_tasks_[name2ids_.at(strtg_name)].end()) {
    return false;
  }
  return result->second < power_tasks_.size();
}

int QorsoStrategy::add_yaml_tech_info(YAML::Node& config) const {
  int num_err = 0;
  config[YAML_DEVICE_NODE_][YAML_DEVICE_NAME_] = "alkaid";
  config[YAML_DEVICE_NODE_][YAML_DEVICE_VERSION_] = "s40ulp";
  /* Dummy string. It will be ignored whatever */
  config[YAML_TECH_NODE_][YAML_TECH_ROOT_] = "${PDK_ROOT}";
  config[YAML_TECH_NODE_][YAML_TECH_CORNER_][YAML_TECH_CORNER_PVT_] = pvt_corner_;
  config[YAML_TECH_NODE_][YAML_TECH_CORNER_][YAML_TECH_CORNER_RC_] = rc_corner_;
  config[YAML_TECH_NODE_][YAML_TECH_CORNER_][YAML_TECH_CORNER_OCV_] = ocv_corner_;

  return num_err;
}

int QorsoStrategy::add_yaml_netlist_info(YAML::Node& config) const {
  int num_err = 0;
  config[YAML_NETLIST_NODE_][YAML_NETLIST_ROOT_] = netlist_input_;
  for (std::string excl_nlist : netlist_exclude_) {
    config[YAML_NETLIST_NODE_][YAML_NETLIST_EXCLUDE_].push_back(excl_nlist);
  }
  config[YAML_NETLIST_NODE_][YAML_NETLIST_TYPE_] = netlist_type2str(netlist_type_);
  /* TODO: Make it switchable */
  config[YAML_NETLIST_NODE_][YAML_NETLIST_OCV_] = "false";

  return num_err;
}

int QorsoStrategy::add_yaml_spef_info(YAML::Node& config) const {
  int num_err = 0;
  std::string actual_spef_root = "./spef";
  if (!spef_root_.empty()) {
    actual_spef_root = spef_root_;
  }
  config[YAML_SPEF_NODE_][YAML_SPEF_ROOT_] = actual_spef_root;
  /* For gate level netlist, no spef is allowed */
  std::string spef_incl = "false";
  if (netlist_type_ == e_netlist_type::PL && !spef_root_.empty()) {
    spef_incl = "true";
  }
  config[YAML_SPEF_NODE_][YAML_SPEF_INCLUDE_] = spef_incl;

  return num_err;
}

std::string QorsoStrategy::report_timing_task_name(
  const size_t& task_id, const std::map<std::string, size_t>& local_gvars) const {
  std::string task_name =
    timing_task_parent_strategy_[task_id] + std::string("_") + timing_tasks_[task_id].name;
  if (!local_gvars.empty()) {
    std::string task_name_postfix;
    for (auto pair : local_gvars) {
      task_name_postfix += pair.first + std::string("_sweep_") + std::to_string(pair.second);
    }
    task_name += task_name_postfix;
  }
  return task_name;
}

std::string QorsoStrategy::report_power_task_name(const size_t& task_id) const {
  std::string task_name =
    power_task_parent_strategy_[task_id] + std::string("_") + power_tasks_[task_id].name;
  return task_name;
}

std::string QorsoStrategy::report_timing_task_report_fpath(
  const size_t& task_id, const std::map<std::string, size_t>& local_gvars) const {
  std::string rpt_fname = timing_tasks_[task_id].name;
  if (local_gvars.empty()) {
    rpt_fname += std::string(".rpt");
  } else {
    for (auto pair : local_gvars) {
      rpt_fname += pair.first + std::string("_sweep_") + std::to_string(pair.second);
    }
    rpt_fname += std::string(".rpt");
  }
  std::filesystem::path yaml_fpath =
    std::filesystem::path(task_rpt_dirpath_) / std::filesystem::path(corner_tag_) /
    std::filesystem::path(TASK_KW_CURRENT_DESIGN_) / std::filesystem::path(rpt_fname);
  return yaml_fpath.string();
}

int QorsoStrategy::add_yaml_report_timing_task(
  YAML::Node& task_node, const size_t& task_id,
  const std::map<std::string, size_t>& local_gvars) const {
  int num_err = 0;
  task_node[YAML_TIMING_TASK_NAME_] = report_timing_task_name(task_id, local_gvars);
  for (std::string dsgn_name : timing_tasks_[task_id].designs) {
    task_node[YAML_TIMING_TASK_DESIGN_].push_back(dsgn_name);
  }
  task_node[YAML_TIMING_TASK_REPORT_] = report_timing_task_report_fpath(task_id, local_gvars);
  /* Path info. */
  for (std::string pin : timing_tasks_[task_id].from_pins) {
    std::string actual_pin = pin;
    for (auto pair : local_gvars) {
      actual_pin = replace_local_global_variables(actual_pin, pair.first, pair.second);
    }
    actual_pin = replace_constant_global_variables(actual_pin);
    task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_FROM_PIN_].push_back(actual_pin);
  }
  for (std::string pin : timing_tasks_[task_id].to_pins) {
    std::string actual_pin = pin;
    for (auto pair : local_gvars) {
      actual_pin = replace_local_global_variables(actual_pin, pair.first, pair.second);
    }
    actual_pin = replace_constant_global_variables(actual_pin);
    task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_TO_PIN_].push_back(actual_pin);
  }
  for (std::string pin : timing_tasks_[task_id].thru_pins) {
    std::string actual_pin = pin;
    actual_pin = replace_constant_global_variables(actual_pin);
    task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_THRU_PIN_].push_back(actual_pin);
  }
  task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_MAX_DELAY_] = timing_tasks_[task_id].max_delay;
  task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_MIN_DELAY_] = timing_tasks_[task_id].min_delay;
  task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_DELAY_TYPE_] =
    timing_tasks_[task_id].delay_type;
  task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_CLOCK_GROUP_] =
    timing_tasks_[task_id].clock_group;
  task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_NWORST_] = timing_tasks_[task_id].nworst;
  task_node[YAML_TIMING_TASK_PATH_][YAML_TIMING_TASK_MAX_PATHS_] = timing_tasks_[task_id].max_paths;
  for (std::string sdc : timing_tasks_[task_id].sdc_files) {
    task_node[YAML_TIMING_TASK_SDC_].push_back(replace_reserved_keywords(sdc));
  }
  if (!timing_tasks_[task_id].bitstream_base_sdc_file.empty()) {
    std::string actual_bit_fname = timing_tasks_[task_id].bitstream_base_sdc_file;
    for (auto pair : local_gvars) {
      actual_bit_fname = replace_local_global_variables(actual_bit_fname, pair.first, pair.second);
    }
    actual_bit_fname = replace_constant_global_variables(actual_bit_fname);
    task_node[YAML_TIMING_TASK_BITSTREAM_][YAML_TIMING_TASK_BITSTREAM_BASE_] =
      replace_reserved_keywords(actual_bit_fname);
  }
  if (!timing_tasks_[task_id].bitstream_value_sdc_file.empty()) {
    std::string actual_bit_fname = timing_tasks_[task_id].bitstream_value_sdc_file;
    for (auto pair : local_gvars) {
      actual_bit_fname = replace_local_global_variables(actual_bit_fname, pair.first, pair.second);
    }
    actual_bit_fname = replace_constant_global_variables(actual_bit_fname);
    task_node[YAML_TIMING_TASK_BITSTREAM_][YAML_TIMING_TASK_BITSTREAM_VALUE_] =
      replace_reserved_keywords(actual_bit_fname);
  }

  return num_err;
}

int QorsoStrategy::add_yaml_report_timing_summary(YAML::Node& smry_node, const std::string& strtg,
                                                  const bool& first_summary) const {
  int num_err = 0;
  smry_node[YAML_TIMING_SUMMARY_NAME_] = strtg;

  size_t s_idx = name2ids_.at(strtg);

  std::filesystem::path rpt_dirpath(summary_rpt_dirpath_);
  rpt_dirpath = rpt_dirpath / std::filesystem::path(corner_tag_);
  std::filesystem::path table_fpath =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_dist.csv"));
  smry_node[YAML_TIMING_SUMMARY_DISTR_NODE_][YAML_TIMING_SUMMARY_DISTR_TABLE_] =
    table_fpath.string();

  std::filesystem::path hist_fpath =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_dist.png"));
  smry_node[YAML_TIMING_SUMMARY_DISTR_NODE_][YAML_TIMING_SUMMARY_DISTR_HISTO_] =
    hist_fpath.string();

  std::filesystem::path yaml_fpath = rpt_dirpath / std::filesystem::path(timing_yaml_filename());
  smry_node[YAML_TIMING_SUMMARY_YAML_NODE_][YAML_TIMING_SUMMARY_YAML_NAME_] = yaml_fpath.string();

  smry_node[YAML_TIMING_SUMMARY_YAML_NODE_][YAML_TIMING_SUMMARY_YAML_APPEND_] =
    first_summary ? "false" : "true";

  for (auto pair : arch_variables_[s_idx]) {
    smry_node[YAML_TIMING_SUMMARY_YAML_NODE_][YAML_TIMING_SUMMARY_YAML_VAR_].push_back(pair.first);
  }

  smry_node[YAML_TIMING_SUMMARY_YAML_NODE_][YAML_TIMING_SUMMARY_YAML_DATA_] = summary_data_[s_idx];
  smry_node[YAML_TIMING_SUMMARY_YAML_NODE_][YAML_TIMING_SUMMARY_YAML_DATA_TYPE_] =
    summary_data_types_[s_idx];
  smry_node[YAML_TIMING_SUMMARY_YAML_NODE_][YAML_TIMING_SUMMARY_YAML_SCALE_FACTOR_] =
    scale_factors_[s_idx];

  std::filesystem::path csv_fpath = rpt_dirpath / std::filesystem::path(timing_csv_filename());
  smry_node[YAML_TIMING_SUMMARY_CSV_NODE_][YAML_TIMING_SUMMARY_CSV_NAME_] = csv_fpath.string();

  smry_node[YAML_TIMING_SUMMARY_CSV_NODE_][YAML_TIMING_SUMMARY_CSV_APPEND_] =
    first_summary ? "false" : "true";
  for (auto pair : arch_variables_[s_idx]) {
    smry_node[YAML_TIMING_SUMMARY_CSV_NODE_][YAML_TIMING_SUMMARY_CSV_VAR_].push_back(pair.second);
  }
  smry_node[YAML_TIMING_SUMMARY_CSV_NODE_][YAML_TIMING_SUMMARY_CSV_DATA_] = summary_data_[s_idx];

  for (auto pair : sub_tasks_[s_idx]) {
    std::vector<std::string> gvars_to_sweep = timing_task_require_sweep_gvar(pair.second);
    if (gvars_to_sweep.empty()) {
      std::map<std::string, size_t> local_gvar;
      smry_node[YAML_TIMING_SUMMARY_TASKS_].push_back(
        report_timing_task_name(pair.second, local_gvar));
    } else {
      if (gvars_to_sweep.size() > 1) {
        VTR_LOG_ERROR(
          "Only support 1 global variable to be swept in timing tasks. But in the timing task "
          "'%s', there are %lu:\n",
          timing_tasks_[pair.second].name.c_str(), gvars_to_sweep.size());
        for (std::string curr_gvar : gvars_to_sweep) {
          VTR_LOG("\t%s\n", curr_gvar.c_str());
        }
        num_err++;
        continue;
      }
      for (std::string curr_gvar : gvars_to_sweep) {
        for (int igvar = gvars_.at(curr_gvar)[0]; igvar <= gvars_.at(curr_gvar)[1];
             igvar = igvar + gvars_.at(curr_gvar)[2]) {
          std::map<std::string, size_t> local_gvar;
          local_gvar[curr_gvar] = igvar;
          smry_node[YAML_TIMING_SUMMARY_TASKS_].push_back(
            report_timing_task_name(pair.second, local_gvar));
        }
      }
    }
  }
  return num_err;
}

int QorsoStrategy::write_yaml_qorso_gclk_timing_task(const std::string& fname,
                                                     const bool& verbose) const {
  check_valid();
  int num_err = 0;

  /* Generate timing sdc file */

  YAML::Node config;
  /* Add tech part */
  num_err += add_yaml_tech_info(config);
  /* Add netlist part */
  num_err += add_yaml_netlist_info(config);
  /* Add spef part */
  num_err += add_yaml_spef_info(config);
  /* Add timing tasks one by one */
  for (size_t t_task_idx = 0; t_task_idx < timing_tasks_.size(); ++t_task_idx) {
    /* Consider global variables to be swept */
    std::vector<std::string> gvars_to_sweep = timing_task_require_sweep_gvar(t_task_idx);
    if (gvars_to_sweep.empty()) {
      YAML::Node t_task_node;
      std::map<std::string, size_t> local_gvar;
      num_err += add_yaml_report_timing_task(t_task_node, t_task_idx, local_gvar);
      config[YAML_TIMING_TASK_NODE_].push_back(t_task_node);
    } else {
      if (gvars_to_sweep.size() > 1) {
        VTR_LOG_ERROR(
          "Only support 1 global variable to be swept in timing tasks. But in the timing task "
          "'%s', there are %lu:\n",
          timing_tasks_[t_task_idx].name.c_str(), gvars_to_sweep.size());
        for (std::string curr_gvar : gvars_to_sweep) {
          VTR_LOG("\t%s\n", curr_gvar.c_str());
        }
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      /* FIXME: support only 1 loop of sweep here. For the sake of computing complexity !!! We can
       * extend to more loops but it may be rarely used. */
      for (std::string curr_gvar : gvars_to_sweep) {
        for (int igvar = gvars_.at(curr_gvar)[0]; igvar <= gvars_.at(curr_gvar)[1];
             igvar = igvar + gvars_.at(curr_gvar)[2]) {
          YAML::Node t_task_node;
          std::map<std::string, size_t> local_gvar;
          local_gvar[curr_gvar] = igvar;
          num_err += add_yaml_report_timing_task(t_task_node, t_task_idx, local_gvar);
          config[YAML_TIMING_TASK_NODE_].push_back(t_task_node);
        }
      }
    }
  }
  /* Add timing summary one by one */
  size_t summary_cnt = 0;
  for (auto pair : name2ids_) {
    YAML::Node t_smry_node;
    num_err += add_yaml_report_timing_summary(t_smry_node, pair.first, summary_cnt == 0);
    config[YAML_TIMING_SUMMARY_NODE_].push_back(t_smry_node);
    summary_cnt++;
  }

  std::filesystem::path fpath = fname;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Qorso tasks are written to '%s'\n", fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::write_yaml_qorso_timing_task(const std::string& fname,
                                                const bool& verbose) const {
  check_valid();
  int num_err = 0;
  YAML::Node config;
  /* Add tech part */
  num_err += add_yaml_tech_info(config);
  /* Add netlist part */
  num_err += add_yaml_netlist_info(config);
  /* Add spef part */
  num_err += add_yaml_spef_info(config);
  /* Add timing tasks one by one */
  for (size_t t_task_idx = 0; t_task_idx < timing_tasks_.size(); ++t_task_idx) {
    /* Consider global variables to be swept */
    std::vector<std::string> gvars_to_sweep = timing_task_require_sweep_gvar(t_task_idx);
    if (gvars_to_sweep.empty()) {
      YAML::Node t_task_node;
      std::map<std::string, size_t> local_gvar;
      num_err += add_yaml_report_timing_task(t_task_node, t_task_idx, local_gvar);
      config[YAML_TIMING_TASK_NODE_].push_back(t_task_node);
    } else {
      if (gvars_to_sweep.size() > 1) {
        VTR_LOG_ERROR(
          "Only support 1 global variable to be swept in timing tasks. But in the timing task "
          "'%s', there are %lu:\n",
          timing_tasks_[t_task_idx].name.c_str(), gvars_to_sweep.size());
        for (std::string curr_gvar : gvars_to_sweep) {
          VTR_LOG("\t%s\n", curr_gvar.c_str());
        }
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      /* FIXME: support only 1 loop of sweep here. For the sake of computing complexity !!! We can
       * extend to more loops but it may be rarely used. */
      for (std::string curr_gvar : gvars_to_sweep) {
        for (int igvar = gvars_.at(curr_gvar)[0]; igvar <= gvars_.at(curr_gvar)[1];
             igvar = igvar + gvars_.at(curr_gvar)[2]) {
          YAML::Node t_task_node;
          std::map<std::string, size_t> local_gvar;
          local_gvar[curr_gvar] = igvar;
          num_err += add_yaml_report_timing_task(t_task_node, t_task_idx, local_gvar);
          config[YAML_TIMING_TASK_NODE_].push_back(t_task_node);
        }
      }
    }
  }
  /* Add timing summary one by one */
  size_t summary_cnt = 0;
  for (auto pair : name2ids_) {
    YAML::Node t_smry_node;
    num_err += add_yaml_report_timing_summary(t_smry_node, pair.first, summary_cnt == 0);
    config[YAML_TIMING_SUMMARY_NODE_].push_back(t_smry_node);
    summary_cnt++;
  }

  std::filesystem::path fpath = fname;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Qorso tasks are written to '%s'\n", fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::add_yaml_report_dynamic_power_task(YAML::Node& task_node,
                                                      const std::string& bm_name) const {
  int num_err = 0;

  task_node[YAML_POWER_TASK_NAME_] = bm_name + "_dynamic";
  task_node[YAML_POWER_TASK_METHOD_] = "time_based";
  task_node[YAML_POWER_TASK_DESIGN_].push_back("fpga_top");
  task_node[YAML_POWER_TASK_BENCHMARK_] = bm_name;
  task_node[YAML_POWER_TASK_BITSTREAM_][YAML_POWER_TASK_BITSTREAM_BASE_] =
    "sdc/fabric_bitstream_base.xml";
  task_node[YAML_POWER_TASK_BITSTREAM_][YAML_POWER_TASK_BITSTREAM_VALUE_] =
    "sdc/fabric_bitstream.xml";
  task_node[YAML_POWER_TASK_ACTIVITY_][YAML_POWER_TASK_ACTIVITY_TYPE_] = "fsdb";
  task_node[YAML_POWER_TASK_ACTIVITY_][YAML_POWER_TASK_ACTIVITY_FILE_] =
    "waveform/" + TASK_KW_CURRENT_BENCHMARK_ + ".fsdb";
  task_node[YAML_POWER_TASK_ACTIVITY_][YAML_POWER_TASK_ACTIVITY_SRIP_PATH_] =
    TASK_KW_CURRENT_BENCHMARK_ + FORMAL_VERIFICATION_TOP_MODULE_POSTFIX + '/' +
    FORMAL_VERIFICATION_TOP_MODULE_UUT_NAME;

  /*report 1*/
  {
    YAML::Node rpt_node;
    rpt_node[YAML_POWER_TASK_REPORT_NAME_] = "power_by_hierarchy";

    std::filesystem::path yaml_fpath = std::filesystem::path(task_rpt_dirpath_) /
                                       std::filesystem::path(corner_tag_) /
                                       std::filesystem::path(TASK_KW_CURRENT_DESIGN_) /
                                       std::filesystem::path(TASK_KW_CURRENT_BENCHMARK_) /
                                       std::filesystem::path("dynamic_power_by_hierarchy.rpt");
    rpt_node[YAML_POWER_TASK_REPORT_PATH_] = yaml_fpath.string();
    rpt_node[YAML_POWER_TASK_REPORT_COMMAND_].push_back(
      "report_power -hierarchy -levels 2 -nosplit -verbose");
    rpt_node[YAML_POWER_TASK_REPORT_REMOVE_TIME_STAMP_] = "true";

    task_node[YAML_POWER_TASK_REPORTS_].push_back(rpt_node);
  }
  /*report 2*/
  {
    YAML::Node rpt_node;
    rpt_node[YAML_POWER_TASK_REPORT_NAME_] = "average_activity";

    std::filesystem::path yaml_fpath =
      std::filesystem::path(task_rpt_dirpath_) / std::filesystem::path(corner_tag_) /
      std::filesystem::path(TASK_KW_CURRENT_DESIGN_) /
      std::filesystem::path(TASK_KW_CURRENT_BENCHMARK_) /
      std::filesystem::path("report_switching_activity.average.rpt");
    rpt_node[YAML_POWER_TASK_REPORT_PATH_] = yaml_fpath.string();
    rpt_node[YAML_POWER_TASK_REPORT_COMMAND_].push_back(
      "report_switching_activity -average_activity");
    rpt_node[YAML_POWER_TASK_REPORT_REMOVE_TIME_STAMP_] = "true";

    task_node[YAML_POWER_TASK_REPORTS_].push_back(rpt_node);
  }
  /*report 3*/
  {
    YAML::Node rpt_node;
    rpt_node[YAML_POWER_TASK_REPORT_NAME_] = "activity_coverage";

    std::filesystem::path yaml_fpath =
      std::filesystem::path(task_rpt_dirpath_) / std::filesystem::path(corner_tag_) /
      std::filesystem::path(TASK_KW_CURRENT_DESIGN_) /
      std::filesystem::path(TASK_KW_CURRENT_BENCHMARK_) /
      std::filesystem::path("report_switching_activity.coverage.rpt");
    rpt_node[YAML_POWER_TASK_REPORT_PATH_] = yaml_fpath.string();
    rpt_node[YAML_POWER_TASK_REPORT_COMMAND_].push_back("report_switching_activity -coverage");
    rpt_node[YAML_POWER_TASK_REPORT_REMOVE_TIME_STAMP_] = "true";

    task_node[YAML_POWER_TASK_REPORTS_].push_back(rpt_node);
  }
  /*report 4*/
  {
    YAML::Node rpt_node;
    rpt_node[YAML_POWER_TASK_REPORT_NAME_] = "activity_list_not_annotated";

    std::filesystem::path yaml_fpath =
      std::filesystem::path(task_rpt_dirpath_) / std::filesystem::path(corner_tag_) /
      std::filesystem::path(TASK_KW_CURRENT_DESIGN_) /
      std::filesystem::path(TASK_KW_CURRENT_BENCHMARK_) /
      std::filesystem::path("report_switching_activity.list_not_annotated.rpt");
    rpt_node[YAML_POWER_TASK_REPORT_PATH_] = yaml_fpath.string();
    rpt_node[YAML_POWER_TASK_REPORT_COMMAND_].push_back(
      "report_switching_activity -list_not_annotated");
    rpt_node[YAML_POWER_TASK_REPORT_REMOVE_TIME_STAMP_] = "true";

    task_node[YAML_POWER_TASK_REPORTS_].push_back(rpt_node);
  }
  return num_err;
}

int QorsoStrategy::add_yaml_report_sta_task(YAML::Node& task_node,
                                            const std::string& bm_name) const {
  int num_err = 0;

  task_node[YAML_TIMING_TASK_NAME_] = bm_name + "_sta";
  task_node[YAML_TIMING_TASK_BENCHMARK_DESIGN_].push_back("fpga_top");
  task_node[YAML_TIMING_TASK_BENCHMARK_] = bm_name;
  task_node[YAML_TIMING_TASK_BITSTREAM_][YAML_TIMING_TASK_BITSTREAM_BASE_] =
    "sdc/fabric_bitstream_base.xml";
  task_node[YAML_TIMING_TASK_BITSTREAM_][YAML_TIMING_TASK_BITSTREAM_VALUE_] =
    "sdc/fabric_bitstream.xml";

  task_node[YAML_TIMING_TASK_SDC_].push_back("custom_sdc/constrain_io.sdc");
  task_node[YAML_TIMING_TASK_SDC_].push_back("custom_sdc/disable_config.tcl");

  /*report*/
  {
    YAML::Node rpt_node;
    rpt_node[YAML_TIMING_TASK_REPORT_NAME_] = "timing";

    std::filesystem::path yaml_fpath = std::filesystem::path(task_rpt_dirpath_) /
                                       std::filesystem::path(corner_tag_) /
                                       std::filesystem::path(TASK_KW_CURRENT_DESIGN_) /
                                       std::filesystem::path(TASK_KW_CURRENT_BENCHMARK_) /
                                       std::filesystem::path(bm_name + "_sta.rpt");
    rpt_node[YAML_TIMING_TASK_REPORT_PATH_] = yaml_fpath.string();
    rpt_node[YAML_TIMING_TASK_REPORT_COMMAND_].push_back(
      "report_timing -nosplit -transition_time -nets -capacitance");
    rpt_node[YAML_TIMING_TASK_REPORT_REMOVE_TIME_STAMP_] = "true";
    task_node[YAML_TIMING_TASK_REPORTS_].push_back(rpt_node);
  }
  return num_err;
}

int QorsoStrategy::add_yaml_report_power_task(YAML::Node& task_node, const size_t& task_id) const {
  int num_err = 0;

  task_node[YAML_POWER_TASK_NAME_] = report_power_task_name(task_id);
  for (std::string dsgn_name : power_tasks_[task_id].designs) {
    task_node[YAML_POWER_TASK_DESIGN_].push_back(dsgn_name);
  }
  task_node[YAML_POWER_TASK_METHOD_] = power_tasks_[task_id].method;
  for (auto appvar_pair : power_tasks_[task_id].appvars) {
    task_node[YAML_POWER_TASK_APPVARS_][appvar_pair.first] = appvar_pair.second;
  }
  for (auto sdc_var_pair : power_tasks_[task_id].sdc_vars) {
    task_node[YAML_POWER_TASK_VARIABLES_][sdc_var_pair.first] = sdc_var_pair.second;
  }
  for (std::string sdc_file : power_tasks_[task_id].sdc_files) {
    task_node[YAML_POWER_TASK_SDC_].push_back(replace_reserved_keywords(sdc_file));
  }
  YAML::Node rpt_node;
  rpt_node[YAML_POWER_TASK_REPORT_NAME_] = power_tasks_[task_id].name;

  std::filesystem::path yaml_fpath =
    std::filesystem::path(task_rpt_dirpath_) / std::filesystem::path(corner_tag_) /
    std::filesystem::path(TASK_KW_CURRENT_DESIGN_) /
    std::filesystem::path(power_tasks_[task_id].name + std::string(".rpt"));
  rpt_node[YAML_POWER_TASK_REPORT_PATH_] = yaml_fpath.string();
  rpt_node[YAML_POWER_TASK_REPORT_COMMAND_].push_back(power_tasks_[task_id].report_command);
  rpt_node[YAML_POWER_TASK_REPORT_REMOVE_TIME_STAMP_] =
    power_tasks_[task_id].report_no_time_stamp ? "true" : "false";

  task_node[YAML_POWER_TASK_REPORTS_].push_back(rpt_node);

  return num_err;
}

std::filesystem::path QorsoStrategy::report_power_summary_dirpath() const {
  std::filesystem::path rpt_dirpath(summary_rpt_dirpath_);
  rpt_dirpath = rpt_dirpath / std::filesystem::path(corner_tag_);
  return rpt_dirpath;
}

std::string QorsoStrategy::report_power_summary_csv_file(const std::string& strtg) const {
  std::filesystem::path rpt_dirpath = report_power_summary_dirpath();
  std::filesystem::path csv_fpath =
    rpt_dirpath / std::filesystem::path(strtg + std::string(".csv"));
  return csv_fpath.string();
}

int QorsoStrategy::add_yaml_report_dynamic_power_summary(YAML::Node& smry_node,
                                                         const std::string& strtg,
                                                         const std::string& bm_name,
                                                         const bool& first_summary) const {
  int num_err = 0;
  smry_node[YAML_POWER_SUMMARY_NAME_] = strtg;

  std::filesystem::path rpt_dirpath(summary_rpt_dirpath_);
  rpt_dirpath = rpt_dirpath / std::filesystem::path(corner_tag_);
  std::filesystem::path histo_fpath =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_hist.png"));
  smry_node[YAML_POWER_SUMMARY_HISTO_][YAML_POWER_SUMMARY_FILE_] = histo_fpath.string();
  smry_node[YAML_POWER_SUMMARY_HISTO_][YAML_POWER_SUMMARY_DATA_TYPE_] = YAML_TOTAL_POWER_VALUE_;
  smry_node[YAML_POWER_SUMMARY_HISTO_][YAML_POWER_SUMMARY_HIERARCHY_LEVEL_] = "2";

  std::filesystem::path distr_fpath =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_dist.png"));
  smry_node[YAML_POWER_SUMMARY_DISTR_][YAML_POWER_SUMMARY_FILE_] = distr_fpath.string();
  smry_node[YAML_POWER_SUMMARY_DISTR_][YAML_POWER_SUMMARY_DATA_TYPE_] = YAML_TOTAL_POWER_VALUE_;
  smry_node[YAML_POWER_SUMMARY_DISTR_][YAML_POWER_SUMMARY_HIERARCHY_LEVEL_] = "2";

  std::string csv_fpath = report_power_summary_csv_file(strtg);
  smry_node[YAML_POWER_SUMMARY_CSV_][YAML_POWER_SUMMARY_CSV_NAME_] = csv_fpath;
  smry_node[YAML_POWER_SUMMARY_CSV_][YAML_POWER_SUMMARY_DATA_TYPE_] = YAML_TOTAL_POWER_VALUE_;
  smry_node[YAML_POWER_SUMMARY_CSV_][YAML_POWER_SUMMARY_HIERARCHY_LEVEL_] = "2";

  {
    std::string s_task_name = bm_name + std::string("_dynamic.fpga_top.power_by_hierarchy");
    smry_node[YAML_POWER_SUMMARY_TASKS_].push_back(s_task_name);
  }
  return num_err;
}

int QorsoStrategy::add_yaml_report_sta_summary(YAML::Node& smry_node, const std::string& strtg,
                                               const std::string& bm_name,
                                               const bool& first_summary) const {
  int num_err = 0;
  smry_node[YAML_TIMING_SUMMARY_NAME_] = strtg;
  std::filesystem::path rpt_dirpath(summary_rpt_dirpath_);
  rpt_dirpath = rpt_dirpath / std::filesystem::path(corner_tag_);
  std::filesystem::path sta_dist_csv =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_sta_dist.csv"));

  std::filesystem::path sta_dist_png =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_sta_dist.png"));

  std::filesystem::path sta_bar_png =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_sta_bar.png"));
  std::filesystem::path sta_summary_csv =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_summary.csv"));

  smry_node[YAML_TIMING_SUMMARY_DISTR_NODE_][YAML_TIMING_SUMMARY_DISTR_TABLE_] =
    sta_dist_csv.string().c_str();
  smry_node[YAML_TIMING_SUMMARY_DISTR_NODE_][YAML_TIMING_SUMMARY_DISTR_HISTO_] =
    sta_dist_png.string().c_str();
  smry_node[YAML_TIMING_SUMMARY_DISTR_NODE_][YAML_TIMING_SUMMARY_DISTR_BARCHART_] =
    sta_bar_png.string().c_str();

  smry_node[YAML_TIMING_SUMMARY_CSV_NODE_][YAML_TIMING_SUMMARY_CSV_NAME_] =
    sta_summary_csv.string().c_str();
  smry_node[YAML_TIMING_SUMMARY_CSV_NODE_][YAML_TIMING_SUMMARY_CSV_VAR_].push_back(bm_name);
  smry_node[YAML_TIMING_SUMMARY_CSV_NODE_][YAML_TIMING_SUMMARY_CSV_DATA_] = "arrival time";

  smry_node[YAML_TIMING_SUMMARY_REPORT_PARSER_NODE_]
           [YAML_TIMING_SUMMARY_REPORT_PARSER_ALLOW_UNCONSTRAINED_PATH_] = true;

  {
    std::string s_task_name = bm_name + std::string("_sta.fpga_top.timing");
    smry_node[YAML_BENCHMARK_TIMING_SUMMARY_TASKS_].push_back(s_task_name);
  }
  return num_err;
}

int QorsoStrategy::add_yaml_report_power_summary(YAML::Node& smry_node, const std::string& strtg,
                                                 const bool& first_summary) const {
  int num_err = 0;
  smry_node[YAML_POWER_SUMMARY_NAME_] = strtg;

  size_t s_idx = name2ids_.at(strtg);

  std::filesystem::path rpt_dirpath(summary_rpt_dirpath_);
  rpt_dirpath = rpt_dirpath / std::filesystem::path(corner_tag_);
  std::filesystem::path histo_fpath =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_hist.png"));
  smry_node[YAML_POWER_SUMMARY_HISTO_][YAML_POWER_SUMMARY_FILE_] = histo_fpath.string();
  smry_node[YAML_POWER_SUMMARY_HISTO_][YAML_POWER_SUMMARY_DATA_TYPE_] = summary_data_[s_idx];
  smry_node[YAML_POWER_SUMMARY_HISTO_][YAML_POWER_SUMMARY_HIERARCHY_LEVEL_] = "2";

  std::filesystem::path distr_fpath =
    rpt_dirpath / std::filesystem::path(strtg + std::string("_dist.png"));
  smry_node[YAML_POWER_SUMMARY_DISTR_][YAML_POWER_SUMMARY_FILE_] = distr_fpath.string();
  smry_node[YAML_POWER_SUMMARY_DISTR_][YAML_POWER_SUMMARY_DATA_TYPE_] = summary_data_[s_idx];
  smry_node[YAML_POWER_SUMMARY_DISTR_][YAML_POWER_SUMMARY_HIERARCHY_LEVEL_] = "2";

  std::string csv_fpath = report_power_summary_csv_file(strtg);
  smry_node[YAML_POWER_SUMMARY_CSV_][YAML_POWER_SUMMARY_CSV_NAME_] = csv_fpath;
  smry_node[YAML_POWER_SUMMARY_CSV_][YAML_POWER_SUMMARY_DATA_TYPE_] = summary_data_[s_idx];
  smry_node[YAML_POWER_SUMMARY_CSV_][YAML_POWER_SUMMARY_HIERARCHY_LEVEL_] = "2";

  for (auto pair : sub_tasks_[s_idx]) {
    VTR_ASSERT(1 == power_tasks_[pair.second].designs.size());
    std::string s_task_name = report_power_task_name(pair.second) + std::string(".") +
                              power_tasks_[pair.second].designs[0] + std::string(".") +
                              power_tasks_[s_idx].name;
    smry_node[YAML_POWER_SUMMARY_TASKS_].push_back(s_task_name);
  }
  return num_err;
}

int QorsoStrategy::write_yaml_qorso_benchmark_power_task(const std::string& fname,
                                                         const std::string& bm_name,
                                                         const bool& verbose) const {
  int num_err = 0;
  YAML::Node config;
  /* Add tech part */
  num_err += add_yaml_tech_info(config);
  /* Add netlist part */
  num_err += add_yaml_netlist_info(config);
  /* Add spef part */
  num_err += add_yaml_spef_info(config);

  /* Add power tasks one by one */

  YAML::Node p_task_node;
  num_err += add_yaml_report_dynamic_power_task(p_task_node, bm_name);
  config[YAML_POWER_TASK_NODE_].push_back(p_task_node);

  /* Add power summary one by one */
  size_t summary_cnt = 0;
  {
    YAML::Node p_smry_node;
    num_err += add_yaml_report_dynamic_power_summary(p_smry_node, "dynamic_power", bm_name,
                                                     summary_cnt == 0);
    config[YAML_POWER_SUMMARY_NODE_].push_back(p_smry_node);
    summary_cnt++;
  }

  std::filesystem::path fpath = fname;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Qorso dynamic power tasks are written to '%s'\n", fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::write_yaml_qorso_benchmark_sta_task(const std::string& fname,
                                                       const std::string& bm_name,
                                                       const bool& verbose) const {
  int num_err = 0;
  YAML::Node config;
  /* Add tech part */
  num_err += add_yaml_tech_info(config);
  /* Add netlist part */
  num_err += add_yaml_netlist_info(config);
  /* Add spef part */
  num_err += add_yaml_spef_info(config);

  /* Add power tasks one by one */

  YAML::Node p_task_node;
  num_err += add_yaml_report_sta_task(p_task_node, bm_name);
  config[YAML_BENCHMARK_TIMING_TASK_NODE_].push_back(p_task_node);

  /* Add power summary one by one */
  size_t summary_cnt = 0;
  {
    YAML::Node p_smry_node;
    num_err += add_yaml_report_sta_summary(p_smry_node, "sta", bm_name, summary_cnt == 0);
    config[YAML_TIMING_SUMMARY_NODE_].push_back(p_smry_node);
    summary_cnt++;
  }

  std::filesystem::path fpath = fname;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Qorso STA tasks are written to '%s'\n", fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int QorsoStrategy::write_yaml_qorso_power_task(const std::string& fname,
                                               const bool& verbose) const {
  check_valid();
  int num_err = 0;
  YAML::Node config;
  /* Add tech part */
  num_err += add_yaml_tech_info(config);
  /* Add netlist part */
  num_err += add_yaml_netlist_info(config);
  /* Add spef part */
  num_err += add_yaml_spef_info(config);

  /* Add power tasks one by one */
  for (size_t p_task_idx = 0; p_task_idx < power_tasks_.size(); ++p_task_idx) {
    YAML::Node p_task_node;
    num_err += add_yaml_report_power_task(p_task_node, p_task_idx);
    config[YAML_POWER_TASK_NODE_].push_back(p_task_node);
  }

  /* Add power summary one by one */
  size_t summary_cnt = 0;
  for (auto pair : name2ids_) {
    YAML::Node p_smry_node;
    num_err += add_yaml_report_power_summary(p_smry_node, pair.first, summary_cnt == 0);
    config[YAML_POWER_SUMMARY_NODE_].push_back(p_smry_node);
    summary_cnt++;
  }

  std::filesystem::path fpath = fname;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Qorso tasks are written to '%s'\n", fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

bool QorsoStrategy::auto_infer_timing_task_designs(const TileRef& tref, const bool& verbose) {
  /* Walk though all the timing tasks */
  for (size_t t_task_idx = 0; t_task_idx < timing_tasks_.size(); ++t_task_idx) {
    if (timing_tasks_[t_task_idx].design_infer_types.empty()) {
      continue;
    }
    std::regex pattern(timing_tasks_[t_task_idx].design_infer_types);
    size_t cnt = 0;
    VTR_LOGV(verbose, "Start auto-inferring designs for timing task '%s'...\n",
             timing_tasks_[t_task_idx].name.c_str());
    /* start regex match and add qualified designs */
    for (std::string tile : tref.tiles()) {
      std::string tile_type = tref.type(tile);
      std::smatch match;
      if (std::regex_search(tile_type, match, pattern)) {
        timing_tasks_[t_task_idx].designs.push_back(tile);
        cnt++;
        VTR_LOGV(verbose, "Auto inferred tile '%s' to be a design of the timing task '%s'\n",
                 tile.c_str(), timing_tasks_[t_task_idx].name.c_str());
      }
    }
    if (cnt == 0) {
      VTR_LOG_ERROR(
        "No tiles have been auto inferred for timing task '%s'! Please double check the matching "
        "pattern!\n",
        timing_tasks_[t_task_idx].name.c_str());
      return false;
    }
    VTR_LOGV(verbose, "\tAuto inferred %ld tiles for the timing task '%s'\n", cnt,
             timing_tasks_[t_task_idx].name.c_str());
  }
  return true;
}

bool QorsoStrategy::auto_infer_power_task_designs(const TileRef& tref, const bool& verbose) {
  /* Walk though all the timing tasks */
  for (size_t t_task_idx = 0; t_task_idx < power_tasks_.size(); ++t_task_idx) {
    if (power_tasks_[t_task_idx].design_infer_types.empty()) {
      continue;
    }
    std::regex pattern(power_tasks_[t_task_idx].design_infer_types);
    size_t cnt = 0;
    VTR_LOGV(verbose, "Start auto-inferring designs for power task '%s'...\n",
             power_tasks_[t_task_idx].name.c_str());
    /* start regex match and add qualified designs */
    for (std::string tile : tref.tiles()) {
      std::string tile_type = tref.type(tile);
      if (std::regex_match(tile_type, pattern)) {
        timing_tasks_[t_task_idx].designs.push_back(tile);
        cnt++;
        VTR_LOGV(verbose, "\tAuto inferred tile '%s' to be a design of the power task '%s'\n",
                 tile.c_str(), power_tasks_[t_task_idx].name.c_str());
      }
    }
    if (cnt == 0) {
      VTR_LOG_ERROR(
        "No tiles have been auto inferred for power task '%s'! Please double check the matching "
        "pattern!\n",
        power_tasks_[t_task_idx].name.c_str());
      return false;
    }
    VTR_LOGV(verbose, "\tAuto inferred %ld tiles for the power task '%s'\n", cnt,
             power_tasks_[t_task_idx].name.c_str());
  }
  return true;
}

bool QorsoStrategy::string_contain_global_variable(const std::string& curr_str,
                                                   const std::string& gvar_name) const {
  std::string gvar_inline = std::string("${") + gvar_name + std::string("}");
  size_t found = curr_str.find(gvar_inline);
  return found != std::string::npos;
}

std::vector<std::string> QorsoStrategy::timing_task_require_sweep_gvar(
  const size_t& task_id) const {
  /* Walk through all the from_pins and to_pins  */
  std::vector<std::string> sweep_vars;
  ;
  for (auto pair : gvars_) {
    size_t match_cnt = 0;
    if (pair.second[0] == pair.second[1]) {
      continue; /* Bypass constant global vars */
    }
    for (std::string pin : timing_tasks_[task_id].from_pins) {
      if (string_contain_global_variable(pin, pair.first)) {
        match_cnt++;
        break;
      }
    }
    for (std::string pin : timing_tasks_[task_id].to_pins) {
      if (string_contain_global_variable(pin, pair.first)) {
        match_cnt++;
        break;
      }
    }
    if (match_cnt) {
      sweep_vars.push_back(pair.first);
    }
  }
  return sweep_vars;
}

std::string QorsoStrategy::replace_local_global_variables(const std::string& curr_str,
                                                          const std::string& var_name,
                                                          const size_t& var_val) const {
  std::string local_copy = curr_str;
  std::string var_inline = std::string("${") + var_name + std::string("}");
  size_t pos = local_copy.find(var_inline);
  while (pos != std::string::npos) {
    local_copy.replace(pos, var_inline.size(), std::to_string(var_val));
    pos = local_copy.find(var_inline, pos + std::to_string(var_val).size());
  }
  return local_copy;
}

std::string QorsoStrategy::replace_constant_global_variables(const std::string& curr_str) const {
  std::string local_copy = curr_str;
  for (auto pair : gvars_) {
    if (pair.second[0] == pair.second[1]) {
      local_copy = replace_local_global_variables(local_copy, pair.first, pair.second[0]);
    }
  }
  return local_copy;
}

std::string QorsoStrategy::timing_yaml_filename() const {
  return TIMING_SUMMARY_FILENAME_PREFIX_ + name_ + std::string("_") + corner_tag_ +
         std::string(".yaml");
}

std::string QorsoStrategy::timing_csv_filename() const {
  return TIMING_SUMMARY_FILENAME_PREFIX_ + name_ + std::string("_") + corner_tag_ +
         std::string(".csv");
}

std::vector<std::string> QorsoStrategy::summary_yaml_files() const {
  std::vector<std::string> ret;
  std::filesystem::path rpt_dirpath(summary_rpt_dirpath_);
  rpt_dirpath = rpt_dirpath / std::filesystem::path(corner_tag_);
  std::filesystem::path yaml_fpath = rpt_dirpath / std::filesystem::path(timing_yaml_filename());
  ret.push_back(yaml_fpath.string());
  return ret;
}

std::vector<std::string> QorsoStrategy::strategies() const {
  check_valid();
  std::vector<std::string> ret;
  for (auto pair : name2ids_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<std::string> QorsoStrategy::timing_tasks(const std::string& strtg_name) const {
  check_valid();
  std::vector<std::string> ret;
  for (size_t itask = 0; itask < timing_tasks_.size(); ++itask) {
    if (timing_task_parent_strategy_[itask] == strtg_name) {
      ret.push_back(timing_tasks_[itask].name);
    }
  }
  return ret;
}

int QorsoStrategy::set_strategy_sweep_variable(const std::string& var_name, const int& start,
                                               const int& end, const int& step) {
  check_valid();
  auto result = gvars_.find(var_name);
  if (result == gvars_.end()) {
    VTR_LOG_ERROR(
      "Global variable '%s' is not defined in the Qorso strategy '%s' yet. Start, end and step "
      "value update cannot be applied\n",
      var_name.c_str(), name_.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  result->second[0] = start;
  result->second[1] = end;
  if (start == end) {
    result->second[2] = 0;
  } else {
    result->second[2] = step;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arcxpress
