#include "fabric_synth_strategy.h"

#include <filesystem>
#include <regex>

#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

FabricSynthStrategy::FabricSynthStrategy(const std::string& root_path, const std::string& tech_name,
                                         const std::string& corner) {
  root_path_ = root_path;
  tech_name_ = tech_name;
  corner_ = corner;
  is_valid_strategies_ = false;
  is_valid_gsettings_ = false;

  YAML_TECH_NODE_ = "technology";
  YAML_TECH_NAME_ = "name";
  YAML_TECH_ROOT_ = "root";
  YAML_TECH_CORNER_ = "corner";
  YAML_NETLIST_NODE_ = "netlist";
  YAML_NETLIST_INPUT_ = "input";
  YAML_NETLIST_EXCLUDE_ = "exclude";
  YAML_NETLIST_SYSTEM_VERILOG_FILTER_ = "system_verilog_filter";
  YAML_NETLIST_OUTPUT_ = "output";
  YAML_SYNTH_TASK_NODE_ = "synth_tasks";
  YAML_SYNTH_TASK_NAME_ = "name";
  YAML_SYNTH_TASK_CURRENT_DESIGN_ = "current_design";
  YAML_SYNTH_TASK_COMPILE_NODE_ = "compile";
  YAML_SYNTH_TASK_COMPILE_TYPE_ = "type";
  YAML_SYNTH_TASK_COMPILE_OPTIMIZATION_ = "optimization";
  YAML_SYNTH_TASK_COMPILE_EFFORT_ = "effort";
  YAML_SYNTH_TASK_TARGET_LIBRARY_ = "target_library";
  YAML_SYNTH_TASK_SDC_ = "sdc";
  YAML_SYNTH_TASK_REPORT_AREA_ = "report_area";
  YAML_SYNTH_TASK_REPORT_TIMING_ = "report_timing";
  YAML_SYNTH_TASK_REPORT_POWER_ = "report_power";
  YAML_SYNTH_TASK_REPORT_FILE_ = "file";
  YAML_SYNTH_TASK_REPORT_HIERARCHY_ = "hierarchy";

  ANALYZE_TASK_NAME_ = "analyze_ppa";

  KW_PWD_ = "${PWD}";
  KW_ROOT_ = "${ROOT_PATH}";
  KW_PROJHOME_ = "${PROJ_PATH}";
  reserved_keywords_[KW_PWD_] = std::filesystem::current_path().string();
  reserved_keywords_[KW_ROOT_] = root_path_;
  reserved_keywords_[KW_PROJHOME_] = std::string();
}

void FabricSynthStrategy::check_valid() const {
  check_valid_strategies();
  check_valid_general_settings();
}

void FabricSynthStrategy::check_valid_strategies() const {
  if (!is_valid_strategies_) {
    VTR_LOG_ERROR("Fabric synthesis strategy data is invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void FabricSynthStrategy::check_valid_general_settings() const {
  if (!is_valid_gsettings_) {
    VTR_LOG_ERROR("Fabric synthesis strategy general settings are invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void FabricSynthStrategy::clear_general_settings() {
  root_path_.clear();
  tech_name_.clear();
  corner_.clear();
  netlist_input_.clear();
  netlist_exclude_.clear();
  netlist_system_verilog_filter_.clear();
  netlist_output_.clear();
  is_valid_gsettings_ = false;
}

void FabricSynthStrategy::clear_strategies() {
  name2ids_.clear();
  default_.clear();
  design_patterns_.clear();
  target_libs_.clear();
  compile_types_.clear();
  compile_opt_targets_.clear();
  compile_opt_effort_.clear();
  sdc_files_.clear();
  is_valid_strategies_ = false;
}

void FabricSynthStrategy::clear() {
  clear_strategies();
  clear_general_settings();
}

int FabricSynthStrategy::add_strategy(const std::string& val) {
  /* Check if there is an collision in names */
  auto result = name2ids_.find(val);
  if (result != name2ids_.end()) {
    VTR_LOG_ERROR("Duplicated name '%s' when fabric synthesis strategy!\n", val.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t new_id = design_patterns_.size();
  name2ids_[val] = new_id;
  design_patterns_.emplace_back();
  target_libs_.emplace_back();
  compile_types_.emplace_back();
  compile_opt_targets_.emplace_back();
  compile_opt_effort_.emplace_back();
  sdc_files_.emplace_back();
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::set_default(const std::string& name) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  default_ = name;
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::add_design_pattern(const std::string& name, const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  design_patterns_[name2ids_.at(name)].push_back(val);
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::add_target_lib(const std::string& name, const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  target_libs_[name2ids_.at(name)].push_back(val);
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::set_compile_type(const std::string& name, const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  compile_types_[name2ids_.at(name)] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::set_compile_optimization_target(const std::string& name,
                                                         const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  compile_opt_targets_[name2ids_.at(name)] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::set_compile_optimization_effort(const std::string& name,
                                                         const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  compile_opt_effort_[name2ids_.at(name)] = val;
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::add_sdc(const std::string& name, const std::string& val) {
  if (!valid_strategy(name)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  sdc_files_[name2ids_.at(name)].push_back(val);
  return openfpga::CMD_EXEC_SUCCESS;
}

bool FabricSynthStrategy::valid_strategy(const std::string& name) const {
  auto result = name2ids_.find(name);
  return result != name2ids_.end();
}

bool FabricSynthStrategy::validate_strategies() const {
  /* Only 1 default is allowed */
  if (default_.empty()) {
    VTR_LOG_ERROR("No default fabric synthesis strategy is specified\n");
    return false;
  }
  if (!valid_strategy(default_)) {
    VTR_LOG_ERROR("The default fabric synthesis strategy '%s' is not a valid one!\n",
                  default_.c_str());
    return false;
  }
  /* Each should have design patterns, target libs and compile optioins */
  for (auto pair : name2ids_) {
    size_t istrtg = pair.second;
    if (design_patterns_[istrtg].empty() && default_ != pair.first) {
      VTR_LOG_ERROR("Missing design patterns for a non-default fabric synthesis strategy '%s'!\n",
                    pair.first.c_str());
      return false;
    }
    if (target_libs_[istrtg].empty()) {
      VTR_LOG_ERROR("Missing target libraries for a fabric synthesis strategy '%s'!\n",
                    pair.first.c_str());
      return false;
    }
    if (compile_types_[istrtg].empty()) {
      VTR_LOG_ERROR("Missing compilation type for a fabric synthesis strategy '%s'!\n",
                    pair.first.c_str());
      return false;
    }
    if (compile_opt_targets_[istrtg].empty()) {
      VTR_LOG_ERROR(
        "Missing compilation optimization target for a fabric synthesis strategy '%s'!\n",
        pair.first.c_str());
      return false;
    }
    if (compile_opt_effort_[istrtg].empty()) {
      VTR_LOG_ERROR(
        "Missing compilation optimization effort for a fabric synthesis strategy '%s'!\n",
        pair.first.c_str());
      return false;
    }
  }
  is_valid_strategies_ = true;
  return true;
}

bool FabricSynthStrategy::validate_general_settings() const {
  if (root_path_.empty()) {
    VTR_LOG_ERROR("Root path is not specified\n");
    return false;
  }
  if (tech_name_.empty()) {
    VTR_LOG_ERROR("Technology name is not specified\n");
    return false;
  }
  if (corner_.empty()) {
    VTR_LOG_ERROR("Technology corner is not specified\n");
    return false;
  }
  if (netlist_input_.empty()) {
    VTR_LOG_ERROR("Input netlist path is not specified\n");
    return false;
  }
  if (netlist_output_.empty()) {
    VTR_LOG_ERROR("Output netlist path is not specified\n");
    return false;
  }
  if (design2strategy_.empty()) {
    VTR_LOG_ERROR("No designs are specified\n");
    return false;
  }
  is_valid_gsettings_ = true;
  return true;
}

void FabricSynthStrategy::set_netlist_input(const std::string& val) { netlist_input_ = val; }

void FabricSynthStrategy::set_netlist_output(const std::string& val) { netlist_output_ = val; }

void FabricSynthStrategy::add_netlist_exclude(const std::string& val) {
  netlist_exclude_.push_back(val);
}

void FabricSynthStrategy::add_netlist_system_verilog_filter(const std::string& val) {
  netlist_system_verilog_filter_.push_back(val);
}

int FabricSynthStrategy::add_design_for_synth(const std::string& val, const bool& verbose) {
  /* Avoid duplicated name */
  auto result = design2strategy_.find(val);
  if (result != design2strategy_.end()) {
    VTR_LOG_ERROR(
      "Duplicated design name '%s' found which is not allowed for synthesis tasks. Each design "
      "should be synthesized only once\n",
      val.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Find a proper strategy */
  std::string strtg_name;
  for (auto pair : name2ids_) {
    for (std::string pattern_str : design_patterns_[pair.second]) {
      VTR_LOGV(verbose, "Apply pattern checking '%s' for block '%s'\n", pattern_str.c_str(),
               val.c_str());
      std::regex pattern(pattern_str);
      std::smatch match;
      if (std::regex_search(val, match, pattern)) {
        VTR_LOGV(verbose, "Found a matched pattern '%s' for block '%s'\n", pattern_str.c_str(),
                 val.c_str());
        strtg_name = pair.first;
        break;
      }
    }
    if (!strtg_name.empty()) {
      break;
    }
  }
  /* No match. Use the default */
  if (strtg_name.empty()) {
    VTR_LOGV(verbose, "Use default strategy '%s' for block '%s'\n", default_.c_str(), val.c_str());
    strtg_name = default_;
  }
  design2strategy_[val] = strtg_name;
  return openfpga::CMD_EXEC_SUCCESS;
}

int FabricSynthStrategy::add_design_for_analyze(const std::string& val) {
  /* Avoid duplicated name */
  auto result = design2strategy_.find(val);
  if (result != design2strategy_.end()) {
    VTR_LOG_ERROR(
      "Duplicated design name '%s' found which is not allowed for synthesis tasks. Each design "
      "should be synthesized only once\n",
      val.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Deposit whatever */
  design2strategy_[val] = "EMPTY";
  return openfpga::CMD_EXEC_SUCCESS;
}

std::string FabricSynthStrategy::replace_reserved_keywords(const std::string& val) const {
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

int FabricSynthStrategy::add_yaml_tech_info(YAML::Node& config) const {
  int num_err = 0;
  config[YAML_TECH_NODE_][YAML_TECH_NAME_] = tech_name_;
  /* Dummy string. It will be ignored whatever */
  config[YAML_TECH_NODE_][YAML_TECH_ROOT_] = "${PDK_ROOT}";
  config[YAML_TECH_NODE_][YAML_TECH_CORNER_] = corner_;

  return num_err;
}

int FabricSynthStrategy::add_yaml_netlist_info(YAML::Node& config) const {
  int num_err = 0;
  config[YAML_NETLIST_NODE_][YAML_NETLIST_INPUT_] = netlist_input_;
  for (std::string sv_nlist : netlist_system_verilog_filter_) {
    config[YAML_NETLIST_NODE_][YAML_NETLIST_SYSTEM_VERILOG_FILTER_].push_back(sv_nlist);
  }
  for (std::string excl_nlist : netlist_exclude_) {
    config[YAML_NETLIST_NODE_][YAML_NETLIST_EXCLUDE_].push_back(excl_nlist);
  }
  config[YAML_NETLIST_NODE_][YAML_NETLIST_OUTPUT_] = netlist_output_;

  return num_err;
}

int FabricSynthStrategy::write_yaml_synth_task(const std::string& fname,
                                               const bool& verbose) const {
  check_valid();
  int num_err = 0;
  YAML::Node config;
  /* Add tech part */
  num_err += add_yaml_tech_info(config);
  /* Add netlist part */
  num_err += add_yaml_netlist_info(config);
  /* Add synth tasks */
  for (auto pair : name2ids_) {
    /* If any design use this, we should add the task */
    if (!strategy_is_used(pair.first)) {
      continue;
    }
    /* Build the task */
    YAML::Node s_task;
    s_task[YAML_SYNTH_TASK_NAME_] = pair.first;
    for (std::string curr_design : strategy_designs(pair.first)) {
      s_task[YAML_SYNTH_TASK_CURRENT_DESIGN_].push_back(curr_design);
    }
    s_task[YAML_SYNTH_TASK_COMPILE_NODE_][YAML_SYNTH_TASK_COMPILE_TYPE_] =
      compile_types_[pair.second];
    s_task[YAML_SYNTH_TASK_COMPILE_NODE_][YAML_SYNTH_TASK_COMPILE_OPTIMIZATION_] =
      compile_opt_targets_[pair.second];
    s_task[YAML_SYNTH_TASK_COMPILE_NODE_][YAML_SYNTH_TASK_COMPILE_EFFORT_] =
      compile_opt_effort_[pair.second];
    for (std::string tgt_lib : target_libs_[pair.second]) {
      s_task[YAML_SYNTH_TASK_TARGET_LIBRARY_].push_back(replace_reserved_keywords(tgt_lib));
    }
    for (std::string sdc_f : sdc_files_[pair.second]) {
      s_task[YAML_SYNTH_TASK_SDC_].push_back(replace_reserved_keywords(sdc_f));
    }
    /* TODO: Make this flexible ? */
    s_task[YAML_SYNTH_TASK_REPORT_AREA_][YAML_SYNTH_TASK_REPORT_FILE_] =
      "./reports/[current_design]/area.rpt";
    s_task[YAML_SYNTH_TASK_REPORT_TIMING_][YAML_SYNTH_TASK_REPORT_FILE_] =
      "./reports/[current_design]/timing.rpt";
    s_task[YAML_SYNTH_TASK_REPORT_POWER_][YAML_SYNTH_TASK_REPORT_FILE_] =
      "./reports/[current_design]/power.rpt";
    s_task[YAML_SYNTH_TASK_REPORT_POWER_][YAML_SYNTH_TASK_REPORT_HIERARCHY_] = "2";

    /* Add the node */
    config[YAML_SYNTH_TASK_NODE_].push_back(s_task);
  }

  std::filesystem::path fpath = fname;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Synth tasks are written to '%s'\n", fpath.c_str());
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

int FabricSynthStrategy::write_yaml_analyze_task(const std::string& fname,
                                                 const bool& verbose) const {
  check_valid_general_settings();
  int num_err = 0;
  YAML::Node config;
  /* Add tech part */
  num_err += add_yaml_tech_info(config);
  /* Add netlist part */
  num_err += add_yaml_netlist_info(config);

  /* Build the task */
  YAML::Node s_task;
  s_task[YAML_SYNTH_TASK_NAME_] = ANALYZE_TASK_NAME_;
  for (auto pair : design2strategy_) {
    s_task[YAML_SYNTH_TASK_CURRENT_DESIGN_].push_back(pair.first);
  }
  /* TODO: Make this flexible ? */
  s_task[YAML_SYNTH_TASK_REPORT_AREA_][YAML_SYNTH_TASK_REPORT_FILE_] =
    "./reports/[current_design]/area.rpt";
  s_task[YAML_SYNTH_TASK_REPORT_POWER_][YAML_SYNTH_TASK_REPORT_FILE_] =
    "./reports/[current_design]/power.rpt";
  s_task[YAML_SYNTH_TASK_REPORT_POWER_][YAML_SYNTH_TASK_REPORT_HIERARCHY_] = "2";

  /* Add the node */
  config[YAML_SYNTH_TASK_NODE_].push_back(s_task);

  std::filesystem::path fpath = fname;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Analyze tasks are written to '%s'\n", fpath.c_str());
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

bool FabricSynthStrategy::strategy_is_used(const std::string& name) const {
  check_valid();
  for (auto pair : design2strategy_) {
    if (pair.second == name) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> FabricSynthStrategy::strategy_designs(const std::string& name) const {
  check_valid();
  std::vector<std::string> ret;
  for (auto pair : design2strategy_) {
    if (pair.second == name) {
      ret.push_back(pair.first);
    }
  }
  return ret;
}

std::vector<std::string> FabricSynthStrategy::target_libs() const {
  check_valid_strategies();
  std::vector<std::string> ret;
  for (auto pair : name2ids_) {
    /* If any design use this, we should add the task */
    if (!strategy_is_used(pair.first)) {
      continue;
    }
    for (auto t_lib : target_libs_[pair.second]) {
      if (std::find(ret.begin(), ret.end(), t_lib) == ret.end()) {
        ret.push_back(t_lib);
      }
    }
  }
  return ret;
}

}  // End namespace arcxpress
