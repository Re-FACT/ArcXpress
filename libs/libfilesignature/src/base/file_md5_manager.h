#pragma once

#include <map>
#include <string>
#include <vector>

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure to generate/check md5 for a list of files
 */
class FileMd5Manager {
 public: /* -- Types -- */
  /* Status of each file MD5 */
  enum class e_status {
    FRESH = 0,       /* No MD5 generated, no reference md5, */
    READY_FOR_CHECK, /* Ref Md5 is given */
    MD5_GENERATED,
    MD5_PASS,
    MD5_FAIL,
    NUM_TYPES
  };

 public: /* -- Constructors -- */
  FileMd5Manager();

 public: /* -- Accessors -- */
  /* Write md5 of all the files to a YAML file */
  int write_md5_to_yaml_file(const std::string& fpath, const bool& verbose) const;

 public: /* -- Mutators -- */
  void add_file(const std::string& val);
  /* Generate md5 for all the files */
  int gen_md5(const bool& verbose);
  /* Load md5 of all the files from a YAML file */
  int load_md5_from_yaml_file(const std::string& fpath, const bool& verbose);
  /* Check md5 for all the files */
  int check_md5(const bool& verbose);
  void clear();

 public: /* -- Validator -- */
  /* Validate if the content is legal, without the validate. Any data access will cause errors */
  void check_valid() const;

 private: /* Utility */
  void reserve(const size_t& count);
  int gen_md5_for_file(const size_t& f_idx, const bool& verbose);

 private: /* -- Internal data -- */
  /* Fname to index */
  std::vector<std::string> file_paths_;
  std::vector<std::string> file_ref_md5_;
  std::vector<std::string> file_md5_;
  std::vector<e_status> file_md5_status_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress
