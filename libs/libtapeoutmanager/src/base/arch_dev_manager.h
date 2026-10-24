#pragma once

#include <map>
#include <string>
#include <vector>

#include "benchmark_packet.h"
#include "log_error_checker.h"
#include "tapeout_manager.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class ArchDevManager {
 public: /* -- Types -- */
  /* The type of step names */
  enum class e_fabric_type {
    DP = 0,
    ULTIMATE,
    NUM_TYPES /* Count the number of types, also this is an invalid type */
  };
  /* Define all the file types including input and outputs
   * The file type is used for file tracking during various development process/steps
   * IMPORTANT: the first 4 elements must match the e_file_type of TapeoutManager !!!
   */
  enum class e_file_type {
    VPR_ARCH = 0,               /* VPR architecture file */
    OPENFPGA_ARCH,              /* Openfpga architecture file */
    OPENFPGA_GNTWK_ARCH,        /* OpenFPGA global network architecture */
    OPENFPGA_CKBUF_LOC_MAP,     /* Openfpga default simulation setting file */
    DESIGN_VAR,                 /* Design variable file */
    OPENFPGA_SIM_SETTING,       /* Openfpga default simulation setting file */
    OPENFPGA_CKBUF_MAP,         /* Openfpga clock buffer cell map file */
    OPENFPGA_BITSTREAM_SETTING, /* Openfpga default simulation setting file */
    OPENFPGA_TILE_CONFIG,       /* Openfpga tile configuration file */
    OPENFPGA_PCF_XML, /* Openfpga XML-based PCF, this is a dummy version which is applied when
                         benchmark does not have one */
    VPR_SDC, /* Vpr sdc file, this is a dummy version which is applied when benchmark does not have
                one */
    OPENFPGA_PCF,       /* Openfpga XML-based PCF, this is a dummy version which is applied when
                               benchmark does not have one */
    CRAFTED_FABRIC_KEY, /* Handcrafted fabric key file by user */
    REF_FABRIC_KEY,     /* Reference fabric key file which is auto generated */
    FINAL_FABRIC_KEY, /* Final fabric key which is generated based on the handcrafted and reference
                       */
    REF_MODULE_NAME,  /* Reference module names which is auto generated */
    CRAFTED_MODULE_NAME,   /* Crafted module names by user */
    RRG,                   /* Routing resource graph, which is cached for fast vpr-run */
    PDL,                   /* Placement delay lookup, which is cached for fast vpr-run  */
    RLA,                   /* Router lookahead, which is cached for fast vpr-run */
    TILE_REF,              /* Tile reference file which is auto generated */
    FABRIC_HIERARCHY_FILE, /* Fabric hierarchy file which is used by physical design */
    IO_COORD,              /* Tile coordinate file which is auto generated */
    PIN_TABLE,             /* Pin table file which is auto generated */
    PIN_TABLE_NAMING_RULE, /* Pin table naming rules by user */
    ARCH_TIMING,
    VPR_FLOORPLAN_FILE, /* VPR floorplanning file */
    NUM_TYPES           /* Count the number of types, also this is an invalid type */
  };
  /* Type of scripts */
  enum class e_script_type {
    OPENFPGA_SCRIPT_BITGEN = 0,     /* Openfpga shell script for bitstream generation */
    OPENFPGA_SCRIPT_BASEBITGEN,     /* Openfpga shell script for bitstream generation */
    OPENFPGA_SCRIPT_DEVICEDATA,     /* Openfpga shell script for device data cache */
    OPENFPGA_SCRIPT_REFMODULENAMES, /* Openfpga shell script for reference module names generation
                                     */
    OPENFPGA_SCRIPT_REFFKEY,        /* Openfpga shell script for reference fabric key generation */
    OPENFPGA_SCRIPT_IOCOORD,        /* Openfpga shell script for io coordinate generation */
    OPENFPGA_SCRIPT_SETIO,          /* Openfpga shell script for io coordinate generation */
    NUM_TYPES                       /* Count the number of types, also this is an invalid type */
  };

 private: /* -- Types for internal use only -- */
 public:  /* -- Constructors -- */
  ArchDevManager(const TapeoutManager& to_mgr, const std::string& run_name);

 public: /* -- Aggregators -- */
  std::array<e_fabric_type, size_t(e_fabric_type::NUM_TYPES)> fabrics() const;

 public: /* -- Accessors -- */
  std::string release_fabric_name() const;
  std::string project_home() const;
  std::string arch_dir() const;
  std::string arch_dir_by_fabric(const e_fabric_type& fb_type) const;
  std::string pin_table_dir() const;
  std::string pin_table_dir_by_fabric(const e_fabric_type& fb_type) const;
  /* Print the path to the file and show the content */
  int add_crafted_file(const e_fabric_type& fb_type, const e_file_type& f_type,
                       const std::string& fname, const bool& verbose) const;
  int show_file(const e_fabric_type& fb_type, const e_file_type& f_type, const bool& verbose) const;
  /* Generate the path to input/output files for each step */
  std::string file(const e_fabric_type& fb_type, const e_file_type& f_type,
                   const std::string& corner, const bool& verbose) const;
  std::string openfpga_script_file(const e_script_type& runtime_type) const;
  /* Generate the path to temporary files under runtime directories for each step */
  std::string openfpga_runtime_file(const e_fabric_type& fb_type, const e_file_type& f_type,
                                    const e_script_type& runtime_type, const std::string& corner,
                                    const bool& verbose) const;
  std::string openfpga_runtime_script_file(const e_fabric_type& fb_type,
                                           const e_script_type& runtime_type,
                                           const std::string& corner, const bool& verbose) const;
  std::string openfpga_runtime_log_file(const e_fabric_type& fb_type,
                                        const e_script_type& runtime_type,
                                        const std::string& corner, const bool& verbose) const;
  /* Initialize the architecture directory with proper files */
  int init_arch_dir(const e_fabric_type& fb_type, const bool& verbose) const;
  int init_arch_dir_all(const bool& verbose) const;
  /* Generate I/O pin table */
  int generate_pin_table(const e_fabric_type& fb_type, const std::string& pin_table_gen_bin_path,
                         const bool& verbose) const;
  /* Save all the results and generate MD5 */
  std::vector<std::string> result_files(const TapeoutManager::e_substep_name& s_step,
                                        const bool& verbose) const;
  int save_results(const std::vector<TapeoutManager::e_substep_name>& s_steps,
                   const bool& verbose) const;
  std::string md5_yaml_file() const;
  int check_results(const bool& verbose) const;
  std::string default_corner() const;
  int show_init_module_name_status(const e_fabric_type& fb_type, const bool& verbose) const;
  int show_module_name_status(const e_fabric_type& fb_type, const bool& verbose) const;
  int show_io_status(const e_fabric_type& fb_type, const std::string& file_path,
                     const bool& verbose) const;

  int check_fabric_consistency(const bool& verbose) const;
  /* Export arch to a package */
  int export_arch(const std::string& fname, const bool& force_overwrite, const bool& verbose) const;
  /* Back-annotate timing */
  int merge_arch_timing(const std::string& corner,
                        const std::vector<std::string>& src_arch_timing_files,
                        const bool& verbose) const;
  /* Modify architecture parameters */
  int set_io_num(const e_side& curr_side, const ArchIoConstraints::e_io_direction& io_type,
                 const size_t& io_num, const bool& verbose);
  int set_sync_dp_dimension(const bool& val, const bool& verbose);
  int set_core_dimension(const size_t& height, const size_t& width, const bool& verbose);
  int set_num_clocks(const size_t& num_clks, const bool& verbose);
  int set_num_pclocks(const size_t& num_pclks, const bool& verbose);
  int set_num_config_regions(const size_t& num_regions, const bool& verbose);
  int set_num_resets(const size_t& num_rsts, const bool& verbose);

 public: /* -- Mutators -- */
  void set_route_chan_width(const size_t& val);
  bool add_file(const e_fabric_type& fb_type, const e_file_type& f_type, const std::string& f_name);
  bool add_openfpga_script(const e_script_type& script_type, const std::string& f_name);
  void add_arch_timing_file(const std::string& corner, const std::string& val);
  /* Must be called after add_arch_timing_file() !!! It will check if the corner name is a valid one
   * or not! */
  bool set_default_corner(const std::string& val);
  /* Run architecture generator */
  int exec_arch_gen(const bool& verbose);
  /* Generate device data */
  int generate_device_data(const e_fabric_type& fb_type, const BenchmarkPacket& bm_pkt,
                           const std::string& openfpga_bin_path, const std::string& corner,
                           const bool& verbose);
  int generate_device_data_all(const BenchmarkPacket& bm_pkt, const std::string& openfpga_bin_path,
                               const bool& verbose);
  /* Generate initial module names */
  int generate_reference_module_names(const e_fabric_type& fb_type, const BenchmarkPacket& bm_pkt,
                                      const std::string& openfpga_bin_path, const bool& verbose);
  int generate_reference_module_names_all(const BenchmarkPacket& bm_pkt,
                                          const std::string& openfpga_bin_path,
                                          const bool& verbose);
  /* Generate initial fabric key */
  int generate_reference_fabric_key(const e_fabric_type& fb_type, const BenchmarkPacket& bm_pkt,
                                    const std::string& openfpga_bin_path, const bool& verbose);
  int generate_reference_fabric_key_all(const BenchmarkPacket& bm_pkt,
                                        const std::string& openfpga_bin_path, const bool& verbose);
  /* Generate I/O coordinate */
  int generate_io_coordinate(const e_fabric_type& fb_type, const BenchmarkPacket& bm_pkt,
                             const std::string& openfpga_bin_path,
                             const std::string& fabric_key_assistant_bin_path, const bool& verbose);
  int generate_io_coordinate_all(const BenchmarkPacket& bm_pkt,
                                 const std::string& openfpga_bin_path,
                                 const std::string& fabric_key_assistant_bin_path,
                                 const bool& verbose);

 public: /* -- Validator -- */
  bool valid_fabric_type(const e_fabric_type& fb_type) const;
  bool valid_file_type(const e_file_type& f_type) const;
  bool valid_script_type(const e_script_type& f_type) const;

 public: /* Public utility */
  bool required_file_type(const TapeoutManager::e_arch_file_type& f_type) const;
  e_file_type convert_file_type(const TapeoutManager::e_arch_file_type& to_f_type) const;
  e_script_type convert_script_type(const TapeoutManager::e_script_type& to_f_type) const;
  bool required_script_type(const TapeoutManager::e_script_type& f_type) const;

  e_file_type str2file_type(const std::string& str, const bool& verbose = false) const;
  std::string file_type_all2str() const;

  e_fabric_type str2fabric_type(const std::string& str, const bool& verbose = false) const;
  std::string fabric_type2str(const e_fabric_type& type, const bool& verbose = false) const;
  std::string fabric_type_all2str() const;

 private: /* Private utility */
  void init(const std::string& root_path, const std::string& proj_name, const std::string& run_name,
            const std::string& openfpga_error_conf_file, const ArchIoConstraints& arch_io_constr,
            const std::string& arch_gen_bin_path, const std::string& py_bin_path);
  /* A complete process to copy a file from a source to a destination. Support to copy as a symbolic
   * link It will create directories if does not exist Sanity checks will be applied:
   * - Source file does not exist
   * - Destination file does not exist
   */
  int copy_file_with_mkdir(const std::string& src_file, const std::string& des_file,
                           const bool& symlink, const bool& verbose) const;
  int init_runtime_dir(const e_fabric_type& fb_type, const e_script_type& runtime_type,
                       const std::string& corner, const bool& verbose) const;
  int init_runtime_script(const e_fabric_type& fb_type, const e_script_type& runtime_type,
                          const BenchmarkPacket& bm_pkt, const std::string& corner,
                          const bool& verbose) const;
  int runtime_exec_openfpga(const e_fabric_type& fb_type, const e_script_type& runtime_type,
                            const std::string& openfpga_bin_path, const std::string& corner,
                            const bool& verbose);
  int check_result_files(const e_fabric_type& fb_type,
                         const std::vector<e_file_type>& f_types_to_check,
                         const std::string& corner, const bool& verbose) const;
  std::vector<std::string> corners() const;
  int find_vpr_layout_dimension(const e_fabric_type& fb_type, int& height, int& width,
                                const bool& verbose) const;

 private: /* -- Private Validator -- */
  void check_valid_fabric_type(const e_fabric_type& fb_type) const;
  void check_valid_file_type(const e_file_type& f_type) const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::string root_path_;
  std::string proj_name_;
  std::string run_name_;
  std::string arch_gen_bin_path_;

  /* Source file lists */
  std::string src_vpr_arch_file_;
  std::string src_openfpga_arch_file_;
  std::array<std::string, size_t(e_fabric_type::NUM_TYPES)> src_design_var_files_;
  std::array<std::string, size_t(e_fabric_type::NUM_TYPES)> src_openfpga_gntwk_files_;
  std::array<std::string, size_t(e_fabric_type::NUM_TYPES)> src_openfpga_ckbuf_loc_map_files_;
  std::array<std::string, size_t(e_script_type::NUM_TYPES)> src_openfpga_script_files_;
  std::map<e_file_type, std::string> src_openfpga_setting_files_;
  std::string default_corner_;
  std::map<std::string, std::string> src_arch_timing_files_;

  /* Arch variables */
  size_t route_chan_width_;
  const ArchIoConstraints* arch_io_constr_;
  vtr::Point<size_t> core_dimension_ = vtr::Point<size_t>(-1, -1);
  bool sync_dp_dimension_;
  size_t num_clocks_;
  size_t num_pclocks_;
  size_t num_config_regions_;
  size_t num_resets_;
  std::array<std::map<e_side, size_t>, size_t(ArchIoConstraints::e_io_direction::NUM_TYPES)>
    num_ios_;

  /* Constants */
  std::array<std::string, size_t(e_fabric_type::NUM_TYPES)> FABRIC_TYPE_STRING_;
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> FILE_NAMES_;
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> FILE_TAGS_;
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> FILE_TYPE_NAMES_;
  std::array<std::string, size_t(e_file_type::NUM_TYPES)> VAR_NAMES_;
  std::array<std::string, size_t(e_script_type::NUM_TYPES)> SCRIPT_NAMES_;
  std::array<std::string, size_t(e_script_type::NUM_TYPES)> SCRIPT_TYPE_STRING_;

  std::string PYTHON_COMMAND_;

  std::string ARCH_DIR_NAME_;
  std::string ARCH_VAR_FILE_;
  std::string OPENFPGA_RUNDIR_NAME_;
  std::string PINTABLE_RUNDIR_NAME_;
  std::string VPR_BLIF_VAR_NAME_;
  std::string VPR_FABRIC_VAR_NAME_;
  std::string VPR_ROUTE_CHAN_W_VAR_NAME_;
  std::string OPENFPGA_LOG_FILENAME_;
  std::string ARCH_GEN_LOG_FILENAME_;
  std::string MD5_FILENAME_;
  std::string ARCH_TIMING_FILE_PREFIX_;
  std::string ARCH_TIMING_FILE_POSTFIX_;

  LogErrorChecker openfpga_log_checker_;
  LogErrorChecker arch_gen_log_checker_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress
