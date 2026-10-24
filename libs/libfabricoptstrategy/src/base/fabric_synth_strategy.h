#pragma once

#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class FabricSynthStrategy {
 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  FabricSynthStrategy(const std::string& root_path, const std::string& tech_name,
                      const std::string& corner);

 public: /* -- Aggregators -- */
  std::vector<std::string> strategy_designs(const std::string& strtg_name) const;
  /* Return a list of unique target libraries used by all the strategies */
  std::vector<std::string> target_libs() const;

 public: /* -- Accessors -- */
  int write_yaml_synth_task(const std::string& fname, const bool& verbose) const;
  int write_yaml_analyze_task(const std::string& fname, const bool& verbose) const;

 public: /* -- Mutators -- */
  void set_netlist_input(const std::string& val);
  void set_netlist_output(const std::string& val);
  void add_netlist_exclude(const std::string& val);
  void add_netlist_system_verilog_filter(const std::string& val);
  /* Add a design for synthesis. Will check the name as auto assign to a valid strategy */
  int add_design_for_synth(const std::string& val, const bool& verbose);
  int add_design_for_analyze(const std::string& val);

  int add_strategy(const std::string& val);
  int set_default(const std::string& name);
  int add_design_pattern(const std::string& name, const std::string& val);
  int add_target_lib(const std::string& name, const std::string& val);
  int set_compile_type(const std::string& name, const std::string& val);
  int set_compile_optimization_target(const std::string& name, const std::string& val);
  int set_compile_optimization_effort(const std::string& name, const std::string& val);
  int add_sdc(const std::string& name, const std::string& val);
  void clear_general_settings();
  void clear_strategies();
  void clear();

 public: /* -- Validator -- */
  /* Validate if the content is legal, without the validate. Any data access will cause errors */
  bool validate_strategies() const;
  bool validate_general_settings() const;
  bool valid_strategy(const std::string& name) const;

 private: /* Private utility */
  bool strategy_is_used(const std::string& name) const;
  std::string replace_reserved_keywords(const std::string& val) const;
  int add_yaml_tech_info(YAML::Node& config) const;
  int add_yaml_netlist_info(YAML::Node& config) const;

 private: /* -- Private Validator -- */
  /* Validate and throw errors if seen */
  void check_valid() const;
  void check_valid_general_settings() const;
  void check_valid_strategies() const;

 private: /* -- Internal data -- */
  std::map<std::string, std::string> reserved_keywords_;
  /* General settings */
  std::string root_path_;
  std::string tech_name_;
  std::string corner_;

  /* Netlists */
  std::string netlist_input_;
  std::vector<std::string> netlist_exclude_;
  std::vector<std::string> netlist_system_verilog_filter_;
  std::string netlist_output_;
  /* A map for an exact design name to its strategy */
  std::map<std::string, std::string> design2strategy_;

  /* Per strategies */
  std::map<std::string, size_t> name2ids_;
  std::string default_;
  std::vector<std::vector<std::string>> design_patterns_;
  std::vector<std::vector<std::string>> target_libs_;
  std::vector<std::string> compile_types_;
  std::vector<std::string> compile_opt_targets_;
  std::vector<std::string> compile_opt_effort_;
  std::vector<std::vector<std::string>> sdc_files_;

  /* Constants for YAML writer */
  std::string YAML_TECH_NODE_;
  std::string YAML_TECH_NAME_;
  std::string YAML_TECH_ROOT_;
  std::string YAML_TECH_CORNER_;
  std::string YAML_NETLIST_NODE_;
  std::string YAML_NETLIST_INPUT_;
  std::string YAML_NETLIST_EXCLUDE_;
  std::string YAML_NETLIST_SYSTEM_VERILOG_FILTER_;
  std::string YAML_NETLIST_OUTPUT_;
  std::string YAML_SYNTH_TASK_NODE_;
  std::string YAML_SYNTH_TASK_NAME_;
  std::string YAML_SYNTH_TASK_CURRENT_DESIGN_;
  std::string YAML_SYNTH_TASK_COMPILE_NODE_;
  std::string YAML_SYNTH_TASK_COMPILE_TYPE_;
  std::string YAML_SYNTH_TASK_COMPILE_OPTIMIZATION_;
  std::string YAML_SYNTH_TASK_COMPILE_EFFORT_;
  std::string YAML_SYNTH_TASK_TARGET_LIBRARY_;
  std::string YAML_SYNTH_TASK_SDC_;
  std::string YAML_SYNTH_TASK_REPORT_AREA_;
  std::string YAML_SYNTH_TASK_REPORT_TIMING_;
  std::string YAML_SYNTH_TASK_REPORT_POWER_;
  std::string YAML_SYNTH_TASK_REPORT_FILE_;
  std::string YAML_SYNTH_TASK_REPORT_HIERARCHY_;

  std::string ANALYZE_TASK_NAME_;

  std::string KW_ROOT_;
  std::string KW_PWD_;
  std::string KW_PROJHOME_;

  /* Flag */
  mutable bool is_valid_strategies_;
  mutable bool is_valid_gsettings_;
};

}  // namespace arcxpress
