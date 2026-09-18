#include "arch_dev_manager.h"

#include <filesystem>

#include "arch_aggregator.h"
#include "arkangel_constants.h"
#include "arkangel_file_os.h"
#include "arkangel_process_util.h"
#include "command_exit_codes.h"
#include "file_md5_manager.h"
#include "generate_pin_table_naming_rules.h"
#include "generate_tile_module_names.h"
#include "openfpga_digest.h"
#include "physical_types.h"
#include "rapidcsv.h"
#include "read_csv_io_pin_table.h"
#include "read_xml_arch_file.h"
#include "read_xml_module_name_map.h"
#include "read_yaml_tile_ref.h"
#include "read_yaml_tile_types.h"
#include "tableprinter.h"
#include "tile_ref.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

ArchDevManager::ArchDevManager(const TapeoutManager& to_mgr, const std::string& run_name) {
  init(to_mgr.path(), to_mgr.name(), run_name, to_mgr.openfpga_error_conf_file(),
       to_mgr.arch_io_constraints(), to_mgr.arch_gen_bin_path(), to_mgr.python_bin_path());
  /* Initialize arch settings */
  set_core_dimension(to_mgr.core_height(run_name), to_mgr.core_width(run_name), false);
  set_sync_dp_dimension(to_mgr.sync_dp_dimension(run_name), false);
  set_num_clocks(to_mgr.num_clocks(run_name), false);
  set_num_pclocks(to_mgr.num_prog_clocks(run_name), false);
  set_num_config_regions(to_mgr.num_config_regions(run_name), false);
  set_num_resets(to_mgr.num_resets(run_name), false);
  for (ArchIoConstraints::e_io_direction io_dir : arch_io_constr_->io_directions()) {
    for (e_side side : TOTAL_2D_SIDES) {
      int curr_num_io = to_mgr.io_num(run_name, side, io_dir);
      if (-1 != curr_num_io) {
        set_io_num(side, io_dir, curr_num_io, false);
      }
    }
  }
}

void ArchDevManager::init(const std::string& root_path, const std::string& proj_name,
                          const std::string& run_name, const std::string& openfpga_error_conf_file,
                          const ArchIoConstraints& arch_io_constr,
                          const std::string& arch_gen_bin_path, const std::string& py_bin_path) {
  root_path_ = root_path;
  proj_name_ = proj_name;
  run_name_ = run_name;
  arch_gen_bin_path_ = arch_gen_bin_path;
  arch_io_constr_ = &arch_io_constr;
  core_dimension_.set(0, 0);
  num_clocks_ = 0;
  num_resets_ = 0;
  is_valid_ = false;
  PYTHON_COMMAND_ = py_bin_path;
  ARCH_DIR_NAME_ = "arch";
  OPENFPGA_RUNDIR_NAME_ = "openfpga_tasks";
  PINTABLE_RUNDIR_NAME_ = "arch";
  ARCH_VAR_FILE_ = "arch_timing_tt_25C_0v9.yml";
  VPR_BLIF_VAR_NAME_ = "VPR_TESTBENCH_BLIF";
  VPR_FABRIC_VAR_NAME_ = "OPENFPGA_VPR_DEVICE_LAYOUT";
  VPR_ROUTE_CHAN_W_VAR_NAME_ = "OPENFPGA_VPR_ROUTE_CHAN_WIDTH";
  FABRIC_TYPE_STRING_ = {"dp", "ultimate"};
  FILE_NAMES_ = {"vpr_arch.xml",
                 "openfpga_arch.xml",
                 "openfpga_global_network.xml",
                 "ckbuf_loc_map.xml",
                 "design_variables.yml", /* Placeholder only */
                 "openfpga_sim_setting.xml",
                 "clk_cell_map.xml",
                 "openfpga_bitstream_setting.xml",
                 "tile_config.xml",
                 "gclk_pcf.xml",
                 "vpr_dummy.sdc",
                 "pcf_dummy.pcf",
                 "crafted_fabric_key.xml",
                 "reference_fabric_key.xml",
                 "fabric_key.xml",
                 "reference_module_names.xml",
                 "module_names.xml",
                 "rr_graph.bin",
                 "placement_delay_lookup.bin",
                 "router_lookahead.bin",
                 "tile_reference.yaml",
                 "fabric_hierarchy.yaml",
                 "io_coordinate.xml",
                 "efpga_io_pin_table.csv",
                 "pin_table_naming_rules.xml",
                 "arch timing data file",
                 "vpr_arch.svg"};
  FILE_TYPE_NAMES_ = {"VPR architecture file",
                      "OpenFPGA architecture file",
                      "OpenFPGA global network architecture file",
                      "OpenFPGA clock buffer location file",
                      "Architecture variable file", /* Placeholder only */
                      "OpenFPGA simulation setting file",
                      "OpenFPGA ckbuf cell map file",
                      "OpenFPGA bitstream setting file",
                      "OpenFPGA tile configuration file",
                      "OpenFPGA XML-based Pin Constraint file",
                      "VPR SDC file",
                      "OpenFPGA Pin Constraint file",
                      "Handcrafted fabric key file",
                      "Reference fabric key file",
                      "Final fabric key file",
                      "Reference module name file",
                      "Final module name file",
                      "Routing resource graph file",
                      "Placement delay matrix file",
                      "Router lookahead file",
                      "Tile reference file",
                      "Fabric hierarchy file",
                      "I/O coordinate file",
                      "I/O pin table file",
                      "I/O pin table naming rule file",
                      "arch timing data file",
                      "vpr floorplanning figure"};
  FILE_TAGS_ = {"vpr_arch",
                "openfpga_arch",
                "openfpga_global_network_arch",
                "ckbuf_loc_map",
                "architecture_variables", /* Placeholder only */
                "simulation_setting",
                "ckbuf_cell_map",
                "bitstream_setting",
                "tile_config",
                "default_pcf_xml",
                "default_vpr_sdc",
                "default_pcf",
                "crafted_fabric_key",
                "reference_fabric_key",
                "fabric_key",
                "init_module_names",
                "module_names",
                "routing_resource_graph",
                "placement_delay_matrix",
                "router_lookahead",
                "tile_references",
                "fabric_hierarchy",
                "io_coorindate",
                "io_pin_table",
                "io_pin_table_naming_rules",
                "arch timing data file",
                "arch_floorplan"};
  VAR_NAMES_ = {"VPR_ARCH_FILE",
                "OPENFPGA_ARCH_FILE",
                "OPENFPGA_CLOCK_ARCH_FILE",
                "OPENFPGA_CKBUF_LOC_MAP_FILE",
                "DESIGN_VAR_YAML", /* Placeholder only */
                "OPENFPGA_SIM_SETTING_FILE",
                "OPENFPGA_CKBUF_MAP_FILE",
                "OPENFPGA_BITSTREAM_SETTING_FILE",
                "OPENFPGA_TILE_CONFIG_FILE",
                "OPENFPGA_PIN_CONSTRAINTS_FILE",
                "OPENFPGA_VPR_SDC",
                "OPENFPGA_PCF",
                "CRAFTED_FABRIC_KEY_FILE",
                "REF_FABRIC_KEY_FILE",
                "EXTERNAL_FABRIC_KEY_FILE",
                "REF_MODULE_NAME_FILE",
                "CRAFTED_MODULE_NAME_FILE",
                "OPENFPGA_VPR_RR_GRAPH_FILE",
                "OPENFPGA_VPR_PLACEMENT_DELAY_LOOKUP_FILE",
                "OPENFPGA_VPR_ROUTER_LOOKAHEAD_FILE",
                "OPENFPGA_TILE_REFERENCE_FILE",
                "OPENFPGA_FABRIC_HIERARCHY_FILE",
                "OPENFPGA_IO_COORDINATE_FILE",
                "EFPGA_IO_PIN_TABLE",
                "PIN_TABLE_NAMING_RULES",
                "arch timing data file",
                "OPENFPGA_VPR_FLOORPLAN_FILE"};
  SCRIPT_NAMES_ = {"generate_bitstream.openfpga",
                   "generate_base_bitstream.openfpga",
                   "generate_device_data.openfpga",
                   "generate_init_module_names.openfpga",
                   "generate_init_fabric_key.openfpga",
                   "generate_io_coordinate.openfpga",
                   "set_io_num.openfpga"};
  SCRIPT_TYPE_STRING_ = {"generate_bitstream",
                         "generate_base_bitstream",
                         "generate_device_data",
                         "generate_init_module_names",
                         "generate_init_fabric_key",
                         "generate_io_coordinate",
                         "set_io_num"};
  OPENFPGA_LOG_FILENAME_ = "openfpga.log";
  ARCH_GEN_LOG_FILENAME_ = "arch_gen.log";
  MD5_FILENAME_ = "results_md5.yaml";
  ARCH_TIMING_FILE_PREFIX_ = "arch_timing_";
  ARCH_TIMING_FILE_POSTFIX_ = ".yaml";
  if (openfpga::CMD_EXEC_SUCCESS !=
      openfpga_log_checker_.load_keywords_from_xml(openfpga_error_conf_file, false)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (openfpga::CMD_EXEC_SUCCESS !=
      arch_gen_log_checker_.load_keywords_from_xml(openfpga_error_conf_file, false)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::array<ArchDevManager::e_fabric_type, size_t(ArchDevManager::e_fabric_type::NUM_TYPES)>
ArchDevManager::fabrics() const {
  return {ArchDevManager::e_fabric_type::DP, ArchDevManager::e_fabric_type::ULTIMATE};
}

std::string ArchDevManager::release_fabric_name() const {
  return FABRIC_TYPE_STRING_[size_t(ArchDevManager::e_fabric_type::ULTIMATE)];
}

std::string ArchDevManager::project_home() const {
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(ARCH_DIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_);
  return proj_home.string();
}

std::string ArchDevManager::arch_dir() const {
  std::filesystem::path ret = project_home() / std::filesystem::path(ARCH_DIR_NAME_);
  return ret.string();
}

std::string ArchDevManager::arch_dir_by_fabric(const e_fabric_type& fb_type) const {
  std::filesystem::path ret = arch_dir();
  ret = ret / std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]);
  return ret.string();
}

std::string ArchDevManager::pin_table_dir() const {
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(ARCH_DIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_);
  proj_home = proj_home / std::filesystem::path(PINTABLE_RUNDIR_NAME_);
  return proj_home.string();
}

std::string ArchDevManager::pin_table_dir_by_fabric(const e_fabric_type& fb_type) const {
  std::filesystem::path ret = pin_table_dir();
  ret = ret / std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]);
  return ret.string();
}

int ArchDevManager::show_file(const e_fabric_type& fb_type, const e_file_type& f_type,
                              const bool& verbose) const {
  int num_err = 0;
  std::string fpath = file(fb_type, f_type, default_corner_, verbose);
  if (f_type == e_file_type::VPR_FLOORPLAN_FILE) {
#ifndef NO_GRAPHICS
    VTR_LOG("\nSkip to show figure content here. Suggest to open it with a graphic viewer\n");
    if (!std::filesystem::exists(fpath)) {
      VTR_LOG_ERROR("File '%s' does not exist!\n", fpath.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("The figure is located:\n\t%s\n", fpath.c_str());
#else
    VTR_LOG_WARN(
      "\nNo figures are available due to VPR graphics are disable. Please ensure related options "
      "are on during compilation\n");
#endif
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG("The architecture data file is located:\n\t%s\n", fpath.c_str());
  if (!std::filesystem::exists(fpath)) {
    VTR_LOG_ERROR("File '%s' does not exist!\n", fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::ifstream fcntt(fpath);
  std::streampos fhead_pos = fcntt.tellg();
  size_t line_cnt =
    std::count(std::istreambuf_iterator<char>(fcntt), std::istreambuf_iterator<char>(), '\n');
  if (line_cnt > 50) {
    VTR_LOG(
      "\nSkip to show file content as it is too large. Suggest to open it with an editor and "
      "view\n");
    return openfpga::CMD_EXEC_SUCCESS;
  }

  if (!fcntt.is_open()) {
    VTR_LOG_ERROR("Failed to open the file!\n");
    num_err++;
  }
  VTR_LOG("\nFile content is shown as follows:\n");
  std::string line;
  fcntt.seekg(fhead_pos);
  while (std::getline(fcntt, line)) {
    VTR_LOG("%s\n", line.c_str());
  }
  fcntt.close();

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

std::string ArchDevManager::file(const e_fabric_type& fb_type, const e_file_type& f_type,
                                 const std::string& corner, const bool& verbose) const {
  check_valid_fabric_type(fb_type);
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(ARCH_DIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_);
  std::filesystem::path ret;
  switch (f_type) {
    case e_file_type::VPR_ARCH:
    case e_file_type::OPENFPGA_ARCH:
    case e_file_type::OPENFPGA_GNTWK_ARCH:
    case e_file_type::OPENFPGA_CKBUF_LOC_MAP:
    case e_file_type::DESIGN_VAR:
    case e_file_type::CRAFTED_FABRIC_KEY:
    case e_file_type::REF_FABRIC_KEY:
    case e_file_type::FINAL_FABRIC_KEY:
    case e_file_type::REF_MODULE_NAME:
    case e_file_type::CRAFTED_MODULE_NAME:
    case e_file_type::FABRIC_HIERARCHY_FILE:
    case e_file_type::TILE_REF:
    case e_file_type::VPR_FLOORPLAN_FILE:
    case e_file_type::OPENFPGA_BITSTREAM_SETTING:
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::RRG:
    case e_file_type::PDL:
    case e_file_type::RLA:
      /* Device data that contains timing data are corner dependent */
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(corner + "_" + FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::IO_COORD:
    case e_file_type::PIN_TABLE:
    case e_file_type::PIN_TABLE_NAMING_RULE:
      ret = proj_home / std::filesystem::path(PINTABLE_RUNDIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    /* External files should come from device data navigator */
    case e_file_type::OPENFPGA_SIM_SETTING:
    case e_file_type::OPENFPGA_CKBUF_MAP:
    case e_file_type::OPENFPGA_TILE_CONFIG:
    case e_file_type::OPENFPGA_PCF_XML:
    case e_file_type::VPR_SDC:
    case e_file_type::OPENFPGA_PCF:
      ret = std::filesystem::path(src_openfpga_setting_files_.at(f_type));
      break;
    case e_file_type::ARCH_TIMING: {
      std::filesystem::path l_file =
        std::filesystem::path(arch_dir()) /
        std::filesystem::path(FABRIC_TYPE_STRING_[size_t(e_fabric_type::ULTIMATE)]);
      std::string l_fname =
        std::string(ARCH_TIMING_FILE_PREFIX_) + corner + std::string(ARCH_TIMING_FILE_POSTFIX_);
      ret = l_file / std::filesystem::path(l_fname);
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid file type!\n");
  }
  return ret.string().c_str();
}

std::string ArchDevManager::openfpga_script_file(const e_script_type& runtime_type) const {
  return std::filesystem::path(src_openfpga_script_files_[size_t(runtime_type)]).string();
}

std::string ArchDevManager::openfpga_runtime_file(const e_fabric_type& fb_type,
                                                  const e_file_type& f_type,
                                                  const e_script_type& runtime_type,
                                                  const std::string& corner,
                                                  const bool& verbose) const {
  check_valid_fabric_type(fb_type);
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(ARCH_DIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_) /
              std::filesystem::path(OPENFPGA_RUNDIR_NAME_);
  if (f_type == e_file_type::VPR_ARCH || f_type == e_file_type::OPENFPGA_ARCH) {
    std::filesystem::path ret =
      proj_home / std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
      std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
      std::filesystem::path(corner) / std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
    return ret.string();
  }
  return "";
}

std::string ArchDevManager::openfpga_runtime_script_file(const e_fabric_type& fb_type,
                                                         const e_script_type& runtime_type,
                                                         const std::string& corner,
                                                         const bool& verbose) const {
  check_valid_fabric_type(fb_type);
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(ARCH_DIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_) /
              std::filesystem::path(OPENFPGA_RUNDIR_NAME_);
  std::filesystem::path ret =
    proj_home / std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
    std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
    std::filesystem::path(corner) / std::filesystem::path(SCRIPT_NAMES_[size_t(runtime_type)]);
  return ret.string();
}

std::string ArchDevManager::openfpga_runtime_log_file(const e_fabric_type& fb_type,
                                                      const e_script_type& runtime_type,
                                                      const std::string& corner,
                                                      const bool& verbose) const {
  check_valid_fabric_type(fb_type);
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(ARCH_DIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_) /
              std::filesystem::path(OPENFPGA_RUNDIR_NAME_);
  std::filesystem::path ret =
    proj_home / std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
    std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
    std::filesystem::path(corner) /
    std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)] + OPENFPGA_LOG_FILENAME_);
  return ret.string();
}

int ArchDevManager::copy_file_with_mkdir(const std::string& src_file, const std::string& des_file,
                                         const bool& sym_link, const bool& verbose) const {
  int num_err = 0;
  if (!std::filesystem::exists(src_file)) {
    VTR_LOG_ERROR("Source file '%s' does not exist!\n", src_file.c_str());
    num_err++;
  }
  VTR_LOGV(verbose, "Copying/link file\n");
  VTR_LOGV(verbose, "\tFrom: %s\n", src_file.c_str());
  VTR_LOGV(verbose, "\tTo:   %s\n", des_file.c_str());

  /* Create directory for des */
  std::filesystem::path des_dirpath = des_file;
  openfpga::create_directory(des_dirpath.parent_path().string(), true, verbose);
  if (!sym_link) {
    std::filesystem::copy(src_file, des_file, std::filesystem::copy_options::overwrite_existing);
  } else {
    if (std::filesystem::exists(des_file)) {
      std::filesystem::remove_all(des_file);
    }
    num_err += universal_create_symlink(src_file, des_file) ? 0 : 1;
  }
  if (!std::filesystem::exists(des_file)) {
    VTR_LOG_ERROR("Destination file '%s' does not exist!\n", des_file.c_str());
    num_err++;
  }
  return num_err;
}

int ArchDevManager::init_arch_dir(const e_fabric_type& fb_type, const bool& verbose) const {
  int num_err = 0;
  /* Copy source file to destination */
  if (fb_type == e_fabric_type::DP) {
    num_err += copy_file_with_mkdir(src_vpr_arch_file_,
                                    file(fb_type, e_file_type::VPR_ARCH, default_corner_, verbose),
                                    false, verbose);
    num_err += copy_file_with_mkdir(
      src_openfpga_arch_file_, file(fb_type, e_file_type::OPENFPGA_ARCH, default_corner_, verbose),
      false, verbose);
  } else {
    num_err += copy_file_with_mkdir(
      file(e_fabric_type::DP, e_file_type::VPR_ARCH, default_corner_, verbose),
      file(fb_type, e_file_type::VPR_ARCH, default_corner_, verbose), true, verbose);
    num_err += copy_file_with_mkdir(
      file(e_fabric_type::DP, e_file_type::OPENFPGA_ARCH, default_corner_, verbose),
      file(fb_type, e_file_type::OPENFPGA_ARCH, default_corner_, verbose), true, verbose);
    /* Copy all the arch timing files with a given name */
    for (auto pair : src_arch_timing_files_) {
      num_err += copy_file_with_mkdir(
        pair.second, file(fb_type, e_file_type::ARCH_TIMING, pair.first, verbose), false, verbose);
    }
  }
  num_err += copy_file_with_mkdir(src_design_var_files_[size_t(fb_type)],
                                  file(fb_type, e_file_type::DESIGN_VAR, default_corner_, verbose),
                                  false, verbose);
  num_err += copy_file_with_mkdir(
    src_openfpga_gntwk_files_[size_t(fb_type)],
    file(fb_type, e_file_type::OPENFPGA_GNTWK_ARCH, default_corner_, verbose), false, verbose);
  num_err += copy_file_with_mkdir(
    src_openfpga_ckbuf_loc_map_files_[size_t(fb_type)],
    file(fb_type, e_file_type::OPENFPGA_CKBUF_LOC_MAP, default_corner_, verbose), false, verbose);
  /* Copy bitstream setting file */
  num_err += copy_file_with_mkdir(
    src_openfpga_setting_files_.at(e_file_type::OPENFPGA_BITSTREAM_SETTING),
    file(fb_type, e_file_type::OPENFPGA_BITSTREAM_SETTING, default_corner_, verbose), false,
    verbose);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::init_arch_dir_all(const bool& verbose) const {
  for (ArchDevManager::e_fabric_type fb_type : fabrics()) {
    int status = init_arch_dir(fb_type, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::init_runtime_dir(const e_fabric_type& fb_type,
                                     const e_script_type& runtime_type, const std::string& corner,
                                     const bool& verbose) const {
  int num_err = 0;

  /* Merge the two yaml files: design var is the base and the arch timing is to overwrite */
  ArchAggregator arch_agg(file(fb_type, e_file_type::DESIGN_VAR, corner, verbose), verbose);
  arch_agg.load_variables(file(fb_type, e_file_type::ARCH_TIMING, corner, verbose), false, false,
                          verbose);
  /* Generate the final vpr and openfpga arch file */
  std::string input_vpr_arch = file(fb_type, e_file_type::VPR_ARCH, corner, verbose);
  std::string runtime_vpr_arch =
    openfpga_runtime_file(fb_type, e_file_type::VPR_ARCH, runtime_type, corner, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_vpr_dirpath = runtime_vpr_arch;
  openfpga::create_directory(runtime_vpr_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_vpr_arch, runtime_vpr_arch, verbose);

  /* Generate the final vpr and openfpga arch file */
  std::string input_openfpga_arch = file(fb_type, e_file_type::OPENFPGA_ARCH, corner, verbose);
  std::string runtime_openfpga_arch =
    openfpga_runtime_file(fb_type, e_file_type::OPENFPGA_ARCH, runtime_type, corner, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_openfpga_dirpath = runtime_openfpga_arch;
  openfpga::create_directory(runtime_openfpga_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_openfpga_arch, runtime_openfpga_arch, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::init_runtime_script(const e_fabric_type& fb_type,
                                        const e_script_type& runtime_type,
                                        const BenchmarkPacket& bm_pkt, const std::string& corner,
                                        const bool& verbose) const {
  int num_err = 0;
  /* Generate the final openfpga script file */
  ArchAggregator script_agg;
  /* Add variables. */
  for (size_t itype = 0; itype < size_t(e_file_type::NUM_TYPES); ++itype) {
    e_file_type curr_type = static_cast<e_file_type>(itype);
    if (curr_type == e_file_type::VPR_ARCH || curr_type == e_file_type::OPENFPGA_ARCH) {
      std::string rt_file =
        openfpga_runtime_file(fb_type, curr_type, runtime_type, corner, verbose);
      script_agg.add_variable(VAR_NAMES_[itype], rt_file);
      continue;
    }
    if (curr_type == e_file_type::DESIGN_VAR) {
      continue;
    }
    script_agg.add_variable(VAR_NAMES_[itype], file(fb_type, curr_type, corner, verbose));
  }
  /* Add other variables, which are related to flow control. i.e., layout, channel width etc. */
  script_agg.add_variable(VPR_FABRIC_VAR_NAME_, fabric_type2str(fb_type));
  script_agg.add_variable(VPR_ROUTE_CHAN_W_VAR_NAME_, route_chan_width_);
  /* Add benchmark info */
  script_agg.add_variable(VPR_BLIF_VAR_NAME_, bm_pkt.post_synth_netlist());
  if (!bm_pkt.sdc().empty()) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::VPR_SDC)], bm_pkt.sdc());
  }
  if (!bm_pkt.pcf_gclk().empty()) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_PCF_XML)], bm_pkt.pcf());
  }

  /* Do aggregation here */
  std::string input_openfpga_script = openfpga_script_file(runtime_type);
  std::string runtime_openfpga_script =
    openfpga_runtime_script_file(fb_type, runtime_type, corner, verbose);
  // std::string runtime_openfpga_log = openfpga_runtime_log_file(fb_type,
  // e_script_type::OPENFPGA_SCRIPT_DEVICEDATA, verbose);
  num_err += script_agg.aggregate(input_openfpga_script, runtime_openfpga_script, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::runtime_exec_openfpga(const e_fabric_type& fb_type,
                                          const e_script_type& runtime_type,
                                          const std::string& openfpga_bin_path,
                                          const std::string& corner, const bool& verbose) {
  int num_err = 0;
  std::string runtime_openfpga_script =
    openfpga_runtime_script_file(fb_type, runtime_type, corner, verbose);
  /* Run openfpga */
  std::string openfpga_cmd = openfpga_bin_path + " -batch -f " + runtime_openfpga_script +
                             std::string(" > ") + OPENFPGA_LOG_FILENAME_;
  ;
  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = std::filesystem::path(runtime_openfpga_script).parent_path();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.string().c_str());

  if (arkangel::system_wrapper(openfpga_cmd, verbose)) {
    VTR_LOG_ERROR("OpenFPGA run failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.string().c_str());
  /*  Check log file errors */
  VTR_LOGV(verbose, "Parsing OpenFPGA log\n");
  std::filesystem::path log_fpath = runtime_dir / std::filesystem::path(OPENFPGA_LOG_FILENAME_);
  int status = openfpga_log_checker_.parse(log_fpath.string(), verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to run OpenFPGA log checker!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (openfpga_log_checker_.num_errors()) {
    VTR_LOG_ERROR("Log: %s\n", log_fpath.string().c_str());
    for (std::string err_msg : openfpga_log_checker_.error_messages()) {
      VTR_LOG_ERROR("%s\n", err_msg.c_str());
    }
    VTR_LOG_ERROR("OpenFPGA run failed with %lu errors!\n", openfpga_log_checker_.num_errors());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::check_result_files(const e_fabric_type& fb_type,
                                       const std::vector<e_file_type>& f_types_to_check,
                                       const std::string& corner, const bool& verbose) const {
  int num_err = 0;
  for (e_file_type f_type_to_check : f_types_to_check) {
    std::string chk_fpath = file(fb_type, f_type_to_check, corner, verbose);
    if (!std::filesystem::exists(chk_fpath)) {
      VTR_LOG_ERROR("Expected resulting file %s is not generated:\n\t%s\n",
                    FILE_TYPE_NAMES_[size_t(f_type_to_check)].c_str(), chk_fpath.c_str());
      num_err++;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_device_data(const e_fabric_type& fb_type,
                                         const BenchmarkPacket& bm_pkt,
                                         const std::string& openfpga_bin_path,
                                         const std::string& corner, const bool& verbose) {
  int status =
    init_runtime_dir(fb_type, e_script_type::OPENFPGA_SCRIPT_DEVICEDATA, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = init_runtime_script(fb_type, e_script_type::OPENFPGA_SCRIPT_DEVICEDATA, bm_pkt, corner,
                               verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = runtime_exec_openfpga(fb_type, e_script_type::OPENFPGA_SCRIPT_DEVICEDATA,
                                 openfpga_bin_path, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<e_file_type> f_types_to_check = {e_file_type::RRG, e_file_type::PDL,
                                               e_file_type::RLA};
  status = check_result_files(fb_type, f_types_to_check, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_device_data_all(const BenchmarkPacket& bm_pkt,
                                             const std::string& openfpga_bin_path,
                                             const bool& verbose) {
  for (ArchDevManager::e_fabric_type fb_type : fabrics()) {
    /* TODO: Sweep all the corners for ultimate fabric !!! */
    if (ArchDevManager::e_fabric_type::ULTIMATE == fb_type) {
      for (auto corner : corners()) {
        int status = generate_device_data(fb_type, bm_pkt, openfpga_bin_path, corner, verbose);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
      }
    } else {
      int status =
        generate_device_data(fb_type, bm_pkt, openfpga_bin_path, default_corner_, verbose);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_reference_module_names(const e_fabric_type& fb_type,
                                                    const BenchmarkPacket& bm_pkt,
                                                    const std::string& openfpga_bin_path,
                                                    const bool& verbose) {
  int status = init_runtime_dir(fb_type, e_script_type::OPENFPGA_SCRIPT_REFMODULENAMES,
                                default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = init_runtime_script(fb_type, e_script_type::OPENFPGA_SCRIPT_REFMODULENAMES, bm_pkt,
                               default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = runtime_exec_openfpga(fb_type, e_script_type::OPENFPGA_SCRIPT_REFMODULENAMES,
                                 openfpga_bin_path, default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<e_file_type> f_types_to_check = {e_file_type::REF_MODULE_NAME};
#ifndef NO_GRAPHICS
  f_types_to_check.push_back(e_file_type::VPR_FLOORPLAN_FILE);
#endif
  status = check_result_files(fb_type, f_types_to_check, default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_reference_module_names_all(const BenchmarkPacket& bm_pkt,
                                                        const std::string& openfpga_bin_path,
                                                        const bool& verbose) {
  for (ArchDevManager::e_fabric_type fb_type : fabrics()) {
    int status = generate_reference_module_names(fb_type, bm_pkt, openfpga_bin_path, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_reference_fabric_key(const e_fabric_type& fb_type,
                                                  const BenchmarkPacket& bm_pkt,
                                                  const std::string& openfpga_bin_path,
                                                  const bool& verbose) {
  int status =
    init_runtime_dir(fb_type, e_script_type::OPENFPGA_SCRIPT_REFFKEY, default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = init_runtime_script(fb_type, e_script_type::OPENFPGA_SCRIPT_REFFKEY, bm_pkt,
                               default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = runtime_exec_openfpga(fb_type, e_script_type::OPENFPGA_SCRIPT_REFFKEY, openfpga_bin_path,
                                 default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<e_file_type> f_types_to_check = {e_file_type::REF_FABRIC_KEY};
  status = check_result_files(fb_type, f_types_to_check, default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_reference_fabric_key_all(const BenchmarkPacket& bm_pkt,
                                                      const std::string& openfpga_bin_path,
                                                      const bool& verbose) {
  for (ArchDevManager::e_fabric_type fb_type : fabrics()) {
    int status = generate_reference_fabric_key(fb_type, bm_pkt, openfpga_bin_path, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_io_coordinate(const e_fabric_type& fb_type,
                                           const BenchmarkPacket& bm_pkt,
                                           const std::string& openfpga_bin_path,
                                           const std::string& fabric_key_assistant_bin_path,
                                           const bool& verbose) {
  std::string ref_fkey_file = file(fb_type, e_file_type::REF_FABRIC_KEY, default_corner_, verbose);
  std::string crafted_fkey_file =
    file(fb_type, e_file_type::CRAFTED_FABRIC_KEY, default_corner_, verbose);
  std::string final_fkey_file =
    file(fb_type, e_file_type::FINAL_FABRIC_KEY, default_corner_, verbose);

  /* For DP fabric, use the reference as the final */
  if (fb_type == e_fabric_type::DP) {
    int num_err = copy_file_with_mkdir(ref_fkey_file, final_fkey_file, false, verbose);
    if (num_err) {
      VTR_LOG_ERROR("Failed to adapt the reference fabric key to the final for fabric '%s'!\n",
                    fabric_type2str(fb_type).c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  } else if (fb_type == e_fabric_type::ULTIMATE) {
    std::string fka_cmd(fabric_key_assistant_bin_path);
    fka_cmd += std::string(" --reference ") + ref_fkey_file;
    fka_cmd += std::string(" --input ") + crafted_fkey_file;
    fka_cmd += std::string(" --output ") + final_fkey_file;

    int fka_run_status = system_wrapper(fka_cmd);
    if (fka_run_status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Fabric key assistant run failed with errors\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* For ultimate fabric, convert the handcrafted to the final through fabric key */
  int status =
    init_runtime_dir(fb_type, e_script_type::OPENFPGA_SCRIPT_IOCOORD, default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = init_runtime_script(fb_type, e_script_type::OPENFPGA_SCRIPT_IOCOORD, bm_pkt,
                               default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = runtime_exec_openfpga(fb_type, e_script_type::OPENFPGA_SCRIPT_IOCOORD, openfpga_bin_path,
                                 default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<e_file_type> f_types_to_check = {e_file_type::FINAL_FABRIC_KEY,
                                               e_file_type::IO_COORD};
  status = check_result_files(fb_type, f_types_to_check, default_corner_, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_io_coordinate_all(const BenchmarkPacket& bm_pkt,
                                               const std::string& openfpga_bin_path,
                                               const std::string& fabric_key_assistant_bin_path,
                                               const bool& verbose) {
  for (ArchDevManager::e_fabric_type fb_type : fabrics()) {
    int status = generate_io_coordinate(fb_type, bm_pkt, openfpga_bin_path,
                                        fabric_key_assistant_bin_path, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::generate_pin_table(const e_fabric_type& fb_type,
                                       const std::string& pin_table_gen_bin_path,
                                       const bool& verbose) const {
  std::string pin_table_naming_rule_file =
    file(fb_type, e_file_type::PIN_TABLE_NAMING_RULE, default_corner_, verbose);
  std::string io_coord_file = file(fb_type, e_file_type::IO_COORD, default_corner_, verbose);
  std::string pin_table_file = file(fb_type, e_file_type::PIN_TABLE, default_corner_, verbose);

  /* For DP fabric, use the reference as the final */
  std::string ptg_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + pin_table_gen_bin_path);
  ptg_cmd += std::string(" --io_coord_file ") + io_coord_file;
  ptg_cmd += std::string(" --naming_rules ") + pin_table_naming_rule_file;
  ptg_cmd += std::string(" --csv_file ") + pin_table_file;

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run pin table generation for fabric '%s'!\n",
                  fabric_type2str(fb_type).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(ptg_cmd.c_str())) {
    VTR_LOG_ERROR("Pin table generation run failed with errors for fabric '%s'!\n",
                  fabric_type2str(fb_type).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

void ArchDevManager::set_route_chan_width(const size_t& val) { route_chan_width_ = val; }

int ArchDevManager::add_crafted_file(const e_fabric_type& fb_type, const e_file_type& f_type,
                                     const std::string& fname, const bool& verbose) const {
  if (!valid_fabric_type(fb_type)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!valid_file_type(f_type)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string des_fpath = file(fb_type, f_type, default_corner_, verbose);
  if (!std::filesystem::exists(des_fpath)) {
    VTR_LOG_WARN("Will overwrite existing copy!\n");
  }
  std::filesystem::path src_fpath;
  bool req_copy_op = true;

  switch (f_type) {
    case ArchDevManager::e_file_type::CRAFTED_FABRIC_KEY: {
      if (fname.empty()) {
        /* Use the default file */
        src_fpath =
          file(fb_type, ArchDevManager::e_file_type::REF_FABRIC_KEY, default_corner_, verbose);
      } else {
        src_fpath = std::filesystem::absolute(fname);
      }
      break;
    }
    case ArchDevManager::e_file_type::CRAFTED_MODULE_NAME: {
      if (fname.empty()) {
        /* Create a dummy file to force no custom naming */
        std::string i_module_name_file =
          file(fb_type, ArchDevManager::e_file_type::REF_MODULE_NAME, default_corner_, verbose);
        std::string i_fb_hie_file = file(
          fb_type, ArchDevManager::e_file_type::FABRIC_HIERARCHY_FILE, default_corner_, verbose);
        std::string i_tile_ref_file =
          file(fb_type, ArchDevManager::e_file_type::TILE_REF, default_corner_, verbose);
        std::string o_module_name_file = file(fb_type, f_type, default_corner_, verbose);
        int gen_status = generate_tile_module_names(i_module_name_file, i_fb_hie_file,
                                                    i_tile_ref_file, o_module_name_file, verbose);
        if (gen_status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
        req_copy_op = false;
      } else {
        src_fpath = std::filesystem::absolute(fname);
      }
      break;
    }
    case ArchDevManager::e_file_type::PIN_TABLE_NAMING_RULE: {
      if (fname.empty()) {
        /* Auto generate pin table naming rule files to the final location */
        std::string io_coord_file =
          file(fb_type, ArchDevManager::e_file_type::IO_COORD, default_corner_, verbose);
        std::string pin_table_naming_file = file(fb_type, f_type, default_corner_, verbose);
        int gen_status =
          generate_pin_table_naming_rules(io_coord_file, pin_table_naming_file, verbose);
        if (gen_status != openfpga::CMD_EXEC_SUCCESS) {
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
        req_copy_op = false;
      } else {
        src_fpath = std::filesystem::absolute(fname);
      }
      break;
    }
    default: {
      VTR_LOG_ERROR("Invalid file type!\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Reach here, the file type is accepted. Wrap up */
  if (!req_copy_op) {
    return openfpga::CMD_EXEC_SUCCESS;
  }
  if (!std::filesystem::exists(src_fpath)) {
    VTR_LOG_ERROR("File '%s' does not exist!\n", src_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Copy the file to the designated location */
  std::filesystem::copy(src_fpath, des_fpath, std::filesystem::copy_options::overwrite_existing);
  VTR_LOGV(verbose, "Copied the source file to arch directory at\n\t%s\n", des_fpath.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

bool ArchDevManager::add_file(const e_fabric_type& fb_type,
                              const ArchDevManager::e_file_type& f_type,
                              const std::string& f_name) {
  if (!valid_fabric_type(fb_type)) {
    return false;
  }
  if (!valid_file_type(f_type)) {
    return false;
  }
  switch (f_type) {
    case ArchDevManager::e_file_type::VPR_ARCH: {
      /* Only accept DP fabric */
      if (fb_type == ArchDevManager::e_fabric_type::DP) {
        src_vpr_arch_file_ = f_name;
      }
      break;
    }
    case ArchDevManager::e_file_type::OPENFPGA_ARCH: {
      /* Only accept DP fabric */
      if (fb_type == ArchDevManager::e_fabric_type::DP) {
        src_openfpga_arch_file_ = f_name;
      }
      break;
    }
    case ArchDevManager::e_file_type::OPENFPGA_GNTWK_ARCH: {
      src_openfpga_gntwk_files_[size_t(fb_type)] = f_name;
      break;
    }
    case ArchDevManager::e_file_type::OPENFPGA_CKBUF_LOC_MAP: {
      src_openfpga_ckbuf_loc_map_files_[size_t(fb_type)] = f_name;
      break;
    }
    case ArchDevManager::e_file_type::DESIGN_VAR: {
      src_design_var_files_[size_t(fb_type)] = f_name;
      break;
    }
    case ArchDevManager::e_file_type::OPENFPGA_SIM_SETTING:
    case ArchDevManager::e_file_type::OPENFPGA_CKBUF_MAP:
    case ArchDevManager::e_file_type::OPENFPGA_BITSTREAM_SETTING:
    case ArchDevManager::e_file_type::OPENFPGA_TILE_CONFIG:
    case ArchDevManager::e_file_type::OPENFPGA_PCF_XML:
    case ArchDevManager::e_file_type::VPR_SDC:
    case ArchDevManager::e_file_type::OPENFPGA_PCF: {
      src_openfpga_setting_files_[f_type] = f_name;
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid file type!\n");
      return false;
  }
  return true;
}

bool ArchDevManager::add_openfpga_script(const ArchDevManager::e_script_type& f_type,
                                         const std::string& f_name) {
  if (!valid_script_type(f_type)) {
    return false;
  }
  src_openfpga_script_files_[size_t(f_type)] = f_name;
  return true;
}

void ArchDevManager::add_arch_timing_file(const std::string& corner, const std::string& val) {
  src_arch_timing_files_[corner] = val;
}

bool ArchDevManager::required_file_type(const TapeoutManager::e_arch_file_type& f_type) const {
  return valid_file_type(convert_file_type(f_type));
}

ArchDevManager::e_file_type ArchDevManager::convert_file_type(
  const TapeoutManager::e_arch_file_type& to_f_type) const {
  switch (to_f_type) {
    case TapeoutManager::e_arch_file_type::VPR_ARCH:
      return e_file_type::VPR_ARCH;
    case TapeoutManager::e_arch_file_type::OPENFPGA_ARCH:
      return e_file_type::OPENFPGA_ARCH;
    case TapeoutManager::e_arch_file_type::OPENFPGA_GNTWK_ARCH:
      return e_file_type::OPENFPGA_GNTWK_ARCH;
    case TapeoutManager::e_arch_file_type::OPENFPGA_CKBUF_LOC_MAP:
      return e_file_type::OPENFPGA_CKBUF_LOC_MAP;
    case TapeoutManager::e_arch_file_type::DESIGN_VAR:
      return e_file_type::DESIGN_VAR;
    case TapeoutManager::e_arch_file_type::OPENFPGA_SIM_SETTING:
      return e_file_type::OPENFPGA_SIM_SETTING;
    case TapeoutManager::e_arch_file_type::OPENFPGA_CKBUF_MAP:
      return e_file_type::OPENFPGA_CKBUF_MAP;
    case TapeoutManager::e_arch_file_type::OPENFPGA_BITSTREAM_SETTING:
      return e_file_type::OPENFPGA_BITSTREAM_SETTING;
    case TapeoutManager::e_arch_file_type::OPENFPGA_TILE_CONFIG:
      return e_file_type::OPENFPGA_TILE_CONFIG;
    case TapeoutManager::e_arch_file_type::OPENFPGA_PCF_XML:
      return e_file_type::OPENFPGA_PCF_XML;
    case TapeoutManager::e_arch_file_type::VPR_SDC:
      return e_file_type::VPR_SDC;
    case TapeoutManager::e_arch_file_type::OPENFPGA_PCF:
      return e_file_type::OPENFPGA_PCF;
    default:
      return e_file_type::NUM_TYPES;
  }
  return e_file_type::NUM_TYPES;
}

ArchDevManager::e_fabric_type ArchDevManager::str2fabric_type(const std::string& str,
                                                              const bool& verbose) const {
  for (int itype = size_t(ArchDevManager::e_fabric_type::DP);
       itype != size_t(ArchDevManager::e_fabric_type::NUM_TYPES); ++itype) {
    if (str == std::string(FABRIC_TYPE_STRING_[itype])) {
      return static_cast<ArchDevManager::e_fabric_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of openfpga fabric file! Expect %s\n",
                 fabric_type_all2str().c_str());
  return ArchDevManager::e_fabric_type::NUM_TYPES;
}

std::string ArchDevManager::fabric_type2str(const ArchDevManager::e_fabric_type& type,
                                            const bool& verbose) const {
  if (!valid_fabric_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of openfpga fabric file! Expect %s\n",
                   fabric_type_all2str().c_str());
    return std::string();
  }
  return std::string(FABRIC_TYPE_STRING_[size_t(type)]);
}

std::string ArchDevManager::fabric_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(ArchDevManager::e_fabric_type::DP);
       itype != size_t(ArchDevManager::e_fabric_type::NUM_TYPES); ++itype) {
    full_types += std::string(FABRIC_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool ArchDevManager::valid_fabric_type(const ArchDevManager::e_fabric_type& fb_type) const {
  return fb_type != ArchDevManager::e_fabric_type::NUM_TYPES;
}

void ArchDevManager::check_valid_fabric_type(const ArchDevManager::e_fabric_type& fb_type) const {
  if (!valid_fabric_type(fb_type)) {
    VTR_LOG_ERROR("Invalid fabric type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

ArchDevManager::e_file_type ArchDevManager::str2file_type(const std::string& str,
                                                          const bool& verbose) const {
  for (int itype = size_t(ArchDevManager::e_file_type::VPR_ARCH);
       itype != size_t(ArchDevManager::e_file_type::NUM_TYPES); ++itype) {
    if (str == std::string(FILE_TAGS_[itype])) {
      return static_cast<ArchDevManager::e_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of file! Expect %s\n", file_type_all2str().c_str());
  return ArchDevManager::e_file_type::NUM_TYPES;
}

std::string ArchDevManager::file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(ArchDevManager::e_file_type::VPR_ARCH);
       itype != size_t(ArchDevManager::e_file_type::NUM_TYPES); ++itype) {
    full_types += std::string(FILE_TAGS_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool ArchDevManager::valid_file_type(const ArchDevManager::e_file_type& f_type) const {
  return f_type != ArchDevManager::e_file_type::NUM_TYPES;
}

ArchDevManager::e_script_type ArchDevManager::convert_script_type(
  const TapeoutManager::e_script_type& to_f_type) const {
  switch (to_f_type) {
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN:
      return e_script_type::OPENFPGA_SCRIPT_BITGEN;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BASEBITGEN:
      return e_script_type::OPENFPGA_SCRIPT_BASEBITGEN;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_DEVICEDATA:
      return e_script_type::OPENFPGA_SCRIPT_DEVICEDATA;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_REFMODULENAMES:
      return e_script_type::OPENFPGA_SCRIPT_REFMODULENAMES;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_REFFKEY:
      return e_script_type::OPENFPGA_SCRIPT_REFFKEY;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_IOCOORD:
      return e_script_type::OPENFPGA_SCRIPT_IOCOORD;
    default:
      return e_script_type::NUM_TYPES;
  }
  return e_script_type::NUM_TYPES;
}

bool ArchDevManager::required_script_type(const TapeoutManager::e_script_type& f_type) const {
  return valid_script_type(convert_script_type(f_type));
}

bool ArchDevManager::valid_script_type(const ArchDevManager::e_script_type& f_type) const {
  return f_type != ArchDevManager::e_script_type::NUM_TYPES;
}

void ArchDevManager::check_valid_file_type(const ArchDevManager::e_file_type& f_type) const {
  if (!valid_file_type(f_type)) {
    VTR_LOG_ERROR("Invalid file type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

bool ArchDevManager::set_default_corner(const std::string& val) {
  /* Validate the corner is a valid one */
  auto result = src_arch_timing_files_.find(val);
  if (result == src_arch_timing_files_.end()) {
    return false;
  }
  default_corner_ = val;
  return true;
}

std::vector<std::string> ArchDevManager::corners() const {
  std::vector<std::string> ret;
  for (auto pair : src_arch_timing_files_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<std::string> ArchDevManager::result_files(const TapeoutManager::e_substep_name& s_step,
                                                      const bool& verbose) const {
  std::vector<std::string> ret;
  std::vector<e_file_type> corner_indep_ftypes;
  std::vector<e_file_type> corner_dep_ftypes;
  switch (s_step) {
    case TapeoutManager::e_substep_name::ARCH_INIT: {
      /* Corner independent */
      corner_indep_ftypes.push_back(e_file_type::VPR_ARCH);
      corner_indep_ftypes.push_back(e_file_type::OPENFPGA_ARCH);
      corner_indep_ftypes.push_back(e_file_type::OPENFPGA_GNTWK_ARCH);
      corner_indep_ftypes.push_back(e_file_type::DESIGN_VAR);
      corner_dep_ftypes.push_back(e_file_type::ARCH_TIMING);
      break;
    }
    case TapeoutManager::e_substep_name::ARCH_DATA: {
      /* Corner related */
      corner_dep_ftypes.push_back(e_file_type::RRG);
      corner_dep_ftypes.push_back(e_file_type::PDL);
      corner_dep_ftypes.push_back(e_file_type::RLA);
      break;
    }
    case TapeoutManager::e_substep_name::ARCH_INIT_MODULE_NAMES: {
      corner_indep_ftypes.push_back(e_file_type::REF_MODULE_NAME);
      break;
    }
    case TapeoutManager::e_substep_name::ARCH_INIT_FABRIC_KEY: {
      corner_indep_ftypes.push_back(e_file_type::CRAFTED_MODULE_NAME);
      corner_indep_ftypes.push_back(e_file_type::REF_FABRIC_KEY);
      break;
    }
    case TapeoutManager::e_substep_name::ARCH_IO_COORD: {
      corner_indep_ftypes.push_back(e_file_type::FINAL_FABRIC_KEY);
      corner_indep_ftypes.push_back(e_file_type::IO_COORD);
      break;
    }
    case TapeoutManager::e_substep_name::ARCH_PIN_TABLE: {
      corner_indep_ftypes.push_back(e_file_type::PIN_TABLE);
      corner_indep_ftypes.push_back(e_file_type::PIN_TABLE_NAMING_RULE);
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid substep name for arch development!\n");
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  for (auto fb_type : fabrics()) {
    /* For corner-independent files */
    for (auto f_type : corner_indep_ftypes) {
      std::string result_file = file(fb_type, f_type, default_corner_, verbose);
      ret.push_back(result_file);
    }
    /* For corner-related files */
    std::vector<std::string> corner_to_sweep;
    if (fb_type == e_fabric_type::DP) {
      corner_to_sweep.push_back(default_corner_);
    } else if (fb_type == e_fabric_type::ULTIMATE) {
      corner_to_sweep = corners();
    }
    for (auto curr_corner : corner_to_sweep) {
      for (auto f_type : corner_dep_ftypes) {
        std::string result_file = file(fb_type, f_type, curr_corner, verbose);
        ret.push_back(result_file);
      }
    }
  }
  return ret;
}

std::string ArchDevManager::md5_yaml_file() const {
  std::filesystem::path ret = project_home() / std::filesystem::path(MD5_FILENAME_);
  return ret.string();
}

int ArchDevManager::check_results(const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Check if the md5 file exists */
  if (!std::filesystem::exists(md5_yaml_file())) {
    VTR_LOG_ERROR("Required Md5 file does not exist! Unable to check results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  FileMd5Manager md5_mgr;
  status = md5_mgr.load_md5_from_yaml_file(md5_yaml_file(), verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Load Md5 from files failed when checking results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = md5_mgr.check_md5(verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Run Md5 check failed when checking results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return status;
}

int ArchDevManager::save_results(const std::vector<TapeoutManager::e_substep_name>& s_steps,
                                 const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  FileMd5Manager md5_mgr;
  for (auto s_step : s_steps) {
    for (auto fpath : result_files(s_step, verbose)) {
      md5_mgr.add_file(fpath);
    }
  }
  status = md5_mgr.gen_md5(verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Md5 generation failed when saving results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = md5_mgr.write_md5_to_yaml_file(md5_yaml_file(), verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Export Md5 to files failed when saving results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return status;
}

std::string ArchDevManager::default_corner() const { return default_corner_; }

int ArchDevManager::show_init_module_name_status(const e_fabric_type& fb_type,
                                                 const bool& verbose) const {
  std::string module_name_file =
    file(fb_type, ArchDevManager::e_file_type::REF_MODULE_NAME, default_corner_, verbose);
  std::string tile_ref_file =
    file(fb_type, ArchDevManager::e_file_type::TILE_REF, default_corner_, verbose);
  std::string fb_hie_file =
    file(fb_type, ArchDevManager::e_file_type::FABRIC_HIERARCHY_FILE, default_corner_, verbose);
  /* Read the io coordinate file */
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  TileRef tile_ref;
  VTR_LOGV(verbose, "Building tile name data...\n");
  VTR_LOGV(verbose, "Parsing tile reference file '%s'...\n", tile_ref_file.c_str());
  status = read_yaml_tile_ref(tile_ref_file.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Parsing fabric hierarchy file '%s'...\n", fb_hie_file.c_str());
  status = read_yaml_tile_types(fb_hie_file.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::ModuleNameMap module_names;
  VTR_LOGV(verbose, "Parsing module name file '%s'...\n", module_name_file.c_str());
  status = openfpga::read_xml_module_name_map(module_name_file.c_str(), module_names);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");
  /* Header line */
  std::string tag_name = "Default Name";
  std::string tag_type = "Type";
  std::string tag_count = "Count";
  /* Check max string size for each column */
  std::map<std::string, size_t> col_max_char;
  col_max_char[tag_name] = tag_name.size();
  for (std::string tile : tile_ref.tiles()) {
    if (col_max_char[tag_name] < tile.size()) {
      col_max_char[tag_name] = tile.size();
    }
  }
  col_max_char[tag_type] = tag_type.size();
  for (std::string tile : tile_ref.tiles()) {
    if (col_max_char[tag_type] < tile_ref.type(tile).size()) {
      col_max_char[tag_type] = tile_ref.type(tile).size();
    }
  }
  col_max_char[tag_count] = tag_count.size();
  for (std::string tile : tile_ref.tiles()) {
    if (col_max_char[tag_count] < std::to_string(tile_ref.num_instances(tile)).size()) {
      col_max_char[tag_count] = std::to_string(tile_ref.num_instances(tile)).size();
    }
  }
  /* Print header line */
  std::vector<std::string> tag_list = {tag_name, tag_type, tag_count};
  for (std::string tag : tag_list) {
    std::string resized_tag = tag;
    resized_tag.resize(col_max_char[tag], ' ');
    VTR_LOG("%s\t", resized_tag.c_str());
  }
  VTR_LOG("\n");
  /* Print data lines */
  for (std::string tile : tile_ref.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    /* Column name */
    std::string resized_name = tile;
    resized_name.resize(col_max_char[tag_name], ' ');
    VTR_LOG("%s\t", resized_name.c_str());
    /* Column type */
    std::string resized_type = tile_ref.type(tile);
    resized_type.resize(col_max_char[tag_type], ' ');
    VTR_LOG("%s\t", resized_type.c_str());
    /* Column type */
    std::string resized_count = std::to_string(tile_ref.num_instances(tile));
    resized_count.resize(col_max_char[tag_count], ' ');
    VTR_LOG("%s\t", resized_count.c_str());
    /* End the line */
    VTR_LOG("\n");
  }
  return status;
}

int ArchDevManager::show_io_status(const e_fabric_type& fb_type, const std::string& file_path,
                                   const bool& verbose) const {
  std::string io_pin_table_file =
    file(fb_type, ArchDevManager::e_file_type::PIN_TABLE, default_corner_, verbose);
  /* Read the io pin table file */
  /* TODO: Should make the direction be configurable ? */
  openfpga::IoPinTable io_pntb = openfpga::read_csv_io_pin_table(
    io_pin_table_file.c_str(), openfpga::e_pin_table_direction_convention::EXPLICIT);

  std::array<size_t, NUM_2D_SIDES> input_cnt_by_side;
  std::array<size_t, NUM_2D_SIDES> output_cnt_by_side;
  std::array<size_t, NUM_2D_SIDES> io_cnt_by_side;
  size_t num_inputs = 0;
  size_t num_outputs = 0;
  for (e_side side : TOTAL_2D_SIDES) {
    input_cnt_by_side[size_t(side)] = 0;
    output_cnt_by_side[size_t(side)] = 0;
    io_cnt_by_side[size_t(side)] = 0;
  }
  for (auto pin_id : io_pntb.pins()) {
    e_side io_side = io_pntb.pin_side(pin_id);
    if (io_side < NUM_2D_SIDES) {
      if (openfpga::IoPinTable::e_io_direction::INPUT == io_pntb.pin_direction(pin_id)) {
        input_cnt_by_side[size_t(io_side)]++;
        num_inputs++;
      } else if (openfpga::IoPinTable::e_io_direction::OUTPUT == io_pntb.pin_direction(pin_id)) {
        output_cnt_by_side[size_t(io_side)]++;
        num_outputs++;
      } else {
        VTR_LOG_ERROR("Invalid direction of the eFPGA I/O '%s'! Expect [ input | output ] \n",
                      io_pntb.external_pin(pin_id).to_verilog_string().c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      io_cnt_by_side[size_t(io_side)]++;
    } else {
      std::string all_side_str;
      for (e_side side : TOTAL_2D_SIDES) {
        all_side_str += std::string(TOTAL_2D_SIDE_STRINGS[size_t(side)]) + std::string("|");
      }
      all_side_str.pop_back(); /* Remove the useless | */
      VTR_LOG_ERROR("Invalid side for eFPGA I/O '%s'! Expect [ %s ] \n",
                    io_pntb.external_pin(pin_id).to_verilog_string().c_str(), all_side_str.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* Show brief */
  /* Header line */
  std::string tag_side = "Side";
  std::string tag_input = "Input";
  std::string tag_output = "Output";
  std::string tag_total = "Total";
  TablePrinter tp({tag_side, tag_input, tag_output, tag_total});

  /* Check max string size for each column */

  for (e_side side : TOTAL_2D_SIDES) {
    std::string side_str = std::string(TOTAL_2D_SIDE_STRINGS[size_t(side)]);
    std::string input_str = std::to_string(input_cnt_by_side[size_t(side)]);
    std::string output_str = std::to_string(output_cnt_by_side[size_t(side)]);
    std::string io_str = std::to_string(io_cnt_by_side[size_t(side)]);
    tp.add_row({side_str, input_str, output_str, io_str});
  }
  tp.print(file_path);
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::show_module_name_status(const e_fabric_type& fb_type,
                                            const bool& verbose) const {
  std::string module_name_file =
    file(fb_type, ArchDevManager::e_file_type::CRAFTED_MODULE_NAME, default_corner_, verbose);
  std::string tile_ref_file =
    file(fb_type, ArchDevManager::e_file_type::TILE_REF, default_corner_, verbose);
  std::string fb_hie_file =
    file(fb_type, ArchDevManager::e_file_type::FABRIC_HIERARCHY_FILE, default_corner_, verbose);
  /* Read the io coordinate file */
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  TileRef tile_ref;
  VTR_LOGV(verbose, "Building tile name data...\n");
  VTR_LOGV(verbose, "Parsing tile reference file '%s'...\n", tile_ref_file.c_str());
  status = read_yaml_tile_ref(tile_ref_file.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Parsing fabric hierarchy file '%s'...\n", fb_hie_file.c_str());
  status = read_yaml_tile_types(fb_hie_file.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  openfpga::ModuleNameMap module_names;
  VTR_LOGV(verbose, "Parsing module name file '%s'...\n", module_name_file.c_str());
  status = openfpga::read_xml_module_name_map(module_name_file.c_str(), module_names);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");
  /* Header line */
  std::string tag_name = "Final Name";
  std::string tag_dft_name = "Default Name";
  std::string tag_type = "Type";
  std::string tag_count = "Count";
  /* Check max string size for each column */
  std::map<std::string, size_t> col_max_char;
  col_max_char[tag_name] = tag_name.size();
  for (std::string tile : tile_ref.tiles()) {
    if (col_max_char[tag_name] < tile.size()) {
      col_max_char[tag_name] = tile.size();
    }
  }
  col_max_char[tag_dft_name] = tag_dft_name.size();
  for (std::string tile : tile_ref.tiles()) {
    if (!module_names.tag_exist(tile)) {
      continue;
    }
    if (col_max_char[tag_dft_name] < module_names.tag(tile).size()) {
      col_max_char[tag_dft_name] = module_names.tag(tile).size();
    }
  }
  col_max_char[tag_type] = tag_type.size();
  for (std::string tile : tile_ref.tiles()) {
    if (col_max_char[tag_type] < tile_ref.type(tile).size()) {
      col_max_char[tag_type] = tile_ref.type(tile).size();
    }
  }
  col_max_char[tag_count] = tag_count.size();
  for (std::string tile : tile_ref.tiles()) {
    if (col_max_char[tag_count] < std::to_string(tile_ref.num_instances(tile)).size()) {
      col_max_char[tag_count] = std::to_string(tile_ref.num_instances(tile)).size();
    }
  }
  /* Print header line */
  std::vector<std::string> tag_list = {tag_name, tag_dft_name, tag_type, tag_count};
  for (std::string tag : tag_list) {
    std::string resized_tag = tag;
    resized_tag.resize(col_max_char[tag], ' ');
    VTR_LOG("%s\t", resized_tag.c_str());
  }
  VTR_LOG("\n");
  /* Print data lines */
  for (std::string tile : tile_ref.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    /* Column name */
    std::string resized_name = tile;
    resized_name.resize(col_max_char[tag_name], ' ');
    VTR_LOG("%s\t", resized_name.c_str());
    /* Column default name */
    std::string resized_dft_name = "N/A";
    if (module_names.tag_exist(tile)) {
      resized_dft_name = module_names.tag(tile);
    }
    resized_dft_name.resize(col_max_char[tag_dft_name], ' ');
    VTR_LOG("%s\t", resized_dft_name.c_str());
    /* Column type */
    std::string resized_type = tile_ref.type(tile);
    resized_type.resize(col_max_char[tag_type], ' ');
    VTR_LOG("%s\t", resized_type.c_str());
    /* Column type */
    std::string resized_count = std::to_string(tile_ref.num_instances(tile));
    resized_count.resize(col_max_char[tag_count], ' ');
    VTR_LOG("%s\t", resized_count.c_str());
    /* End the line */
    VTR_LOG("\n");
  }
  return status;
}

int ArchDevManager::check_fabric_consistency(const bool& verbose) const {
  std::string dp_tile_ref_file =
    file(ArchDevManager::e_fabric_type::DP, ArchDevManager::e_file_type::TILE_REF, default_corner_,
         verbose);
  VTR_LOGV(verbose, "Parsing tile reference for fabric '%s'\n",
           fabric_type2str(ArchDevManager::e_fabric_type::DP).c_str());
  TileRef dp_tile_ref;
  int status = read_yaml_tile_ref(dp_tile_ref_file.c_str(), dp_tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string dp_fb_hie_file =
    file(ArchDevManager::e_fabric_type::DP, ArchDevManager::e_file_type::FABRIC_HIERARCHY_FILE,
         default_corner_, verbose);
  VTR_LOGV(verbose, "Parsing fabric hierarchy for fabric '%s'...\n",
           fabric_type2str(ArchDevManager::e_fabric_type::DP).c_str());
  status = read_yaml_tile_types(dp_fb_hie_file.c_str(), dp_tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string ut_tile_ref_file =
    file(ArchDevManager::e_fabric_type::ULTIMATE, ArchDevManager::e_file_type::TILE_REF,
         default_corner_, verbose);
  VTR_LOGV(verbose, "Parsing tile reference for fabric '%s'\n",
           fabric_type2str(ArchDevManager::e_fabric_type::ULTIMATE).c_str());
  TileRef ut_tile_ref;
  status = read_yaml_tile_ref(ut_tile_ref_file.c_str(), ut_tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string ut_fb_hie_file =
    file(ArchDevManager::e_fabric_type::ULTIMATE,
         ArchDevManager::e_file_type::FABRIC_HIERARCHY_FILE, default_corner_, verbose);
  VTR_LOGV(verbose, "Parsing fabric hierarchy for fabric '%s'...\n",
           fabric_type2str(ArchDevManager::e_fabric_type::ULTIMATE).c_str());
  status = read_yaml_tile_types(ut_fb_hie_file.c_str(), ut_tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Checking tile name and type consistency...\n");
  bool consistency = ut_tile_ref.check_equivalent(dp_tile_ref, verbose);
  return consistency ? openfpga::CMD_EXEC_SUCCESS : openfpga::CMD_EXEC_FATAL_ERROR;
}

int ArchDevManager::export_arch(const std::string& fname, const bool& force_overwritten,
                                const bool& verbose) const {
  /* File name must comply tar.gz*/
  std::string f_suffix = ".tar.gz";
  if (fname.size() >= f_suffix.size() &&
      fname.compare(fname.size() - f_suffix.size(), f_suffix.size(), f_suffix) == 0) {
    VTR_LOGV(verbose, "Package name check passed\n");
  } else {
    VTR_LOG_ERROR(
      "The destination file '%s' contains a suffix which is not supported. Expect to end with "
      "'%s'\n!",
      f_suffix.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Remove previous */
  std::filesystem::path fpath = std::filesystem::absolute(fname);
  if (std::filesystem::exists(fpath)) {
    if (std::filesystem::is_directory(fpath)) {
      VTR_LOG_ERROR("The destination file '%s' is actually a directory. Required to be a file\n!",
                    fpath.c_str());
    } else if (force_overwritten) {
      VTR_LOG("Remove the existing file '%s' to overwrite, per users request\n", fpath.c_str());
      std::filesystem::remove(fpath);
    } else {
      VTR_LOG(
        "The file for netlist exporting '%s' already exists. Please double check to avoid data "
        "loss\n",
        fpath.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* TODO: Check required file */
  /* Create a command to package files */
  std::string export_cmd = "tar -czvf";
  export_cmd += std::string(" ") + fpath.string();
  export_cmd += std::string(" -C ") + arch_dir();
  export_cmd += std::string(" .");
  /* VTR_LOGV(verbose,"Command: %s\n", export_cmd.c_str()); */

  VTR_LOG("Exporting architecture data to \n\t%s\n", fpath.string().c_str());

  if (arkangel::system_wrapper(export_cmd, verbose)) {
    VTR_LOG_ERROR("Export arch package failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::merge_arch_timing(const std::string& corner,
                                      const std::vector<std::string>& src_arch_timing_files,
                                      const bool& verbose) const {
  /* Validate if the corner exists */
  auto result = src_arch_timing_files_.find(corner);
  if (result == src_arch_timing_files_.end()) {
    VTR_LOG_ERROR("The required corner '%s' is not a valid one in the architecture!\n",
                  corner.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Get the desired arch timing file and start aggregate */
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  std::string des_arch_timing_file = file(fb_type, e_file_type::ARCH_TIMING, corner, verbose);
  VTR_LOG("Back-annotating timing for corner '%s'...\n", corner.c_str());
  ArchAggregator arch_agg(des_arch_timing_file, verbose);

  for (std::string src_arch_timing_file : src_arch_timing_files) {
    VTR_LOGV(verbose, "Back-annotating timing from '%s'\n", src_arch_timing_file.c_str());
    arch_agg.load_variables(src_arch_timing_file, false, true, verbose);
  }
  arch_agg.write_variables(des_arch_timing_file, verbose);
  VTR_LOG("Done\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::find_vpr_layout_dimension(const e_fabric_type& fb_type, int& height, int& width,
                                              const bool& verbose) const {
  size_t num_err = 0;
  std::string arch_var_fpath = file(fb_type, e_file_type::DESIGN_VAR, default_corner_, verbose);
  /* TODO: Get the height/width depending on the side */
  ArchAggregator arch_agg(arch_var_fpath, verbose);
  /* Generate the final vpr and openfpga arch file */
  std::string input_vpr_arch = file(fb_type, e_file_type::VPR_ARCH, default_corner_, verbose);
  std::string runtime_vpr_arch = openfpga_runtime_file(
    fb_type, e_file_type::VPR_ARCH, e_script_type::OPENFPGA_SCRIPT_SETIO, default_corner_, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_vpr_dirpath = runtime_vpr_arch;
  openfpga::create_directory(runtime_vpr_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_vpr_arch, runtime_vpr_arch, verbose);
  if (num_err != 0) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  t_arch vpr_arch_data;
  std::vector<t_physical_tile_type> phy_tile_types;
  std::vector<t_logical_block_type> lb_types;
  xml_read_arch(runtime_vpr_arch.c_str(), false, &vpr_arch_data, phy_tile_types, lb_types, verbose);
  int curr_h = -1;
  int curr_w = -1;
  bool valid_vpr_layout = false;
  for (const auto& vpr_layout : vpr_arch_data.grid_layouts) {
    if (fabric_type2str(fb_type) == vpr_layout.name) {
      curr_h = vpr_layout.height;
      curr_w = vpr_layout.width;
      valid_vpr_layout = true;
      break;
    }
  }
  if (!valid_vpr_layout) {
    VTR_LOG_ERROR("Invalid VPR architecture which does not contain a layout named as '%s'!\n",
                  fabric_type2str(fb_type).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (curr_h == -1 || curr_w == -1) {
    VTR_LOG_ERROR(
      "Invalid layout '%s' in VPR architecture whose height or width is not a positive number!\n",
      fabric_type2str(fb_type).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (curr_h < 3 || curr_w < 3) {
    VTR_LOG_ERROR("Invalid layout '%s' in VPR architecture whose height or width is less than 3!\n",
                  fabric_type2str(fb_type).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  height = curr_h;
  width = curr_w;
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::set_sync_dp_dimension(const bool& val, const bool& verbose) {
  /* Cache the options. Later we will collect I/O options and run arch configurator once */
  if (val) {
    VTR_LOGV(verbose,
             "User chooses to apply same dimension on dp and ultimate fabric. This ensures "
             "consistency between the two fabrics\n");
  } else {
    VTR_LOGV(verbose,
             "User chooses to apply different dimension on dp and ultimate fabric. A small dp "
             "fabric reduces design planning effort\n");
  }
  sync_dp_dimension_ = val;

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::set_core_dimension(const size_t& height, const size_t& width,
                                       const bool& verbose) {
  /* Cache the options. Later we will collect I/O options and run arch configurator once */
  VTR_LOGV(verbose, "Strongly suggest to set the height and width in even number\n");
  VTR_LOGV(
    verbose,
    "The min. core size is typically 2x2. Refer to device datasheet for detailed requirements\n");
  if (height != 0 && width != 0) {
    VTR_LOG("Set eFPGA core fabric size to %lu (H) x %lu (W)\n", height, width);
  }
  VTR_LOG("This excludes the I/O rows and columns\n");
  VTR_LOG("Final check will be applied when running architecture generation\n");
  core_dimension_.set(width, height);

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::set_num_clocks(const size_t& num_clks, const bool& verbose) {
  if (num_clks % 2 == 1) {
    VTR_LOG_ERROR("The number of clocks must be an even number, e.g., 2\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (num_clks == 0) {
    VTR_LOG("Will use the default number of clocks for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of clocks of eFPGA core fabric size to %lu\n", num_clks);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_clocks_ = num_clks;
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::set_num_pclocks(const size_t& num_pclks, const bool& verbose) {
  if (num_pclks == 0) {
    VTR_LOG("Will use the default number of programming clocks for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of programming clocks of eFPGA core fabric size to %lu\n",
             num_pclks);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_pclocks_ = num_pclks;
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::set_num_config_regions(const size_t& num_regions, const bool& verbose) {
  if (num_regions == 0) {
    VTR_LOG("Will use the default number of configurable regions for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of configurable regions of eFPGA core fabric size to %lu\n",
             num_regions);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_config_regions_ = num_regions;
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::set_num_resets(const size_t& num_rsts, const bool& verbose) {
  if (num_rsts % 2 == 1) {
    VTR_LOG_ERROR("The number of resets must be an even number, e.g., 2\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (num_rsts == 0) {
    VTR_LOG("Will use the default number of resets for device family\n");
  } else {
    /* Cache the options. Later we will collect I/O options and run arch configurator once */
    VTR_LOGV(verbose, "Set the number of resets of eFPGA core fabric size to %lu\n", num_rsts);
  }
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_resets_ = num_rsts;
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::exec_arch_gen(const bool& verbose) {
  /* Get the path to the arch configurator binary through tape-out manager */
  std::filesystem::path arch_gen_bin_path_nom = arch_gen_bin_path_;
  arch_gen_bin_path_nom = arch_gen_bin_path_nom.lexically_normal();
  arch_gen_bin_path_nom.make_preferred();
  std::string arch_cfg_cmd(arch_gen_bin_path_nom.string());
  /* Arch files to be auto generated */
  arch_cfg_cmd += std::string(" --vpr_arch ") +
                  file(e_fabric_type::DP, e_file_type::VPR_ARCH, default_corner_, verbose);
  arch_cfg_cmd += std::string(" --openfpga_arch ") +
                  file(e_fabric_type::DP, e_file_type::OPENFPGA_ARCH, default_corner_, verbose);
  arch_cfg_cmd +=
    std::string(" --openfpga_dp_global_network ") +
    file(e_fabric_type::DP, e_file_type::OPENFPGA_GNTWK_ARCH, default_corner_, verbose);
  arch_cfg_cmd +=
    std::string(" --openfpga_ultimate_global_network ") +
    file(e_fabric_type::ULTIMATE, e_file_type::OPENFPGA_GNTWK_ARCH, default_corner_, verbose);
  arch_cfg_cmd += std::string(" --openfpga_bitstream_setting ") +
                  file(e_fabric_type::ULTIMATE, e_file_type::OPENFPGA_BITSTREAM_SETTING,
                       default_corner_, verbose);
  arch_cfg_cmd +=
    std::string(" --dp_ckbuf_loc_map ") +
    file(e_fabric_type::DP, e_file_type::OPENFPGA_CKBUF_LOC_MAP, default_corner_, verbose);
  arch_cfg_cmd +=
    std::string(" --ultimate_ckbuf_loc_map ") +
    file(e_fabric_type::ULTIMATE, e_file_type::OPENFPGA_CKBUF_LOC_MAP, default_corner_, verbose);
  /* Arch parameters */
  arch_cfg_cmd += std::string(" --height ") + std::to_string(core_dimension_.y());
  arch_cfg_cmd += std::string(" --width ") + std::to_string(core_dimension_.x());
  arch_cfg_cmd += std::string(" --num_clks ") + std::to_string(num_clocks_);
  arch_cfg_cmd += std::string(" --num_rsts ") + std::to_string(num_resets_);
  arch_cfg_cmd += std::string(" --num_pclks ") + std::to_string(num_pclocks_);
  arch_cfg_cmd += std::string(" --num_config_regions ") + std::to_string(num_config_regions_);
  /* Organize io string */
  std::string num_inputs_str;
  for (auto pair : num_ios_[size_t(ArchIoConstraints::e_io_direction::INPUT)]) {
    num_inputs_str +=
      arch_io_constr_->side2str(pair.first) + std::string(":") + std::to_string(pair.second);
    num_inputs_str += std::string(",");
  }
  if (!num_inputs_str.empty()) {
    num_inputs_str.pop_back();
    arch_cfg_cmd += std::string(" --num_inputs ") + num_inputs_str;
  }

  std::string num_outputs_str;
  for (auto pair : num_ios_[size_t(ArchIoConstraints::e_io_direction::OUTPUT)]) {
    num_outputs_str +=
      arch_io_constr_->side2str(pair.first) + std::string(":") + std::to_string(pair.second);
    num_outputs_str += std::string(",");
  }
  if (!num_outputs_str.empty()) {
    num_outputs_str.pop_back();
    arch_cfg_cmd += std::string(" --num_outputs ") + num_outputs_str;
  }
  if (sync_dp_dimension_) {
    arch_cfg_cmd += std::string(" --sync_dp_dimension ");
  }

  arch_cfg_cmd += std::string(" > ") + ARCH_GEN_LOG_FILENAME_;
  VTR_LOGV(verbose, "Run command:\n\t%s\n", arch_cfg_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir =
    std::filesystem::path(file(e_fabric_type::DP, e_file_type::VPR_ARCH, default_corner_, verbose))
      .parent_path();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.string().c_str());

  if (arkangel::system_wrapper(arch_cfg_cmd, verbose)) {
    VTR_LOG_ERROR("Architecture configurator run failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.string().c_str());

  /*  Check log file errors */
  VTR_LOGV(verbose, "Parsing architecture configurator log\n");
  std::filesystem::path log_fpath = runtime_dir / std::filesystem::path(ARCH_GEN_LOG_FILENAME_);
  int status = arch_gen_log_checker_.parse(log_fpath.string(), verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to run architecture configurator log checker!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (arch_gen_log_checker_.num_errors()) {
    VTR_LOG_ERROR("Log: %s\n", log_fpath.string().c_str());
    for (std::string err_msg : arch_gen_log_checker_.error_messages()) {
      VTR_LOG_ERROR("%s\n", err_msg.c_str());
    }
    VTR_LOG_ERROR("Architecture configurator run failed with %lu errors!\n",
                  arch_gen_log_checker_.num_errors());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchDevManager::set_io_num(const e_side& curr_side,
                               const ArchIoConstraints::e_io_direction& io_type,
                               const size_t& io_num, const bool& verbose) {
  /* TODO: Check the side, type are valid or not */
  if (!arch_io_constr_->valid_side(curr_side)) {
    VTR_LOG_ERROR("Invalid side '%s' for I/O number setting. Expect %s!\n",
                  arch_io_constr_->side2str(curr_side).c_str(),
                  arch_io_constr_->side_all2str().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!arch_io_constr_->valid_io_direction(io_type)) {
    VTR_LOG_ERROR("Invalid type '%s' for I/O number setting. Expect %s!\n",
                  arch_io_constr_->io_direction2str(io_type).c_str(),
                  arch_io_constr_->io_direction_all2str().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbose, "Set the number of %s on %s side of eFPGA to %lu\n",
           arch_io_constr_->io_direction2str(io_type).c_str(),
           arch_io_constr_->side2str(curr_side).c_str(), io_num);
  VTR_LOGV(verbose, "Final check will be applied when running architecture generation\n");
  num_ios_[size_t(io_type)][curr_side] = io_num;

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arkangel
