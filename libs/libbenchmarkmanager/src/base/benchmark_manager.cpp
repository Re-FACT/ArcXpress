#include "benchmark_manager.h"

#include <filesystem>

#include "arcxpress_constants.h"
#include "arcxpress_sys_env.h"
#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
namespace arcxpress {  // Begin namespace arcxpress

BenchmarkManager::BenchmarkManager() {
  is_valid_ = false;
  FILE_TYPE_STRING_ = {"verilog", "system_verilog", "blif", "sdc",     "pcf_gclk",
                       "pcf",     "source",         "top",  "makefile"};
  KW_PWD_ = "${PWD}";
  KW_ROOT_ = "${ROOT_PATH}";
  KW_PROJHOME_ = "${PROJ_PATH}";
  reserved_keywords_[KW_PWD_] = std::filesystem::current_path().string();
  reserved_keywords_[KW_ROOT_] = std::string();
  reserved_keywords_[KW_PROJHOME_] = std::string();
}

BenchmarkManager::benchmark_suite_range BenchmarkManager::suites() const {
  return vtr::make_range(suite_ids_.begin(), suite_ids_.end());
}

std::vector<std::string> BenchmarkManager::authors() const {
  check_valid();
  return authors_;
}

std::vector<std::string> BenchmarkManager::files() const {
  check_valid();
  return files_;
}

std::vector<std::string> BenchmarkManager::date_created() const {
  check_valid();
  return date_created_;
}

std::vector<std::string> BenchmarkManager::date_last_modified() const {
  check_valid();
  return date_last_modified_;
}

std::vector<std::string> BenchmarkManager::tool_version() const {
  check_valid();
  return tool_version_;
}

std::string BenchmarkManager::suite_name(const BenchmarkSuiteId& suite_id) const {
  check_valid();
  check_valid_suite_id(suite_id);
  return suite_names_[suite_id];
}

std::vector<BenchmarkId> BenchmarkManager::benchmarks(const BenchmarkSuiteId& suite_id) const {
  check_valid();
  check_valid_suite_id(suite_id);
  return suite_benchmarks_[suite_id];
}

std::string BenchmarkManager::benchmark_name(const BenchmarkId& bm_id) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  return benchmark_names_[bm_id];
}

std::vector<BenchmarkDesignSourceId> BenchmarkManager::design_sources(
  const BenchmarkId& bm_id) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  std::vector<BenchmarkDesignSourceId> ret;
  ret.reserve(benchmark_design_source_types_[bm_id].size());
  for (size_t isrc = 0; isrc < benchmark_design_source_types_[bm_id].size(); isrc++) {
    ret.push_back(BenchmarkDesignSourceId(isrc));
  }
  return ret;
}

BenchmarkManager::e_file_type BenchmarkManager::design_source_type(
  const BenchmarkId& bm_id, const BenchmarkDesignSourceId& f_id) const {
  check_valid();
  check_valid_benchmark_design_source_id(bm_id, f_id);
  return benchmark_design_source_types_[bm_id][size_t(f_id)];
}

std::string BenchmarkManager::design_source_type_str(const BenchmarkId& bm_id,
                                                     const BenchmarkDesignSourceId& f_id) const {
  return design_source_file_type2str(design_source_type(bm_id, f_id));
}

std::string BenchmarkManager::design_source_file(const BenchmarkId& bm_id,
                                                 const BenchmarkDesignSourceId& f_id) const {
  check_valid();
  check_valid_benchmark_design_source_id(bm_id, f_id);
  return replace_reserved_keywords(benchmark_design_source_files_[bm_id][size_t(f_id)]);
}

std::string BenchmarkManager::top_module(const BenchmarkId& bm_id) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  return benchmark_top_modules_[bm_id];
}

std::vector<BenchmarkCocotbSourceId> BenchmarkManager::cocotb_sources(
  const BenchmarkId& bm_id) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  std::vector<BenchmarkCocotbSourceId> ret;
  ret.reserve(benchmark_cocotb_types_[bm_id].size());
  for (size_t isrc = 0; isrc < benchmark_cocotb_types_[bm_id].size(); isrc++) {
    ret.push_back(BenchmarkCocotbSourceId(isrc));
  }
  return ret;
}

int BenchmarkManager::convert_make_file_simulation(
  const std::string& py_bin_path, const std::string& py_site_packages_path,
  const std::string& cocotb_convert_makefile_application_sim_path, const bool& verbose) const {
  if (openfpga::CMD_EXEC_SUCCESS !=
      set_system_env_var(PYTHON_PATH_ENV_VAR_NAME, py_site_packages_path, false)) {
    VTR_LOG_ERROR("Failed to set PYTHONPATH for cocotb makefile conversion.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string py_cmd(py_bin_path + std::string(" ") + cocotb_convert_makefile_application_sim_path);
  py_cmd += std::string(" --make_file ") + " Makefile";
  VTR_LOGV(verbose, "Running command:\n\t%s\n", py_cmd.c_str());

  if (system(py_cmd.c_str())) {
    VTR_LOG_ERROR("Convert cocotb application simulation makefile run failed with error!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  return openfpga::CMD_EXEC_SUCCESS;
}

BenchmarkManager::e_file_type BenchmarkManager::cocotb_type(
  const BenchmarkId& bm_id, const BenchmarkCocotbSourceId& f_id) const {
  check_valid();
  check_valid_benchmark_cocotb_source_id(bm_id, f_id);
  return benchmark_cocotb_types_[bm_id][size_t(f_id)];
}

std::string BenchmarkManager::cocotb_type_str(const BenchmarkId& bm_id,
                                              const BenchmarkCocotbSourceId& f_id) const {
  return cocotb_file_type2str(cocotb_type(bm_id, f_id));
}

std::string BenchmarkManager::cocotb_file(const BenchmarkId& bm_id,
                                          const BenchmarkCocotbSourceId& f_id) const {
  check_valid();
  check_valid_benchmark_cocotb_source_id(bm_id, f_id);
  return replace_reserved_keywords(benchmark_cocotb_files_[bm_id][size_t(f_id)]);
}

std::vector<BenchmarkCocotbSourceId> BenchmarkManager::cocotb_sources_by_type(
  const BenchmarkId& bm_id, const e_file_type& f_type) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  std::vector<BenchmarkCocotbSourceId> ret;
  ret.reserve(benchmark_cocotb_types_[bm_id].size());
  for (size_t isrc = 0; isrc < benchmark_cocotb_types_[bm_id].size(); isrc++) {
    if (f_type == benchmark_cocotb_types_[bm_id][isrc]) {
      ret.push_back(BenchmarkCocotbSourceId(isrc));
    }
  }
  return ret;
}

std::string BenchmarkManager::cocotb_makefile(const BenchmarkId& bm_id) const {
  std::vector<BenchmarkCocotbSourceId> constr_f =
    cocotb_sources_by_type(bm_id, e_file_type::COCOTB_MAKEFILE);
  if (constr_f.empty()) {
    VTR_LOG_ERROR("Benchmark '%s' has not valid cocotb makefile!\n", benchmark_name(bm_id).c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (constr_f.size() > 1) {
    VTR_LOG_ERROR("Benchmark '%s' has more than 1 cocotb makefile!\n",
                  benchmark_name(bm_id).c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return cocotb_file(bm_id, constr_f[0]);
}

std::vector<BenchmarkConstraintId> BenchmarkManager::constraints(const BenchmarkId& bm_id) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  std::vector<BenchmarkConstraintId> ret;
  ret.reserve(benchmark_constraint_types_[bm_id].size());
  for (size_t isrc = 0; isrc < benchmark_constraint_types_[bm_id].size(); isrc++) {
    ret.push_back(BenchmarkConstraintId(isrc));
  }
  return ret;
}

std::vector<BenchmarkConstraintId> BenchmarkManager::constraints_by_type(
  const BenchmarkId& bm_id, const e_file_type& f_type) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  std::vector<BenchmarkConstraintId> ret;
  ret.reserve(benchmark_constraint_types_[bm_id].size());
  for (size_t isrc = 0; isrc < benchmark_constraint_types_[bm_id].size(); isrc++) {
    if (f_type == benchmark_constraint_types_[bm_id][isrc]) {
      ret.push_back(BenchmarkConstraintId(isrc));
    }
  }
  return ret;
}

BenchmarkManager::e_file_type BenchmarkManager::constraint_type(
  const BenchmarkId& bm_id, const BenchmarkConstraintId& f_id) const {
  check_valid();
  check_valid_benchmark_constraint_id(bm_id, f_id);
  return benchmark_constraint_types_[bm_id][size_t(f_id)];
}

std::string BenchmarkManager::constraint_type_str(const BenchmarkId& bm_id,
                                                  const BenchmarkConstraintId& f_id) const {
  return constraint_file_type2str(constraint_type(bm_id, f_id));
}

std::string BenchmarkManager::constraint_file(const BenchmarkId& bm_id,
                                              const BenchmarkConstraintId& f_id) const {
  check_valid();
  check_valid_benchmark_constraint_id(bm_id, f_id);
  return replace_reserved_keywords(benchmark_constraint_files_[bm_id][size_t(f_id)]);
}

BenchmarkId BenchmarkManager::find_benchmark_by_name(const std::string& name) const {
  auto result = name2id_map_.find(name);
  if (result == name2id_map_.end()) {
    return BenchmarkId::INVALID();
  }
  return result->second;
}

bool BenchmarkManager::empty() const { return suite_ids_.empty(); }

void BenchmarkManager::set_kw_root_path(const std::string& val) {
  reserved_keywords_[KW_ROOT_] = val;
}

void BenchmarkManager::set_kw_proj_path(const std::string& val) {
  reserved_keywords_[KW_PROJHOME_] = val;
}

bool BenchmarkManager::add_file_info(const std::string& fname, const std::string& author,
                                     const std::string& date_created,
                                     const std::string& date_last_modified,
                                     const std::string& tool_ver) {
  /* TODO: Check file name, date format and tool version syntax */
  files_.push_back(fname);
  authors_.push_back(author);
  date_created_.push_back(date_created);
  date_last_modified_.push_back(date_last_modified);
  tool_version_.push_back(tool_ver);
  return true;
}

BenchmarkSuiteId BenchmarkManager::create_suite(const std::string& name) {
  /* Register the name, if existed, reuse the id. Otherwise, create a new one */
  BenchmarkSuiteId st_id = BenchmarkSuiteId(suite_ids_.size());
  auto result = suite_name2id_map_.find(name);
  if (result != suite_name2id_map_.end()) {
    VTR_LOG_WARN(
      "The benchmark suite name '%s' already exist. Benchmarks will be automatically appended to "
      "the existing one!\n",
      name.c_str());
    return result->second;
  }
  suite_ids_.push_back(st_id);
  suite_names_.push_back(name);
  suite_benchmarks_.emplace_back();
  /* Register to name map*/
  suite_name2id_map_[name] = st_id;
  return st_id;
}

BenchmarkId BenchmarkManager::add_benchmark(const BenchmarkSuiteId& suite_id,
                                            const std::string& name) {
  check_valid_suite_id(suite_id);
  auto result = name2id_map_.find(name);
  if (result != name2id_map_.end()) {
    VTR_LOG_WARN(
      "The benchmark name '%s' already exist. Benchmarks will be automatically appended to "
      "the existing one!\n",
      name.c_str());
    return result->second;
  }
  BenchmarkId bm_id = BenchmarkId(benchmark_ids_.size());
  benchmark_ids_.push_back(bm_id);
  benchmark_disabled_.push_back(false);
  benchmark_names_.push_back(name);
  benchmark_design_source_types_.emplace_back();
  benchmark_design_source_files_.emplace_back();
  benchmark_top_modules_.emplace_back();
  benchmark_cocotb_types_.emplace_back();
  benchmark_cocotb_files_.emplace_back();
  benchmark_constraint_types_.emplace_back();
  benchmark_constraint_files_.emplace_back();
  /* Register to name map*/
  name2id_map_[name] = bm_id;
  /* Register the benchmark to a specific suite */
  suite_benchmarks_[suite_id].push_back(bm_id);
  benchmark_suite_ids_.push_back(suite_id);
  return bm_id;
}

void BenchmarkManager::set_top_module(const BenchmarkId& bm_id, const std::string& name) {
  check_valid_benchmark_id(bm_id);
  benchmark_top_modules_[bm_id] = name;
}

void BenchmarkManager::add_design_source(const BenchmarkId& bm_id, const std::string& ftype,
                                         const std::string& fname) {
  check_valid_benchmark_id(bm_id);
  benchmark_design_source_types_[bm_id].push_back(str2design_source_file_type(ftype, true));
  benchmark_design_source_files_[bm_id].push_back(fname);
}

void BenchmarkManager::add_cocotb_source(const BenchmarkId& bm_id, const std::string& ftype,
                                         const std::string& fname) {
  check_valid_benchmark_id(bm_id);
  benchmark_cocotb_types_[bm_id].push_back(str2cocotb_file_type(ftype, true));
  benchmark_cocotb_files_[bm_id].push_back(fname);
}

void BenchmarkManager::add_constraint(const BenchmarkId& bm_id, const std::string& ftype,
                                      const std::string& fname) {
  check_valid_benchmark_id(bm_id);
  benchmark_constraint_types_[bm_id].push_back(str2constraint_file_type(ftype, true));
  benchmark_constraint_files_[bm_id].push_back(fname);
}

bool BenchmarkManager::valid_suite_id(const BenchmarkSuiteId& suite_id) const {
  return ((size_t(suite_id) < suite_ids_.size()) && (suite_id == suite_ids_[suite_id]));
}

bool BenchmarkManager::valid_benchmark_id(const BenchmarkId& bm_id) const {
  return ((size_t(bm_id) < benchmark_ids_.size()) && (bm_id == benchmark_ids_[bm_id]));
}

bool BenchmarkManager::validate() const {
  is_valid_ = false;
  /* Must have at least 1 time stamp */
  size_t num_err = 0;
  if (files_.empty()) {
    VTR_LOG_ERROR(
      "No time stamp is kept for the benchmark suites! Your data source may be damaged!\n");
    num_err++;
  }
  /* Each benchmark should have required file */
  for (BenchmarkId bm_id : benchmark_ids_) {
    BenchmarkSuiteId st_id = benchmark_suite_ids_[bm_id];
    if (benchmark_design_source_types_[bm_id].empty()) {
      VTR_LOG_ERROR("Benchmark '%s' of suite '%s' has no design sources!\n",
                    benchmark_names_[bm_id].c_str(), suite_names_[st_id].c_str());
      num_err++;
    }
    if (benchmark_top_modules_[bm_id].empty()) {
      VTR_LOG_ERROR("Benchmark '%s' of suite '%s' has no top module name!\n",
                    benchmark_names_[bm_id].c_str(), suite_names_[st_id].c_str());
      num_err++;
    }
    if (benchmark_cocotb_types_[bm_id].empty()) {
      VTR_LOG_ERROR("Benchmark '%s' of suite '%s' has no cocotb testbenches!\n",
                    benchmark_names_[bm_id].c_str(), suite_names_[st_id].c_str());
      num_err++;
    }
    /* Must have a valid cocotb makefile */
    bool found_cocotb_top = false;
    bool found_cocotb_mkf = false;
    for (BenchmarkManager::e_file_type ftype : benchmark_cocotb_types_[bm_id]) {
      if (ftype == BenchmarkManager::e_file_type::COCOTB_TOP) {
        found_cocotb_top = true;
      } else if (ftype == BenchmarkManager::e_file_type::COCOTB_MAKEFILE) {
        found_cocotb_mkf = true;
      }
    }
    if (!found_cocotb_top) {
      VTR_LOG_ERROR("Benchmark '%s' of suite '%s' has no cocotb top-level python script!\n",
                    benchmark_names_[bm_id].c_str(), suite_names_[st_id].c_str());
      num_err++;
    }
    if (!found_cocotb_mkf) {
      VTR_LOG_ERROR("Benchmark '%s' of suite '%s' has no cocotb Makefile!\n",
                    benchmark_names_[bm_id].c_str(), suite_names_[st_id].c_str());
      num_err++;
    }
  }
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err == 0;
}

void BenchmarkManager::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Benchmark data is invalid!");
    VTR_LOG("Check the data sources where it is loaded from:\n\n");
    for (size_t isrc = 0; isrc < files_.size(); isrc++) {
      VTR_LOG("\t%s\t%s\n", files_[isrc].c_str(), authors_[isrc].c_str());
    }
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void BenchmarkManager::check_valid_suite_id(const BenchmarkSuiteId& suite_id) const {
  if (!valid_suite_id(suite_id)) {
    VTR_LOG_ERROR("Benchmark suite id '%ld' is invalid!", size_t(suite_id));
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void BenchmarkManager::check_valid_benchmark_id(const BenchmarkId& bm_id) const {
  if (!valid_benchmark_id(bm_id)) {
    VTR_LOG_ERROR("Benchmark id '%ld' is invalid!", size_t(bm_id));
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void BenchmarkManager::check_valid_benchmark_design_source_id(
  const BenchmarkId& bm_id, const BenchmarkDesignSourceId& f_id) const {
  check_valid_benchmark_id(bm_id);
  if (size_t(f_id) >= benchmark_design_source_types_[bm_id].size()) {
    VTR_LOG_ERROR("Benchmark design source id '%ld' under benchmark '%s' is invalid!", size_t(f_id),
                  benchmark_names_[bm_id].c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void BenchmarkManager::check_valid_benchmark_cocotb_source_id(
  const BenchmarkId& bm_id, const BenchmarkCocotbSourceId& f_id) const {
  check_valid_benchmark_id(bm_id);
  if (size_t(f_id) >= benchmark_cocotb_types_[bm_id].size()) {
    VTR_LOG_ERROR("Benchmark cocotb source id '%ld' under benchmark '%s' is invalid!", size_t(f_id),
                  benchmark_names_[bm_id].c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void BenchmarkManager::check_valid_benchmark_constraint_id(
  const BenchmarkId& bm_id, const BenchmarkConstraintId& f_id) const {
  check_valid_benchmark_id(bm_id);
  if (size_t(f_id) >= benchmark_constraint_types_[bm_id].size()) {
    VTR_LOG_ERROR("Benchmark constraint source id '%ld' under benchmark '%s' is invalid!",
                  size_t(f_id), benchmark_names_[bm_id].c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

BenchmarkManager::e_file_type BenchmarkManager::str2design_source_file_type(
  const std::string& str, const bool& verbose) const {
  for (int itype = size_t(BenchmarkManager::e_file_type::VERILOG);
       itype <= size_t(BenchmarkManager::e_file_type::BLIF); ++itype) {
    if (str == std::string(FILE_TYPE_STRING_[itype])) {
      return static_cast<BenchmarkManager::e_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for design source file! Expect %s\n",
                 design_source_file_type_all2str().c_str());
  return BenchmarkManager::e_file_type::NUM_TYPES;
}

std::string BenchmarkManager::design_source_file_type2str(const BenchmarkManager::e_file_type& type,
                                                          const bool& verbose) const {
  if (!valid_design_source_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for design source file! Expect %s\n",
                   design_source_file_type_all2str().c_str());
    return std::string();
  }
  return std::string(FILE_TYPE_STRING_[size_t(type)]);
}

std::string BenchmarkManager::design_source_file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(BenchmarkManager::e_file_type::VERILOG);
       itype <= size_t(BenchmarkManager::e_file_type::BLIF); ++itype) {
    full_types += std::string(FILE_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool BenchmarkManager::valid_design_source_file_type(const e_file_type& type) const {
  return size_t(BenchmarkManager::e_file_type::VERILOG) <= size_t(type) &&
         size_t(type) <= size_t(BenchmarkManager::e_file_type::BLIF);
}

BenchmarkManager::e_file_type BenchmarkManager::str2cocotb_file_type(const std::string& str,
                                                                     const bool& verbose) const {
  for (int itype = size_t(BenchmarkManager::e_file_type::COCOTB_SOURCE);
       itype <= size_t(BenchmarkManager::e_file_type::COCOTB_MAKEFILE); ++itype) {
    if (str == std::string(FILE_TYPE_STRING_[itype])) {
      return static_cast<BenchmarkManager::e_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for cocotb source file! Expect %s\n",
                 cocotb_file_type_all2str().c_str());
  return BenchmarkManager::e_file_type::NUM_TYPES;
}

std::string BenchmarkManager::cocotb_file_type2str(const BenchmarkManager::e_file_type& type,
                                                   const bool& verbose) const {
  if (!valid_cocotb_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for cocotb source file! Expect %s\n",
                   cocotb_file_type_all2str().c_str());
    return std::string();
  }
  return std::string(FILE_TYPE_STRING_[size_t(type)]);
}

std::string BenchmarkManager::cocotb_file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(BenchmarkManager::e_file_type::COCOTB_SOURCE);
       itype <= size_t(BenchmarkManager::e_file_type::COCOTB_MAKEFILE); ++itype) {
    full_types += std::string(FILE_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool BenchmarkManager::valid_cocotb_file_type(const e_file_type& type) const {
  return size_t(BenchmarkManager::e_file_type::COCOTB_SOURCE) <= size_t(type) &&
         size_t(type) <= size_t(BenchmarkManager::e_file_type::COCOTB_MAKEFILE);
}

BenchmarkManager::e_file_type BenchmarkManager::str2constraint_file_type(
  const std::string& str, const bool& verbose) const {
  for (int itype = size_t(BenchmarkManager::e_file_type::SDC);
       itype <= size_t(BenchmarkManager::e_file_type::PCF); ++itype) {
    if (str == std::string(FILE_TYPE_STRING_[itype])) {
      return static_cast<BenchmarkManager::e_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for constraint file! Expect %s\n",
                 constraint_file_type_all2str().c_str());
  return BenchmarkManager::e_file_type::NUM_TYPES;
}

std::string BenchmarkManager::constraint_file_type2str(const BenchmarkManager::e_file_type& type,
                                                       const bool& verbose) const {
  if (!valid_constraint_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for constraint file! Expect %s\n",
                   constraint_file_type_all2str().c_str());
    return std::string();
  }
  return std::string(FILE_TYPE_STRING_[size_t(type)]);
}

std::string BenchmarkManager::constraint_file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(BenchmarkManager::e_file_type::SDC);
       itype <= size_t(BenchmarkManager::e_file_type::PCF); ++itype) {
    full_types += std::string(FILE_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool BenchmarkManager::valid_constraint_file_type(const e_file_type& type) const {
  return size_t(BenchmarkManager::e_file_type::SDC) <= size_t(type) &&
         size_t(type) <= size_t(BenchmarkManager::e_file_type::PCF);
}

void BenchmarkManager::clear() {
  files_.clear();
  authors_.clear();
  date_created_.clear();
  date_last_modified_.clear();
  tool_version_.clear();
  suite_ids_.clear();
  suite_names_.clear();
  suite_benchmarks_.clear();
  suite_name2id_map_.clear();
  benchmark_ids_.clear();
  benchmark_disabled_.clear();
  benchmark_suite_ids_.clear();
  benchmark_names_.clear();
  benchmark_design_source_types_.clear();
  benchmark_design_source_files_.clear();
  benchmark_top_modules_.clear();
  benchmark_cocotb_types_.clear();
  benchmark_cocotb_files_.clear();
  benchmark_constraint_types_.clear();
  benchmark_constraint_files_.clear();
  is_valid_ = false;
}

std::string BenchmarkManager::replace_reserved_keywords(const std::string& val) const {
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

std::string BenchmarkManager::benchmark_suite_name(const BenchmarkId& bm_id) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  return suite_name(benchmark_suite_ids_[bm_id]);
}

int BenchmarkManager::try_set_benchmark_disabled(const std::string& bm_name, const bool& verbose) {
  BenchmarkId bm_id = find_benchmark_by_name(bm_name);
  if (valid_benchmark_id(bm_id)) {
    if (benchmark_disabled_[bm_id] == true) {
      VTR_LOG("Benchmark '%s' is already disabled. No changes will be applied\n", bm_name.c_str());
    } else {
      benchmark_disabled_[bm_id] = true;
      VTR_LOGV(verbose, "Successfully disabled benchmark '%s'\n", bm_name.c_str());
    }
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG("Invalid benchmark name '%s' which does not exist in any benchmark suite!\n",
          bm_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

bool BenchmarkManager::is_benchmark_disabled(const BenchmarkId& bm_id) const {
  check_valid();
  check_valid_benchmark_id(bm_id);
  return benchmark_disabled_[bm_id];
}

BenchmarkSuiteId BenchmarkManager::find_benchmark_suite(const std::string& benchmark_name) const {
  BenchmarkId bm_id = find_benchmark_by_name(benchmark_name);
  /* Not found, return invalid id */
  if (!bm_id) {
    return BenchmarkSuiteId::INVALID();
  }
  return benchmark_suite_ids_[bm_id];
}

}  // End namespace arcxpress
