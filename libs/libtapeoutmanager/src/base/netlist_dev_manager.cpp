#include "netlist_dev_manager.h"

#include <filesystem>

#include "arch_aggregator.h"
#include "arch_gen.h"
#include "arkangel_constants.h"
#include "arkangel_file_os.h"
#include "arkangel_report_util.cpp"
#include "check_floorplan.h"
#include "command_exit_codes.h"
#include "fabric_synth_strategy.h"
#include "file_md5_manager.h"
#include "openfpga_digest.h"
#include "read_floorplan_params.h"
#include "read_xml_bitstream_distribution.h"
#include "read_xml_clk_loc_map.h"
#include "read_xml_floorplan_strategy.h"
#include "read_xml_openfpga_arch.h"
#include "read_xml_pin_constraints.h"
#include "read_xml_synth_strategy.h"
#include "read_yaml_tile_ref.h"
#include "read_yaml_tile_types.h"
#include "report_floorplan.h"
#include "run_floorplan_flow.h"
#include "tableprinter.h"
#include "verilog_port_types.h"
#include "verilog_writer_utils.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "write_xml_openfpga_arch.h"
#include "write_xml_pdk_manager.h"
#include "write_xml_pin_constraints.h"

namespace arcxpress {  // Begin namespace arcxpress

NetlistDevManager::NetlistDevManager(const TapeoutManager& to_mgr, const std::string& run_name,
                                     const PdkManager& pdk_mgr) {
  init(to_mgr.booting_root(), to_mgr.path(), to_mgr.name(), to_mgr.family(), run_name, pdk_mgr,
       to_mgr.openfpga_error_conf_file(), to_mgr.python_bin_path());
}

void NetlistDevManager::init(const std::string& booting_root, const std::string& root_path,
                             const std::string& proj_name, const std::string& family_name,
                             const std::string& run_name, const PdkManager& pdk_mgr,
                             const std::string& openfpga_error_conf_file,
                             const std::string& py_bin_path) {
  booting_root_ = booting_root;
  pdk_mgr_ = &pdk_mgr;
  root_path_ = root_path;
  proj_name_ = proj_name;
  family_name_ = family_name;
  run_name_ = run_name;
  PYTHON_COMMAND_ = py_bin_path;
  is_valid_ = false;
  SUBDIR_NAME_ = "netlist";
  ARCH_DIR_NAME_ = "arch";
  NETLIST_DIR_NAME_ = "netlist";
  NETLIST_BLOCK_DIR_NAME_ = "subblock";
  NETLIST_TILE_DIR_NAME_ = "tile";
  NETLIST_TECHSRC_DIR_NAME_ = "techsrc";
  NETLIST_CUSTOM_MODULE_DIR_NAME_ = "custom_modules";
  SWEEP_DIR_PREFIX_ = "sweep";
  RRGSB_DIR_NAME_ = "rr_gsb";
  SYNTH_DIR_NAME_ = "synth";
  SYNTH_CONFIG_DIR_NAME_ = "config";
  SYNTH_CUSTOM_DIR_NAME_ = "custom_synth_modules";
  SYNTH_SDC_DIR_NAME_ = "sdc";
  SYNTH_CCFF_DIR_NAME_ = "ccff_buf";
  SYNTH_RUNTIME_DIR_NAME_ = "_work";
  RPT_DIR_NAME_ = "reports";
  RPT_AREA_DIR_NAME_ = "area";
  RPT_TIMING_DIR_NAME_ = "timing";
  RPT_SUMMARY_DIR_NAME_ = "summary";
  RPT_BITSTREAM_DIST_DIR_NAME_ = "bitstream_distribution";
  OPENFPGA_RUNDIR_NAME_ = "openfpga_tasks";
  PDK_DIR_NAME_ = "pdk";

  ARCH_VAR_FILE_ = "arch_timing_tt_25C_0v9.yml";
  VPR_BLIF_VAR_NAME_ = "VPR_TESTBENCH_BLIF";
  VPR_FABRIC_VAR_NAME_ = "OPENFPGA_VPR_DEVICE_LAYOUT";
  VPR_ROUTE_CHAN_W_VAR_NAME_ = "OPENFPGA_VPR_ROUTE_CHAN_WIDTH";
  OPENFPGA_RRGSB_DIR_VAR_NAME_ = "OPENFPGA_RRGSB_DIR";
  OPENFPGA_NETLIST_DIR_VAR_NAME_ = "OPENFPGA_VERILOG_OUTPUT_DIR";

  FABRIC_TYPE_STRING_ = {"dp", "ultimate"};
  NETLIST_TYPE_STRING_ = {"raw_rtl", "rtl", "gl"};
  FILE_NAMES_ = {"vpr_arch.xml",
                 "openfpga_arch.xml",
                 "openfpga_global_network.xml",
                 "ckbuf_loc_map.xml",
                 "design_variables.yml", /* Placeholder only */
                 "openfpga_sim_setting.xml",
                 "openfpga_bitstream_setting.xml",
                 "openfpga_bitstream_setting_append.xml",
                 "tile_config.xml",
                 "gclk_pcf.xml",
                 "vpr_dummy.sdc",
                 "pcf_dummy.pcf",
                 "fabric_key.xml",
                 "reference_module_names.xml",
                 "module_names.xml",
                 "rr_graph.bin",
                 "placement_delay_lookup.bin",
                 "router_lookahead.bin",
                 "tile_reference.yaml",
                 "efpga_io_pin_table.csv",
                 "module_muxes.yaml",
                 "config_mem.yaml",
                 "fabric_pin_phy_loc.xml",
                 "fabric_hierarchy.yaml",
                 "undriven_makeup_key.xml",
                 "bscan_makeup_key.xml",
                 "clkn_makeup_key.xml",
                 "synth_pdk_config.xml",
                 "synth_qor.csv",
                 "tile_qor.csv",
                 "base_bitstream.xml",
                 "bitstream_value.xml",
                 "bitstream_distribution.xml",
                 "arch_timing",
                 "shaping.txt"};
  FILE_TYPE_NAMES_ = {"VPR architecture file",
                      "OpenFPGA architecture file",
                      "OpenFPGA global network architecture file",
                      "OpenFPGA clock buffer location file",
                      "Architecture variable file", /* Placeholder only */
                      "OpenFPGA simulation setting file",
                      "OpenFPGA bitstream setting file",
                      "OpenFPGA bitstream setting file to append",
                      "OpenFPGA tile configuration file",
                      "OpenFPGA XML-based Pin Constraint file",
                      "VPR SDC file",
                      "OpenFPGA Pin Constraint file",
                      "Final fabric key file",
                      "reference module name file",
                      "Final module name file",
                      "Routing resource graph file",
                      "Placement delay matrix file",
                      "Router lookahead file",
                      "Tile reference file",
                      "I/O pin table file",
                      "MUX module list",
                      "Configuration memory module list",
                      "Fabric pin physical location file",
                      "Fabric hierarchy file",
                      "undriven_makeup_key.xml",
                      "bscan_makeup_key.xml",
                      "clkn_makeup_key.xml",
                      "synth-oriented pdk config file",
                      "synth_qor.csv",
                      "tile_qor.csv",
                      "base_bitstream.xml",
                      "bitstream_value.xml",
                      "bitstream_distribution.xml",
                      "arch_timing",
                      "shaping.txt"};
  FILE_TAGS_ = {"vpr_arch",
                "openfpga_arch",
                "openfpga_global_network_arch",
                "ckbuf_loc_map",
                "architecture_variables", /* Placeholder only */
                "simulation_setting",
                "bitstream_setting",
                "bitstream_setting_to_append",
                "tile_config",
                "default_pcf_xml",
                "default_vpr_sdc",
                "default_pcf",
                "fabric_key",
                "ref_module_names",
                "module_names",
                "routing_resource_graph",
                "placement_delay_matrix",
                "router_lookahead",
                "tile_references",
                "io_pin_table",
                "mux_module_file",
                "config_mem_file",
                "fabric_pin_phy_loc",
                "fabric_hierarchy",
                "undriven_makeup_key.xml",
                "bscan_makeup_key.xml",
                "clkn_makeup_key.xml",
                "synth_pdk_config",
                "synth_qor.csv",
                "tile_qor.csv",
                "base_bitstream.xml",
                "bitstream_value.xml",
                "bitstream_distribution.xml",
                "arch_timing",
                "shaping.txt"};
  VAR_NAMES_ = {"VPR_ARCH_FILE",
                "OPENFPGA_ARCH_FILE",
                "OPENFPGA_CLOCK_ARCH_FILE",
                "OPENFPGA_CKBUF_LOC_MAP_FILE",
                "DESIGN_VAR_YAML", /* Placeholder only */
                "OPENFPGA_SIM_SETTING_FILE",
                "OPENFPGA_BITSTREAM_SETTING_FILE",
                "OPENFPGA_BITSTREAM_SETTING4GCLK_FILE",
                "OPENFPGA_TILE_CONFIG_FILE",
                "OPENFPGA_PIN_CONSTRAINTS_FILE",
                "OPENFPGA_VPR_SDC",
                "OPENFPGA_PCF",
                "EXTERNAL_FABRIC_KEY_FILE",
                "OPENFPGA_REF_MODULE_FILE",
                "OPENFPGA_RENAME_MODULE_FILE",
                "OPENFPGA_VPR_RR_GRAPH_FILE",
                "OPENFPGA_VPR_PLACEMENT_DELAY_LOOKUP_FILE",
                "OPENFPGA_VPR_ROUTER_LOOKAHEAD_FILE",
                "OPENFPGA_TILE_REFERENCE_FILE",
                "EFPGA_IO_PIN_TABLE",
                "OPENFPGA_MUX_MODULE_FILE",
                "OPENFPGA_CONFIG_MEM_MODULE_FILE",
                "OPENFPGA_FABRIC_PIN_PHY_LOC_FILE",
                "OPENFPGA_FABRIC_HIERARCHY_FILE",
                "undriven_makeup_key.xml",
                "bscan_makeup_key.xml",
                "clkn_makeup_key.xml",
                "synth_pdk_config",
                "synth_qor.csv",
                "tile_qor.csv",
                "OPENFPGA_BASE_BITSTREAM_FILE",
                "OPENFPGA_BITSTREAM_XML_FILE",
                "OPENFPGA_BITSTREAM_DISTRIBUTION_FILE",
                "arch_timing",
                "shaping.txt"};
  SCRIPT_NAMES_ = {"generate_bitstream.openfpga", "generate_gclk_bitstream.openfpga",
                   "generate_grst_bitstream.openfpga", "generate_base_bitstream.openfpga",
                   "generate_fabric.openfpga"};
  SCRIPT_TYPE_STRING_ = {"generate_bitstream", "generate_gclk_bitstream", "generate_grst_bitstream",
                         "generate_base_bitstream", "generate_fabric"};

  FABRIC_NETLIST_FILENAME_ = "fabric_netlists.v";
  NETLIST_FLAG_FILENAME_ = "fpga_defines.v";
  NETLIST_FPGA_TOP_FILENAME_ = "fpga_top.v";
  NETLIST_FPGA_CORE_FILENAME_ = "fpga_core.v";
  NETLIST_INCLUDE_FILENAME_ = "efpga_circuits.v";
  OPENFPGA_LOG_FILENAME_ = "openfpga.log";
  SYNTH_COMPILE_CONFIG_FILENAME_ = "synth_task.yaml";
  SYNTH_ANALYZE_CONFIG_FILENAME_ = "analyze_task.yaml";
  ARCH_TIMING_FILE_PREFIX_ = "arch_timing_";
  ARCH_TIMING_FILE_POSTFIX_ = ".yaml";

  OPT_STAGE_ALL_ = "all";
  OPT_STAGE_SETUP_ = "setup";
  OPT_STAGE_SYNTH_ = "synth";
  OPT_STAGE_GEN_GL_ = "gen_gl";

  MD5_FILENAME_ = "results_md5.yaml";

  if (openfpga::CMD_EXEC_SUCCESS !=
      openfpga_log_checker_.load_keywords_from_xml(openfpga_error_conf_file, false)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::array<NetlistDevManager::e_fabric_type, size_t(NetlistDevManager::e_fabric_type::NUM_TYPES)>
NetlistDevManager::fabrics() const {
  return {NetlistDevManager::e_fabric_type::DP, NetlistDevManager::e_fabric_type::ULTIMATE};
}

std::array<NetlistDevManager::e_netlist_type, size_t(NetlistDevManager::e_netlist_type::NUM_TYPES)>
NetlistDevManager::netlist_types() const {
  return {NetlistDevManager::e_netlist_type::RAW_RTL, NetlistDevManager::e_netlist_type::RTL,
          NetlistDevManager::e_netlist_type::GL};
}

int NetlistDevManager::show_file(const e_fabric_type& fb_type, const e_file_type& f_type,
                                 const bool& verbose) const {
  int num_err = 0;
  std::string fpath = file(fb_type, f_type, verbose);
  VTR_LOG("The architecture data file is located:\n\t%s\n", fpath.c_str());
  if (!std::filesystem::exists(fpath)) {
    VTR_LOG_ERROR("File '%s' does not exist!\n", fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::ifstream fcntt(fpath);
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
  while (std::getline(fcntt, line)) {
    VTR_LOG("%s\n", line.c_str());
  }
  fcntt.close();

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

std::filesystem::path NetlistDevManager::project_home_dir() const {
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(SUBDIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_);
  return proj_home;
}

std::string NetlistDevManager::report_dir() const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(RPT_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::rr_gsb_dir(const e_fabric_type& fb_type) const {
  std::filesystem::path dirpath = std::filesystem::path(report_dir()) /
                                  std::filesystem::path(fabric_type2str(fb_type)) /
                                  std::filesystem::path(RRGSB_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::netlist_dir(const e_fabric_type& fb_type,
                                           const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(NETLIST_DIR_NAME_) /
                                  std::filesystem::path(netlist_type2str(nlist_type)) /
                                  std::filesystem::path(fabric_type2str(fb_type));
  return dirpath.string();
}

std::string NetlistDevManager::netlist_release_dir(const e_netlist_type& nlist_type) const {
  return netlist_dir(NetlistDevManager::e_fabric_type::ULTIMATE, nlist_type);
}

std::string NetlistDevManager::netlist_block_dir(const e_fabric_type& fb_type,
                                                 const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = std::filesystem::path(netlist_dir(fb_type, nlist_type)) /
                                  std::filesystem::path(NETLIST_BLOCK_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::netlist_tile_dir(const e_fabric_type& fb_type,
                                                const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = std::filesystem::path(netlist_dir(fb_type, nlist_type)) /
                                  std::filesystem::path(NETLIST_TILE_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::synth_dir() const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(SYNTH_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::synth_sdc_ccff_dir() const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(SYNTH_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_SDC_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_CCFF_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::synth_sdc_dir() const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(SYNTH_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_SDC_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::synth_runtime_dir() const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(SYNTH_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_RUNTIME_DIR_NAME_);
  return dirpath.string();
}

std::string NetlistDevManager::synth_compile_config_file() const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(SYNTH_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_CONFIG_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_COMPILE_CONFIG_FILENAME_);
  return dirpath.string();
}

std::string NetlistDevManager::synth_analyze_config_file() const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(SYNTH_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_CONFIG_DIR_NAME_) /
                                  std::filesystem::path(SYNTH_ANALYZE_CONFIG_FILENAME_);
  return dirpath.string();
}

std::string NetlistDevManager::report_timing_dirpath(const e_fabric_type& fb_type,
                                                     const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(RPT_DIR_NAME_) /
                                  std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
                                  std::filesystem::path(RPT_TIMING_DIR_NAME_) /
                                  std::filesystem::path(NETLIST_TYPE_STRING_[size_t(nlist_type)]);
  return dirpath.string();
}

std::string NetlistDevManager::ppa_summary_dirpath(const e_fabric_type& fb_type,
                                                   const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(RPT_DIR_NAME_) /
                                  std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
                                  std::filesystem::path(RPT_SUMMARY_DIR_NAME_) /
                                  std::filesystem::path(NETLIST_TYPE_STRING_[size_t(nlist_type)]);
  return dirpath.string();
}

std::string NetlistDevManager::global_port_sweep_file(const e_file_type& f_type,
                                                      const e_script_type& runtime_type,
                                                      const size_t& sweep_id,
                                                      const bool& verbose) const {
  /* Error out as the sweep type requires a different function */
  if (runtime_type != e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN &&
      runtime_type != e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The non-global signal sweep script should use a regular runtime "
      "directory\n");
    return "";
  }

  check_valid_file_type(f_type);
  std::filesystem::path proj_home = project_home_dir();
  std::filesystem::path ret;
  switch (f_type) {
    case e_file_type::BITSTREAM_XML_FILE:
      ret = proj_home / std::filesystem::path(RPT_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(e_fabric_type::ULTIMATE)]) /
            std::filesystem::path(RPT_BITSTREAM_DIST_DIR_NAME_) /
            std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
            std::filesystem::path(SWEEP_DIR_PREFIX_ + std::to_string(sweep_id)) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    default: {
      VTR_LOG_ERROR("Invalid file type!\n");
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
  }
  return ret.string();
}

std::string NetlistDevManager::file(const e_fabric_type& fb_type, const e_file_type& f_type,
                                    const bool& verbose) const {
  check_valid_fabric_type(fb_type);
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = project_home_dir();
  std::filesystem::path ret;
  switch (f_type) {
    case e_file_type::VPR_ARCH:
    case e_file_type::OPENFPGA_ARCH:
    case e_file_type::OPENFPGA_GNTWK_ARCH:
    case e_file_type::OPENFPGA_CKBUF_LOC_MAP:
    case e_file_type::DESIGN_VAR:
    case e_file_type::FINAL_FABRIC_KEY:
    case e_file_type::REF_MODULE_NAME:
    case e_file_type::OPENFPGA_BITSTREAM_SETTING:
    case e_file_type::CRAFTED_MODULE_NAME: {
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    }
    case e_file_type::RRG:
    case e_file_type::PDL:
    case e_file_type::RLA: {
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(default_corner_ + "_" + FILE_NAMES_[size_t(f_type)]);
      break;
    }
    case e_file_type::PIN_TABLE: {
      ret = proj_home / std::filesystem::path(PINTABLE_RUNDIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    }
    /* External files should come from device data navigator */
    case e_file_type::OPENFPGA_SIM_SETTING:
    case e_file_type::OPENFPGA_TILE_CONFIG:
    case e_file_type::OPENFPGA_PCF_XML:
    case e_file_type::VPR_SDC:
    case e_file_type::OPENFPGA_PCF:
      ret = std::filesystem::path(src_openfpga_setting_files_.at(f_type));
      break;
    case e_file_type::MUX_MODULE_FILE:
    case e_file_type::CONFIG_MEM_MODULE_FILE:
    case e_file_type::FABRIC_PIN_PHY_LOC_FILE:
    case e_file_type::FABRIC_HIERARCHY_FILE:
    case e_file_type::FLOORPLAN_SHAPING_TXT:
    case e_file_type::TILE_REF: {
      ret = proj_home / std::filesystem::path(RPT_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    }
    case e_file_type::UNDRIVEN_MAKEUP_KEY:
    case e_file_type::BSCAN_MAKEUP_KEY:
    case e_file_type::CLKN_MAKEUP_KEY:
      ret = std::filesystem::path(src_netlist_makeup_dir_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::SYNTH_PDK_CONFIG:
      ret = proj_home / std::filesystem::path(SYNTH_DIR_NAME_) /
            std::filesystem::path(PDK_DIR_NAME_) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::SYNTH_BLOCK_QOR_FILE:
    case e_file_type::SYNTH_TILE_QOR_FILE:
      ret = proj_home / std::filesystem::path(RPT_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(RPT_AREA_DIR_NAME_) /
            std::filesystem::path(NETLIST_TYPE_STRING_[size_t(e_netlist_type::GL)]) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::BASE_BITSTREAM_FILE:
    case e_file_type::BITSTREAM_DIST_FILE:
      ret = proj_home / std::filesystem::path(RPT_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
            std::filesystem::path(RPT_BITSTREAM_DIST_DIR_NAME_) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::ARCH_TIMING: {
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(FABRIC_TYPE_STRING_[size_t(e_fabric_type::ULTIMATE)]);
      std::string l_fname = std::string(ARCH_TIMING_FILE_PREFIX_) + default_corner_ +
                            std::string(ARCH_TIMING_FILE_POSTFIX_);
      ret = ret / std::filesystem::path(l_fname);
      break;
    }
    default: {
      VTR_LOG_ERROR("Invalid file type!\n");
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
  }
  return ret.string();
}

std::string NetlistDevManager::openfpga_script_file(const e_script_type& runtime_type) const {
  return std::filesystem::path(src_openfpga_script_files_[size_t(runtime_type)]).string();
}

std::string NetlistDevManager::openfpga_runtime_file(const e_fabric_type& fb_type,
                                                     const e_file_type& f_type,
                                                     const e_script_type& runtime_type,
                                                     const bool& verbose) const {
  /* Error out as the sweep type requires a different function */
  if (runtime_type == e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN ||
      runtime_type == e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The global signal sweep script should use a dedicated runtime "
      "directory\n");
    return "";
  }

  check_valid_fabric_type(fb_type);
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = project_home_dir();
  proj_home = proj_home / std::filesystem::path(OPENFPGA_RUNDIR_NAME_);

  if (f_type == e_file_type::VPR_ARCH || f_type == e_file_type::OPENFPGA_ARCH) {
    std::filesystem::path ret = proj_home /
                                std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
                                std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
                                std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
    return ret.string();
  }
  return "";
}

std::string NetlistDevManager::openfpga_runtime_sweep_file(const e_fabric_type& fb_type,
                                                           const e_file_type& f_type,
                                                           const e_script_type& runtime_type,
                                                           const size_t& sweep_id,
                                                           const bool& verbose) const {
  /* Error out as the sweep type requires a different function */
  if (runtime_type != e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN &&
      runtime_type != e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The non-global signal sweep script should use a regular runtime "
      "directory\n");
    return "";
  }
  check_valid_fabric_type(fb_type);
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = project_home_dir();
  proj_home = proj_home / std::filesystem::path(OPENFPGA_RUNDIR_NAME_);

  std::filesystem::path ret = proj_home /
                              std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
                              std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
                              std::filesystem::path(SWEEP_DIR_PREFIX_ + std::to_string(sweep_id)) /
                              std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
  return ret.string();
}

std::string NetlistDevManager::openfpga_runtime_script_file(const e_fabric_type& fb_type,
                                                            const e_script_type& runtime_type,
                                                            const bool& verbose) const {
  /* Error out as the sweep type requires a different function */
  if (runtime_type == e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN ||
      runtime_type == e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The global signal sweep script should use a dedicated runtime "
      "directory\n");
    return "";
  }

  check_valid_fabric_type(fb_type);
  std::filesystem::path proj_home = project_home_dir();
  proj_home = proj_home / std::filesystem::path(OPENFPGA_RUNDIR_NAME_);
  std::filesystem::path ret = proj_home /
                              std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
                              std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
                              std::filesystem::path(SCRIPT_NAMES_[size_t(runtime_type)]);
  return ret.string();
}

std::string NetlistDevManager::openfpga_runtime_sweep_script_file(const e_fabric_type& fb_type,
                                                                  const e_script_type& runtime_type,
                                                                  const size_t& sweep_id,
                                                                  const bool& verbose) const {
  /* Error out as the sweep type requires a different function */
  if (runtime_type != e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN &&
      runtime_type != e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The non-global signal sweep script should use a regular runtime "
      "directory\n");
    return "";
  }

  check_valid_fabric_type(fb_type);
  std::filesystem::path proj_home = project_home_dir();
  proj_home = proj_home / std::filesystem::path(OPENFPGA_RUNDIR_NAME_);
  std::filesystem::path ret = proj_home /
                              std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
                              std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
                              std::filesystem::path(SWEEP_DIR_PREFIX_ + std::to_string(sweep_id)) /
                              std::filesystem::path(SCRIPT_NAMES_[size_t(runtime_type)]);
  return ret.string();
}

std::string NetlistDevManager::openfpga_runtime_log_file(const e_fabric_type& fb_type,
                                                         const e_script_type& runtime_type,
                                                         const bool& verbose) const {
  check_valid_fabric_type(fb_type);
  std::filesystem::path proj_home = project_home_dir();
  proj_home = proj_home / std::filesystem::path(OPENFPGA_RUNDIR_NAME_);
  std::filesystem::path ret =
    proj_home / std::filesystem::path(FABRIC_TYPE_STRING_[size_t(fb_type)]) /
    std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
    std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)] + OPENFPGA_LOG_FILENAME_);
  return ret.string();
}

int NetlistDevManager::copy_file_with_mkdir(const std::string& src_file,
                                            const std::string& des_file, const bool& sym_link,
                                            const bool& verbose) const {
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
      if (std::filesystem::is_directory(des_file)) {
        std::filesystem::remove_all(des_file);
      } else {
        std::filesystem::remove(des_file);
      }
    }
    num_err += universal_create_symlink(src_file, des_file) ? 0 : 1;
  }
  if (!std::filesystem::exists(des_file)) {
    VTR_LOG_ERROR("Destination file '%s' does not exist!\n", des_file.c_str());
    num_err++;
  }
  return num_err;
}

int NetlistDevManager::init_netlist_dir(const e_fabric_type& fb_type, const bool& verbose) const {
  int num_err = 0;
  std::filesystem::path proj_home = project_home_dir();
  /* Link arch files */
  std::filesystem::path src_arch_dir = src_arch_dir_;
  std::filesystem::path des_arch_dir = proj_home / std::filesystem::path(ARCH_DIR_NAME_);
  num_err += copy_file_with_mkdir(src_arch_dir.string(), des_arch_dir.string(), true, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::init_netlist_dir_all(const bool& verbose) const {
  for (NetlistDevManager::e_fabric_type fb_type : fabrics()) {
    int status = init_netlist_dir(fb_type, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::init_runtime_dir(const e_fabric_type& fb_type,
                                        const e_script_type& runtime_type,
                                        const bool& verbose) const {
  /* Error out as the non-sweep type requires a different function */
  if (runtime_type == e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN ||
      runtime_type == e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The global signal sweep script should use a dedicated runtime "
      "directory\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  int num_err = 0;

  ArchAggregator arch_agg(file(fb_type, e_file_type::DESIGN_VAR, verbose), verbose);
  arch_agg.load_variables(file(fb_type, e_file_type::ARCH_TIMING, verbose), false, false, verbose);
  /* Generate the final vpr and openfpga arch file */
  std::string input_vpr_arch = file(fb_type, e_file_type::VPR_ARCH, verbose);
  std::string runtime_vpr_arch =
    openfpga_runtime_file(fb_type, e_file_type::VPR_ARCH, runtime_type, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_vpr_dirpath = runtime_vpr_arch;
  openfpga::create_directory(runtime_vpr_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_vpr_arch, runtime_vpr_arch, verbose);

  /* Generate the final vpr and openfpga arch file */
  std::string input_openfpga_arch = file(fb_type, e_file_type::OPENFPGA_ARCH, verbose);
  std::string runtime_openfpga_arch =
    openfpga_runtime_file(fb_type, e_file_type::OPENFPGA_ARCH, runtime_type, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_openfpga_dirpath = runtime_openfpga_arch;
  openfpga::create_directory(runtime_openfpga_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_openfpga_arch, runtime_openfpga_arch, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::init_runtime_sweep_dir(const e_fabric_type& fb_type,
                                              const e_script_type& runtime_type,
                                              const size_t& sweep_id, const bool& verbose) const {
  /* Error out as the sweep type requires a different function */
  if (runtime_type != e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN &&
      runtime_type != e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The non-global signal sweep script should use a regular runtime "
      "directory\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  int num_err = 0;

  ArchAggregator arch_agg(file(fb_type, e_file_type::DESIGN_VAR, verbose), verbose);
  arch_agg.load_variables(file(fb_type, e_file_type::ARCH_TIMING, verbose), false, false, verbose);
  /* Generate the final vpr and openfpga arch file */
  std::string input_vpr_arch = file(fb_type, e_file_type::VPR_ARCH, verbose);
  std::string runtime_vpr_arch =
    openfpga_runtime_sweep_file(fb_type, e_file_type::VPR_ARCH, runtime_type, sweep_id, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_vpr_dirpath = runtime_vpr_arch;
  openfpga::create_directory(runtime_vpr_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_vpr_arch, runtime_vpr_arch, verbose);

  /* Generate the final vpr and openfpga arch file */
  std::string input_openfpga_arch = file(fb_type, e_file_type::OPENFPGA_ARCH, verbose);
  std::string runtime_openfpga_arch = openfpga_runtime_sweep_file(
    fb_type, e_file_type::OPENFPGA_ARCH, runtime_type, sweep_id, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_openfpga_dirpath = runtime_openfpga_arch;
  openfpga::create_directory(runtime_openfpga_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_openfpga_arch, runtime_openfpga_arch, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::init_runtime_script(const e_fabric_type& fb_type,
                                           const e_script_type& runtime_type,
                                           const BenchmarkPacket& bm_pkt,
                                           const bool& verbose) const {
  /* Error out as the non-sweep type requires a different function */
  if (runtime_type == e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN ||
      runtime_type == e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The global signal sweep script should use a dedicated runtime "
      "directory\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  int num_err = 0;
  /* Generate the final openfpga script file */
  ArchAggregator script_agg;
  /* Add variables. */
  for (size_t itype = 0; itype < size_t(e_file_type::NUM_TYPES); ++itype) {
    e_file_type curr_type = static_cast<e_file_type>(itype);
    if (curr_type == e_file_type::VPR_ARCH || curr_type == e_file_type::OPENFPGA_ARCH) {
      std::string rt_file = openfpga_runtime_file(fb_type, curr_type, runtime_type, verbose);
      script_agg.add_variable(VAR_NAMES_[itype], rt_file);
      continue;
    }
    if (curr_type == e_file_type::DESIGN_VAR || curr_type == e_file_type::BITSTREAM_XML_FILE ||
        curr_type == e_file_type::OPENFPGA_BITSTREAM_SETTING_APPEND) {
      continue;
    }
    script_agg.add_variable(VAR_NAMES_[itype], file(fb_type, curr_type, verbose));
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
  /* Add netlist info */
  script_agg.add_variable(OPENFPGA_RRGSB_DIR_VAR_NAME_, rr_gsb_dir(fb_type));
  script_agg.add_variable(OPENFPGA_NETLIST_DIR_VAR_NAME_,
                          netlist_dir(fb_type, e_netlist_type::RAW_RTL));

  /* Do aggregation here */
  std::string input_openfpga_script = openfpga_script_file(runtime_type);
  std::string runtime_openfpga_script =
    openfpga_runtime_script_file(fb_type, runtime_type, verbose);
  // std::string runtime_openfpga_log = openfpga_runtime_log_file(fb_type,
  // e_script_type::OPENFPGA_SCRIPT_DEVICEDATA, verbose);
  num_err += script_agg.aggregate(input_openfpga_script, runtime_openfpga_script, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::init_runtime_sweep_script(const e_fabric_type& fb_type,
                                                 const e_script_type& runtime_type,
                                                 const size_t& sweep_id,
                                                 const BenchmarkPacket& bm_pkt,
                                                 const bool& verbose) const {
  /* Error out as the non-sweep type requires a different function */
  if (runtime_type != e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN &&
      runtime_type != e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The non-global signal sweep script should use a regular runtime "
      "directory\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  int num_err = 0;
  /* Generate the final openfpga script file */
  ArchAggregator script_agg;
  /* Add variables. */
  for (size_t itype = 0; itype < size_t(e_file_type::NUM_TYPES); ++itype) {
    e_file_type curr_type = static_cast<e_file_type>(itype);
    if (curr_type == e_file_type::VPR_ARCH || curr_type == e_file_type::OPENFPGA_ARCH) {
      std::string rt_file =
        openfpga_runtime_sweep_file(fb_type, curr_type, runtime_type, sweep_id, verbose);
      script_agg.add_variable(VAR_NAMES_[itype], rt_file);
      continue;
    }
    if (curr_type == e_file_type::DESIGN_VAR || curr_type == e_file_type::OPENFPGA_PCF_XML ||
        curr_type == e_file_type::BITSTREAM_XML_FILE ||
        curr_type == e_file_type::OPENFPGA_BITSTREAM_SETTING_APPEND) {
      continue;
    }
    script_agg.add_variable(VAR_NAMES_[itype], file(fb_type, curr_type, verbose));
  }
  /* Add other variables, which are related to flow control. i.e., layout, channel width etc. */
  script_agg.add_variable(VPR_FABRIC_VAR_NAME_, fabric_type2str(fb_type));
  script_agg.add_variable(VPR_ROUTE_CHAN_W_VAR_NAME_, route_chan_width_);
  /* Add benchmark info */
  script_agg.add_variable(VPR_BLIF_VAR_NAME_, bm_pkt.post_synth_netlist());
  if (!bm_pkt.sdc().empty()) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::VPR_SDC)], bm_pkt.sdc());
  }
  /* Use the generated PCF not the original one */
  script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_PCF_XML)],
                          openfpga_runtime_sweep_file(fb_type, e_file_type::OPENFPGA_PCF_XML,
                                                      runtime_type, sweep_id, verbose));
  /* Add bitstream setting to append */
  script_agg.add_variable(
    VAR_NAMES_[size_t(e_file_type::OPENFPGA_BITSTREAM_SETTING_APPEND)],
    openfpga_runtime_sweep_file(fb_type, e_file_type::OPENFPGA_BITSTREAM_SETTING_APPEND,
                                runtime_type, sweep_id, verbose));
  /* Special: only for global port sweeping runs. The bitstream file have to kept for later use
   * during design verification and qor sign-off */
  script_agg.add_variable(
    VAR_NAMES_[size_t(e_file_type::BITSTREAM_XML_FILE)],
    global_port_sweep_file(e_file_type::BITSTREAM_XML_FILE, runtime_type, sweep_id, verbose));
  /* Do aggregation here */
  std::string input_openfpga_script = openfpga_script_file(runtime_type);
  std::string runtime_openfpga_script =
    openfpga_runtime_sweep_script_file(fb_type, runtime_type, sweep_id, verbose);
  // std::string runtime_openfpga_log = openfpga_runtime_log_file(fb_type,
  // e_script_type::OPENFPGA_SCRIPT_DEVICEDATA, verbose);
  num_err += script_agg.aggregate(input_openfpga_script, runtime_openfpga_script, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::runtime_exec_openfpga(const e_fabric_type& fb_type,
                                             const e_script_type& runtime_type,
                                             const std::string& openfpga_bin_path,
                                             const bool& verbose) {
  /* Error out as the non-sweep type requires a different function */
  if (runtime_type == e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN ||
      runtime_type == e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The global signal sweep script should use a dedicated runtime "
      "directory\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  int num_err = 0;
  std::string runtime_openfpga_script =
    openfpga_runtime_script_file(fb_type, runtime_type, verbose);
  /* Run openfpga */
  std::string openfpga_cmd = openfpga_bin_path + " -batch -f " + runtime_openfpga_script +
                             std::string(" > ") + OPENFPGA_LOG_FILENAME_;
  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = std::filesystem::path(runtime_openfpga_script).parent_path();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run OpenFPGA!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(openfpga_cmd.c_str())) {
    VTR_LOG_ERROR("OpenFPGA run failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

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

int NetlistDevManager::runtime_exec_sweep_openfpga(const e_fabric_type& fb_type,
                                                   const e_script_type& runtime_type,
                                                   const size_t& sweep_id,
                                                   const std::string& openfpga_bin_path,
                                                   const bool& verbose) {
  /* Error out as the non-sweep type requires a different function */
  if (runtime_type != e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN &&
      runtime_type != e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN) {
    VTR_LOG_ERROR(
      "This is an internal error. The non-global signal sweep script should use a regular runtime "
      "directory\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  int num_err = 0;
  std::string runtime_openfpga_script =
    openfpga_runtime_sweep_script_file(fb_type, runtime_type, sweep_id, verbose);
  /* Run openfpga */
  std::string openfpga_cmd = openfpga_bin_path + " -batch -f " + runtime_openfpga_script +
                             std::string(" > ") + OPENFPGA_LOG_FILENAME_;
  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = std::filesystem::path(runtime_openfpga_script).parent_path();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run OpenFPGA!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(openfpga_cmd.c_str())) {
    VTR_LOG_ERROR("OpenFPGA run failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

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

int NetlistDevManager::check_global_port_sweep_result_files(
  const std::vector<e_file_type>& f_types_to_check, const e_script_type& runtime_type,
  const size_t& sweep_id, const bool& verbose) const {
  int num_err = 0;
  for (e_file_type f_type_to_check : f_types_to_check) {
    std::string chk_fpath =
      global_port_sweep_file(f_type_to_check, runtime_type, sweep_id, verbose);
    if (!std::filesystem::exists(chk_fpath)) {
      VTR_LOG_ERROR("Expected resulting file %s is not generated:\n\t%s\n",
                    FILE_TYPE_NAMES_[size_t(f_type_to_check)].c_str(), chk_fpath.c_str());
      num_err++;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::check_result_files(const e_fabric_type& fb_type,
                                          const std::vector<e_file_type>& f_types_to_check,
                                          const bool& verbose) const {
  int num_err = 0;
  for (e_file_type f_type_to_check : f_types_to_check) {
    std::string chk_fpath = file(fb_type, f_type_to_check, verbose);
    if (!std::filesystem::exists(chk_fpath)) {
      VTR_LOG_ERROR("Expected resulting file %s is not generated:\n\t%s\n",
                    FILE_TYPE_NAMES_[size_t(f_type_to_check)].c_str(), chk_fpath.c_str());
      num_err++;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::import_external_netlists(const e_fabric_type& fb_type,
                                                const bool& verbose) const {
  int num_err = 0;
  std::filesystem::path des_nlist_dir = netlist_dir(fb_type, e_netlist_type::RAW_RTL);
  des_nlist_dir = des_nlist_dir / std::filesystem::path(NETLIST_CUSTOM_MODULE_DIR_NAME_);
  std::vector<std::string> ext_fnames;
  /* Default net type for each external netlist */
  std::vector<openfpga::e_verilog_default_net_type> ext_nlist_default_nettypes;

  VTR_LOG("Importing external IP netlists...\n");
  for (std::string src_fpath : ip_files_) {
    std::string src_fname = std::filesystem::path(src_fpath).filename().string();
    ext_fnames.push_back(src_fname);
    /* This is hardcoded. Should be driven by an option from project file */
    ext_nlist_default_nettypes.push_back(openfpga::VERILOG_DEFAULT_NET_TYPE_WIRE);
    std::string des_fpath = std::filesystem::path(des_nlist_dir / src_fname).string();
    num_err += copy_file_with_mkdir(src_fpath, des_fpath, true, verbose);
  }
  VTR_LOG("Done\n");

  VTR_LOG("Importing external PDK netlists...\n");
  for (std::string src_fpath : pdk_verilog_files_) {
    std::string src_fname = std::filesystem::path(src_fpath).filename().string();
    ext_fnames.push_back(src_fname);
    /* This is hardcoded. Should be driven by an option from project file. Most PDK files follow the
     * convention where nets are implicitedly defined. So the flag is enforced */
    ext_nlist_default_nettypes.push_back(openfpga::VERILOG_DEFAULT_NET_TYPE_WIRE);
    std::string des_fpath = std::filesystem::path(des_nlist_dir / src_fname).string();
    num_err += copy_file_with_mkdir(src_fpath, des_fpath, true, verbose);
  }
  VTR_LOG("Done\n");

  /* Generate the file list */
  VTR_LOG("Creating include files for external netlists...\n");
  std::string incl_fname =
    std::filesystem::path(des_nlist_dir / std::filesystem::path(NETLIST_INCLUDE_FILENAME_))
      .string();
  openfpga::mmostream o_incl_f(incl_fname, false); // TODO: Enable compression through a command-line option
  if (!valid_file_mmostream(o_incl_f)) {
    VTR_LOG_ERROR("Fail to open the custom module include file '%s'!\n", incl_fname.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (size_t f_idx = 0; f_idx < ext_fnames.size(); f_idx++) {
    openfpga::print_verilog_default_net_type_declaration(o_incl_f,
                                                         ext_nlist_default_nettypes[f_idx]);
    openfpga::print_verilog_include_netlist(o_incl_f, ext_fnames[f_idx]);
    /* Follow the convention of most Verilog files. Assume the next one follow the default net type
     */
    openfpga::print_verilog_default_net_type_declaration(o_incl_f,
                                                         openfpga::VERILOG_DEFAULT_NET_TYPE_WIRE);
  }
  o_incl_f.close();
  VTR_LOG("Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::append_flag_to_netlist(const e_fabric_type& fb_type,
                                              const bool& verbose) const {
  int num_err = 0;
  std::filesystem::path des_nlist = netlist_dir(fb_type, e_netlist_type::RAW_RTL);
  des_nlist = des_nlist / std::filesystem::path(NETLIST_FLAG_FILENAME_);
  std::ofstream nlist_f(des_nlist.c_str(), std::ios::app);
  if (!nlist_f.is_open()) {
    VTR_LOG_ERROR("Fail to open the netlist flag file '%s'!\n", des_nlist.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Appending flags to netlist '%s'...\n", des_nlist.c_str());
  size_t num_flags_added = 0;
  for (auto result : netlist_flags_) {
    // Using fstream sees bugs on CentOS: line is not appended. Ubuntu works fine
    nlist_f << openfpga::generate_verilog_define_flag(result.first, std::stoi(result.second))
            << std::endl;
    num_flags_added++;
  }
  VTR_LOG("Appended %ld flags\n", num_flags_added);
  nlist_f.close();
  VTR_LOG("Done\n");
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::netlist_makeup(const e_fabric_type& fb_type,
                                      const std::string& netlist_makeup_bin_path,
                                      const bool& duplicate_netlist_only,
                                      const bool& verbose) const {
  int num_err = 0;
  /* Duplicate netlists */
  std::filesystem::path src_dirpath = netlist_dir(fb_type, e_netlist_type::RAW_RTL);
  std::filesystem::path des_dirpath = netlist_dir(fb_type, e_netlist_type::RTL);
  if (std::filesystem::exists(des_dirpath)) {
    std::filesystem::remove_all(des_dirpath);
  }
  openfpga::create_directory(des_dirpath.parent_path().string(), true, verbose);
  std::filesystem::copy(src_dirpath, des_dirpath,
                        std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::copy_symlinks);
  if (duplicate_netlist_only) {
    VTR_LOG("Netlist makeup is skipped\n");
    VTR_LOG("Done\n");
    return openfpga::CMD_EXEC_SUCCESS;
  }

  VTR_LOG("Apply netlist makeup...\n");
  std::string nmkup_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + netlist_makeup_bin_path);
  std::filesystem::path fabric_nlist_file =
    netlist_dir(fb_type, e_netlist_type::RAW_RTL) / std::filesystem::path(FABRIC_NETLIST_FILENAME_);
  nmkup_cmd += std::string(" --fabric_netlist ") + fabric_nlist_file.string();
  nmkup_cmd +=
    std::string(" --fabric_key ") + file(fb_type, e_file_type::FINAL_FABRIC_KEY, verbose);
  nmkup_cmd += std::string(" --reference_module_names ") +
               file(fb_type, e_file_type::REF_MODULE_NAME, verbose);
  nmkup_cmd +=
    std::string(" --module_names ") + file(fb_type, e_file_type::CRAFTED_MODULE_NAME, verbose);
  nmkup_cmd += std::string(" --undriven_makeup_key_file ") +
               file(fb_type, e_file_type::UNDRIVEN_MAKEUP_KEY, verbose);
  nmkup_cmd += std::string(" --bscan_makeup_key_file ") +
               file(fb_type, e_file_type::BSCAN_MAKEUP_KEY, verbose);
  nmkup_cmd +=
    std::string(" --clkn_makeup_key_file ") + file(fb_type, e_file_type::CLKN_MAKEUP_KEY, verbose);
  nmkup_cmd += std::string(" --output_dir ") + des_dirpath.string() +
               "/"; /* Due to a bug of the makeup script, the backslash has to be added */
  if (!netlist_makeup_config_file_.empty()) {
    VTR_LOGV(verbose, "Netlist makeup will run under a custom configuration\n");
    nmkup_cmd += std::string(" --config_file ") + netlist_makeup_config_file_;
  }

  VTR_LOGV(verbose, "Running command:\n\t%s\n", nmkup_cmd.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run netlist makeup for fabric '%s'!\n",
                  fabric_type2str(fb_type).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(nmkup_cmd.c_str())) {
    VTR_LOG_ERROR("Netlist makeup run failed with errors for fabric '%s'!\n",
                  fabric_type2str(fb_type).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::generate_netlist(const e_fabric_type& fb_type, const BenchmarkPacket& bm_pkt,
                                        const std::string& openfpga_bin_path,
                                        const std::string& netlist_makeup_bin_path,
                                        const bool& enable_netlist_makeup, const bool& verbose) {
  int status = init_runtime_dir(fb_type, e_script_type::OPENFPGA_SCRIPT_FABRIC, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = init_runtime_script(fb_type, e_script_type::OPENFPGA_SCRIPT_FABRIC, bm_pkt, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = runtime_exec_openfpga(fb_type, e_script_type::OPENFPGA_SCRIPT_FABRIC, openfpga_bin_path,
                                 verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<e_file_type> f_types_to_check = {
    e_file_type::MUX_MODULE_FILE, e_file_type::CONFIG_MEM_MODULE_FILE,
    e_file_type::FABRIC_PIN_PHY_LOC_FILE, e_file_type::FABRIC_HIERARCHY_FILE};
  status = check_result_files(fb_type, f_types_to_check, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* TODO: Check netlist files and rr_gsb files */
  /* Copy ip files and pdk files */
  status = import_external_netlists(fb_type, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Append PDK flags */
  status = append_flag_to_netlist(fb_type, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Netlist makeup */
  bool duplicate_netlist_only = enable_netlist_makeup ? false : true;
  status = netlist_makeup(fb_type, netlist_makeup_bin_path, duplicate_netlist_only, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::generate_netlist_all(const BenchmarkPacket& bm_pkt,
                                            const std::string& openfpga_bin_path,
                                            const std::string& netlist_makeup_bin_path,
                                            const bool& enable_netlist_makeup,
                                            const bool& verbose) {
  for (NetlistDevManager::e_fabric_type fb_type : fabrics()) {
    int status = generate_netlist(fb_type, bm_pkt, openfpga_bin_path, netlist_makeup_bin_path,
                                  enable_netlist_makeup, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::generate_base_bitstream(const BenchmarkPacket& bm_pkt,
                                               const std::string& openfpga_bin_path,
                                               const bool& verbose) {
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  int status = init_runtime_dir(fb_type, e_script_type::OPENFPGA_SCRIPT_BASEBITGEN, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = init_runtime_script(fb_type, e_script_type::OPENFPGA_SCRIPT_BASEBITGEN, bm_pkt, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = runtime_exec_openfpga(fb_type, e_script_type::OPENFPGA_SCRIPT_BASEBITGEN,
                                 openfpga_bin_path, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::vector<e_file_type> f_types_to_check = {e_file_type::BASE_BITSTREAM_FILE,
                                               e_file_type::BITSTREAM_DIST_FILE};
  status = check_result_files(fb_type, f_types_to_check, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::generate_clock_sweep_bitstream(const BenchmarkPacket& bm_pkt,
                                                      const std::string& openfpga_bin_path,
                                                      const bool& verbose) {
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  e_script_type runtime_type = e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN;
  /* Parse clock buffer location map file to
   - find the number of clocks
   - find the pin name of clocks */
  std::string ckbuf_loc_fpath = file(fb_type, e_file_type::OPENFPGA_CKBUF_LOC_MAP, verbose);
  GlobalPortLoc gport_loc;
  int status = set_up_global_ports(ckbuf_loc_fpath, gport_loc);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<openfpga::BasicPort> ck_ports = gport_loc.clock_ports();
  std::vector<openfpga::BasicPort> rst_ports = gport_loc.reset_ports();
  /* Read the PCF of the default benchmark, to get the clock net name */
  std::string pcf_file_path = bm_pkt.pcf_gclk();
  VTR_LOG("Reading PCF file from: %s\n", pcf_file_path.c_str());
  if (!std::filesystem::exists(pcf_file_path)) {
    VTR_LOG_ERROR("PCF file does not exist: %s\n", pcf_file_path.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  PinConstraints pcf_info = read_xml_pin_constraints(pcf_file_path.c_str());
  std::string ck_net = std::string();
  size_t num_ck_nets = 0;
  for (auto ipcf : pcf_info.pin_constraints()) {
    std::string curr_net = pcf_info.net(ipcf);
    if (pcf_info.unconstrained_net(curr_net)) {
      continue;
    }
    openfpga::BasicPort pin = pcf_info.pin(ipcf);
    if (gport_loc.is_global_port_clk(pin)) {
      ck_net = curr_net;
      num_ck_nets++;
    }
  }
  /* Sanity checks: only 1 clock net is allowed */
  if (num_ck_nets == 0) {
    VTR_LOG_ERROR(
      "No clock nets are found in benchmark for clock sweeping. Expect 1 clock net in the pin "
      "constraint file\n\t%s\n",
      bm_pkt.pcf_gclk().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (num_ck_nets > 1) {
    VTR_LOG_ERROR(
      "Found %ld clock nets in benchmark for clock sweeping. Expect only 1 clock net in the pin "
      "constraint file\n\t%s\n",
      num_ck_nets, bm_pkt.pcf_gclk().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (ck_net.empty()) {
    VTR_LOG_ERROR(
      "Invalid clock net name found in benchmark for clock sweeping. Expect a non-empty string as "
      "the name in the pin constraint file\n\t%s\n",
      bm_pkt.pcf_gclk().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Found clock net name '%s'. Will use it in clock sweeping\n", ck_net.c_str());

  /* Sweep all the clocks available in the eFPGA */
  for (size_t iport = 0; iport < ck_ports.size(); ++iport) {
    VTR_LOG("Generate bitstream to force clock port '%s'...\n",
            ck_ports[iport].to_verilog_string().c_str());
    /* Generate PCF for the specific clock */
    PinConstraints curr_pcf_info;
    for (auto curr_ck_port : ck_ports) {
      if (curr_ck_port == ck_ports[iport]) {
        curr_pcf_info.create_pin_constraint(curr_ck_port, ck_net);
      } else {
        curr_pcf_info.create_pin_constraint(curr_ck_port, std::string(PIN_CONSTRAINT_OPEN_NET));
      }
    }
    for (auto curr_rst_port : rst_ports) {
      curr_pcf_info.create_pin_constraint(curr_rst_port, std::string(PIN_CONSTRAINT_OPEN_NET));
    }

    /* Generate Bitstream setting file for the specific clock */
    ArchGen arch_gen;
    openfpga::BitstreamSetting bitset;
    bitset.add_bitstream_clock_routing_setting(arch_gen.gclock_network_name(), ck_ports[iport]);

    /* Run */
    status = init_runtime_sweep_dir(fb_type, runtime_type, iport, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Some function may not be able to create directories itself. Use the init_runtime_dir to
     * create the directories */
    write_xml_pin_constraints(openfpga_runtime_sweep_file(fb_type, e_file_type::OPENFPGA_PCF_XML,
                                                          runtime_type, iport, verbose)
                                .c_str(),
                              curr_pcf_info);
    write_xml_openfpga_bitstream_settings(
      openfpga_runtime_sweep_file(fb_type, e_file_type::OPENFPGA_BITSTREAM_SETTING_APPEND,
                                  runtime_type, iport, verbose)
        .c_str(),
      bitset);
    status = init_runtime_sweep_script(fb_type, runtime_type, iport, bm_pkt, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    status = runtime_exec_sweep_openfpga(fb_type, runtime_type, iport, openfpga_bin_path, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* The expected output is the bitstream value file */
    std::vector<e_file_type> f_types_to_check = {e_file_type::BITSTREAM_XML_FILE};
    status = check_global_port_sweep_result_files(f_types_to_check, runtime_type, iport, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::generate_reset_sweep_bitstream(const BenchmarkPacket& bm_pkt,
                                                      const std::string& openfpga_bin_path,
                                                      const bool& verbose) {
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  e_script_type runtime_type = e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN;
  /* Parse clock buffer location map file to
   - find the number of clocks
   - find the pin name of clocks */
  std::string ckbuf_loc_fpath = file(fb_type, e_file_type::OPENFPGA_CKBUF_LOC_MAP, verbose);
  GlobalPortLoc gport_loc;
  int status = set_up_global_ports(ckbuf_loc_fpath, gport_loc);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::vector<openfpga::BasicPort> ck_ports = gport_loc.clock_ports();
  std::vector<openfpga::BasicPort> rst_ports = gport_loc.reset_ports();
  /* Read the PCF of the default benchmark, to get the clock net name */
  std::string pcf_file_path_reset = bm_pkt.pcf_gclk();
  VTR_LOG("Reading PCF file from: %s\n", pcf_file_path_reset.c_str());
  if (!std::filesystem::exists(pcf_file_path_reset)) {
    VTR_LOG_ERROR("[Reset Sweep] PCF file does not exist: %s\n", pcf_file_path_reset.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  PinConstraints pcf_info = read_xml_pin_constraints(pcf_file_path_reset.c_str());
  std::string rst_net = std::string();
  size_t num_rst_nets = 0;
  for (auto ipcf : pcf_info.pin_constraints()) {
    std::string curr_net = pcf_info.net(ipcf);
    if (pcf_info.unconstrained_net(curr_net)) {
      continue;
    }
    openfpga::BasicPort pin = pcf_info.pin(ipcf);
    if (gport_loc.is_global_port_reset(pin)) {
      rst_net = curr_net;
      num_rst_nets++;
    }
  }
  /* Sanity checks: only 1 clock net is allowed */
  if (num_rst_nets == 0) {
    VTR_LOG_ERROR(
      "No reset nets are found in benchmark for reset sweeping. Expect 1 reset net in the pin "
      "constraint file\n\t%s\n",
      bm_pkt.pcf_gclk().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (num_rst_nets > 1) {
    VTR_LOG_ERROR(
      "Found %ld reset nets in benchmark for reset sweeping. Expect only 1 reset net in the pin "
      "constraint file\n\t%s\n",
      num_rst_nets, bm_pkt.pcf_gclk().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (rst_net.empty()) {
    VTR_LOG_ERROR(
      "Invalid reset net name found in benchmark for reset sweeping. Expect a non-empty string as "
      "the name in the pin constraint file\n\t%s\n",
      bm_pkt.pcf_gclk().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Found reset net name '%s'. Will use it in clock sweeping\n", rst_net.c_str());

  /* Sweep all the clocks available in the eFPGA */
  for (size_t iport = 0; iport < rst_ports.size(); ++iport) {
    VTR_LOG("Generate bitstream to force reset port '%s'...\n",
            rst_ports[iport].to_verilog_string().c_str());
    /* Generate PCF for the specific clock */
    PinConstraints curr_pcf_info;
    for (auto curr_ck_port : ck_ports) {
      std::string curr_ck_net = pcf_info.pin_net(curr_ck_port);
      if (pcf_info.unconstrained_net(curr_ck_net)) {
        curr_pcf_info.create_pin_constraint(curr_ck_port, std::string(PIN_CONSTRAINT_OPEN_NET));
      } else {
        curr_pcf_info.create_pin_constraint(curr_ck_port, curr_ck_net);
      }
    }
    for (auto curr_rst_port : rst_ports) {
      if (curr_rst_port == rst_ports[iport]) {
        curr_pcf_info.create_pin_constraint(curr_rst_port, rst_net);
      } else {
        curr_pcf_info.create_pin_constraint(curr_rst_port, std::string(PIN_CONSTRAINT_OPEN_NET));
      }
    }

    /* Generate Bitstream setting file for the specific clock */
    ArchGen arch_gen;
    openfpga::BitstreamSetting bitset;
    bitset.add_bitstream_clock_routing_setting(arch_gen.greset_network_name(), rst_ports[iport]);

    /* Run */
    status = init_runtime_sweep_dir(fb_type, runtime_type, iport, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Some function may not be able to create directories itself. Use the init_runtime_dir to
     * create the directories */
    write_xml_pin_constraints(openfpga_runtime_sweep_file(fb_type, e_file_type::OPENFPGA_PCF_XML,
                                                          runtime_type, iport, verbose)
                                .c_str(),
                              curr_pcf_info);
    write_xml_openfpga_bitstream_settings(
      openfpga_runtime_sweep_file(fb_type, e_file_type::OPENFPGA_BITSTREAM_SETTING_APPEND,
                                  runtime_type, iport, verbose)
        .c_str(),
      bitset);
    status = init_runtime_sweep_script(fb_type, runtime_type, iport, bm_pkt, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    status = runtime_exec_sweep_openfpga(fb_type, runtime_type, iport, openfpga_bin_path, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* The expected output is the bitstream value file */
    std::vector<e_file_type> f_types_to_check = {e_file_type::BITSTREAM_XML_FILE};
    status = check_global_port_sweep_result_files(f_types_to_check, runtime_type, iport, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

void NetlistDevManager::set_source_arch_dir(const std::string& arch_dir) {
  src_arch_dir_ = arch_dir;
}

void NetlistDevManager::set_source_pdk_dir(const std::string& pdk_dir) { src_pdk_dir_ = pdk_dir; }

void NetlistDevManager::set_source_netlist_makeup_dir(const std::string& nmakeup_dir) {
  src_netlist_makeup_dir_ = nmakeup_dir;
}

void NetlistDevManager::set_source_synth_sdc_dir(const std::string& sdc_dir) {
  src_synth_sdc_dir_ = sdc_dir;
}

void NetlistDevManager::set_source_synth_strategy_file(const std::string& val) {
  src_synth_strategy_file_ = val;
}

void NetlistDevManager::set_source_floorplan_strategy_file(const std::string& val) {
  src_floorplan_strategy_file_ = val;
}

void NetlistDevManager::set_route_chan_width(const size_t& val) { route_chan_width_ = val; }

void NetlistDevManager::set_ccff_max_fanout(const size_t& val) { ccff_max_fanout_ = val; }

void NetlistDevManager::add_ip_file(const std::string& f_name,
                                    const TapeoutManager::e_ip_file_format& f_fmt) {
  ip_files_.push_back(f_name);
  ip_file_formats_.push_back(f_fmt);
}

void NetlistDevManager::add_ip_gl_file(const std::string& f_name) {
  ip_gl_files_.push_back(f_name);
}

bool NetlistDevManager::add_file(const e_fabric_type& fb_type,
                                 const NetlistDevManager::e_file_type& f_type,
                                 const std::string& f_name) {
  if (!valid_fabric_type(fb_type)) {
    return false;
  }
  if (!valid_file_type(f_type)) {
    return false;
  }
  switch (f_type) {
    case NetlistDevManager::e_file_type::OPENFPGA_SIM_SETTING:
    case NetlistDevManager::e_file_type::OPENFPGA_BITSTREAM_SETTING:
    case NetlistDevManager::e_file_type::OPENFPGA_TILE_CONFIG:
    case NetlistDevManager::e_file_type::OPENFPGA_PCF_XML:
    case NetlistDevManager::e_file_type::VPR_SDC:
    case NetlistDevManager::e_file_type::OPENFPGA_PCF: {
      src_openfpga_setting_files_[f_type] = f_name;
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid file type!\n");
      return false;
  }
  return true;
}

bool NetlistDevManager::add_openfpga_script(const NetlistDevManager::e_script_type& f_type,
                                            const std::string& f_name) {
  if (!valid_script_type(f_type)) {
    return false;
  }
  src_openfpga_script_files_[size_t(f_type)] = f_name;
  return true;
}

NetlistDevManager::e_file_type NetlistDevManager::convert_file_type(
  const TapeoutManager::e_arch_file_type& to_f_type) const {
  switch (to_f_type) {
    case TapeoutManager::e_arch_file_type::VPR_ARCH:
      return e_file_type::VPR_ARCH;
    case TapeoutManager::e_arch_file_type::OPENFPGA_ARCH:
      return e_file_type::OPENFPGA_ARCH;
    case TapeoutManager::e_arch_file_type::OPENFPGA_GNTWK_ARCH:
      return e_file_type::OPENFPGA_GNTWK_ARCH;
    case TapeoutManager::e_arch_file_type::DESIGN_VAR:
      return e_file_type::DESIGN_VAR;
    case TapeoutManager::e_arch_file_type::OPENFPGA_SIM_SETTING:
      return e_file_type::OPENFPGA_SIM_SETTING;
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

NetlistDevManager::e_script_type NetlistDevManager::convert_script_type(
  const TapeoutManager::e_script_type& to_f_type) const {
  switch (to_f_type) {
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN:
      return e_script_type::OPENFPGA_SCRIPT_BITGEN;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN:
      return e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN:
      return e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BASEBITGEN:
      return e_script_type::OPENFPGA_SCRIPT_BASEBITGEN;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_FABRIC:
      return e_script_type::OPENFPGA_SCRIPT_FABRIC;
    default:
      return e_script_type::NUM_TYPES;
  }
  return e_script_type::NUM_TYPES;
}

NetlistDevManager::e_fabric_type NetlistDevManager::str2fabric_type(const std::string& str,
                                                                    const bool& verbose) const {
  for (int itype = size_t(NetlistDevManager::e_fabric_type::DP);
       itype != size_t(NetlistDevManager::e_fabric_type::NUM_TYPES); ++itype) {
    if (str == std::string(FABRIC_TYPE_STRING_[itype])) {
      return static_cast<NetlistDevManager::e_fabric_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of openfpga fabric file! Expect %s\n",
                 fabric_type_all2str().c_str());
  return NetlistDevManager::e_fabric_type::NUM_TYPES;
}

std::string NetlistDevManager::fabric_type2str(const NetlistDevManager::e_fabric_type& type,
                                               const bool& verbose) const {
  if (!valid_fabric_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of openfpga fabric file! Expect %s\n",
                   fabric_type_all2str().c_str());
    return std::string();
  }
  return std::string(FABRIC_TYPE_STRING_[size_t(type)]);
}

std::string NetlistDevManager::fabric_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(NetlistDevManager::e_fabric_type::DP);
       itype != size_t(NetlistDevManager::e_fabric_type::NUM_TYPES); ++itype) {
    full_types += std::string(FABRIC_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool NetlistDevManager::valid_fabric_type(const NetlistDevManager::e_fabric_type& fb_type) const {
  return fb_type != NetlistDevManager::e_fabric_type::NUM_TYPES;
}

void NetlistDevManager::check_valid_fabric_type(
  const NetlistDevManager::e_fabric_type& fb_type) const {
  if (!valid_fabric_type(fb_type)) {
    VTR_LOG_ERROR("Invalid fabric type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

NetlistDevManager::e_file_type NetlistDevManager::str2file_type(const std::string& str,
                                                                const bool& verbose) const {
  for (int itype = size_t(NetlistDevManager::e_file_type::VPR_ARCH);
       itype != size_t(NetlistDevManager::e_file_type::NUM_TYPES); ++itype) {
    if (str == std::string(FILE_TAGS_[itype])) {
      return static_cast<NetlistDevManager::e_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of file! Expect %s\n", file_type_all2str().c_str());
  return NetlistDevManager::e_file_type::NUM_TYPES;
}

std::string NetlistDevManager::file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(NetlistDevManager::e_file_type::VPR_ARCH);
       itype != size_t(NetlistDevManager::e_file_type::NUM_TYPES); ++itype) {
    full_types += std::string(FILE_TAGS_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool NetlistDevManager::valid_file_type(const NetlistDevManager::e_file_type& f_type) const {
  return f_type != NetlistDevManager::e_file_type::NUM_TYPES;
}

bool NetlistDevManager::valid_netlist_type(
  const NetlistDevManager::e_netlist_type& nlist_type) const {
  return nlist_type != NetlistDevManager::e_netlist_type::NUM_TYPES;
}

bool NetlistDevManager::valid_script_type(const NetlistDevManager::e_script_type& f_type) const {
  return f_type != NetlistDevManager::e_script_type::NUM_TYPES;
}

bool NetlistDevManager::required_file_type(const TapeoutManager::e_arch_file_type& f_type) const {
  return valid_file_type(convert_file_type(f_type));
}

void NetlistDevManager::check_valid_file_type(const NetlistDevManager::e_file_type& f_type) const {
  if (!valid_file_type(f_type)) {
    VTR_LOG_ERROR("Invalid file type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void NetlistDevManager::check_valid_netlist_type(
  const NetlistDevManager::e_netlist_type& nlist_type) const {
  if (!valid_netlist_type(nlist_type)) {
    VTR_LOG_ERROR("Invalid netlist type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

NetlistDevManager::e_netlist_type NetlistDevManager::str2netlist_type(const std::string& str,
                                                                      const bool& verbose) const {
  for (int itype = size_t(NetlistDevManager::e_netlist_type::RAW_RTL);
       itype != size_t(NetlistDevManager::e_netlist_type::NUM_TYPES); ++itype) {
    if (str == std::string(NETLIST_TYPE_STRING_[itype])) {
      return static_cast<NetlistDevManager::e_netlist_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of netlist file! Expect %s\n",
                 netlist_type_all2str().c_str());
  return NetlistDevManager::e_netlist_type::NUM_TYPES;
}

std::string NetlistDevManager::netlist_type2str(const NetlistDevManager::e_netlist_type& type,
                                                const bool& verbose) const {
  if (!valid_netlist_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of netlist file! Expect %s\n",
                   netlist_type_all2str().c_str());
    return std::string();
  }
  return std::string(NETLIST_TYPE_STRING_[size_t(type)]);
}

std::string NetlistDevManager::netlist_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(NetlistDevManager::e_netlist_type::RAW_RTL);
       itype != size_t(NetlistDevManager::e_netlist_type::NUM_TYPES); ++itype) {
    full_types += std::string(NETLIST_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

void NetlistDevManager::add_pdk_verilog(const std::string& f_name) {
  pdk_verilog_files_.push_back(f_name);
}

bool NetlistDevManager::add_netlist_flag(const std::string& flag, const std::string& val) {
  auto result = netlist_flags_.find(flag);
  if (result != netlist_flags_.end() && result->second != val) {
    VTR_LOG_WARN("Attempt to redefine netlist flag '%s' from val '%s' to val '%s'!\n", flag.c_str(),
                 result->second.c_str(), val.c_str());
    return false;
  }
  netlist_flags_[flag] = val;
  return true;
}

int NetlistDevManager::ccff_makeup(const std::string& ccff_makeup_bin_path,
                                   const bool& verbose) const {
  int num_err = 0;
  std::filesystem::path rtl_dirpath = netlist_dir(e_fabric_type::ULTIMATE, e_netlist_type::RTL);
  std::filesystem::path ccff_dirpath = synth_sdc_ccff_dir();

  VTR_LOG("Apply CCFF makeup...\n");
  std::string cmkup_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + ccff_makeup_bin_path);
  std::filesystem::path fabric_nlist_file =
    rtl_dirpath / std::filesystem::path(FABRIC_NETLIST_FILENAME_);
  cmkup_cmd += std::string(" --fabric_netlist ") + fabric_nlist_file.string();
  cmkup_cmd += std::string(" --output_dir ") + ccff_dirpath.string();
  cmkup_cmd += std::string(" --fanout ") + std::to_string(ccff_max_fanout_);
  cmkup_cmd += std::string(" --buf_config ") + ccff_makeup_buf_config_file_;

  VTR_LOGV(verbose, "Running command:\n\t%s\n", cmkup_cmd.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run CCFF makeup!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(cmkup_cmd.c_str())) {
    VTR_LOG_ERROR("CCFF makeup run failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::setup_gl_placeholder_dir(const bool& verbose) const {
  int num_err = 0;

  VTR_LOGV(verbose, "Initialize gate-level netlist directories...\n");
  for (auto fb_type : fabrics()) {
    std::filesystem::path gl_nlist_dirpath = netlist_dir(fb_type, e_netlist_type::GL);
    VTR_LOGV(verbose, "Initialize gate-level netlist directory \n\t'%s'\n for fabric '%s'\n",
             gl_nlist_dirpath.string().c_str(), fabric_type2str(fb_type, verbose).c_str());
    openfpga::create_directory(gl_nlist_dirpath.string(), true, verbose);
    VTR_LOGV(verbose, "Done\n");
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::setup_rtl_synth(const bool& verbose) const {
  int num_err = 0;

  /* Copy sdc files */
  VTR_LOG("Initializing SDC files for netlist synthesis\n");
  std::filesystem::path src_sdc_dirpath = src_synth_sdc_dir_;
  std::filesystem::path des_sdc_dirpath = synth_sdc_dir();
  openfpga::create_directory(des_sdc_dirpath.parent_path().string(), true, verbose);
  std::filesystem::copy(src_sdc_dirpath, des_sdc_dirpath,
                        std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::copy_symlinks);
  VTR_LOG("Done\n");

  std::string pdk_config_fpath =
    file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_PDK_CONFIG, verbose);
  VTR_LOG("Generating PDK configuration to '%s'\n", pdk_config_fpath.c_str());
  num_err += write_xml_pdk_config_for_synth(pdk_config_fpath.c_str(), (*pdk_mgr_));
  if (num_err != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  /* Generate configuration file */
  std::string strtg_fpath = src_synth_strategy_file_;
  std::string des_config_fpath = synth_compile_config_file();
  VTR_LOG("Generating synthesis task configuration to '%s'\n", des_config_fpath.c_str());
  FabricSynthStrategy syn_strtg(booting_root_, pdk_mgr_->name(), default_corner_);
  /* Generate the task file */
  e_fabric_type fb_type = e_fabric_type::DP;
  syn_strtg.set_netlist_input(netlist_dir(fb_type, e_netlist_type::RTL));
  for (size_t ipf_idx = 0; ipf_idx < ip_files_.size(); ++ipf_idx) {
    if (ip_file_formats_[ipf_idx] == TapeoutManager::e_ip_file_format::SYSTEM_VERILOG) {
      std::filesystem::path ip_fpath(ip_files_[ipf_idx]);
      syn_strtg.add_netlist_system_verilog_filter(ip_fpath.filename().string());
    }
  }
  syn_strtg.set_netlist_output(netlist_block_dir(fb_type, e_netlist_type::GL));
  /* Exclude files include PDK files */
  syn_strtg.add_netlist_exclude(std::string(FABRIC_NETLIST_FILENAME_));
  syn_strtg.add_netlist_exclude(std::string(NETLIST_INCLUDE_FILENAME_));
  for (std::string pdk_vfile : pdk_verilog_files_) {
    std::filesystem::path pdk_vfpath = pdk_vfile;
    syn_strtg.add_netlist_exclude(pdk_vfpath.filename().string());
  }

  int status = read_xml_synth_strategy(strtg_fpath.c_str(), syn_strtg, verbose);
  if (!syn_strtg.validate_strategies()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Get all the cb/sb/grid blocks */
  fb_type = e_fabric_type::ULTIMATE;
  std::string fb_hie_file = file(fb_type, e_file_type::FABRIC_HIERARCHY_FILE, verbose);
  std::vector<std::string> syn_blk_names;
  status = read_yaml_tile_subblocks(fb_hie_file.c_str(), syn_blk_names, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (std::string syn_blk_name : syn_blk_names) {
    syn_strtg.add_design_for_synth(syn_blk_name, verbose);
  }
  if (!syn_strtg.validate_general_settings()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Sanity checks */
  float ref_cpp = 0.;
  status = check_synth_sclib_cpp(*pdk_mgr_, syn_strtg, ref_cpp, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Standard cell lib CPP check failed!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  float ref_sc_h = 0.;
  status = check_synth_sclib_height(*pdk_mgr_, syn_strtg, ref_sc_h, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Standard cell lib height check failed!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  status = syn_strtg.write_yaml_synth_task(des_config_fpath, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::synth_rtl(const std::string& synth_bin_path, const bool& dryrun,
                                 const size_t& max_jobs, const bool& verbose) const {
  int num_err = 0;
  std::string pdk_config_fpath =
    file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_PDK_CONFIG, verbose);

  VTR_LOG("Run netlist synthesis...\n");
  std::string syn_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + synth_bin_path);
  syn_cmd += std::string(" --config ") + synth_compile_config_file();
  syn_cmd += std::string(" --dc_rundir ") + synth_runtime_dir();
  syn_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;
  syn_cmd += std::string(" --jobs ") + std::to_string(max_jobs);
  syn_cmd += std::string(" --mux_modules ") +
             file(e_fabric_type::ULTIMATE, e_file_type::MUX_MODULE_FILE, verbose);
  syn_cmd += std::string(" --config_mem_instances ") +
             file(e_fabric_type::ULTIMATE, e_file_type::CONFIG_MEM_MODULE_FILE, verbose);
  if (dryrun) {
    syn_cmd += std::string(" --file_generation_only ");
  }

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = synth_dir();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  VTR_LOGV(verbose, "Running command:\n\t%s\n", syn_cmd.c_str());
  if (dryrun) {
    VTR_LOG_WARN("User choose to dryrun netlist synthesis.\n");
  }

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run netlist synthesis!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(syn_cmd.c_str())) {
    VTR_LOG_ERROR("Netlist synthesis failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::parse_post_synth_block_report(const std::string& synth_bin_path,
                                                     const size_t& max_jobs,
                                                     const bool& verbose) const {
  int num_err = 0;
  std::string pdk_config_fpath =
    file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_PDK_CONFIG, verbose);

  VTR_LOG("Generating block-level QoR summary...\n");
  std::string syn_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + synth_bin_path);
  syn_cmd += std::string(" --config ") + synth_compile_config_file();
  syn_cmd += std::string(" --dc_rundir ") + synth_runtime_dir();
  syn_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;
  syn_cmd += std::string(" --jobs ") + std::to_string(max_jobs);
  syn_cmd += std::string(" --parse_report_only ");
  syn_cmd += std::string(" --report_summary ") +
             file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_BLOCK_QOR_FILE, verbose);

  VTR_LOGV(verbose, "Running command:\n\t%s\n", syn_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = synth_dir();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run block-level QoR summary generation!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(syn_cmd.c_str())) {
    VTR_LOG_ERROR("Block-level QoR summary generation failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::gen_gl(const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  status = gen_common_gl(e_fabric_type::DP, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* For ultimate fabric, link the subblock files */
  std::filesystem::path src_blk_dirpath = netlist_block_dir(e_fabric_type::DP, e_netlist_type::GL);
  std::filesystem::path des_blk_dirpath =
    netlist_block_dir(e_fabric_type::ULTIMATE, e_netlist_type::GL);
  openfpga::create_directory(des_blk_dirpath.parent_path().string(), true, verbose);
  if (std::filesystem::exists(des_blk_dirpath)) {
    std::filesystem::remove_all(des_blk_dirpath);
  }
  bool link_status = universal_create_symlink(src_blk_dirpath, des_blk_dirpath);
  if (!link_status) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  status = gen_common_gl(e_fabric_type::ULTIMATE, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return status;
}

int NetlistDevManager::gen_common_gl(const e_fabric_type& fb_type, const bool& verbose) const {
  int num_err = 0;
  VTR_LOGV(verbose, "Copy reusable netlists for fabric '%s'\n", fabric_type2str(fb_type).c_str());
  std::filesystem::path rtl_dir = netlist_dir(fb_type, e_netlist_type::GL);
  std::filesystem::path gl_dir = netlist_dir(fb_type, e_netlist_type::GL);
  openfpga::create_directory(gl_dir.string(), true, verbose);
  /* Copy tile netlists */
  std::filesystem::path src_tile_dirpath = netlist_tile_dir(fb_type, e_netlist_type::RTL);
  std::filesystem::path des_tile_dirpath = netlist_tile_dir(fb_type, e_netlist_type::GL);
  std::filesystem::copy(src_tile_dirpath, des_tile_dirpath,
                        std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::copy_symlinks);
  /* Copy top-level netlists */
  for (std::string fname : {NETLIST_FPGA_TOP_FILENAME_, NETLIST_FPGA_CORE_FILENAME_}) {
    std::filesystem::path src_fpath =
      std::filesystem::path(netlist_dir(fb_type, e_netlist_type::RTL)) /
      std::filesystem::path(fname);
    std::filesystem::path des_fpath =
      std::filesystem::path(netlist_dir(fb_type, e_netlist_type::GL)) /
      std::filesystem::path(fname);
    num_err += copy_file_with_mkdir(src_fpath.string(), des_fpath.string(), false, verbose);
  }
  /* Copy external gate-level netlists */
  for (std::string ext_gl_nlist : ip_gl_files_) {
    std::filesystem::path src_fpath = std::filesystem::path(ext_gl_nlist);
    std::string fname = src_fpath.filename().string();
    std::filesystem::path des_fpath =
      std::filesystem::path(netlist_dir(fb_type, e_netlist_type::GL)) /
      std::filesystem::path(fname);
    num_err += copy_file_with_mkdir(src_fpath.string(), des_fpath.string(), false, verbose);
  }

  /* Link PDK files */
  VTR_LOG("Importing external PDK netlists...\n");
  for (std::string src_fpath : pdk_verilog_files_) {
    std::string src_fname = std::filesystem::path(src_fpath).filename().string();
    std::string des_fpath =
      std::filesystem::path(gl_dir / NETLIST_TECHSRC_DIR_NAME_ / src_fname).string();
    num_err += copy_file_with_mkdir(src_fpath, des_fpath, true, verbose);
  }
  VTR_LOG("Done\n");

  /* Generate fabric netlists */
  std::filesystem::path nlist_dir = netlist_dir(fb_type, e_netlist_type::GL);
  std::string incl_fname = std::filesystem::path(netlist_dir(fb_type, e_netlist_type::GL) /
                                                 std::filesystem::path(FABRIC_NETLIST_FILENAME_))
                             .string();
  int status = generate_fabric_netlist(nlist_dir.string(), incl_fname, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::generate_fabric_netlist(const std::string& src_dir,
                                               const std::string& o_fname,
                                               const bool& verbose) const {
  VTR_LOGV(verbose, "Creating gate-level include netlists '%s'\n", o_fname.c_str());
  openfpga::mmostream o_incl_f(o_fname, false); // TODO: Enable compression through a command-line option;
  if (!valid_file_mmostream(o_incl_f)) {
    VTR_LOG_ERROR("Fail to open the gate-level include file '%s'!\n", o_fname.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(
         std::filesystem::path(src_dir),
         std::filesystem::directory_options::follow_directory_symlink)) {
    /* Bypass directory */
    if (!std::filesystem::is_regular_file(dir_entry)) {
      continue;
    }
    /* Bypass the file itself */
    if (dir_entry.path().filename().string() ==
        std::filesystem::path(o_fname).filename().string()) {
      continue;
    }
    /* Follow the convention of most Verilog files. Assume the next one follow the default net type
     */
    openfpga::print_verilog_default_net_type_declaration(o_incl_f,
                                                         openfpga::VERILOG_DEFAULT_NET_TYPE_WIRE);
    openfpga::print_verilog_include_netlist(o_incl_f, dir_entry.path().string());
    openfpga::print_verilog_default_net_type_declaration(o_incl_f,
                                                         openfpga::VERILOG_DEFAULT_NET_TYPE_WIRE);
  }
  o_incl_f.close();
  VTR_LOGV(verbose, "Done\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::analyze_tile_ppa(const std::string& synth_bin_path, const bool& dryrun,
                                        const size_t& max_jobs, const bool& verbose) const {
  int num_err = 0;
  std::string pdk_config_fpath =
    file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_PDK_CONFIG, verbose);

  std::string strtg_fpath = src_synth_strategy_file_;
  std::string des_config_fpath = synth_analyze_config_file();
  VTR_LOG("Generating analyze task configuration to '%s'\n", des_config_fpath.c_str());
  FabricSynthStrategy syn_strtg(booting_root_, pdk_mgr_->name(), default_corner_);
  /* Generate the task file */
  e_fabric_type fb_type = e_fabric_type::DP;
  syn_strtg.set_netlist_input(netlist_dir(fb_type, e_netlist_type::GL));
  syn_strtg.set_netlist_output(netlist_block_dir(fb_type, e_netlist_type::GL));
  /* Exclude files include PDK files */
  syn_strtg.add_netlist_exclude(std::string(FABRIC_NETLIST_FILENAME_));
  syn_strtg.add_netlist_exclude(std::string(NETLIST_INCLUDE_FILENAME_));
  for (std::string pdk_vfile : pdk_verilog_files_) {
    std::filesystem::path pdk_vfpath = pdk_vfile;
    syn_strtg.add_netlist_exclude(pdk_vfpath.filename().string());
  }

  /* Get all the tiles */
  TileRef tile_ref;
  fb_type = e_fabric_type::ULTIMATE;
  std::string tref_file = file(fb_type, e_file_type::TILE_REF, verbose);
  int status = read_yaml_tile_ref(tref_file.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string fb_hie_file = file(fb_type, e_file_type::FABRIC_HIERARCHY_FILE, verbose);
  status = read_yaml_tile_types(fb_hie_file.c_str(), tile_ref, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (std::string tile : tile_ref.tiles()) {
    syn_strtg.add_design_for_analyze(tile);
  }

  if (!syn_strtg.validate_general_settings()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  status = syn_strtg.write_yaml_analyze_task(des_config_fpath, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  VTR_LOG("Running tile-level PPA analyze...\n");
  std::string syn_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + synth_bin_path);
  syn_cmd += std::string(" --config ") + synth_analyze_config_file();
  syn_cmd += std::string(" --dc_rundir ") + synth_runtime_dir();
  syn_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;
  syn_cmd += std::string(" --jobs ") + std::to_string(max_jobs);
  if (dryrun) {
    syn_cmd += std::string(" --file_generation_only ");
  }

  VTR_LOGV(verbose, "Running command:\n\t%s\n", syn_cmd.c_str());
  if (dryrun) {
    VTR_LOG_WARN("User choose to dryrun netlist synthesis.\n");
  }

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = synth_dir();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run tile-level PPA analyze!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(syn_cmd.c_str())) {
    VTR_LOG_ERROR("Tile-level PPA analyze failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::parse_post_synth_tile_report(const std::string& synth_bin_path,
                                                    const size_t& max_jobs,
                                                    const bool& verbose) const {
  int num_err = 0;
  std::string pdk_config_fpath =
    file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_PDK_CONFIG, verbose);

  VTR_LOG("Generating tile-level QoR summary...\n");
  std::string syn_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + synth_bin_path);
  syn_cmd += std::string(" --config ") + synth_analyze_config_file();
  syn_cmd += std::string(" --dc_rundir ") + synth_runtime_dir();
  syn_cmd += std::string(" --custom_pdk ") + pdk_config_fpath;
  syn_cmd += std::string(" --jobs ") + std::to_string(max_jobs);
  syn_cmd += std::string(" --parse_report_only ");
  syn_cmd += std::string(" --report_summary ") +
             file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_TILE_QOR_FILE, verbose);

  VTR_LOGV(verbose, "Running command:\n\t%s\n", syn_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = synth_dir();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run tile-level QoR summary generation!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(syn_cmd.c_str())) {
    VTR_LOG_ERROR("Tile-level QoR summary generation failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::optimize_netlist(const std::string& ccff_makeup_bin_path,
                                        const std::string& synth_bin_path,
                                        const std::string& stage_sel, const bool& dryrun,
                                        const size_t& max_jobs, const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  if (!valid_opt_stage(stage_sel, true)) {
    return status;
  }
  if (stage_sel == OPT_STAGE_SETUP_ || stage_sel == OPT_STAGE_ALL_) {
    /* TODO: Create gate-level netlist directory for future usage */
    status = setup_gl_placeholder_dir(verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Setup synthesis tasks */
    status = setup_rtl_synth(verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* CCFF makeup */
    status = ccff_makeup(ccff_makeup_bin_path, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    if (stage_sel != OPT_STAGE_ALL_) {
      return status;
    }
  }
  if (stage_sel == OPT_STAGE_SYNTH_ || stage_sel == OPT_STAGE_ALL_) {
    status = synth_rtl(synth_bin_path, dryrun, max_jobs, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    if (dryrun) {
      return status;
    }
    if (stage_sel != OPT_STAGE_ALL_) {
      return status;
    }
  }
  /* TODO: Organize gate-level netlists */
  if (stage_sel == OPT_STAGE_GEN_GL_ || stage_sel == OPT_STAGE_ALL_) {
    status = gen_gl(verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::coarse_analyze_netlist(const std::string& synth_bin_path, const bool& dryrun,
                                              const size_t& max_jobs, const bool& verbose) const {
  int status = parse_post_synth_block_report(synth_bin_path, max_jobs, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* TODO: Sanity checks and run area analysis */
  status = analyze_tile_ppa(synth_bin_path, dryrun, max_jobs, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (dryrun) {
    return status;
  }
  status = parse_post_synth_tile_report(synth_bin_path, max_jobs, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Run a coarse floorplanning to estimate area (Height & width) for the whole efpga fabric */
int NetlistDevManager::coarse_floorplan(const std::string& rpt_fpath,
                                        const std::string& shaping_rpt, const size_t& precision,
                                        const bool& verbose, const e_fabric_type& fb_type,
                                        const std::string& strategy_file) {
  std::string final_floorplan_strategy_file = src_floorplan_strategy_file_;
  if (!strategy_file.empty()) {
    final_floorplan_strategy_file = strategy_file;
    VTR_LOGV("Loading custom floorplan strategy from: %s\n", strategy_file.c_str());
  } else {
    VTR_LOGV("Loading default floorplan strategy from: %s\n", src_floorplan_strategy_file_.c_str());
  }
  std::string strtg_fpath = src_synth_strategy_file_;
  FabricSynthStrategy syn_strtg(booting_root_, pdk_mgr_->name(), default_corner_);
  /* Deposit info required to be a valid general setting */

  syn_strtg.set_netlist_input(netlist_dir(fb_type, e_netlist_type::RTL));
  syn_strtg.set_netlist_output(netlist_block_dir(fb_type, e_netlist_type::GL));
  int status = read_xml_synth_strategy(strtg_fpath.c_str(), syn_strtg, verbose);
  /* Get all the cb/sb/grid blocks */

  std::string fb_hie_file = file(fb_type, e_file_type::FABRIC_HIERARCHY_FILE, verbose);
  std::vector<std::string> syn_blk_names;
  status = read_yaml_tile_subblocks(fb_hie_file.c_str(), syn_blk_names, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (std::string syn_blk_name : syn_blk_names) {
    syn_strtg.add_design_for_synth(syn_blk_name, verbose);
  }

  if (!syn_strtg.validate_general_settings()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!syn_strtg.validate_strategies()) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  FloorplanStrategy flrpln_strtg;
  status = init_floorplan_from_synth_sclib(*pdk_mgr_, syn_strtg, flrpln_strtg, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR(
      "Cannot obtain feature sizes required by floorplanning!\n\tIt is possible that your "
      "synthesis strategy mixes the use of standard cells in different height (number of tracks), "
      "which is not supported yet!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = read_xml_floorplan_strategy(final_floorplan_strategy_file.c_str(), flrpln_strtg);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!flrpln_strtg.validate()) {
    VTR_LOG_ERROR("Floorplanning strategy is invalid!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string tref_file = file(fb_type, e_file_type::TILE_REF, verbose);
  /*dp and ultimate share the same tile_qor.csv*/
  std::string synth_tile_qor_file =
    file(e_fabric_type::ULTIMATE, e_file_type::SYNTH_TILE_QOR_FILE, verbose);
  status = read_qor_file(tref_file.c_str(), synth_tile_qor_file.c_str(), tile_qor_);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = run_floorplan_flow(flrpln_strtg, tile_qor_, "", verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string default_shaping_report_path =
    file(fb_type, e_file_type::FLOORPLAN_SHAPING_TXT, verbose);
  status = report_floorplan(default_shaping_report_path, proj_name_, family_name_, pdk_mgr_->name(),
                            run_name_, tile_qor_, pdk_mgr_->shrink_factor(), precision, verbose);
  if (!shaping_rpt.empty()) {
    int status = report_floorplan_shaping_txt(tile_qor_, shaping_rpt, precision);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Can not generate shaping file to the path %s! \n", shaping_rpt.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("\n");
  }

  status = report_floorplan(shaping_rpt, proj_name_, family_name_, pdk_mgr_->name(), run_name_,
                            tile_qor_, pdk_mgr_->shrink_factor(), precision, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Report to file if required */
  if (!rpt_fpath.empty()) {
    status =
      report_floorplan_to_csv(rpt_fpath, proj_name_, family_name_, pdk_mgr_->name(), run_name_,
                              tile_qor_, pdk_mgr_->shrink_factor(), precision, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

bool NetlistDevManager::valid_opt_stage(const std::string& stage, const bool& verbose) const {
  if (stage != OPT_STAGE_ALL_ && stage != OPT_STAGE_SETUP_ && stage != OPT_STAGE_SYNTH_ &&
      stage != OPT_STAGE_GEN_GL_) {
    VTR_LOGV_ERROR(verbose, "Invalid optimization stage '%s'! Expect [ %s | %s | %s | %s ]\n",
                   stage.c_str(), OPT_STAGE_ALL_.c_str(), OPT_STAGE_SETUP_.c_str(),
                   OPT_STAGE_SYNTH_.c_str(), OPT_STAGE_GEN_GL_.c_str());
    return false;
  }
  return true;
}

std::string NetlistDevManager::opt_stage_all_str() const { return OPT_STAGE_ALL_; }

void NetlistDevManager::set_default_corner(const std::string& val) { default_corner_ = val; }

std::vector<std::string> NetlistDevManager::result_files(
  const TapeoutManager::e_substep_name& s_step, const bool& verbose) const {
  std::vector<std::string> ret;
  switch (s_step) {
    case TapeoutManager::e_substep_name::NETLIST_GEN: {
      for (auto fb_type : fabrics()) {
        /* Netlists */
        std::filesystem::path nlist_dir = netlist_dir(fb_type, e_netlist_type::RTL);
        if (!std::filesystem::exists(nlist_dir)) {
          VTR_LOG_ERROR("Expected netlist directory '%s' does not exist!\n",
                        nlist_dir.string().c_str());
          exit(openfpga::CMD_EXEC_FATAL_ERROR);
        }
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(nlist_dir)) {
          /* Bypass directory */
          if (!std::filesystem::is_regular_file(dir_entry)) {
            continue;
          }
          ret.push_back(dir_entry.path().string());
        }
        /* Report files */
        std::vector<e_file_type> f_types_to_check = {
          e_file_type::MUX_MODULE_FILE, e_file_type::CONFIG_MEM_MODULE_FILE,
          e_file_type::FABRIC_PIN_PHY_LOC_FILE, e_file_type::FABRIC_HIERARCHY_FILE};
        std::vector<e_file_type> ultm_f_types_to_check = {e_file_type::BASE_BITSTREAM_FILE,
                                                          e_file_type::BITSTREAM_DIST_FILE};
        for (auto f_type : f_types_to_check) {
          ret.push_back(file(fb_type, f_type, verbose));
        }
        if (fb_type == e_fabric_type::ULTIMATE) {
          for (auto f_type : ultm_f_types_to_check) {
            ret.push_back(file(fb_type, f_type, verbose));
          }
        }
        // std::filesystem::path rr_gsb_dirpath = rr_gsb_dir(fb_type);
        // if (!std::filesystem::exists(rr_gsb_dirpath)) {
        //   VTR_LOG_ERROR("Expected report directory '%s' does not exist!\n",
        //                 rr_gsb_dirpath.string().c_str());
        //   exit(openfpga::CMD_EXEC_FATAL_ERROR);
        // }
        // for (auto const& dir_entry :
        //      std::filesystem::recursive_directory_iterator(rr_gsb_dirpath)) {
        //   /* Bypass directory */
        //   if (!std::filesystem::is_regular_file(dir_entry)) {
        //     continue;
        //   }
        //   ret.push_back(dir_entry.path().string());
        // }
      }
      break;
    }
    case TapeoutManager::e_substep_name::NETLIST_OPT: {
      for (auto fb_type : fabrics()) {
        /* Netlists */
        std::filesystem::path nlist_dir = netlist_dir(fb_type, e_netlist_type::GL);
        if (!std::filesystem::exists(nlist_dir)) {
          VTR_LOG_ERROR("Expected netlist directory '%s' does not exist!\n",
                        nlist_dir.string().c_str());
          exit(openfpga::CMD_EXEC_FATAL_ERROR);
        }
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(nlist_dir)) {
          /* Bypass directory */
          if (!std::filesystem::is_regular_file(dir_entry)) {
            continue;
          }
          ret.push_back(dir_entry.path().string());
        }
        /* TODO: Report files */
      }
      break;
    }
    case TapeoutManager::e_substep_name::NETLIST_COARSE_ANALYZE: {
      /* TODO: Report files ONLY for ultimate fabric */
      break;
    }
    case TapeoutManager::e_substep_name::NETLIST_COARSE_FLOORPLAN: {
      /* TODO: Report files ONLY for ultimate fabric */
      break;
    }
    case TapeoutManager::e_substep_name::NETLIST_QOR: {
      /* TODO: Report files ONLY for ultimate fabric */
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid substep name for netlist development!\n");
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }

  return ret;
}

std::string NetlistDevManager::md5_yaml_file() const {
  std::filesystem::path ret = project_home_dir() / std::filesystem::path(MD5_FILENAME_);
  return ret.string();
}

int NetlistDevManager::save_results(const std::vector<TapeoutManager::e_substep_name>& s_steps,
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

int NetlistDevManager::check_results(const bool& verbose) const {
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

int NetlistDevManager::export_netlist(const std::string& fname, const bool& force_overwritten,
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
  std::string nlist_dir =
    std::filesystem::path(project_home_dir() / std::filesystem::path(NETLIST_DIR_NAME_)).string();
  std::string rpt_dir = report_dir();
  /* Must follow the reference */
  std::string export_cmd = "tar -czvf";
  export_cmd += std::string(" ") + fpath.string();
  export_cmd += std::string(" ") + nlist_dir;
  export_cmd += std::string(" ") + rpt_dir;

  VTR_LOG("Exporting netlists and associated reports to \n\t%s\n", fpath.string().c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to export netlist!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(export_cmd.c_str())) {
    VTR_LOG_ERROR("Export netlist package failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  return openfpga::CMD_EXEC_SUCCESS;
}

int NetlistDevManager::show_config_stats(const float& pclk_freq,
                                         const std::string& report_file_path, const int& precision,
                                         const bool& verbose) const {
  /* Get the bitstream distribution file */
  std::string bit_dist_fpath =
    file(e_fabric_type::ULTIMATE, e_file_type::BITSTREAM_DIST_FILE, verbose);
  VTR_LOGV(verbose, "Collecting information from source: %s\n", bit_dist_fpath.c_str());
  /* TODO: Parse the bitstream distribution file */
  BitstreamDistribution bit_dist;
  int status = read_xml_bitstream_distribution(bit_dist_fpath, bit_dist);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed when parsing bitstream distribution file\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string bitstream_size = std::to_string(bit_dist.bitstream_size());
  std::string clock_frequency = to_string_with_precision(pclk_freq, precision);
  std::string region_num = std::to_string(bit_dist.num_regions());
  std::string serial_mode =
    to_string_with_precision((float)(bit_dist.bitstream_size()) * 1. / pclk_freq, precision);
  std::string burst_mode = to_string_with_precision(
    (float)(bit_dist.max_region_bitstream_size()) * 1. / pclk_freq, precision);

  TablePrinter tp({"Size of bitstream", "Considered programming clock frequency (Hz)",
                   "Number of configuration regions",
                   "Estimated configuration time in serial mode (sec.)",
                   "Estimated configuration time in burst mode (sec.)"});
  tp.add_row({bitstream_size, clock_frequency, region_num, serial_mode, burst_mode});
  tp.print(report_file_path);
  return openfpga::CMD_EXEC_SUCCESS;
}

void NetlistDevManager::set_netlist_makeup_config_file(const std::string& val) {
  netlist_makeup_config_file_ = val;
}

void NetlistDevManager::set_ccff_makeup_buf_config_file(const std::string& val) {
  ccff_makeup_buf_config_file_ = val;
}

std::vector<size_t> NetlistDevManager::gclock_sweep_ids() const {
  /* Parse clock buffer location map file to
   - find the number of clocks
   - find the pin name of clocks */
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  std::string ckbuf_loc_fpath = file(fb_type, e_file_type::OPENFPGA_CKBUF_LOC_MAP, false);
  GlobalPortLoc gport_loc;
  int status = set_up_global_ports(ckbuf_loc_fpath, gport_loc);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::vector<openfpga::BasicPort> ck_ports = gport_loc.clock_ports();
  std::vector<size_t> ret(ck_ports.size());
  std::iota(ret.begin(), ret.end(), 0);
  return ret;
}

std::vector<size_t> NetlistDevManager::greset_sweep_ids() const {
  /* Parse clock buffer location map file to
   - find the number of clocks
   - find the pin name of clocks */
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  std::string ckbuf_loc_fpath = file(fb_type, e_file_type::OPENFPGA_CKBUF_LOC_MAP, false);
  GlobalPortLoc gport_loc;
  int status = set_up_global_ports(ckbuf_loc_fpath, gport_loc);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::vector<openfpga::BasicPort> rst_ports = gport_loc.reset_ports();
  std::vector<size_t> ret(rst_ports.size());
  std::iota(ret.begin(), ret.end(), 0);
  return ret;
}

openfpga::BasicPort NetlistDevManager::gclk_port() const {
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  std::string ckbuf_loc_fpath = file(fb_type, e_file_type::OPENFPGA_CKBUF_LOC_MAP, false);
  GlobalPortLoc gport_loc;
  int status = set_up_global_ports(ckbuf_loc_fpath, gport_loc);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::vector<openfpga::BasicPort> ck_ports = gport_loc.clock_ports();
  openfpga::BasicPort ret(ck_ports[0].get_name(), ck_ports.size());
  return ret;
}
openfpga::BasicPort NetlistDevManager::grst_port() const {
  e_fabric_type fb_type = e_fabric_type::ULTIMATE;
  std::string ckbuf_loc_fpath = file(fb_type, e_file_type::OPENFPGA_CKBUF_LOC_MAP, false);
  GlobalPortLoc gport_loc;
  int status = set_up_global_ports(ckbuf_loc_fpath, gport_loc);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::vector<openfpga::BasicPort> rst_ports = gport_loc.reset_ports();
  openfpga::BasicPort ret(rst_ports[0].get_name(), rst_ports.size());
  return ret;
}

std::string NetlistDevManager::gclock_bitstream_value_file(const size_t& clk_sweep_id,
                                                           const bool& verbose) const {
  return global_port_sweep_file(e_file_type::BITSTREAM_XML_FILE,
                                e_script_type::OPENFPGA_SCRIPT_GCLKBITGEN, clk_sweep_id, verbose);
}
std::string NetlistDevManager::greset_bitstream_value_file(const size_t& rst_sweep_id,
                                                           const bool& verbose) const {
  return global_port_sweep_file(e_file_type::BITSTREAM_XML_FILE,
                                e_script_type::OPENFPGA_SCRIPT_GRSTBITGEN, rst_sweep_id, verbose);
}

}  // End namespace arcxpress
