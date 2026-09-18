#include "pdk_manager.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

PdkManager::PdkManager() {
  is_valid_ = false;
  FILE_TYPE_STRING_ = {"lib",         "gds",
                       "cdl",         "lef",
                       "verilog",     "tluplus",
                       "nxtgrd",      "empty_spice_subckt",
                       "innovus_lef", "redhawk_tech_file",
                       "layer_map",   "gds_layer_map"};
  VALID_SCLIB_FILE_TYPES_ = {PdkManager::e_file_type::LIB, PdkManager::e_file_type::GDS,
                             PdkManager::e_file_type::CDL, PdkManager::e_file_type::LEF,
                             PdkManager::e_file_type::VERILOG};
  VALID_TECH_FILE_TYPES_ = {PdkManager::e_file_type::TLUPLUS,
                            PdkManager::e_file_type::NXTGRD,
                            PdkManager::e_file_type::EMPTY_SPICE_SUBCKT,
                            PdkManager::e_file_type::INNOVUS_LEF,
                            PdkManager::e_file_type::REDHAWK_TECH_FILE,
                            PdkManager::e_file_type::LAYER_MAP,
                            PdkManager::e_file_type::GDS_LAYER_MAP};
  KW_PWD_ = "${PWD}";
  KW_ROOT_ = "${ROOT_PATH}";
  KW_PROJHOME_ = "${PROJ_PATH}";
  reserved_keywords_[KW_PWD_] = std::filesystem::current_path().string();
  reserved_keywords_[KW_ROOT_] = std::string();
  reserved_keywords_[KW_PROJHOME_] = std::string();
  shrink_factor_ = 1.0;
}

PdkManager::sclib_range PdkManager::sclibs() const {
  return vtr::make_range(sclib_ids_.begin(), sclib_ids_.end());
}

PdkManager::techfile_range PdkManager::techfiles() const {
  return vtr::make_range(techfile_ids_.begin(), techfile_ids_.end());
}

std::string PdkManager::name() const {
  check_valid();
  return name_;
}

std::string PdkManager::root() const {
  check_valid();
  return replace_reserved_keywords(root_);
}

float PdkManager::shrink_factor() const {
  check_valid();
  return shrink_factor_;
}

std::string PdkManager::author() const {
  check_valid();
  return author_;
}

std::string PdkManager::date_created() const {
  check_valid();
  return date_created_;
}

std::string PdkManager::date_last_modified() const {
  check_valid();
  return date_last_modified_;
}

std::string PdkManager::tool_version() const {
  check_valid();
  return tool_version_;
}

std::string PdkManager::sclib_name(const SclibId& sclib_id) const {
  check_valid();
  check_valid_sclib_id(sclib_id);
  return sclib_names_[sclib_id];
}

float PdkManager::sclib_cell_height(const SclibId& sclib_id) const {
  check_valid();
  check_valid_sclib_id(sclib_id);
  return sclib_cell_height_[sclib_id];
}

float PdkManager::sclib_cpp(const SclibId& sclib_id) const {
  check_valid();
  check_valid_sclib_id(sclib_id);
  return sclib_cpp_[sclib_id];
}

std::vector<SclibFileId> PdkManager::sclib_files(const SclibId& sclib_id) const {
  check_valid();
  check_valid_sclib_id(sclib_id);
  std::vector<SclibFileId> ret;
  ret.reserve(sclib_file_types_[sclib_id].size());
  for (size_t i = 0; i < sclib_file_types_[sclib_id].size(); ++i) {
    ret.push_back(SclibFileId(i));
  }
  return ret;
}

PdkManager::e_file_type PdkManager::sclib_file_type(const SclibId& sclib_id,
                                                    const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  return sclib_file_types_[sclib_id][size_t(f_id)];
}

std::string PdkManager::sclib_file_type_str(const SclibId& sclib_id,
                                            const SclibFileId& f_id) const {
  return file_type2str(sclib_file_type(sclib_id, f_id), true);
}

std::string PdkManager::sclib_file_tag(const SclibId& sclib_id, const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  return sclib_file_tags_[sclib_id][size_t(f_id)];
}

std::string PdkManager::sclib_file_process(const SclibId& sclib_id, const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  return sclib_file_process_[sclib_id][size_t(f_id)];
}

std::string PdkManager::sclib_file_vdd(const SclibId& sclib_id, const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  return sclib_file_vdds_[sclib_id][size_t(f_id)];
}

std::string PdkManager::sclib_file_temperature(const SclibId& sclib_id,
                                               const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  return sclib_file_temperatures_[sclib_id][size_t(f_id)];
}

std::string PdkManager::sclib_file_model_type(const SclibId& sclib_id,
                                              const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  return sclib_file_models_[sclib_id][size_t(f_id)];
}

std::string PdkManager::sclib_file_path(const SclibId& sclib_id, const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  return sclib_file_paths_[sclib_id][size_t(f_id)];
}

std::string PdkManager::sclib_file_fullpath(const SclibId& sclib_id,
                                            const SclibFileId& f_id) const {
  check_valid();
  check_valid_sclib_file_id(sclib_id, f_id);
  std::filesystem::path r_path = root();
  std::filesystem::path f_path = sclib_file_paths_[sclib_id][size_t(f_id)];
  return std::filesystem::path(r_path / f_path).string();
}

std::vector<std::string> PdkManager::sclib_files_by_type(const e_file_type& f_type) const {
  std::vector<std::string> ret;
  for (SclibId sclib_id : sclibs()) {
    for (SclibFileId& f_id : sclib_files(sclib_id)) {
      if (sclib_file_type(sclib_id, f_id) == f_type) {
        ret.push_back(sclib_file_fullpath(sclib_id, f_id));
      }
    }
  }
  return ret;
}

PdkManager::e_file_type PdkManager::techfile_type(const TechfileId& tf_id) const {
  check_valid();
  check_valid_techfile_id(tf_id);
  return techfile_types_[tf_id];
}

std::string PdkManager::techfile_type_str(const TechfileId& tf_id) const {
  return file_type2str(techfile_type(tf_id), true);
}

std::string PdkManager::techfile_metal_stack(const TechfileId& tf_id) const {
  check_valid();
  check_valid_techfile_id(tf_id);
  return techfile_metal_stacks_[tf_id];
}

std::string PdkManager::techfile_corner(const TechfileId& tf_id) const {
  check_valid();
  check_valid_techfile_id(tf_id);
  return techfile_corners_[tf_id];
}

std::string PdkManager::techfile_path(const TechfileId& tf_id) const {
  check_valid();
  check_valid_techfile_id(tf_id);
  return techfile_paths_[tf_id];
}

std::vector<std::string> PdkManager::metal_stacks() const {
  std::map<std::string, int> ret;
  for (TechfileId tf_id : techfile_ids_) {
    auto result = ret.find(techfile_metal_stacks_[tf_id]);
    if (result == ret.end()) {
      ret[techfile_metal_stacks_[tf_id]] = 0;
    } else {
      ret[techfile_metal_stacks_[tf_id]]++;
    }
  }
  std::vector<std::string> ret_vec;
  ret_vec.reserve(ret.size());
  for (auto it = ret.begin(); it != ret.end(); ++it) {
    ret_vec.push_back(it->first);
  }
  return ret_vec;
}

std::vector<std::string> PdkManager::corners(const std::string& m_stack) const {
  std::map<std::string, int> ret;
  for (TechfileId tf_id : techfile_ids_) {
    if (m_stack != techfile_metal_stacks_[tf_id]) {
      continue;
    }
    auto result = ret.find(techfile_corners_[tf_id]);
    if (result == ret.end()) {
      ret[techfile_corners_[tf_id]] = 0;
    } else {
      ret[techfile_corners_[tf_id]]++;
    }
  }
  std::vector<std::string> ret_vec;
  ret_vec.reserve(ret.size());
  for (auto it = ret.begin(); it != ret.end(); ++it) {
    ret_vec.push_back(it->first);
  }
  return ret_vec;
}

std::string PdkManager::techfile_path(const e_file_type& f_type, const std::string& m_stack,
                                      const std::string& corner) const {
  for (TechfileId tf_id : techfile_ids_) {
    if (techfile_types_[tf_id] == f_type && techfile_metal_stacks_[tf_id] == m_stack &&
        techfile_corners_[tf_id] == corner) {
      return techfile_paths_[tf_id];
    }
  }
  /* TODO: Error out? */
  return std::string("");
}

void PdkManager::set_name(const std::string& val) { name_ = val; }

void PdkManager::set_kw_proj_path(const std::string& val) {
  reserved_keywords_[KW_PROJHOME_] = val;
}

void PdkManager::set_kw_root_path(const std::string& val) { reserved_keywords_[KW_ROOT_] = val; }

void PdkManager::set_root(const std::string& val) { root_ = val; }

void PdkManager::set_shrink_factor(const float& val) { shrink_factor_ = val; }

void PdkManager::set_author(const std::string& val) { author_ = val; }

void PdkManager::set_date_created(const std::string& val) { date_created_ = val; }

void PdkManager::set_date_last_modified(const std::string& val) { date_last_modified_ = val; }

void PdkManager::set_tool_version(const std::string& val) { tool_version_ = val; }

SclibId PdkManager::create_sclib(const std::string& name) {
  /* Register the name, if existed, reuse the id. Otherwise, create a new one */
  SclibId sl_id = SclibId(sclib_ids_.size());
  auto result = sclib_name2id_map_.find(name);
  if (result != sclib_name2id_map_.end()) {
    VTR_LOG_WARN(
      "The standard cell library name '%s' already exist. File will be automatically appended to "
      "the existing one!\n",
      name.c_str());
    return result->second;
  }
  sclib_ids_.push_back(sl_id);
  sclib_names_.push_back(name);
  sclib_cell_height_.emplace_back(0.);
  sclib_cpp_.emplace_back(0.);
  sclib_file_types_.emplace_back();
  sclib_file_tags_.emplace_back();
  sclib_file_process_.emplace_back();
  sclib_file_vdds_.emplace_back();
  sclib_file_temperatures_.emplace_back();
  sclib_file_models_.emplace_back();
  sclib_file_paths_.emplace_back();
  sclib_name2id_map_[name] = sl_id;
  return sl_id;
}

SclibFileId PdkManager::add_sclib_file(const SclibId& sclib_id, const std::string& f_type,
                                       const std::string& f_path) {
  check_valid_sclib_id(sclib_id);
  SclibFileId ret = SclibFileId(sclib_file_types_[sclib_id].size());
  sclib_file_types_[sclib_id].push_back(str2file_type(f_type, true));
  sclib_file_tags_[sclib_id].emplace_back();
  sclib_file_process_[sclib_id].emplace_back();
  sclib_file_vdds_[sclib_id].emplace_back();
  sclib_file_temperatures_[sclib_id].emplace_back();
  sclib_file_models_[sclib_id].emplace_back();
  sclib_file_paths_[sclib_id].push_back(f_path);
  return ret;
}

void PdkManager::set_sclib_cell_height(const SclibId& sclib_id, const float& val) {
  check_valid_sclib_id(sclib_id);
  sclib_cell_height_[sclib_id] = val;
}

void PdkManager::set_sclib_cpp(const SclibId& sclib_id, const float& val) {
  check_valid_sclib_id(sclib_id);
  sclib_cpp_[sclib_id] = val;
}

void PdkManager::set_sclib_file_tag(const SclibId& sclib_id, const SclibFileId& f_id,
                                    const std::string& tag) {
  check_valid_sclib_file_id(sclib_id, f_id);
  sclib_file_tags_[sclib_id][size_t(f_id)] = tag;
}

void PdkManager::set_sclib_file_process(const SclibId& sclib_id, const SclibFileId& f_id,
                                        const std::string& process) {
  check_valid_sclib_file_id(sclib_id, f_id);
  sclib_file_process_[sclib_id][size_t(f_id)] = process;
}

void PdkManager::set_sclib_file_vdd(const SclibId& sclib_id, const SclibFileId& f_id,
                                    const std::string& vdd) {
  check_valid_sclib_file_id(sclib_id, f_id);
  sclib_file_vdds_[sclib_id][size_t(f_id)] = vdd;
}

void PdkManager::set_sclib_file_temperature(const SclibId& sclib_id, const SclibFileId& f_id,
                                            const std::string& temp) {
  check_valid_sclib_file_id(sclib_id, f_id);
  sclib_file_temperatures_[sclib_id][size_t(f_id)] = temp;
}

void PdkManager::set_sclib_file_model_type(const SclibId& sclib_id, const SclibFileId& f_id,
                                           const std::string& model) {
  check_valid_sclib_file_id(sclib_id, f_id);
  sclib_file_models_[sclib_id][size_t(f_id)] = model;
}

TechfileId PdkManager::add_techfile(const std::string& f_type, const std::string& f_path) {
  TechfileId tf_id = TechfileId(techfile_ids_.size());
  techfile_ids_.push_back(tf_id);
  techfile_types_.push_back(str2file_type(f_type, true));
  techfile_metal_stacks_.emplace_back();
  techfile_corners_.emplace_back();
  techfile_paths_.push_back(f_path);
  return tf_id;
}

void PdkManager::set_techfile_metal_stack(const TechfileId& tf_id, const std::string& m_stack) {
  check_valid_techfile_id(tf_id);
  techfile_metal_stacks_[tf_id] = m_stack;
}

void PdkManager::set_techfile_corner(const TechfileId& tf_id, const std::string& corner) {
  check_valid_techfile_id(tf_id);
  techfile_corners_[tf_id] = corner;
}

void PdkManager::clear() {
  name_.clear();
  root_.clear();
  author_.clear();
  date_created_.clear();
  date_last_modified_.clear();
  tool_version_.clear();
  sclib_ids_.clear();
  sclib_names_.clear();
  sclib_file_types_.clear();
  sclib_file_tags_.clear();
  sclib_file_process_.clear();
  sclib_file_vdds_.clear();
  sclib_file_temperatures_.clear();
  sclib_file_models_.clear();
  sclib_file_paths_.clear();
  sclib_name2id_map_.clear();
  techfile_ids_.clear();
  techfile_types_.clear();
  techfile_metal_stacks_.clear();
  techfile_corners_.clear();
  techfile_paths_.clear();
  shrink_factor_ = 1.0;
  is_valid_ = false;
}

bool PdkManager::valid_sclib_id(const SclibId& sclib_id) const {
  return ((size_t(sclib_id) < sclib_ids_.size()) && (sclib_id == sclib_ids_[sclib_id]));
}

bool PdkManager::valid_sclib_file_id(const SclibId& sclib_id, const SclibFileId& f_id) const {
  return valid_sclib_id(sclib_id) && (size_t(f_id) < sclib_file_types_[sclib_id].size());
}

bool PdkManager::valid_techfile_id(const TechfileId& tf_id) const {
  return ((size_t(tf_id) < techfile_ids_.size()) && (tf_id == techfile_ids_[tf_id]));
}

bool PdkManager::validate() const {
  is_valid_ = false;
  /* Must have at least 1 time stamp */
  size_t num_err = 0;
  if (name_.empty()) {
    VTR_LOG_ERROR("Pdk has no name!\n");
    num_err++;
  }
  if (root_.empty()) {
    VTR_LOG_ERROR("Pdk has no root path!\n");
    num_err++;
  }
  if (author_.empty()) {
    VTR_LOG_ERROR("Pdk has no author!\n");
    num_err++;
  }
  if (date_created_.empty()) {
    VTR_LOG_ERROR("Pdk has no date creation!\n");
    num_err++;
  }
  if (date_last_modified_.empty()) {
    VTR_LOG_ERROR("Pdk has no date last modified!\n");
    num_err++;
  }
  if (tool_version_.empty()) {
    VTR_LOG_ERROR("Pdk has no tool version!\n");
    num_err++;
  }
  if (shrink_factor_ > 1.0) {
    VTR_LOG_ERROR("Invalid shrink factor '%g' which should be (0, 1]!\n", shrink_factor_);
    num_err++;
  }
  if (sclib_ids_.empty()) {
    VTR_LOG_ERROR("Pdk has no standard cell libraries!\n");
    num_err++;
  }
  /* Each sclib should have required file */
  for (SclibId sl_id : sclib_ids_) {
    /* Must have valid height and cpp */
    if (sclib_cell_height_[sl_id] == 0.) {
      VTR_LOG_ERROR("Standard cell library '%s' of pdk '%s' has an invalid cell height ('%f')\n",
                    sclib_names_[sl_id].c_str(), name_.c_str(), sclib_cell_height_[sl_id]);
      num_err++;
    }
    if (sclib_cpp_[sl_id] == 0.) {
      VTR_LOG_ERROR(
        "Standard cell library '%s' of pdk '%s' has an invalid Contact Poly Pitch ('%f')\n",
        sclib_names_[sl_id].c_str(), name_.c_str(), sclib_cpp_[sl_id]);
      num_err++;
    }
    std::map<PdkManager::e_file_type, size_t> f_type_scoreboard;
    for (size_t iscf = 0; iscf < sclib_file_types_[sl_id].size(); ++iscf) {
      auto result = f_type_scoreboard.find(sclib_file_types_[sl_id][iscf]);
      if (result == f_type_scoreboard.end()) {
        f_type_scoreboard[sclib_file_types_[sl_id][iscf]] = 1;
      } else {
        f_type_scoreboard[sclib_file_types_[sl_id][iscf]]++;
      }
    }
    /* At least one liberty file should be defined! */
    /* At least one gds file should be defined! */
    /* At least one cdl file should be defined! */
    /* At least one lef file should be defined! */
    for (PdkManager::e_file_type valid_f_type : VALID_SCLIB_FILE_TYPES_) {
      if (f_type_scoreboard[valid_f_type] == 0) {
        VTR_LOG_ERROR("Standard cell library '%s' of pdk '%s' has no %s files\n",
                      sclib_names_[sl_id].c_str(), name_.c_str(),
                      file_type2str(valid_f_type).c_str());
        num_err++;
      }
    }
    /* Each liberty file should have valid pvt corner and model type */
    for (size_t iscf = 0; iscf < sclib_file_types_[sl_id].size(); ++iscf) {
      if (sclib_file_paths_[sl_id][iscf].empty()) {
        VTR_LOG_ERROR("Empty file path is found for \npdk: %s\nStandard cell library: %s\n",
                      name_.c_str(), sclib_names_[sl_id].c_str());
        num_err++;
      }
      if (PdkManager::e_file_type::LIB != sclib_file_types_[sl_id][iscf]) {
        continue;
      }
      if (sclib_file_tags_[sl_id][iscf].empty()) {
        VTR_LOG_ERROR(
          "Requires definition of tag is missing for \npdk: %s\nStandard cell library: "
          "%s\nLiberty file %s\n",
          name_.c_str(), sclib_names_[sl_id].c_str(), sclib_file_paths_[sl_id][iscf].c_str());
        num_err++;
      }
      if (sclib_file_process_[sl_id][iscf].empty()) {
        VTR_LOG_ERROR(
          "Requires definition of process is missing for \npdk: %s\nStandard cell library: "
          "%s\nLiberty file %s\n",
          name_.c_str(), sclib_names_[sl_id].c_str(), sclib_file_paths_[sl_id][iscf].c_str());
        num_err++;
      }
      if (sclib_file_vdds_[sl_id][iscf].empty()) {
        VTR_LOG_ERROR(
          "Requires definition of vdd is missing for \npdk: %s\nStandard cell library: %s\nLiberty "
          "file %s\n",
          name_.c_str(), sclib_names_[sl_id].c_str(), sclib_file_paths_[sl_id][iscf].c_str());
        num_err++;
      }
      if (sclib_file_temperatures_[sl_id][iscf].empty()) {
        VTR_LOG_ERROR(
          "Requires definition of temperature is missing for \npdk: %s\nStandard cell library: "
          "%s\nLiberty file %s\n",
          name_.c_str(), sclib_names_[sl_id].c_str(), sclib_file_paths_[sl_id][iscf].c_str());
        num_err++;
      }
      if (sclib_file_models_[sl_id][iscf].empty()) {
        VTR_LOG_ERROR(
          "Requires definition of model type is missing for \npdk: %s\nStandard cell library: "
          "%s\nLiberty file %s\n",
          name_.c_str(), sclib_names_[sl_id].c_str(), sclib_file_paths_[sl_id][iscf].c_str());
        num_err++;
      }
    }
  }
  /* Each techfile should has a valid metal stack */
  for (TechfileId tf_id : techfile_ids_) {
    if (techfile_paths_[tf_id].empty()) {
      VTR_LOG_ERROR("Empty file path is found for \npdk: %s\nTech file type: %s\n", name_.c_str(),
                    file_type2str(techfile_types_[tf_id]).c_str());
      num_err++;
    }
    if (techfile_metal_stacks_[tf_id].empty()) {
      VTR_LOG_ERROR("No metal stack is defined for \npdk: %s\nTech file type: %s\n", name_.c_str(),
                    file_type2str(techfile_types_[tf_id]).c_str());
      num_err++;
    }
    /* Techfile has a limited set of types */
    if (std::find(VALID_TECH_FILE_TYPES_.begin(), VALID_TECH_FILE_TYPES_.end(),
                  techfile_types_[tf_id]) == VALID_TECH_FILE_TYPES_.end()) {
      VTR_LOG_ERROR("Invalid file type '%s' is defined as tech file for \npdk: %s\n",
                    file_type2str(techfile_types_[tf_id]).c_str(), name_.c_str());
      num_err++;
    }
  }
  /* TODO: Under a specific metal stack and corner, there should be a list of required file types */
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err == 0;
}

void PdkManager::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Pdk data is invalid!");
    VTR_LOG("Check the data sources where it is loaded from!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void PdkManager::check_valid_sclib_id(const SclibId& sclib_id) const {
  if (!valid_sclib_id(sclib_id)) {
    VTR_LOG_ERROR("Pdk standard cell library id '%ld' is invalid!", size_t(sclib_id));
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void PdkManager::check_valid_sclib_file_id(const SclibId& sclib_id, const SclibFileId& f_id) const {
  if (!valid_sclib_file_id(sclib_id, f_id)) {
    VTR_LOG_ERROR("Pdk standard cell library '%s' file id '%ld' is invalid!",
                  sclib_names_[sclib_id].c_str(), size_t(f_id));
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void PdkManager::check_valid_techfile_id(const TechfileId& tf_id) const {
  if (!valid_techfile_id(tf_id)) {
    VTR_LOG_ERROR("Pdk tech file id '%ld' is invalid!", size_t(tf_id));
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

PdkManager::e_file_type PdkManager::str2file_type(const std::string& str,
                                                  const bool& verbose) const {
  for (int itype = size_t(PdkManager::e_file_type::LIB);
       itype != size_t(PdkManager::e_file_type::NUM_TYPES); ++itype) {
    if (str == std::string(FILE_TYPE_STRING_[itype])) {
      return static_cast<PdkManager::e_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid file type! Expect %s\n", file_type_all2str().c_str());
  return PdkManager::e_file_type::NUM_TYPES;
}

std::string PdkManager::file_type2str(const PdkManager::e_file_type& type,
                                      const bool& verbose) const {
  if (!valid_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid file type! Expect %s\n", file_type_all2str().c_str());
    return std::string();
  }
  return std::string(FILE_TYPE_STRING_[size_t(type)]);
}

std::string PdkManager::file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(PdkManager::e_file_type::LIB);
       itype != size_t(PdkManager::e_file_type::NUM_TYPES); ++itype) {
    full_types += std::string(FILE_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool PdkManager::valid_file_type(const e_file_type& type) const {
  return type != PdkManager::e_file_type::NUM_TYPES;
}

std::string PdkManager::replace_reserved_keywords(const std::string& val) const {
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

SclibId PdkManager::find_sclib_id_by_name(const std::string& sclib_name) const {
  check_valid();
  auto result = sclib_name2id_map_.find(sclib_name);
  if (result == sclib_name2id_map_.end()) {
    return SclibId::INVALID();
  }
  return result->second;
}

float PdkManager::find_sclib_height_by_name(const std::string& sclib_name) const {
  check_valid();
  SclibId sclib_id = find_sclib_id_by_name(sclib_name);
  if (!valid_sclib_id(sclib_id)) {
    VTR_LOG_ERROR("Pdk standard cell library name '%s' is invalid!", sclib_name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return sclib_cell_height_[sclib_id];
}

float PdkManager::find_sclib_cpp_by_name(const std::string& sclib_name) const {
  check_valid();
  SclibId sclib_id = find_sclib_id_by_name(sclib_name);
  if (!valid_sclib_id(sclib_id)) {
    VTR_LOG_ERROR("Pdk standard cell library name '%s' is invalid!", sclib_name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return sclib_cpp_[sclib_id];
}

}  // End namespace arkangel
