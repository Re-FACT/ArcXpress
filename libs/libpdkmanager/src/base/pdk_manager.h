#ifndef PDK_MANAGER_H
#define PDK_MANAGER_H

#include <array>
#include <map>
#include <string>
#include <vector>

#include "pdk_manager_fwd.h"
#include "vtr_vector.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class PdkManager {
 public: /* -- Types -- */
  /* The type of design source files */
  enum class e_file_type {
    LIB = 0,
    GDS,
    CDL,
    LEF,
    VERILOG,
    TLUPLUS,
    NXTGRD,
    EMPTY_SPICE_SUBCKT,
    INNOVUS_LEF,
    REDHAWK_TECH_FILE,
    LAYER_MAP,
    GDS_LAYER_MAP,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };

 public: /*  -- Ranges -- */
  typedef vtr::vector<SclibId, SclibId>::const_iterator sclib_iterator;
  typedef vtr::vector<TechfileId, TechfileId>::const_iterator techfile_iterator;
  typedef vtr::Range<sclib_iterator> sclib_range;
  typedef vtr::Range<techfile_iterator> techfile_range;

 public: /* -- Constructors -- */
  PdkManager();
  /* Not allow to copy and copy assignment operator. Keep the copy unique */
  PdkManager(const PdkManager&) = delete;
  void operator=(const PdkManager&) = delete;

 public: /* -- Aggregators -- */
  sclib_range sclibs() const;
  techfile_range techfiles() const;

 public: /* -- Accessors -- */
  std::string name() const;
  std::string root() const;
  float shrink_factor() const;

  std::string author() const;
  std::string date_created() const;
  std::string date_last_modified() const;
  std::string tool_version() const;

  SclibId find_sclib_id_by_name(const std::string& sclib_name) const;
  std::string sclib_name(const SclibId& sclib_id) const;
  float sclib_cell_height(const SclibId& sclib_id) const;
  float sclib_cpp(const SclibId& sclib_id) const;
  float find_sclib_height_by_name(const std::string& sclib_name) const;
  float find_sclib_cpp_by_name(const std::string& sclib_name) const;
  std::vector<SclibFileId> sclib_files(const SclibId& sclib_id) const;
  std::string sclib_file_tag(const SclibId& sclib_id, const SclibFileId& f_id) const;
  e_file_type sclib_file_type(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::string sclib_file_type_str(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::string sclib_file_process(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::string sclib_file_vdd(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::string sclib_file_temperature(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::string sclib_file_model_type(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::string sclib_file_path(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::string sclib_file_fullpath(const SclibId& sclib_id, const SclibFileId& f_id) const;
  std::vector<std::string> sclib_files_by_type(const e_file_type& f_type) const;

  e_file_type techfile_type(const TechfileId& tf_id) const;
  std::string techfile_type_str(const TechfileId& tf_id) const;
  std::string techfile_metal_stack(const TechfileId& tf_id) const;
  std::string techfile_corner(const TechfileId& tf_id) const;
  std::string techfile_path(const TechfileId& tf_id) const;
  /* List all the metal stacks defined in the tech file list */
  std::vector<std::string> metal_stacks() const;
  /* List all the corners under a given metal stack, defined in the tech file list */
  std::vector<std::string> corners(const std::string& m_stack) const;
  /* Get the specified file path with given metal stack and corner */
  std::string techfile_path(const e_file_type& f_type, const std::string& m_stack,
                            const std::string& corner) const;

 public: /* -- Mutators -- */
  void set_name(const std::string& val);
  /* This is to change the internal keyword which will be applied to the root value */
  void set_kw_proj_path(const std::string& val);
  void set_kw_root_path(const std::string& val);
  void set_root(const std::string& val);
  void set_shrink_factor(const float& val);
  void set_author(const std::string& val);
  void set_date_created(const std::string& val);
  void set_date_last_modified(const std::string& val);
  void set_tool_version(const std::string& val);

  SclibId create_sclib(const std::string& name);
  SclibFileId add_sclib_file(const SclibId& sclib_id, const std::string& f_type,
                             const std::string& f_path);
  void set_sclib_cell_height(const SclibId& sclib_id, const float& val);
  void set_sclib_cpp(const SclibId& sclib_id, const float& val);
  void set_sclib_file_tag(const SclibId& sclib_id, const SclibFileId& f_id, const std::string& tag);
  void set_sclib_file_process(const SclibId& sclib_id, const SclibFileId& f_id,
                              const std::string& process);
  void set_sclib_file_vdd(const SclibId& sclib_id, const SclibFileId& f_id, const std::string& vdd);
  void set_sclib_file_temperature(const SclibId& sclib_id, const SclibFileId& f_id,
                                  const std::string& temp);
  void set_sclib_file_model_type(const SclibId& sclib_id, const SclibFileId& f_id,
                                 const std::string& model);

  TechfileId add_techfile(const std::string& f_type, const std::string& f_path);
  void set_techfile_metal_stack(const TechfileId& tf_id, const std::string& m_stack);
  void set_techfile_corner(const TechfileId& tf_id, const std::string& corner);

  void clear();

 public: /* -- Validator -- */
  bool valid_sclib_id(const SclibId& sclib_id) const;
  bool valid_sclib_file_id(const SclibId& sclib_id, const SclibFileId& f_id) const;
  bool valid_techfile_id(const TechfileId& f_id) const;
  /* Validate if the content is legal, without the validate. Any data access will cause errors */
  bool validate() const;

 public: /* Public utility */
  e_file_type str2file_type(const std::string& str, const bool& verbose = false) const;
  std::string file_type2str(const e_file_type& type, const bool& verbose = false) const;
  std::string file_type_all2str() const;

 private: /* -- Private Validator -- */
  void check_valid() const;
  void check_valid_sclib_id(const SclibId& sclib_id) const;
  void check_valid_sclib_file_id(const SclibId& sclib_id, const SclibFileId& f_id) const;
  void check_valid_techfile_id(const TechfileId& tf_id) const;
  bool valid_file_type(const e_file_type& type) const;
  std::string replace_reserved_keywords(const std::string& val) const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::string name_;
  std::string root_;
  float shrink_factor_;

  /* Reserved keyword which will be applied when generating the path to benchmark and pdk config
   * files . For example, PWD -> getcwd() */
  std::map<std::string, std::string> reserved_keywords_;

  /* Time stamps */
  std::string author_;             /* Author of pdk file */
  std::string date_created_;       /* the date that created for the first time */
  std::string date_last_modified_; /* the date that is lastly modified */
  std::string tool_version_;       /* the version of ArkAngel that creates the project */

  /* A list of standard cell libs */
  vtr::vector<SclibId, SclibId> sclib_ids_;
  vtr::vector<SclibId, std::string> sclib_names_;
  vtr::vector<SclibId, float> sclib_cell_height_;
  vtr::vector<SclibId, float> sclib_cpp_;
  vtr::vector<SclibId, std::vector<e_file_type>> sclib_file_types_;
  vtr::vector<SclibId, std::vector<std::string>> sclib_file_tags_;
  vtr::vector<SclibId, std::vector<std::string>> sclib_file_process_;
  vtr::vector<SclibId, std::vector<std::string>> sclib_file_vdds_;
  vtr::vector<SclibId, std::vector<std::string>> sclib_file_temperatures_;
  vtr::vector<SclibId, std::vector<std::string>> sclib_file_models_;
  vtr::vector<SclibId, std::vector<std::string>> sclib_file_paths_;

  /* fast lookup */
  std::map<std::string, SclibId> sclib_name2id_map_;

  /* A list of tech files */
  vtr::vector<TechfileId, TechfileId> techfile_ids_;
  vtr::vector<TechfileId, e_file_type> techfile_types_;
  vtr::vector<TechfileId, std::string> techfile_metal_stacks_;
  vtr::vector<TechfileId, std::string> techfile_corners_;
  vtr::vector<TechfileId, std::string> techfile_paths_;

  /* Constants */
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> FILE_TYPE_STRING_;
  std::vector<e_file_type> VALID_SCLIB_FILE_TYPES_;
  std::vector<e_file_type> VALID_TECH_FILE_TYPES_;
  std::string KW_ROOT_;
  std::string KW_PWD_;
  std::string KW_PROJHOME_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress

#endif
