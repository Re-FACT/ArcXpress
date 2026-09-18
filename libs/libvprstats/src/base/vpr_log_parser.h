#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

/* namespace arkangel begins */
namespace arkangel {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class VprLogParser {
 public: /* -- Types -- */
  enum class e_keyword_type { INT = 0, FLOAT, SCIENTIFIC, NUM_TYPES };
  enum class e_data_type { CPD = 0, NUM_TYPES };

 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  VprLogParser();

 public: /* -- Aggregators -- */
  std::vector<std::string> keywords() const;

 public: /* -- Accessors -- */
  float value(const std::string& kw) const;
  bool find_critical_path_delay_keyword(std::string& kw) const;

 public: /* -- Mutators -- */
  int load_keywords_from_xml(const std::string& fname, const bool& verbose);
  /* Must load keywords before parsing */
  int parse(const std::string& fname, const bool& verbose);
  void clear();
  void clear_results();
  void clear_keywords();

 private: /* -- Private Validator -- */
  /* Validate and throw errors if seen */
  void check_valid() const;
  void check_keyword_valid() const;
  void check_result_valid() const;

 public: /* Public utility */
  e_keyword_type str2keyword_type(const std::string& str, const bool& verbose = false) const;
  std::string keyword_type2str(const e_keyword_type& type, const bool& verbose = false) const;
  std::string keyword_type_all2str() const;
  bool valid_keyword_type(const e_keyword_type& type) const;
  void check_valid_keyword_type(const e_keyword_type& type) const;

  e_data_type str2data_type(const std::string& str, const bool& verbose = false) const;
  std::string data_type2str(const e_data_type& type, const bool& verbose = false) const;
  std::string data_type_all2str() const;
  bool valid_data_type(const e_data_type& type) const;
  void check_valid_data_type(const e_data_type& type) const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::map<std::string, float> stats_;
  /* keyword name -> regular expression */
  std::map<std::string, std::string> kw_regex_;
  std::map<std::string, e_keyword_type> kw_types_;
  std::map<std::string, e_data_type> kw_data_types_;
  std::array<const char*, size_t(e_keyword_type::NUM_TYPES)> KW_TYPE_STRING_;
  std::array<const char*, size_t(e_data_type::NUM_TYPES)> KW_DATA_TYPE_STRING_;

  /* Constants */
  const char* XML_ROOT_NODE_;
  const char* XML_KW_NODE_;
  const char* XML_KW_ATTR_NAME_;
  const char* XML_KW_REGEX_NODE_;
  const char* XML_KW_ATTR_TYPE_;
  const char* XML_KW_ATTR_DATA_TYPE_;

  /* Flag */
  mutable bool kw_is_valid_;
  mutable bool result_is_valid_;
};

}  // namespace arkangel
