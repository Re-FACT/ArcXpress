#pragma once

#include <map>
#include <regex>
#include <string>
#include <vector>

/* namespace arkangel begins */
namespace arkangel {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class LogErrorChecker {
 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  LogErrorChecker();

 public: /* -- Aggregators -- */
  std::vector<std::string> error_messages() const;

 public: /* -- Accessors -- */
  size_t num_errors() const;

 public: /* -- Mutators -- */
  int load_keywords_from_xml(const std::string& fname, const bool& verbose);
  /* Must load keywords before parsing */
  int parse(const std::string& fname, const bool& verbose);
  void clear();
  void clear_keywords();
  void clear_results();

 private: /* -- Private Validator -- */
  /* Validate and throw errors if seen */
  void check_valid() const;
  bool search_and_log_err(const std::string& line, const std::regex& pattern,
                          const size_t& line_cnt);

 private: /* -- Internal data -- */
  /* Basic information */
  std::vector<std::string> err_lines_;
  std::vector<size_t> err_line_no_;
  /* keyword name -> regular expression */
  std::map<std::string, std::string> kw_regex_;
  std::map<std::string, bool> kw_ignore_case_;

  /* Constants */
  const char* XML_ROOT_NODE_;
  const char* XML_KW_NODE_;
  const char* XML_KW_ATTR_NAME_;
  const char* XML_KW_ATTR_IGNORECASE_;
  const char* XML_KW_REGEX_NODE_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arkangel
