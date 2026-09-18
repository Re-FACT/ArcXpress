#pragma once

#include <map>
#include <string>
#include <vector>

/* namespace arkangel begins */
namespace arkangel {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class VprBlockUsageParser {
 public: /* -- Types -- */
 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  VprBlockUsageParser();

 public: /* -- Aggregators -- */
  std::vector<std::string> block_types() const;

 public: /* -- Accessors -- */
  size_t num_nets() const;
  size_t num_blocks() const;
  size_t num_inputs() const;
  size_t num_outputs() const;
  size_t block_usage(const std::string& type) const;

 public: /* -- Mutators -- */
  int parse(const std::string& fname, const bool& verbose);
  void clear();

 private: /* -- Private Validator -- */
  /* Validate and throw errors if seen */
  void check_valid() const;

 private: /* -- Internal data -- */
  /* Basic information */
  size_t num_nets_;
  size_t num_inputs_;
  size_t num_outputs_;
  std::map<std::string, size_t> block_stats_;

  /* Constants for XML parsing */
  const char* XML_ROOT_NODE_;
  const char* XML_NET_NODE_;
  const char* XML_NET_ATTR_NUM_;
  const char* XML_INPUTPINS_NODE_;
  const char* XML_INPUTPINS_ATTR_NUM_;
  const char* XML_OUTPUTPINS_NODE_;
  const char* XML_OUTPUTPINS_ATTR_NUM_;
  const char* XML_BLOCKS_NODE_;
  const char* XML_BLOCKS_ATTR_NUM_;
  const char* XML_BLOCK_NODE_;
  const char* XML_BLOCK_ATTR_TYPE_;
  const char* XML_BLOCK_ATTR_USAGE_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arkangel
