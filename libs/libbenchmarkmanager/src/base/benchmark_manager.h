#ifndef BENCHMARK_MANAGER_H
#define BENCHMARK_MANAGER_H

#include <array>
#include <map>
#include <string>
#include <vector>

#include "benchmark_manager_fwd.h"
#include "vtr_vector.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArcXpress benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class BenchmarkManager {
 public: /* -- Types -- */
  /* The type of design source files */
  enum class e_file_type {
    VERILOG = 0,     /* Design sources */
    SYSTEM_VERILOG,  /* system verilog netlist */
    BLIF,            /* Post-synthesis netlist */
    SDC,             /* VPR sdc file */
    PCF_GCLK,        /* Openfpga XML-based Pin-constraint file */
    PCF,             /* Openfpga Pin-constraint file */
    COCOTB_SOURCE,   /* Cocotb source files */
    COCOTB_TOP,      /* Cocotb top-level testbench */
    COCOTB_MAKEFILE, /* Cocotb makefile */
    NUM_TYPES        /* Count the number of types, also this is an invalid type */
  };

 public: /*  -- Ranges -- */
  typedef vtr::vector<BenchmarkSuiteId, BenchmarkSuiteId>::const_iterator benchmark_suite_iterator;
  typedef vtr::Range<benchmark_suite_iterator> benchmark_suite_range;

 public: /* -- Constructors -- */
  BenchmarkManager();
  /* Not allow to copy and copy assignment operator. Keep the copy unique */
  BenchmarkManager(const BenchmarkManager&) = delete;
  void operator=(const BenchmarkManager&) = delete;

 public: /* -- Aggregators -- */
  benchmark_suite_range suites() const;

 public:                                               /* -- Accessors -- */
  std::vector<std::string> authors() const;            /* Get all the authors */
  std::vector<std::string> files() const;              /* Get a list files which are created from */
  std::vector<std::string> date_created() const;       /* Get all the date created */
  std::vector<std::string> date_last_modified() const; /* Get all the date last modified */
  std::vector<std::string> tool_version() const;       /* Get all the tool vesion */

  BenchmarkSuiteId find_benchmark_suite(const std::string& benchmark_name) const;
  std::string suite_name(const BenchmarkSuiteId& suite_id) const;
  std::vector<BenchmarkId> benchmarks(const BenchmarkSuiteId& suite_id) const;
  std::string benchmark_name(const BenchmarkId& bm_id) const;
  bool is_benchmark_disabled(const BenchmarkId& bm_id) const;
  /* Return the suite name for a given benchmark */
  std::string benchmark_suite_name(const BenchmarkId& bm_id) const;
  std::vector<BenchmarkDesignSourceId> design_sources(const BenchmarkId& bm_id) const;
  e_file_type design_source_type(const BenchmarkId& bm_id,
                                 const BenchmarkDesignSourceId& f_id) const;
  std::string design_source_type_str(const BenchmarkId& bm_id,
                                     const BenchmarkDesignSourceId& f_id) const;
  std::string design_source_file(const BenchmarkId& bm_id,
                                 const BenchmarkDesignSourceId& f_id) const;
  std::string top_module(const BenchmarkId& bm_id) const;
  std::vector<BenchmarkCocotbSourceId> cocotb_sources(const BenchmarkId& bm_id) const;
  int convert_make_file_simulation(const std::string& py_bin_path,
                                   const std::string& py_site_packages_path,
                                   const std::string& cocotb_convert_makefile_application_sim_path,
                                   const bool& verbose) const;
  std::vector<BenchmarkCocotbSourceId> cocotb_sources_by_type(const BenchmarkId& bm_id,
                                                              const e_file_type& f_type) const;
  e_file_type cocotb_type(const BenchmarkId& bm_id, const BenchmarkCocotbSourceId& f_id) const;
  std::string cocotb_type_str(const BenchmarkId& bm_id, const BenchmarkCocotbSourceId& f_id) const;
  std::string cocotb_file(const BenchmarkId& bm_id, const BenchmarkCocotbSourceId& f_id) const;
  std::string cocotb_makefile(const BenchmarkId& bm_id) const;
  std::vector<BenchmarkConstraintId> constraints(const BenchmarkId& bm_id) const;
  std::vector<BenchmarkConstraintId> constraints_by_type(const BenchmarkId& bm_id,
                                                         const e_file_type& f_type) const;
  e_file_type constraint_type(const BenchmarkId& bm_id, const BenchmarkConstraintId& f_id) const;
  std::string constraint_type_str(const BenchmarkId& bm_id,
                                  const BenchmarkConstraintId& f_id) const;
  std::string constraint_file(const BenchmarkId& bm_id, const BenchmarkConstraintId& f_id) const;

  BenchmarkId find_benchmark_by_name(const std::string& name) const;

  /* Check if there are any buses */
  bool empty() const;

 public: /* -- Mutators -- */
  void set_kw_root_path(const std::string& val);
  void set_kw_proj_path(const std::string& val);
  bool add_file_info(const std::string& fname, const std::string& author,
                     const std::string& date_created, const std::string& date_last_modified,
                     const std::string& tool_ver);

  BenchmarkSuiteId create_suite(const std::string& name);
  BenchmarkId add_benchmark(const BenchmarkSuiteId& suite_id, const std::string& name);
  void set_top_module(const BenchmarkId& bm_id, const std::string& name);
  void add_design_source(const BenchmarkId& bm_id, const std::string& ftype,
                         const std::string& fname);
  void add_cocotb_source(const BenchmarkId& bm_id, const std::string& ftype,
                         const std::string& fname);
  void add_constraint(const BenchmarkId& bm_id, const std::string& ftype, const std::string& fname);
  int try_set_benchmark_disabled(const std::string& bm_name, const bool& verbose);
  void clear();

 public: /* -- Validator -- */
  bool valid_suite_id(const BenchmarkSuiteId& suite_id) const;
  bool valid_benchmark_id(const BenchmarkId& bm_id) const;
  /* Validate if the content is legal, without the validate. Any data access will cause errors */
  bool validate() const;

 public: /* Public utility */
  e_file_type str2design_source_file_type(const std::string& str,
                                          const bool& verbose = false) const;
  std::string design_source_file_type2str(const e_file_type& type,
                                          const bool& verbose = false) const;
  std::string design_source_file_type_all2str() const;
  bool valid_design_source_file_type(const e_file_type& type) const;

  e_file_type str2cocotb_file_type(const std::string& str, const bool& verbose = false) const;
  std::string cocotb_file_type2str(const e_file_type& type, const bool& verbose = false) const;
  std::string cocotb_file_type_all2str() const;
  bool valid_cocotb_file_type(const e_file_type& type) const;

  e_file_type str2constraint_file_type(const std::string& str, const bool& verbose = false) const;
  std::string constraint_file_type2str(const e_file_type& type, const bool& verbose = false) const;
  std::string constraint_file_type_all2str() const;
  bool valid_constraint_file_type(const e_file_type& type) const;

 private: /* -- Private Validator -- */
  /* Validate and throw errors if seen */
  void check_valid() const;
  void check_valid_suite_id(const BenchmarkSuiteId& suite_id) const;
  void check_valid_benchmark_id(const BenchmarkId& bm_id) const;
  void check_valid_benchmark_design_source_id(const BenchmarkId& bm_id,
                                              const BenchmarkDesignSourceId& f_id) const;
  void check_valid_benchmark_cocotb_source_id(const BenchmarkId& bm_id,
                                              const BenchmarkCocotbSourceId& f_id) const;
  void check_valid_benchmark_constraint_id(const BenchmarkId& bm_id,
                                           const BenchmarkConstraintId& f_id) const;
  std::string replace_reserved_keywords(const std::string& val) const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::vector<std::string> files_;   /* From which file the benchmarks are loaded from */
  std::vector<std::string> authors_; /* Authors of each file */

  /* Reserved keyword which will be applied when generating the path to benchmark and pdk config
   * files . For example, PWD -> getcwd() */
  std::map<std::string, std::string> reserved_keywords_;

  /* Time stamps */
  std::vector<std::string> date_created_; /* For each file, the date that the benchmark suite is
                                             created for the first time */
  std::vector<std::string>
    date_last_modified_; /* For each file, the date that the benchmark suite is lastly modified */
  std::vector<std::string>
    tool_version_; /* For each file, the version of ArcXpress that creates the project */

  /* A list of benchmark suites */
  vtr::vector<BenchmarkSuiteId, BenchmarkSuiteId> suite_ids_;
  vtr::vector<BenchmarkSuiteId, std::string> suite_names_;
  vtr::vector<BenchmarkSuiteId, std::vector<BenchmarkId>> suite_benchmarks_;

  /* Fast look-up */
  std::map<std::string, BenchmarkSuiteId> suite_name2id_map_;
  std::map<std::string, BenchmarkId> name2id_map_;

  /* A list of benchmarks */
  vtr::vector<BenchmarkId, BenchmarkId> benchmark_ids_;
  vtr::vector<BenchmarkId, bool> benchmark_disabled_;
  vtr::vector<BenchmarkId, BenchmarkSuiteId> benchmark_suite_ids_;
  vtr::vector<BenchmarkId, std::string> benchmark_names_;
  vtr::vector<BenchmarkId, std::vector<e_file_type>> benchmark_design_source_types_;
  vtr::vector<BenchmarkId, std::vector<std::string>> benchmark_design_source_files_;
  vtr::vector<BenchmarkId, std::string> benchmark_top_modules_;
  vtr::vector<BenchmarkId, std::vector<e_file_type>> benchmark_cocotb_types_;
  vtr::vector<BenchmarkId, std::vector<std::string>> benchmark_cocotb_files_;
  vtr::vector<BenchmarkId, std::vector<e_file_type>> benchmark_constraint_types_;
  vtr::vector<BenchmarkId, std::vector<std::string>> benchmark_constraint_files_;

  /* Constants */
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> FILE_TYPE_STRING_;
  std::string KW_ROOT_;
  std::string KW_PWD_;
  std::string KW_PROJHOME_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress

#endif
