#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArcXpress benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class BootConfig {
 public: /* -- Types -- */
  /* The type of design source files */
  enum class e_boot_type {
    BUILD,
    INSTALL,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  enum class e_bin_type {
    OPENFPGA = 0,
    FKEY_ASSIST,
    PIN_TABLE_GEN,
    NETLIST_MAKEUP,
    CCFF_MAKEUP,
    NETLIST_SYNTH,
    FRONTEND,
    FRONTEND_CELL_LIB,
    QORSO,
    BUS_GROUP_GEN,
    COCOTB_UTIL,
    COCOTB_APPLICATION_SIM_UTIL,
    COCOTB_FORCE_BIT,
    COCOTB_MAKEFILE_INC,
    BENCHMARK_SIM_COCOTB_MAKEFILE_INC,
    PYTHON,
    PYTHON_SITE_PACKAGES,
    PYTHON_COCOTB_CONFIG,
    LIBPYTHON_LOC,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };

 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  BootConfig();
  /* Not allow to copy and copy assignment operator. Keep the copy unique */
  BootConfig(const BootConfig&) = delete;
  void operator=(const BootConfig&) = delete;

 public: /* -- Aggregators -- */
  std::vector<e_bin_type> bin_types() const;

 public:                                  /* -- Accessors -- */
  std::string author() const;             /* Get all the authors */
  std::string date_created() const;       /* Get all the date created */
  std::string date_last_modified() const; /* Get all the date last modified */
  std::string tool_version() const;       /* Get all the tool vesion */

  e_boot_type boot_type() const;
  std::string boot_type_str() const;
  std::string bin_relpath(const e_bin_type& type) const;

 public: /* -- Mutators -- */
  void set_author(const std::string& val);
  void set_date_created(const std::string& val);
  void set_date_last_modified(const std::string& val);
  void set_tool_version(const std::string& val);

  void set_boot_type(const std::string& type_str);
  void set_bin_relpath(const std::string& type_str, const std::string& val);
  void clear();

 public: /* -- Validator -- */
  /* Validate if the content is legal, without the validate. Any data access will cause errors */
  bool validate() const;

 public: /* Public utility */
  e_bin_type str2bin_type(const std::string& str, const bool& verbose = false) const;
  std::string bin_type2str(const e_bin_type& type, const bool& verbose = false) const;
  std::string bin_type_all2str() const;
  bool valid_bin_type(const e_bin_type& type) const;

  e_boot_type str2boot_type(const std::string& str, const bool& verbose = false) const;
  std::string boot_type2str(const e_boot_type& type, const bool& verbose = false) const;
  std::string boot_type_all2str() const;
  bool valid_boot_type(const e_boot_type& type) const;

 private: /* -- Private Validator -- */
  /* Validate and throw errors if seen */
  void check_valid() const;
  void check_valid_bin_type(const e_bin_type& type) const;
  void check_valid_boot_type(const e_boot_type& type) const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::string author_; /* Authors of each file */

  /* Time stamps */
  std::string date_created_; /* For each file, the date that the benchmark suite is
                                             created for the first time */
  std::string
    date_last_modified_; /* For each file, the date that the benchmark suite is lastly modified */
  std::string tool_version_; /* For each file, the version of ArcXpress that creates the project */

  e_boot_type boot_type_;

  /* Bin paths */
  std::array<std::string, size_t(e_bin_type::NUM_TYPES)> bin_relpaths_;

  /* Constants */
  std::array<const char*, size_t(e_bin_type::NUM_TYPES)> BIN_TYPE_STRING_;
  std::array<const char*, size_t(e_boot_type::NUM_TYPES)> BOOT_TYPE_STRING_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress
