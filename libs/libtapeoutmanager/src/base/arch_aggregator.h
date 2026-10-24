#pragma once

#include <map>
#include <string>
#include <vector>

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure replace the variables defined in an architecture file from the values defined
 * in a variable file and output the result to third file
 */
class ArchAggregator {
 public: /* -- Types -- */
 public: /* -- Constructors -- */
  ArchAggregator();
  ArchAggregator(const std::string& var_fpath, const bool& verbose);

 public: /* -- Accessors -- */
  int aggregate(const std::string& arch_fpath, const std::string& output_fpath,
                const bool& verbose) const;
  int write_variables(const std::string& var_fpath, const bool& verbose) const;

 public: /* -- Mutators -- */
  void add_variable(const std::string& name, const std::string& val);
  void add_1toN_variable(const std::string& name, const std::string& val);
  /* The remove variable will trigger a line to be removed in the final output, when there is a
   * match in the pattern */
  void add_remove_variable(const std::string& name);
  void add_remove_line(const std::string& name);
  void add_variable(const std::string& name, const size_t& val);
  int load_variables(const std::string& var_fpath, const bool& overwrite, const bool& show_diff,
                     const bool& verbose);
  void clear();

 private: /* Internal validators */
  std::string to_var_format(const std::string& name) const;

 private: /* -- Internal data -- */
  /* Variable information */
  std::map<std::string, std::string> vars_;
  std::map<std::string, std::vector<std::string>> vars_1toN_;
  std::vector<std::string> vars_remove_;
  std::vector<std::string> lines_remove_;
};

}  // namespace arcxpress
