#include "arch_eval_manager.h"

#include <filesystem>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

/* For pugiutil::ReqOpt::OPTIONAL
 * On Windows builds (especially MSVC / Windows SDK / COM headers),
 * OPTIONAL is a macro defined in system headers.
 */
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef OPTIONAL
#undef OPTIONAL
#endif
#endif

#include "arch_aggregator.h"
#include "arcxpress_file_os.h"
#include "arcxpress_report_util.h"
#include "blif_param2vpr_constraint.h"
#include "command_exit_codes.h"
#include "file_md5_manager.h"
#include "global_signal_assigner.h"
#include "json.hpp"
#include "log_error_checker.h"
#include "openfpga_digest.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "rapidcsv.h"
#include "tableprinter.h"
#include "task_scheduler.h"
#include "vpr_block_usage_parser.h"
#include "vpr_constraint_file_constants.h"
#include "vpr_log_parser.h"
#include "vtr_log.h"
#include "write_xml_tapeout_manager_custom_constraints.h"

namespace arcxpress {  // Begin namespace arcxpress

ArchEvalManager::ArchEvalManager(const TapeoutManager& to_mgr, const std::string& run_name,
                                 const BenchmarkManager& bm_mgr)
  : to_mgr_(&to_mgr) {
  init(to_mgr.path(), to_mgr.name(), to_mgr.family(), run_name, bm_mgr,
       to_mgr.openfpga_stats_conf_file(), to_mgr.yosys_error_conf_file(),
       to_mgr.openfpga_error_conf_file(), to_mgr.python_bin_path());
}

void ArchEvalManager::init(const std::string& root_path, const std::string& proj_name,
                           const std::string& family_name, const std::string& run_name,
                           const BenchmarkManager& bm_mgr, const std::string& vpr_stats_kw_file,
                           const std::string& yosys_error_conf_file,
                           const std::string& openfpga_error_conf_file,
                           const std::string& py_bin_path) {
  root_path_ = root_path;
  proj_name_ = proj_name;
  family_name_ = family_name;
  run_name_ = run_name;
  is_valid_ = false;
  bm_mgr_ = &bm_mgr;

  PYTHON_COMMAND_ = py_bin_path;

  YOSYS_VAR_DESIGN_SOURCES_VERILOG_ = "BENCHMARK_VERILOG_FILES";
  YOSYS_VAR_DESIGN_SOURCES_SYSTEMVERILOG_ = "BENCHMARK_SYSTEMVERILOG_FILES";
  YOSYS_CMD_SYSTEMVERILOG_ = "plugin -i slang";
  YOSYS_VAR_TOP_MODULE_ = "TOP_MODULE";
  YOSYS_VAR_DEVICE_FAMILY_ = "DEVICE_FAMILY";
  YOSYS_VAR_OUT_BLIF_ = "BLIF_FILE";
  YOSYS_VAR_SAVE_BLOCK_DIAGRAM_ = "SAVE_BLOCK_DIAGRAM";
  YOSYS_VAR_STAT_JSON_ = "STAT_JSON_FILE";
  YOSYS_VAR_K_ = "LUT_SIZE";

  VPR_BLIF_VAR_NAME_ = "VPR_TESTBENCH_BLIF";
  VPR_FABRIC_VAR_NAME_ = "OPENFPGA_VPR_DEVICE_LAYOUT";
  VPR_ROUTE_CHAN_W_VAR_NAME_ = "OPENFPGA_VPR_ROUTE_CHAN_WIDTH";

  SCRIPT_TYPE_STRING_ = {"sweep_k", "synth", "bitgen"};
  FILE_NAMES_ = {"post_synth.blif",
                 "post_synth_rewrite.v",
                 "post_synth.v",
                 "ckbuf_cell_map.xml",
                 "stats.json",
                 "vpr_arch.xml",
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
                 "vpr_final.sdc",
                 "boundary_timing.xml",
                 "pcf_custom_command.xml",
                 "pcf_dummy.pcf",
                 "final_gclk_pcf.xml",
                 "final_gclk_pcf4dv.xml",
                 "ckbuf_vpr_constraints.xml",
                 "fabric_key.xml",
                 "module_names.xml",
                 "rr_graph.bin",
                 "placement_delay_lookup.bin",
                 "router_lookahead.bin",
                 "io_coordinate.xml",
                 "efpga_io_pin_table.csv",
                 "vpr.net",
                 "vpr.place",
                 "vpr.route",
                 "fabric_bitstream.bit",
                 "fabric_bitstream.xml",
                 "PinMapping.xml",
                 "arch timing data file",
                 "block_usage.xml",
                 "_post_synthesis.v",
                 "post_synth.dot"};
  VAR_NAMES_ = {"BLIF_FILE",
                "REFERENCE_VERILOG_REWRITE",
                "REFERENCE_VERILOG",
                "GLOBAL_SIGNAL_BUF_MAP",
                "STAT_JSON_FILE",
                "VPR_ARCH_FILE",
                "OPENFPGA_ARCH_FILE",
                "OPENFPGA_CLOCK_ARCH_FILE",
                "OPENFPGA_CKBUF_LOC_LOC_FILE",
                "DESIGN_VAR_YAML", /* Placeholder only */
                "OPENFPGA_SIM_SETTING_FILE",
                "OPENFPGA_CKBUF_CELL_MAP_FILE",
                "OPENFPGA_BITSTREAM_SETTING_FILE",
                "OPENFPGA_TILE_CONFIG_FILE",
                "OPENFPGA_USER_PIN_CONSTRAINTS_FILE",
                "OPENFPGA_VPR_SDC",
                "OPENFPGA_VPR_FINAL_SDC",
                "OPENFPGA_BOUNDARY_TIMING_FILE",
                "OPENFPGA_PCF_CONFIG_FILE",
                "OPENFPGA_PCF",
                "OPENFPGA_PIN_CONSTRAINTS_FILE",
                "OPENFPGA_PIN_CONSTRAINTS_FILE4DV",
                "OPENFPGA_VPR_CONSTRAINT_FILE",
                "EXTERNAL_FABRIC_KEY_FILE",
                "OPENFPGA_RENAME_MODULE_FILE",
                "OPENFPGA_VPR_RR_GRAPH_FILE",
                "OPENFPGA_VPR_PLACEMENT_DELAY_LOOKUP_FILE",
                "OPENFPGA_VPR_ROUTER_LOOKAHEAD_FILE",
                "OPENFPGA_IO_COORDINATE_FILE",
                "OPENFPGA_PIN_TABLE",
                "VPR_NET_FILE",
                "VPR_PLACE_FILE",
                "VPR_ROUTE_FILE",
                "OPENFPGA_BITSTREAM_BIT_FILE",
                "OPENFPGA_BITSTREAM_XML_FILE",
                "OPENFPGA_BITSTREAM_PIN_MAP_FILE",
                "arch timing data file",
                "OPENFPGA_VPR_BLOCK_USAGE_FILE",
                "SAVE_BLOCK_DIAGRAM"};

  PRE_ARCH_SUBDIR_NAME_ = "pre_arch_analysis";
  BITGEN_SUBDIR_NAME_ = "bitstream";
  ARCH_DIR_NAME_ = "arch";
  PINTABLE_DIR_NAME_ = "arch";
  BITSTREAM_DIR_NAME_ = "bitstream";
  APP_SIM_DIR_NAME_ = "app_sim";
  VPR_REPORT_DIR_NAME_ = "vpr_results";
  STATS_JSON_DESIGN_TAG_ = "design";
  STATS_JSON_CELL_DETAILS_TAG_ = "num_cells_by_type";

  OPENFPGA_RUNDIR_NAME_ = "openfpga_tasks";

  YOSYS_SCRIPT_FILENAME_ = "run.ys";
  YOSYS_REWRITE_SCRIPT_FILENAME_ = "rewrite.ys";
  YOSYS_LOG_FILENAME_ = "yosys.log";
  OPENFPGA_LOG_FILENAME_ = "openfpga.log";
  OPENFPGA_SCRIPT_FILENAME_ = "run.openfpga";
  ARCH_TIMING_FILE_PREFIX_ = "arch_timing_";
  ARCH_TIMING_FILE_POSTFIX_ = ".yaml";

  MD5_FILENAME_ = "results_md5.yaml";
  BITGEN_QOR_REPORT_FILENAME_ = "qor.csv";

  VPR_STATS_NUM_INPUTS_TAG_ = "num_inputs";
  VPR_STATS_NUM_OUTPUTS_TAG_ = "num_outputs";
  VPR_STATS_NUM_NETS_TAG_ = "num_nets";

  min_k_ = 4;
  max_k_ = 6;

  /* Initialize the log parser */
  if (openfpga::CMD_EXEC_SUCCESS !=
      vpr_log_parser_.load_keywords_from_xml(vpr_stats_kw_file, false)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (openfpga::CMD_EXEC_SUCCESS !=
      yosys_log_checker_.load_keywords_from_xml(yosys_error_conf_file, false)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (openfpga::CMD_EXEC_SUCCESS !=
      openfpga_log_checker_.load_keywords_from_xml(openfpga_error_conf_file, false)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void ArchEvalManager::set_source_arch_dir(const std::string& arch_dir) { src_arch_dir_ = arch_dir; }

void ArchEvalManager::set_source_pin_table_dir(const std::string& pin_table_dir) {
  src_pin_table_dir_ = pin_table_dir;
}

std::filesystem::path ArchEvalManager::pre_arch_project_home_dir() const {
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(PRE_ARCH_SUBDIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_);
  return proj_home;
}

std::filesystem::path ArchEvalManager::bitgen_project_home_dir() const {
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(BITGEN_SUBDIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_);
  return proj_home;
}

std::string ArchEvalManager::pre_arch_sweepk_subdir_name(const BenchmarkId& bm_id,
                                                         const size_t& k) const {
  std::filesystem::path ret = std::filesystem::path(bm_mgr_->benchmark_name(bm_id));
  ret =
    ret / std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(e_script_type::YOSYS_SCRIPT_SWEEP_K)] +
                                std::to_string(k));
  return ret.string();
}

std::string ArchEvalManager::bitgen_subdir_name(const BenchmarkId& bm_id,
                                                const std::string& corner) const {
  std::filesystem::path ret = std::filesystem::path(OPENFPGA_RUNDIR_NAME_);
  ret = ret / std::filesystem::path(bm_mgr_->benchmark_name(bm_id));
  ret = ret / std::filesystem::path(corner);
  return ret.string();
}

std::string ArchEvalManager::pre_arch_file(const std::string& subdir_name,
                                           const e_file_type& f_type, const bool& verbose) const {
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = pre_arch_project_home_dir();
  std::filesystem::path ret;
  switch (f_type) {
    case e_file_type::OUT_BLIF:
    case e_file_type::STAT_JSON:
      ret = proj_home / std::filesystem::path(subdir_name) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;

    default:
      VTR_LOG_ERROR("Invalid file type!\n");
  }
  return ret.string();
}

std::string ArchEvalManager::bitgen_file(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                         const e_file_type& f_type, const std::string& corner,
                                         const bool& verbose) const {
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret;
  switch (f_type) {
    case e_file_type::STAT_JSON:
    case e_file_type::VPR_BLOCK_USAGE_FILE:
      ret = proj_home / std::filesystem::path(bitgen_subdir_name(bm_id, corner)) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::VPR_POST_IMPLEMENT_VERILOG_FILE:
      ret = proj_home / std::filesystem::path(bitgen_subdir_name(bm_id, corner)) /
            std::filesystem::path(bm_mgr_->top_module(bm_id) + FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::VPR_ARCH:
    case e_file_type::OPENFPGA_ARCH:
    case e_file_type::OPENFPGA_GNTWK_ARCH:
    case e_file_type::OPENFPGA_CKBUF_LOC_MAP:
    case e_file_type::DESIGN_VAR:
    case e_file_type::FINAL_FABRIC_KEY:
    case e_file_type::CRAFTED_MODULE_NAME:
    case e_file_type::OPENFPGA_BITSTREAM_SETTING:
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::RRG:
    case e_file_type::PDL:
    case e_file_type::RLA:
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(corner + "_" + FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::IO_COORD:
    case e_file_type::PIN_TABLE:
      ret = proj_home / std::filesystem::path(PINTABLE_DIR_NAME_) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::OPENFPGA_SIM_SETTING:
    case e_file_type::OPENFPGA_CKBUF_MAP:
    case e_file_type::OPENFPGA_TILE_CONFIG:
    case e_file_type::OPENFPGA_PCF_XML:
    case e_file_type::VPR_SDC:
    case e_file_type::OPENFPGA_BOUNDARY_TIMING:
    case e_file_type::OPENFPGA_PCF_CONFIG:
    case e_file_type::OPENFPGA_PCF:
      ret = std::filesystem::path(src_openfpga_setting_files_.at(f_type));
      break;
    case e_file_type::OUT_BLIF:
    case e_file_type::OUT_CKBUF_MAP:
    case e_file_type::OUT_VERILOG:
    case e_file_type::OUT_VERILOG_REWRITE:
    case e_file_type::VPR_NET_FILE:
    case e_file_type::VPR_PLACE_FILE:
    case e_file_type::VPR_ROUTE_FILE:
    case e_file_type::OPENFPGA_VPR_CONSTRAINT_FILE:
    case e_file_type::OPENFPGA_FINAL_PCF_XML:
    case e_file_type::OPENFPGA_FINAL_PCF_XML4DV:
    case e_file_type::OPENFPGA_FINAL_SDC:
      ret = proj_home / std::filesystem::path(vpr_report_output_dir(st_id, bm_id, corner)) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::BITSTREAM_TEXT_FILE:
    case e_file_type::BITSTREAM_XML_FILE:
    case e_file_type::BITSTREAM_PIN_MAP_FILE:
      ret = proj_home / std::filesystem::path(bitstream_output_dir(st_id, bm_id, corner)) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::ARCH_TIMING: {
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_);
      std::string l_fname =
        std::string(ARCH_TIMING_FILE_PREFIX_) + corner + std::string(ARCH_TIMING_FILE_POSTFIX_);
      ret = ret / std::filesystem::path(l_fname);
      break;
    }
    case e_file_type::OUT_BLOCK_DIAGRAM:
      ret = proj_home / std::filesystem::path(vpr_report_output_dir(st_id, bm_id, corner)) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    default:
      VTR_LOG_ERROR("Invalid file type!\n");
  }
  return ret.string();
}

std::string ArchEvalManager::pre_arch_yosys_runtime_file(const std::string& subdir_name) const {
  std::filesystem::path proj_home = pre_arch_project_home_dir();
  std::filesystem::path ret;
  ret =
    proj_home / std::filesystem::path(subdir_name) / std::filesystem::path(YOSYS_SCRIPT_FILENAME_);
  return ret.string();
}

std::string ArchEvalManager::bitgen_yosys_runtime_file(const std::string& subdir_name) const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret;
  ret =
    proj_home / std::filesystem::path(subdir_name) / std::filesystem::path(YOSYS_SCRIPT_FILENAME_);
  return ret.string();
}

std::string ArchEvalManager::rewrite_yosys_runtime_file(const std::string& subdir_name) const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret;
  ret = proj_home / std::filesystem::path(subdir_name) /
        std::filesystem::path(YOSYS_REWRITE_SCRIPT_FILENAME_);
  return ret.string();
}

int ArchEvalManager::prepare_synth_sweep_k(const BenchmarkId& bm_id, const size_t& k,
                                           std::string& runtime_ys_script,
                                           const bool& verbose) const {
  int num_err = 0;
  /* Prepare yosys scripts for selected benchmarks */
  std::string pre_arch_subdir_name = pre_arch_sweepk_subdir_name(bm_id, k);
  ArchAggregator ys_agg;
  /* Add variables: design sources, top module, lut size, device family, output blif file */
  size_t cnt_v_files = 0;
  size_t cnt_sv_files = 0;
  for (BenchmarkDesignSourceId src_id : bm_mgr_->design_sources(bm_id)) {
    if (bm_mgr_->design_source_type(bm_id, src_id) == BenchmarkManager::e_file_type::VERILOG) {
      ys_agg.add_1toN_variable(YOSYS_VAR_DESIGN_SOURCES_VERILOG_,
                               bm_mgr_->design_source_file(bm_id, src_id));
      cnt_v_files++;
    }
    if (bm_mgr_->design_source_type(bm_id, src_id) ==
        BenchmarkManager::e_file_type::SYSTEM_VERILOG) {
      ys_agg.add_1toN_variable(YOSYS_VAR_DESIGN_SOURCES_SYSTEMVERILOG_,
                               bm_mgr_->design_source_file(bm_id, src_id));
      cnt_sv_files++;
    }
  }
  /* If the frontend does not support system verilog, but system verilog netlists are found. Error
   * out */
#ifdef FRONTEND_NO_SLANG
  if (cnt_sv_files > 0) {
    VTR_LOG_ERROR(
      "Benchmark '%s' includes system Verilog HDL but the frontend does not support. Please "
      "contact us if the license enables the feature\n",
      bm_mgr_ > benchmark_name(bm_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  if (cnt_v_files == 0) {
    ys_agg.add_remove_variable(YOSYS_VAR_DESIGN_SOURCES_VERILOG_);
  }
  if (cnt_sv_files == 0) {
    ys_agg.add_remove_variable(YOSYS_VAR_DESIGN_SOURCES_SYSTEMVERILOG_);
  }
  if (cnt_v_files == 0 && cnt_sv_files == 0) {
    VTR_LOG_ERROR("No design source files are found!\n");
    num_err++;
  }
  /* Remove the plugin required for system verilog support */
#ifdef FRONTEND_NO_SLANG
  ys_agg.add_remove_line(YOSYS_CMD_SYSTEMVERILOG_);
#endif
  ys_agg.add_variable(YOSYS_VAR_TOP_MODULE_, bm_mgr_->top_module(bm_id));
  ys_agg.add_variable(YOSYS_VAR_DEVICE_FAMILY_, family_name_);
  ys_agg.add_variable(YOSYS_VAR_K_, k);
  ys_agg.add_variable(YOSYS_VAR_OUT_BLIF_,
                      pre_arch_file(pre_arch_subdir_name, e_file_type::OUT_BLIF, verbose));
  ys_agg.add_variable(YOSYS_VAR_STAT_JSON_,
                      pre_arch_file(pre_arch_subdir_name, e_file_type::STAT_JSON, verbose));
  /* Create directory for destination file */
  runtime_ys_script = pre_arch_yosys_runtime_file(pre_arch_subdir_name);
  std::filesystem::path runtime_ys_dirpath = runtime_ys_script;
  openfpga::create_directory(runtime_ys_dirpath.parent_path().string(), true, verbose);
  num_err += ys_agg.aggregate(src_script_files_[size_t(e_script_type::YOSYS_SCRIPT_SWEEP_K)],
                              runtime_ys_script, verbose);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::prepare_synth_rewrite(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                           const std::string& corner,
                                           std::string& runtime_ys_script,
                                           const bool& verbose) const {
  int num_err = 0;
  /* Prepare yosys scripts for selected benchmarks */
  std::string subdir_name = bitgen_subdir_name(bm_id, corner);
  ArchAggregator ys_agg;
  /* Add variables: design sources, top module, lut size, device family, output blif file */
  size_t cnt_v_files = 0;
  size_t cnt_sv_files = 0;
  for (BenchmarkDesignSourceId src_id : bm_mgr_->design_sources(bm_id)) {
    if (bm_mgr_->design_source_type(bm_id, src_id) == BenchmarkManager::e_file_type::VERILOG) {
      ys_agg.add_1toN_variable(YOSYS_VAR_DESIGN_SOURCES_VERILOG_,
                               bm_mgr_->design_source_file(bm_id, src_id));
      cnt_v_files++;
    }
    if (bm_mgr_->design_source_type(bm_id, src_id) ==
        BenchmarkManager::e_file_type::SYSTEM_VERILOG) {
      ys_agg.add_1toN_variable(YOSYS_VAR_DESIGN_SOURCES_SYSTEMVERILOG_,
                               bm_mgr_->design_source_file(bm_id, src_id));
      cnt_sv_files++;
    }
  }
  /* If the frontend does not support system verilog, but system verilog netlists are found. Error
   * out */
#ifdef FRONTEND_NO_SLANG
  if (cnt_sv_files > 0) {
    VTR_LOG_ERROR(
      "Benchmark '%s' includes system Verilog HDL but the frontend does not support. Please "
      "contact us if the license enables the feature\n",
      bm_mgr_ > benchmark_name(bm_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  if (cnt_v_files == 0) {
    ys_agg.add_remove_variable(YOSYS_VAR_DESIGN_SOURCES_VERILOG_);
  }
  if (cnt_sv_files == 0) {
    ys_agg.add_remove_variable(YOSYS_VAR_DESIGN_SOURCES_SYSTEMVERILOG_);
  }
  if (cnt_v_files == 0 && cnt_sv_files == 0) {
    VTR_LOG_ERROR("No design source files are found!\n");
    num_err++;
  }
  /* Remove the plugin required for system verilog support */
#ifdef FRONTEND_NO_SLANG
  ys_agg.add_remove_line(YOSYS_CMD_SYSTEMVERILOG_);
#endif
  ys_agg.add_variable(YOSYS_VAR_TOP_MODULE_, bm_mgr_->top_module(bm_id));
  ys_agg.add_variable(YOSYS_VAR_DEVICE_FAMILY_, family_name_);
  ys_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OUT_VERILOG_REWRITE)],
                      bitgen_file(st_id, bm_id, e_file_type::OUT_VERILOG_REWRITE, corner, verbose));
  /* Create runtime directories, which yosys cannot be done automatically */
  std::filesystem::path o_blif_dirpath =
    bitgen_file(st_id, bm_id, e_file_type::OUT_BLIF, corner, verbose);
  openfpga::create_directory(o_blif_dirpath.parent_path().string(), true, verbose);

  /* Create directory for destination file */
  runtime_ys_script = rewrite_yosys_runtime_file(subdir_name);

  std::filesystem::path runtime_ys_dirpath = runtime_ys_script;
  openfpga::create_directory(runtime_ys_dirpath.parent_path().string(), true, verbose);
  num_err += ys_agg.aggregate(src_script_files_[size_t(e_script_type::YOSYS_SCRIPT_REWRITE)],
                              runtime_ys_script, verbose);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::prepare_synth_bitgen(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                          const std::string& corner, std::string& runtime_ys_script,
                                          const bool& verbose) const {
  int num_err = 0;
  /* Prepare yosys scripts for selected benchmarks */
  std::string subdir_name = bitgen_subdir_name(bm_id, corner);
  ArchAggregator ys_agg;
  /* Add variables: design sources, top module, lut size, device family, output blif file */
  size_t cnt_v_files = 0;
  size_t cnt_sv_files = 0;
  for (BenchmarkDesignSourceId src_id : bm_mgr_->design_sources(bm_id)) {
    if (bm_mgr_->design_source_type(bm_id, src_id) == BenchmarkManager::e_file_type::VERILOG) {
      ys_agg.add_1toN_variable(YOSYS_VAR_DESIGN_SOURCES_VERILOG_,
                               bm_mgr_->design_source_file(bm_id, src_id));
      cnt_v_files++;
    }
    if (bm_mgr_->design_source_type(bm_id, src_id) ==
        BenchmarkManager::e_file_type::SYSTEM_VERILOG) {
      ys_agg.add_1toN_variable(YOSYS_VAR_DESIGN_SOURCES_SYSTEMVERILOG_,
                               bm_mgr_->design_source_file(bm_id, src_id));
      cnt_sv_files++;
    }
  }
  /* If the frontend does not support system verilog, but system verilog netlists are found. Error
   * out */
#ifdef FRONTEND_NO_SLANG
  if (cnt_sv_files > 0) {
    VTR_LOG_ERROR(
      "Benchmark '%s' includes system Verilog HDL but the frontend does not support. Please "
      "contact us if the license enables the feature\n",
      bm_mgr_ > benchmark_name(bm_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  if (cnt_v_files == 0) {
    ys_agg.add_remove_variable(YOSYS_VAR_DESIGN_SOURCES_VERILOG_);
  }
  if (cnt_sv_files == 0) {
    ys_agg.add_remove_variable(YOSYS_VAR_DESIGN_SOURCES_SYSTEMVERILOG_);
  }
  if (cnt_v_files == 0 && cnt_sv_files == 0) {
    VTR_LOG_ERROR("No design source files are found!\n");
    num_err++;
  }
  /* Remove the plugin required for system verilog support */
#ifdef FRONTEND_NO_SLANG
  ys_agg.add_remove_line(YOSYS_CMD_SYSTEMVERILOG_);
#endif
  ys_agg.add_variable(YOSYS_VAR_TOP_MODULE_, bm_mgr_->top_module(bm_id));
  ys_agg.add_variable(YOSYS_VAR_DEVICE_FAMILY_, family_name_);
  ys_agg.add_variable(YOSYS_VAR_OUT_BLIF_,
                      bitgen_file(st_id, bm_id, e_file_type::OUT_BLIF, corner, verbose));
  ys_agg.add_variable(YOSYS_VAR_STAT_JSON_,
                      bitgen_file(st_id, bm_id, e_file_type::STAT_JSON, corner, verbose));
  ys_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OUT_VERILOG)],
                      bitgen_file(st_id, bm_id, e_file_type::OUT_VERILOG, corner, verbose));
  /* clock buffer cell map is only required for full bitgen flow */
  ys_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OUT_CKBUF_MAP)],
                      bitgen_file(st_id, bm_id, e_file_type::OUT_CKBUF_MAP, corner, verbose));
  ys_agg.add_variable(YOSYS_VAR_SAVE_BLOCK_DIAGRAM_,
                      bitgen_file(st_id, bm_id, e_file_type::OUT_BLOCK_DIAGRAM, corner, verbose));
  /* Create runtime directories, which yosys cannot be done automatically */
  std::filesystem::path o_blif_dirpath =
    bitgen_file(st_id, bm_id, e_file_type::OUT_BLIF, corner, verbose);
  openfpga::create_directory(o_blif_dirpath.parent_path().string(), true, verbose);

  /* Create directory for destination file */
  runtime_ys_script = bitgen_yosys_runtime_file(subdir_name);
  std::filesystem::path runtime_ys_dirpath = runtime_ys_script;
  openfpga::create_directory(runtime_ys_dirpath.parent_path().string(), true, verbose);
  num_err += ys_agg.aggregate(src_script_files_[size_t(e_script_type::YOSYS_SCRIPT_SYNTH)],
                              runtime_ys_script, verbose);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::run_synth(const std::string& fe_bin_path, const std::string& runtime_ys,
                               const bool& verbose) {
  /* Run yosys */
  std::string ys_cmd(fe_bin_path + std::string(" ") + runtime_ys + std::string(" > ") +
                     YOSYS_LOG_FILENAME_);
  VTR_LOGV(verbose, "Running Yosys command:\n\t%s\n", ys_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = std::filesystem::path(runtime_ys).parent_path();
  std::filesystem::path log_fpath = runtime_dir / std::filesystem::path(YOSYS_LOG_FILENAME_);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run yosys!\n");
    std::filesystem::current_path(curr_dir);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(ys_cmd.c_str())) {
    VTR_LOG_ERROR("Yosys run failed with errors! Log: %s\n", log_fpath.string().c_str());
    std::filesystem::current_path(curr_dir);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOG("Done\n");

  /* Parse yosys log */
  VTR_LOGV(verbose, "Parsing Yosys log\n");
  int status = yosys_log_checker_.parse(log_fpath.string(), verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to run Yosys log checker!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (yosys_log_checker_.num_errors()) {
    VTR_LOG_ERROR("Log: %s\n", log_fpath.string().c_str());
    for (std::string err_msg : yosys_log_checker_.error_messages()) {
      VTR_LOG_ERROR("%s\n", err_msg.c_str());
    }

    VTR_LOG_ERROR("Yosys failed with %lu errors!\n", yosys_log_checker_.num_errors());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::parse_synth_stats_sweepk(const BenchmarkId& bm_id, const size_t& k,
                                              const std::string& runtime_ys,
                                              const std::string& stats_fpath, const bool& verbose) {
  int num_err = 0;
  /* TODO: Parse stats json */
  VTR_LOGV(verbose, "Parsing Yosys results\n");
  std::ifstream f(stats_fpath);
  nlohmann::json data = nlohmann::json::parse(f);
  /* Walk through the cell types */
  for (auto& [key, value] : data.at(STATS_JSON_DESIGN_TAG_).items()) {
    /* For detailed cell type stats, keep walk through */
    if (key == STATS_JSON_CELL_DETAILS_TAG_) {
      for (auto& [cell, count] : value.items()) {
        sweep_k_data_[cell][bm_id][std::to_string(k)] = count.get<size_t>();
      }
      continue;
    }
    /* Add the stat */
    sweep_k_data_[key][bm_id][std::to_string(k)] = value.get<size_t>();
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::parse_synth_stats_bitgen(const BenchmarkId& bm_id,
                                              const std::string& runtime_ys,
                                              const std::string& stats_fpath,
                                              const std::string& corner, const bool& verbose) {
  int num_err = 0;
  /* TODO: Parse yosys log */
  /* Parse stats json */
  VTR_LOGV(verbose, "Parsing Yosys results\n");
  std::ifstream f(stats_fpath);
  nlohmann::json data = nlohmann::json::parse(f);
  /* Walk through the cell types */
  for (auto& [key, value] : data.at(STATS_JSON_DESIGN_TAG_).items()) {
    /* For detailed cell type stats, keep walk through */
    if (key == STATS_JSON_CELL_DETAILS_TAG_) {
      for (auto& [cell, count] : value.items()) {
        bitgen_frontend_data_[cell][bm_id][corner] = count.get<float>();
      }
      continue;
    }
    /* Add the stat */
    bitgen_frontend_data_[key][bm_id][corner] = value.get<float>();
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::run_benchmark_synth_sweep_k(const std::string& fe_bin_path, const size_t& k,
                                                 const bool& verbose) {
  int num_err = 0;
  for (auto st_id : bm_mgr_->suites()) {
    VTR_LOGV(verbose, "Running benchmarking for suite '%s'...\n",
             bm_mgr_->suite_name(st_id).c_str());
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      if (bm_mgr_->is_benchmark_disabled(bm_id)) {
        VTR_LOGV(verbose, "Bypass benchmark '%s' as it is disabled by user\n",
                 bm_mgr_->benchmark_name(bm_id).c_str());
        continue;
      }
      std::string pre_arch_subdir_name = pre_arch_sweepk_subdir_name(bm_id, k);
      std::string runtime_ys;
      num_err += prepare_synth_sweep_k(bm_id, k, runtime_ys, verbose);
      num_err += run_synth(fe_bin_path, runtime_ys, verbose);
      num_err += parse_synth_stats_sweepk(
        bm_id, k, runtime_ys, pre_arch_file(pre_arch_subdir_name, e_file_type::STAT_JSON, verbose),
        verbose);
    }
    VTR_LOGV(verbose, "Done\n");
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::sweep_lut_size(const std::string& fe_bin_path, const bool& verbose) {
  int num_err = 0;
  for (size_t k = min_k_; k <= max_k_; ++k) {
    /* For selected benchmark, run synthesis */
    num_err += run_benchmark_synth_sweep_k(fe_bin_path, k, verbose);
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

void ArchEvalManager::set_min_k(const size_t& val) { min_k_ = val; }
void ArchEvalManager::set_max_k(const size_t& val) { max_k_ = val; }

bool ArchEvalManager::add_yosys_script(const ArchEvalManager::e_script_type& f_type,
                                       const std::string& f_name) {
  if (!valid_yosys_script_type(f_type)) {
    return false;
  }
  src_script_files_[size_t(f_type)] = f_name;
  return true;
}

bool ArchEvalManager::add_openfpga_script(const ArchEvalManager::e_script_type& f_type,
                                          const std::string& f_name) {
  if (!valid_openfpga_script_type(f_type)) {
    return false;
  }
  src_script_files_[size_t(f_type)] = f_name;
  return true;
}

bool ArchEvalManager::valid_script_type(const ArchEvalManager::e_script_type& f_type) const {
  return f_type != ArchEvalManager::e_script_type::NUM_TYPES;
}

bool ArchEvalManager::valid_openfpga_script_type(
  const ArchEvalManager::e_script_type& f_type) const {
  return size_t(ArchEvalManager::e_script_type::OPENFPGA_SCRIPT_BITGEN) <= size_t(f_type) &&
         size_t(f_type) < size_t(ArchEvalManager::e_script_type::NUM_TYPES);
}

bool ArchEvalManager::valid_yosys_script_type(const ArchEvalManager::e_script_type& f_type) const {
  return size_t(ArchEvalManager::e_script_type::YOSYS_SCRIPT_SWEEP_K) <= size_t(f_type) &&
         size_t(f_type) <= size_t(ArchEvalManager::e_script_type::YOSYS_SCRIPT_SYNTH);
}

bool ArchEvalManager::valid_file_type(const ArchEvalManager::e_file_type& f_type) const {
  return f_type != ArchEvalManager::e_file_type::NUM_TYPES;
}

void ArchEvalManager::check_valid_file_type(const ArchEvalManager::e_file_type& f_type) const {
  if (!valid_file_type(f_type)) {
    VTR_LOG_ERROR("Invalid file type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

ArchEvalManager::e_script_type ArchEvalManager::convert_script_type(
  const TapeoutManager::e_script_type& to_f_type) const {
  switch (to_f_type) {
    case TapeoutManager::e_script_type::YOSYS_SCRIPT_SWEEPK:
      return e_script_type::YOSYS_SCRIPT_SWEEP_K;
    case TapeoutManager::e_script_type::YOSYS_SCRIPT_SYNTH:
      return e_script_type::YOSYS_SCRIPT_SYNTH;
    case TapeoutManager::e_script_type::YOSYS_SCRIPT_REWRITE:
      return e_script_type::YOSYS_SCRIPT_REWRITE;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_BITGEN:
      return e_script_type::OPENFPGA_SCRIPT_BITGEN;
    default:
      return e_script_type::NUM_TYPES;
  }
  return e_script_type::NUM_TYPES;
}

bool ArchEvalManager::required_script_type(const TapeoutManager::e_script_type& f_type) const {
  return valid_script_type(convert_script_type(f_type));
}

int ArchEvalManager::copy_file_with_mkdir(const std::string& src_file, const std::string& des_file,
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
    std::filesystem::copy_options cp_opts = std::filesystem::copy_options::overwrite_existing;
    if (std::filesystem::is_directory(src_file)) {
      cp_opts = std::filesystem::copy_options::overwrite_existing |
                std::filesystem::copy_options::recursive;
    }
    if (std::filesystem::exists(des_file) && !std::filesystem::is_directory(des_file)) {
      VTR_LOGV(verbose, "Remove existing file '%s'\n", des_file.c_str());
      std::filesystem::remove(des_file);
    }
    if (std::filesystem::exists(des_file) && std::filesystem::is_directory(des_file)) {
      VTR_LOGV(verbose, "Remove existing directory '%s'\n", des_file.c_str());
      std::filesystem::remove_all(des_file);
    }
    std::filesystem::copy(src_file, des_file, cp_opts);
  } else {
    if (std::filesystem::exists(des_file)) {
      std::filesystem::remove(des_file);
    }
    num_err += universal_create_symlink(src_file, des_file) ? 0 : 1;
  }
  if (!std::filesystem::exists(des_file)) {
    VTR_LOG_ERROR("Destination file '%s' does not exist!\n", des_file.c_str());
    num_err++;
  }
  return num_err;
}

int ArchEvalManager::init_bitgen_dir(const bool& link2arch_data, const bool& verbose) const {
  int num_err = 0;
  std::filesystem::path proj_home = bitgen_project_home_dir();
  /* Link arch files */
  std::filesystem::path src_arch_dir = src_arch_dir_;
  std::filesystem::path des_arch_dir = proj_home / std::filesystem::path(ARCH_DIR_NAME_);
  /* Windows does not support symbolic links well. This causes VPR to fail when reading routing
   * resource graph, force to copy as a solution. Even though this is a dirty fix leading to disk
   * space overhead, it works */
#if defined(_WIN32) || defined(__MINGW64__)
  VTR_LOG("Automatically force arch data copy in the runtime directory\n");
  num_err += copy_file_with_mkdir(src_arch_dir.string(), des_arch_dir.string(), false, verbose);
#else
  if (!link2arch_data) {
    VTR_LOG("User force arch data copy in the runtime directory\n");
  }
  num_err +=
    copy_file_with_mkdir(src_arch_dir.string(), des_arch_dir.string(), link2arch_data, verbose);
#endif

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

std::string ArchEvalManager::bitgen_openfpga_runtime_file(
  const ArchEvalManager::e_file_type& f_type, const std::string& subdir_name,
  const bool& verbose) const {
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = bitgen_project_home_dir();
  if (f_type == e_file_type::VPR_ARCH || f_type == e_file_type::OPENFPGA_ARCH) {
    std::filesystem::path ret = proj_home / std::filesystem::path(subdir_name) /
                                std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
    return ret.string();
  }
  return "";
}

int ArchEvalManager::init_bitgen_runtime_dir(const std::string& subdir_name,
                                             const std::string& corner, const bool& verbose) const {
  int num_err = 0;

  ArchAggregator arch_agg(bitgen_file(BenchmarkSuiteId::INVALID(), BenchmarkId::INVALID(),
                                      ArchEvalManager::e_file_type::DESIGN_VAR, corner, verbose),
                          verbose);
  arch_agg.load_variables(bitgen_file(BenchmarkSuiteId::INVALID(), BenchmarkId::INVALID(),
                                      ArchEvalManager::e_file_type::ARCH_TIMING, corner, verbose),
                          false, false, verbose);
  /* Generate the final vpr and openfpga arch file */
  std::string input_vpr_arch = bitgen_file(BenchmarkSuiteId::INVALID(), BenchmarkId::INVALID(),
                                           ArchEvalManager::e_file_type::VPR_ARCH, corner, verbose);
  std::string runtime_vpr_arch =
    bitgen_openfpga_runtime_file(ArchEvalManager::e_file_type::VPR_ARCH, subdir_name, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_vpr_dirpath = runtime_vpr_arch;
  openfpga::create_directory(runtime_vpr_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_vpr_arch, runtime_vpr_arch, verbose);

  /* Generate the final vpr and openfpga arch file */
  std::string input_openfpga_arch =
    bitgen_file(BenchmarkSuiteId::INVALID(), BenchmarkId::INVALID(),
                ArchEvalManager::e_file_type::OPENFPGA_ARCH, corner, verbose);
  std::string runtime_openfpga_arch =
    bitgen_openfpga_runtime_file(ArchEvalManager::e_file_type::OPENFPGA_ARCH, subdir_name, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_openfpga_dirpath = runtime_openfpga_arch;
  openfpga::create_directory(runtime_openfpga_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_openfpga_arch, runtime_openfpga_arch, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

std::string ArchEvalManager::bitstream_output_dir(const BenchmarkSuiteId& st_id,
                                                  const BenchmarkId& bm_id,
                                                  const std::string& corner) const {
  std::filesystem::path ret;
  ret = std::filesystem::path(BITSTREAM_DIR_NAME_) /
        std::filesystem::path(bm_mgr_->suite_name(st_id)) /
        std::filesystem::path(bm_mgr_->benchmark_name(bm_id)) / std::filesystem::path(corner);
  return ret.string();
}

std::string ArchEvalManager::application_sim_output_dir(const BenchmarkSuiteId& st_id,
                                                        const BenchmarkId& bm_id,
                                                        const std::string& corner,
                                                        const std::string& sim_type) const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret;
  ret = proj_home / std::filesystem::path(APP_SIM_DIR_NAME_) /
        std::filesystem::path(bm_mgr_->suite_name(st_id)) /
        std::filesystem::path(bm_mgr_->benchmark_name(bm_id)) / std::filesystem::path(corner) /
        std::filesystem::path(sim_type);

  return ret.string();
}

/*run cocotb testbench*/
int ArchEvalManager::invoke_cocotb_for_application_simulation(
  const TapeoutManager& to_mgr, const BenchmarkId& benchmark_id,
  const BenchmarkSuiteId& benchmark_suite_id, const std::string& sim_input_files,
  const std::string& simulator, const std::string& corner, const std::string& sim_type,
  const std::string waveform_option) const {
  /*set environment*/
  std::string make_cmd = "make sim ";
  std::vector<BenchmarkCocotbSourceId> cocotb_source_id = bm_mgr_->cocotb_sources(benchmark_id);
  std::string cocotb_make_file;
  std::string cocotb_top_file;
  for (auto source_id : cocotb_source_id) {
    if (bm_mgr_->cocotb_type(benchmark_id, source_id) ==
        BenchmarkManager::e_file_type::COCOTB_TOP) {
      cocotb_top_file = bm_mgr_->cocotb_file(benchmark_id, source_id);

    } else if (bm_mgr_->cocotb_type(benchmark_id, source_id) ==
               BenchmarkManager::e_file_type::COCOTB_MAKEFILE) {
      cocotb_make_file = bm_mgr_->cocotb_file(benchmark_id, source_id);
    }
  }

  std::filesystem::path src_cocotb_dir = std::filesystem::canonical(cocotb_make_file).parent_path();
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path application_simulation = std::filesystem::path(
    application_sim_output_dir(benchmark_suite_id, benchmark_id, corner, sim_type));
  if (std::filesystem::exists(application_simulation)) {
    std::filesystem::remove_all(application_simulation);
  }
  openfpga::create_directory(application_simulation.string(), true, false);
  std::filesystem::copy(src_cocotb_dir, application_simulation,
                        std::filesystem::copy_options::recursive);
  std::filesystem::current_path(application_simulation);
  std::string cocotb_convert_makefile_application_sim_path =
    to_mgr.cocotb_convert_makefile_application_simulation();
  int convert_status =
    bm_mgr_->convert_make_file_simulation(PYTHON_COMMAND_, to_mgr.python_site_package_path(),
                                          cocotb_convert_makefile_application_sim_path, true);
  if (convert_status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string env_value =
    std::filesystem::path(to_mgr.benchmark_simulation_cocotb_makefile_include_file_path())
      .parent_path()
      .string();
  if (waveform_option.empty()) {
    make_cmd += "VERILOG_SOURCES_USER=" + sim_input_files + " SIM=" + simulator +
                " MAKEFILE_INC_HOME=" + env_value + " ";
  } else {
    make_cmd += "VERILOG_SOURCES_USER=" + sim_input_files + " SIM=" + simulator +
                " MAKEFILE_INC_HOME=" + env_value + " " + waveform_option + "=1 ";
  }
  /* FIXME: This is a dirty patch on the misleading error where cocotb complains that the FSDB
   * dumper version is older than the VCS simulation version. This only occurs to VCS simulator.
   * Such error messages should be checked by the cocotb utility scripts not by AAEE. AAEE only
   * checks the cocotb sim return codes. */
  make_cmd += " > benchmark_cocotb_sim.log";
  if (system(make_cmd.c_str())) {
    std::filesystem::current_path(curr_dir);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!waveform_option.empty()) {
    std::filesystem::current_path(application_simulation);
    std::filesystem::path search_dir = std::filesystem::current_path();
    std::string post_fix = "";
    bool found_file;
    if (waveform_option == "DUMP_FSDB") {
      post_fix = ".fsdb";
    } else if (waveform_option == "DUMP_VCD") {
      post_fix = ".vcd";
    }
    for (const auto& entry : std::filesystem::directory_iterator(search_dir)) {
      if (entry.is_regular_file()) {
        std::string ext = entry.path().extension().string();
        if (ext == post_fix) {
          found_file = true;
          break;
        }
      }
    }
    if (!found_file) {
      VTR_LOG_ERROR("expected file with format %s is not generated. ", post_fix.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  std::filesystem::current_path(curr_dir);
  return openfpga::CMD_EXEC_SUCCESS;
}

std::string ArchEvalManager::vpr_report_output_dir(const BenchmarkSuiteId& st_id,
                                                   const BenchmarkId& bm_id,
                                                   const std::string& corner) const {
  std::filesystem::path ret;
  ret = std::filesystem::path(VPR_REPORT_DIR_NAME_) /
        std::filesystem::path(bm_mgr_->suite_name(st_id)) /
        std::filesystem::path(bm_mgr_->benchmark_name(bm_id)) / std::filesystem::path(corner);
  return ret.string();
}

int ArchEvalManager::run_global_clock_assigner(const std::string& subdir_name,
                                               const BenchmarkSuiteId& st_id,
                                               const BenchmarkId& bm_id, const std::string& corner,
                                               pugi::xml_node& vpr_constr_xml_root,
                                               const bool& verbose) const {
  int num_err = 0;

  /* TODO: Use the final PCF GCLK generated by Global Signal Assigner */
  std::string user_gclk_pcf =
    bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_PCF_XML, corner, verbose);
  size_t cnt_user_pcf = 0;
  for (auto f_id : bm_mgr_->constraints_by_type(bm_id, BenchmarkManager::e_file_type::PCF_GCLK)) {
    user_gclk_pcf = bm_mgr_->constraint_file(bm_id, f_id);
    cnt_user_pcf++;
  }
  if (cnt_user_pcf > 1) {
    VTR_LOG_ERROR(
      "More than 1 pcf for global signal is detected. Currently, only support one PCF per "
      "benchmark!\n");
    num_err++;
  }

  std::string ckbuf_cell_map_file =
    bitgen_file(st_id, bm_id, e_file_type::OUT_CKBUF_MAP, corner, verbose);
  if (!std::filesystem::exists(ckbuf_cell_map_file)) {
    /* Use the default one */
    ckbuf_cell_map_file =
      bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_CKBUF_MAP, corner, verbose);
    VTR_LOGV(verbose,
             "Use the default ckbuf cell map file as users default do not have internally "
             "generated global signals\n");
  }

  std::string ckbuf_loc_map_file =
    bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_CKBUF_LOC_MAP, corner, verbose);
  std::string final_gclk_pcf =
    bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_FINAL_PCF_XML, corner, verbose);
  std::string final_gclk_pcf4dv =
    bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_FINAL_PCF_XML4DV, corner, verbose);

  VTR_LOGV(verbose, "Generating final pcf for global signals...\n");
  VTR_LOGV(verbose, "\tckbuf cell map: %s\n", ckbuf_cell_map_file.c_str());
  VTR_LOGV(verbose, "\tckbuf loc map: %s\n", ckbuf_loc_map_file.c_str());
  VTR_LOGV(verbose, "\tuser pcf: %s\n", user_gclk_pcf.c_str());
  VTR_LOGV(verbose, "\tfinal pcf: %s\n", final_gclk_pcf.c_str());
  VTR_LOGV(verbose, "\tfinal pcf for design verification: %s\n", final_gclk_pcf4dv.c_str());
  int status = generate_global_signal_vpr_constraint_file(
    ckbuf_cell_map_file, user_gclk_pcf, ckbuf_loc_map_file, vpr_constr_xml_root, final_gclk_pcf);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    num_err++;
  }

  status = generate_global_signal_pcf4dv(ckbuf_cell_map_file, user_gclk_pcf, ckbuf_loc_map_file,
                                         final_gclk_pcf4dv);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::run_blif_param2vpr_constraint(
  const std::string& subdir_name, const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
  const std::string& corner, pugi::xml_node& vpr_constr_xml_root, const bool& verbose) const {
  int num_err = 0;

  std::string vpr_arch_fpath = bitgen_file(st_id, bm_id, e_file_type::VPR_ARCH, corner, verbose);
  std::string blif_fpath = bitgen_file(st_id, bm_id, e_file_type::OUT_BLIF, corner, verbose);

  VTR_LOGV(verbose, "Generating vpr constraints from location parameters...\n");
  VTR_LOGV(verbose, "\tblif: %s\n", blif_fpath.c_str());
  VTR_LOGV(verbose, "\tvpr architecture: %s\n", vpr_arch_fpath.c_str());

  /* Find the proper node to insert information, if there is no node available, skip */
  pugiutil::loc_data loc_data;
  pugi::xml_node xml_parti_root =
    get_single_child(vpr_constr_xml_root, XML_VPR_CONSTRAINT_FILE_SUBROOT_NAME, loc_data,
                     pugiutil::ReqOpt::OPTIONAL);
  if (!xml_parti_root) {
    xml_parti_root = vpr_constr_xml_root.append_child(XML_VPR_CONSTRAINT_FILE_SUBROOT_NAME);
  }
  int status = blif_param2vpr_constraint(blif_fpath, vpr_arch_fpath, xml_parti_root, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    num_err++;
  }
  /* Remove if there is no child appended under the partition list, because VPR cannot accept it now
   */
  if (count_children(xml_parti_root, loc_data, pugiutil::ReqOpt::OPTIONAL) == 0) {
    vpr_constr_xml_root.remove_child(xml_parti_root);
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::generate_runtime_vpr_constraints(const std::string& subdir_name,
                                                      const BenchmarkSuiteId& st_id,
                                                      const BenchmarkId& bm_id,
                                                      const std::string& corner,
                                                      const bool& verbose) const {
  int num_err = 0;

  /* Aggregate XML document here */
  pugi::xml_document out_xml;
  pugi::xml_node root_node = out_xml.append_child(XML_VPR_CONSTRAINT_FILE_ROOT_NAME);

  num_err += run_global_clock_assigner(subdir_name, st_id, bm_id, corner, root_node, verbose);
  num_err += run_blif_param2vpr_constraint(subdir_name, st_id, bm_id, corner, root_node, verbose);

  std::string rt_vpr_constr_file =
    bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_VPR_CONSTRAINT_FILE, corner, verbose);

  out_xml.save_file(rt_vpr_constr_file.c_str());
  VTR_LOGV(verbose, "\tGenerate runtime vpr constraint: %s\n", rt_vpr_constr_file.c_str());

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::init_bitgen_runtime_script(const std::string& subdir_name,
                                                const BenchmarkSuiteId& st_id,
                                                const BenchmarkId& bm_id, const std::string& corner,
                                                const bool& verbose) const {
  int num_err = 0;
  /* Generate the final openfpga script file */
  ArchAggregator script_agg;
  /* Add variables. */
  for (size_t itype = 0; itype < size_t(e_file_type::NUM_TYPES); ++itype) {
    e_file_type curr_type = static_cast<e_file_type>(itype);
    if (curr_type == e_file_type::VPR_ARCH || curr_type == e_file_type::OPENFPGA_ARCH) {
      std::string rt_file = bitgen_openfpga_runtime_file(curr_type, subdir_name, verbose);
      script_agg.add_variable(VAR_NAMES_[itype], rt_file);
      continue;
    }
    if (curr_type == e_file_type::DESIGN_VAR) {
      continue;
    }
    script_agg.add_variable(VAR_NAMES_[itype],
                            bitgen_file(st_id, bm_id, curr_type, corner, verbose));
  }
  /* Add other variables, which are related to flow control. i.e., layout, channel width etc. */
  script_agg.add_variable(VPR_FABRIC_VAR_NAME_, fabric_name_);
  script_agg.add_variable(VPR_ROUTE_CHAN_W_VAR_NAME_, route_chan_width_);
  /* Add benchmark info */
  script_agg.add_variable(VPR_BLIF_VAR_NAME_,
                          bitgen_file(st_id, bm_id, e_file_type::OUT_BLIF, corner, verbose));

  // Add system-generated SDC constraint files (retrieved from BenchmarkManager)
  for (auto f_id : bm_mgr_->constraints_by_type(bm_id, BenchmarkManager::e_file_type::SDC)) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::VPR_SDC)],
                            bm_mgr_->constraint_file(bm_id, f_id));
  }

  /* Boundary timing may come from the source */
  if (!src_boundary_timing_files_.at(corner).empty()) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_BOUNDARY_TIMING)],
                            src_boundary_timing_files_.at(corner));
  }
  /* TODO: Use the final PCF GCLK generated by Global Signal Assigner */
  for (auto f_id : bm_mgr_->constraints_by_type(bm_id, BenchmarkManager::e_file_type::PCF_GCLK)) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_PCF_XML)],
                            bm_mgr_->constraint_file(bm_id, f_id));
  }
  for (auto f_id : bm_mgr_->constraints_by_type(bm_id, BenchmarkManager::e_file_type::PCF)) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_PCF)],
                            bm_mgr_->constraint_file(bm_id, f_id));
  }

  // Collect all constraint files to merge
  std::vector<std::string> all_vpr_constraint_files;
  std::set<std::string> unique_vpr_constraint_files;

  // Add system-generated VPR constraint file (e.g., ckbuf_vpr_constraints.xml)
  std::string system_vpr_constraint_file =
    bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_VPR_CONSTRAINT_FILE, corner, verbose);
  if (std::filesystem::exists(system_vpr_constraint_file)) {
    if (unique_vpr_constraint_files.insert(system_vpr_constraint_file).second) {
      all_vpr_constraint_files.push_back(system_vpr_constraint_file);
    }
    VTR_LOGV(verbose, "System VPR constraint file found: %s\n", system_vpr_constraint_file.c_str());
  } else {
    VTR_LOGV(verbose, "System VPR constraint file not found: %s\n",
             system_vpr_constraint_file.c_str());
  }

  // Then add constraint files added by users via add_benchmark_vpr_constraints
  if (to_mgr_ != nullptr) {
    std::string benchmark_name = bm_mgr_->benchmark_name(bm_id);
    std::vector<std::string> user_vpr_constraint_files =
      to_mgr_->get_vpr_constraint_files(benchmark_name, run_name_, corner);

    // Add all user constraint files
    for (const auto& constraint_file : user_vpr_constraint_files) {
      // Check if file exists (resolve relative paths against current or project root directory)
      std::filesystem::path file_path(constraint_file);
      std::string resolved_path = constraint_file;

      if (file_path.is_relative()) {
        // If it's a relative path, try to resolve it relative to the project directory
        std::filesystem::path full_path = std::filesystem::path(root_path_) / file_path;
        resolved_path = full_path.string();
      }

      // Check if resolved path exists
      if (std::filesystem::exists(resolved_path)) {
        if (unique_vpr_constraint_files.insert(resolved_path).second) {
          all_vpr_constraint_files.push_back(resolved_path);
          VTR_LOGV(verbose, "User VPR constraint file added: %s\n", resolved_path.c_str());
        } else {
          VTR_LOGV(verbose, "Skipped duplicate VPR constraint file: %s\n", resolved_path.c_str());
        }
      } else {
        VTR_LOG_WARN("User constraint file does not exist: %s\n", resolved_path.c_str());
      }
    }
  }

  // Log the number of constraint files to help with debugging
  VTR_LOGV(verbose, "Found %zu constraint files to merge for benchmark %s, corner %s\n",
           all_vpr_constraint_files.size(), bm_mgr_->benchmark_name(bm_id).c_str(), corner.c_str());

  for (const auto& file : all_vpr_constraint_files) {
    VTR_LOGV(verbose, "  - %s\n", file.c_str());
  }

  // Merge multiple constraint files into a single file if there are more than one
  if (all_vpr_constraint_files.size() > 1) {
    // Get the base directory for constraint files using bitgen_file()
    std::string original_constraint_file =
      bitgen_file(st_id, bm_id, e_file_type::OPENFPGA_VPR_CONSTRAINT_FILE, corner, verbose);

    std::filesystem::path orig_path(original_constraint_file);
    std::string base_name = orig_path.stem().string();
    std::string extension = orig_path.extension().string();
    std::filesystem::path constraint_dir = orig_path.parent_path();

    // Create merged constraint file path in the same directory as system-generated constraints
    std::string merged_constraint_file =
      (constraint_dir / (base_name + "_merged" + extension)).string();

    VTR_LOGV(verbose, "Creating merged constraint file: %s\n", merged_constraint_file.c_str());

    // Merge XML constraint files
    merge_vpr_constraint_files(all_vpr_constraint_files, merged_constraint_file);

    // Add merged file to script aggregator
    script_agg.add_1toN_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_VPR_CONSTRAINT_FILE)],
                                 merged_constraint_file);
  } else if (all_vpr_constraint_files.size() == 1) {
    // Add the single file directly if only one exists
    script_agg.add_1toN_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_VPR_CONSTRAINT_FILE)],
                                 all_vpr_constraint_files[0]);
  } else {
    // No constraint files to add
    VTR_LOGV(verbose, "No constraint files to add for benchmark %s, corner %s\n",
             bm_mgr_->benchmark_name(bm_id).c_str(), corner.c_str());
  }

  // Handle pin utilization - merge custom values with defaults from arch template

  std::string benchmark_name = bm_mgr_->benchmark_name(bm_id);

  // Get merged pin utilization: custom values + defaults from arch template
  std::string pin_util_str =
    to_mgr_->get_target_ext_pin_utilization_str(benchmark_name, run_name_, corner);

  if (!pin_util_str.empty()) {
    VTR_LOGV(verbose, "Using merged pin utilization (custom + defaults): %s\n",
             pin_util_str.c_str());
    script_agg.add_variable("OPENFPGA_VPR_TARGET_EXT_PIN_UTILIZATION", pin_util_str);
  } else {
    // Always get default pin utilization from arch template
    std::string default_pin_util_str = to_mgr_->get_all_default_pin_utilization_str();

    // If default_pin_util_str is empty but should not be, log this issue but still assign the
    // value to ensure the script variable gets replaced properly
    if (default_pin_util_str.empty()) {
      VTR_LOG_ERROR(
        "Issue: Could not retrieve default pin utilization from arch template. The "
        "efpga_arch_template.xml should contain default_pack_pin_utilization section.\n");
    }

    // Always set the variable to ensure script substitution happens
    script_agg.add_variable("OPENFPGA_VPR_TARGET_EXT_PIN_UTILIZATION", default_pin_util_str);
  }

  /* Do aggregation here */
  std::string input_openfpga_script = openfpga_script_file(e_script_type::OPENFPGA_SCRIPT_BITGEN);
  std::string runtime_openfpga_script = openfpga_runtime_script_file(subdir_name, verbose);
  num_err += script_agg.aggregate(input_openfpga_script, runtime_openfpga_script, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

std::string ArchEvalManager::openfpga_script_file(const e_script_type& runtime_type) const {
  return std::filesystem::path(src_script_files_[size_t(runtime_type)]).string();
}

std::string ArchEvalManager::openfpga_runtime_script_file(const std::string& subdir_name,
                                                          const bool& verbose) const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret = proj_home / std::filesystem::path(subdir_name) /
                              std::filesystem::path(OPENFPGA_SCRIPT_FILENAME_);
  return ret.string();
}

std::string ArchEvalManager::openfpga_runtime_log_file(const std::string& subdir_name,
                                                       const bool& verbose) const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret =
    proj_home / std::filesystem::path(subdir_name) / std::filesystem::path(OPENFPGA_LOG_FILENAME_);
  return ret.string();
}

void ArchEvalManager::set_route_chan_width(const size_t& val) { route_chan_width_ = val; }
void ArchEvalManager::set_fabric_name(const std::string& val) { fabric_name_ = val; }

int ArchEvalManager::parse_vpr_block_usage_bitgen(const BenchmarkId& bm_id,
                                                  const std::string& log_fpath,
                                                  const std::string& corner, const bool& verbose) {
  int num_err = 0;
  VTR_LOGV(verbose, "Parsing VTR block usage results\n");
  /* TODO: Call the Block Usage File parser */
  VprBlockUsageParser bu_parser;
  int status = bu_parser.parse(log_fpath, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Load the data */
  bitgen_openfpga_data_[VPR_STATS_NUM_INPUTS_TAG_][bm_id][corner] =
    static_cast<float>(bu_parser.num_inputs());
  bitgen_openfpga_data_[VPR_STATS_NUM_OUTPUTS_TAG_][bm_id][corner] =
    static_cast<float>(bu_parser.num_outputs());
  bitgen_openfpga_data_[VPR_STATS_NUM_NETS_TAG_][bm_id][corner] =
    static_cast<float>(bu_parser.num_nets());
  for (auto blk_type : bu_parser.block_types()) {
    bitgen_openfpga_data_[blk_type][bm_id][corner] =
      static_cast<float>(bu_parser.block_usage(blk_type));
  }
  VTR_LOGV(verbose, "Done\n");
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::parse_openfpga_stats_bitgen(const BenchmarkId& bm_id,
                                                 const std::string& log_fpath,
                                                 const std::string& corner, const bool& verbose) {
  int num_err = 0;
  VTR_LOGV(verbose, "Parsing OpenFPGA results\n");

  /* TODO: Need a dedicated file containing keywords when parse results */
  vpr_log_parser_.clear_results();
  int status = vpr_log_parser_.parse(log_fpath, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto kw : vpr_log_parser_.keywords()) {
    bitgen_openfpga_data_[kw][bm_id][corner] = vpr_log_parser_.value(kw);
  }
  VTR_LOGV(verbose, "Done\n");

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::run_bitgen(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                const std::string& fe_bin_path,
                                const std::string& openfpga_bin_path, const std::string& corner,
                                const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Create runtime directory */
  std::string subdir_name = bitgen_subdir_name(bm_id, corner);

  /* Run yosys rewrite process before starting anything */
  std::string rewrite_runtime_ys;
  status = prepare_synth_rewrite(st_id, bm_id, corner, rewrite_runtime_ys, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Run yosys */
  status = run_synth(fe_bin_path, rewrite_runtime_ys, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Yosys rewrite failed for benchmark %s. Runtime directory: %s\n",
                  bm_mgr_->benchmark_name(bm_id).c_str(),
                  std::filesystem::path(rewrite_runtime_ys).parent_path().string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Check result files */
  std::vector<e_file_type> rewrite_ys_f_types_to_check = {e_file_type::OUT_VERILOG_REWRITE};
  status = check_bitgen_result_files(st_id, bm_id, rewrite_ys_f_types_to_check, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Now we start formally the bitgen process: Generate yosys runtime scripts */
  std::string runtime_ys;
  status = prepare_synth_bitgen(st_id, bm_id, corner, runtime_ys, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Run yosys */
  status = run_synth(fe_bin_path, runtime_ys, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Yosys run failed for benchmark %s. Runtime directory: %s\n",
                  bm_mgr_->benchmark_name(bm_id).c_str(),
                  std::filesystem::path(runtime_ys).parent_path().string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Check result files */
  std::vector<e_file_type> ys_f_types_to_check = {e_file_type::OUT_BLIF, e_file_type::OUT_VERILOG,
                                                  e_file_type::STAT_JSON};
  status = check_bitgen_result_files(st_id, bm_id, ys_f_types_to_check, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Check log and parse results. TODO: Early exit if failed! */
  status = parse_synth_stats_bitgen(
    bm_id, runtime_ys, bitgen_file(st_id, bm_id, e_file_type::STAT_JSON, corner, verbose), corner,
    verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Parse yosys stats failed for benchmark %s! \n",
                  bm_mgr_->benchmark_name(bm_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Generate architecture runtime files */
  status = init_bitgen_runtime_dir(subdir_name, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Generate runtime vpr constraints */
  status = generate_runtime_vpr_constraints(subdir_name, st_id, bm_id, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Generate openfpga runtime scripts */
  status = init_bitgen_runtime_script(subdir_name, st_id, bm_id, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Run OpenFPGA */
  status = runtime_exec_openfpga(subdir_name, openfpga_bin_path, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Check result files */
  std::vector<e_file_type> of_f_types_to_check = {e_file_type::OUT_BLIF,
                                                  e_file_type::VPR_NET_FILE,
                                                  e_file_type::VPR_PLACE_FILE,
                                                  e_file_type::VPR_ROUTE_FILE,
                                                  e_file_type::BITSTREAM_TEXT_FILE,
                                                  e_file_type::BITSTREAM_XML_FILE,
                                                  e_file_type::BITSTREAM_PIN_MAP_FILE};
  status = check_bitgen_result_files(st_id, bm_id, of_f_types_to_check, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Parse results */
  status = parse_vpr_block_usage_bitgen(
    bm_id, bitgen_file(st_id, bm_id, e_file_type::VPR_BLOCK_USAGE_FILE, corner, verbose), corner,
    verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = parse_openfpga_stats_bitgen(bm_id, openfpga_runtime_log_file(subdir_name, verbose),
                                       corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::check_bitgen_result_files(const BenchmarkSuiteId& st_id,
                                               const BenchmarkId& bm_id,
                                               const std::vector<e_file_type>& f_types_to_check,
                                               const std::string& corner,
                                               const bool& verbose) const {
  int num_err = 0;
  for (e_file_type f_type_to_check : f_types_to_check) {
    std::string chk_fpath = bitgen_file(st_id, bm_id, f_type_to_check, corner, verbose);
    if (!std::filesystem::exists(chk_fpath)) {
      VTR_LOG_ERROR("Expected resulting file %s is not generated:\n\t%s\n",
                    FILE_NAMES_[size_t(f_type_to_check)].c_str(), chk_fpath.c_str());
      num_err++;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::runtime_exec_openfpga(const std::string& subdir_name,
                                           const std::string& openfpga_bin_path,
                                           const bool& verbose) {
  int num_err = 0;
  std::string runtime_openfpga_script = openfpga_runtime_script_file(subdir_name, verbose);
  /* Run openfpga */
  std::string openfpga_cmd = openfpga_bin_path + " -batch -f " + runtime_openfpga_script +
                             std::string(" > ") + OPENFPGA_LOG_FILENAME_;
  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = std::filesystem::path(runtime_openfpga_script).parent_path();
  std::filesystem::path log_fpath = runtime_dir / std::filesystem::path(OPENFPGA_LOG_FILENAME_);
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run OpenFPGA!\n");
    std::filesystem::current_path(curr_dir);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(openfpga_cmd.c_str())) {
    VTR_LOG_ERROR("OpenFPGA run failed with errors! Log: %s\n", log_fpath.string().c_str());
    std::filesystem::current_path(curr_dir);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());
  /*  Check log file errors */
  VTR_LOGV(verbose, "Parsing OpenFPGA log\n");
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

int ArchEvalManager::generate_bitstream(const std::string& fe_bin_path,
                                        const std::string& openfpga_bin_path,
                                        const std::string& corner,
                                        ParallelTaskScheduler& task_scheduler,
                                        const bool& force_copy_arch_data, const bool& verbose) {
  int num_err = 0;
  /* Initializing directory */
  VTR_LOG("Initializing bitstream generation workspace...\n");
  init_bitgen_dir(!force_copy_arch_data, verbose);
  VTR_LOG("Done\n");

  /* Run bitstream generation one by one */
  for (auto st_id : bm_mgr_->suites()) {
    VTR_LOG("Running bitstream generation for suite '%s' for corner '%s'...\n",
            bm_mgr_->suite_name(st_id).c_str(), corner.c_str());
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      if (bm_mgr_->is_benchmark_disabled(bm_id)) {
        VTR_LOGV(verbose, "Bypass benchmark '%s' as it is disabled by user\n",
                 bm_mgr_->benchmark_name(bm_id).c_str());
        continue;
      }
/* Task schedule has bugs in windows system. Disable it for now */
#ifdef _WIN32
      num_err += run_bitgen(st_id, bm_id, fe_bin_path, openfpga_bin_path, corner, verbose);
#else
      task_scheduler.add_task([=, this]() -> int {
        return run_bitgen(st_id, bm_id, fe_bin_path, openfpga_bin_path, corner, verbose);
      });
#endif
      if (num_err) {
        VTR_LOG_ERROR("Failed to run bitstream generation for benchmark '%s'!\n",
                      bm_mgr_->benchmark_name(bm_id).c_str());
        return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
      }
    }
    VTR_LOG("Done\n");
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::extract_bitgen_qor(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                        const std::string& corner, const bool& verbose) {
  int num_err = 0;
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  std::string block_usage_file_path =
    bitgen_file(st_id, bm_id, e_file_type::VPR_BLOCK_USAGE_FILE, corner, verbose);

  if (!std::filesystem::exists(block_usage_file_path)) {
    VTR_LOG_ERROR("Block usage file does not exist: %s\n", block_usage_file_path.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Reading block usage file from: %s\n", block_usage_file_path.c_str());
  /* Parse results */
  num_err += parse_vpr_block_usage_bitgen(
    bm_id, bitgen_file(st_id, bm_id, e_file_type::VPR_BLOCK_USAGE_FILE, corner, verbose), corner,
    verbose);
  std::string subdir_name = bitgen_subdir_name(bm_id, corner);
  num_err += parse_openfpga_stats_bitgen(bm_id, openfpga_runtime_log_file(subdir_name, verbose),
                                         corner, verbose);

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::extract_arch_eval_qor(const std::string& corner, const bool& verbose) {
  int num_err = 0;
  /* Run bitstream generation one by one */
  for (auto st_id : bm_mgr_->suites()) {
    VTR_LOG("Extracting QoR from arch evaluation reports for suite '%s' for corner '%s'...\n",
            bm_mgr_->suite_name(st_id).c_str(), corner.c_str());
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      if (bm_mgr_->is_benchmark_disabled(bm_id)) {
        VTR_LOGV(verbose, "Bypass benchmark '%s' as it is disabled by user\n",
                 bm_mgr_->benchmark_name(bm_id).c_str());
        continue;
      }
      VTR_LOGV(verbose, "Extracting QoR from benchmark '%s' for corner '%s'...\n",
               bm_mgr_->benchmark_name(bm_id).c_str(), corner.c_str());
      num_err += extract_bitgen_qor(st_id, bm_id, corner, verbose);
      VTR_LOGV(verbose, "Done - Extracting QoR from benchmark '%s' for corner '%s'\n",
               bm_mgr_->benchmark_name(bm_id).c_str(), corner.c_str());
    }
    VTR_LOG("Done - Extracting QoR for suite '%s' for corner '%s'\n",
            bm_mgr_->suite_name(st_id).c_str(), corner.c_str());
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::show_arch_eval_qor(const std::string& corner, TablePrinter& tp,
                                        const int& precision, const bool& verbose) {
  int num_err = 0;
  std::string kw_cpd;
  VTR_LOGV(verbose, "Find critical path delay for corner '%s'...\n", corner.c_str());
  bool kw_cpd_status = vpr_log_parser_.find_critical_path_delay_keyword(kw_cpd);
  if (!kw_cpd_status) {
    VTR_LOGV(verbose, "Failed to spot critical path delay keyword for corner '%s'...\n",
             corner.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  bitgen_qor_data_.clear();
  std::string kw_fmax = "Fmax";
  for (auto st_id : bm_mgr_->suites()) {
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      if (bm_mgr_->is_benchmark_disabled(bm_id)) {
        VTR_LOGV(verbose, "Bypass benchmark '%s' as it is disabled by user\n",
                 bm_mgr_->benchmark_name(bm_id).c_str());
        continue;
      }
      float cpd_data = bitgen_openfpga_data_[kw_cpd][bm_id][corner];
      /* Fmax in the MHz unit */
      float fmax = (1.0 / cpd_data) / (1e6);
      bitgen_qor_data_[kw_fmax][bm_id][corner] = fmax;
    }
  }
  /* Header line */
  /* Print data lines */
  for (auto st_id : bm_mgr_->suites()) {
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      /* Column name */
      std::string benchmark_name = bm_mgr_->benchmark_name(bm_id);
      std::string fmax_data =
        to_string_with_precision(bitgen_qor_data_[kw_fmax][bm_id][corner], precision);
      tp.add_row({benchmark_name, fmax_data, corner});
    }
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

ArchEvalManager::e_file_type ArchEvalManager::convert_file_type(
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
    case TapeoutManager::e_arch_file_type::OPENFPGA_CKBUF_LOC_MAP:
      return e_file_type::OPENFPGA_CKBUF_LOC_MAP;
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
    case TapeoutManager::e_arch_file_type::OPENFPGA_BOUNDARY_TIMING:
      return e_file_type::OPENFPGA_BOUNDARY_TIMING;
    case TapeoutManager::e_arch_file_type::OPENFPGA_PCF_CONFIG:
      return e_file_type::OPENFPGA_PCF_CONFIG;
    case TapeoutManager::e_arch_file_type::OPENFPGA_PCF:
      return e_file_type::OPENFPGA_PCF;
    default:
      return e_file_type::NUM_TYPES;
  }
  return e_file_type::NUM_TYPES;
}

bool ArchEvalManager::required_file_type(const TapeoutManager::e_arch_file_type& f_type) const {
  return valid_file_type(convert_file_type(f_type));
}

bool ArchEvalManager::add_file(const ArchEvalManager::e_file_type& f_type,
                               const std::string& f_name) {
  if (!valid_file_type(f_type)) {
    return false;
  }
  switch (f_type) {
    case ArchEvalManager::e_file_type::OPENFPGA_SIM_SETTING:
    case ArchEvalManager::e_file_type::OPENFPGA_CKBUF_MAP:
    case ArchEvalManager::e_file_type::OPENFPGA_BITSTREAM_SETTING:
    case ArchEvalManager::e_file_type::OPENFPGA_TILE_CONFIG:
    case ArchEvalManager::e_file_type::OPENFPGA_PCF_XML:
    case ArchEvalManager::e_file_type::VPR_SDC:
    case ArchEvalManager::e_file_type::OPENFPGA_PCF_CONFIG:
    case ArchEvalManager::e_file_type::OPENFPGA_BOUNDARY_TIMING:
    case ArchEvalManager::e_file_type::OPENFPGA_PCF: {
      src_openfpga_setting_files_[f_type] = f_name;
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid file type!\n");
      return false;
  }
  return true;
}

std::filesystem::path ArchEvalManager::arch_dir() const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_);
  return ret;
}

std::filesystem::path ArchEvalManager::report_dir() const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret = proj_home / std::filesystem::path(VPR_REPORT_DIR_NAME_);
  return ret;
}

std::filesystem::path ArchEvalManager::bitstream_dir() const {
  std::filesystem::path proj_home = bitgen_project_home_dir();
  std::filesystem::path ret = proj_home / std::filesystem::path(BITSTREAM_DIR_NAME_);
  return ret;
}

const BenchmarkManager* ArchEvalManager::benchmark_manager() const { return bm_mgr_; }

std::vector<std::string> ArchEvalManager::corners() const {
  std::vector<std::string> ret;
  for (auto pair : src_arch_timing_files_) {
    ret.push_back(pair.first);
  }
  return ret;
}

void ArchEvalManager::add_boundary_timing_file(const std::string& corner, const std::string& val) {
  src_boundary_timing_files_[corner] = val;
}

void ArchEvalManager::add_arch_timing_file(const std::string& corner, const std::string& val) {
  src_arch_timing_files_[corner] = val;
}

std::vector<std::string> ArchEvalManager::report_sweep_k_csv_header_row() const {
  std::vector<std::string> ret;
  ret.push_back("Benchmark/Metrics");
  ret.push_back("Suite");
  ret.push_back("K");
  for (auto pair : sweep_k_data_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<BenchmarkId> ArchEvalManager::pre_arch_benchmarks() const {
  std::vector<BenchmarkId> ret;
  for (auto st_id : bm_mgr_->suites()) {
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      ret.push_back(bm_id);
    }
  }
  return ret;
}

std::vector<size_t> ArchEvalManager::pre_arch_k_range() const {
  std::vector<size_t> ret;
  for (size_t k = min_k_; k <= max_k_; k++) {
    ret.push_back(k);
  }
  return ret;
}

std::vector<std::string> ArchEvalManager::report_sweep_k_csv_data_row(const BenchmarkId& bm_id,
                                                                      const size_t& k) const {
  std::vector<std::string> ret;
  ret.push_back(bm_mgr_->benchmark_name(bm_id));
  ret.push_back(bm_mgr_->benchmark_suite_name(bm_id));
  ret.push_back(std::to_string(k));
  for (auto pair : sweep_k_data_) {
    ret.push_back(std::to_string(pair.second[bm_id][std::to_string(k)]));
  }
  return ret;
}

std::vector<std::string> ArchEvalManager::report_bitgen_csv_header_row() const {
  std::vector<std::string> ret;
  ret.push_back("Benchmark/Metrics");
  ret.push_back("Suite");
  ret.push_back("Corner");
  for (auto pair : bitgen_frontend_data_) {
    ret.push_back(pair.first);
  }
  for (auto pair : bitgen_openfpga_data_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<BenchmarkId> ArchEvalManager::bitgen_benchmarks() const {
  std::vector<BenchmarkId> ret;
  for (auto st_id : bm_mgr_->suites()) {
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      ret.push_back(bm_id);
    }
  }
  return ret;
}

std::vector<std::string> ArchEvalManager::report_bitgen_csv_data_row(
  const BenchmarkId& bm_id, const std::string& corner) const {
  std::vector<std::string> ret;
  ret.push_back(bm_mgr_->benchmark_name(bm_id));
  ret.push_back(bm_mgr_->benchmark_suite_name(bm_id));
  ret.push_back(corner);
  for (auto pair : bitgen_frontend_data_) {
    ret.push_back(std::to_string(pair.second[bm_id][corner]));
  }
  for (auto pair : bitgen_openfpga_data_) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(12) << pair.second[bm_id][corner];
    ret.push_back(oss.str());
  }
  return ret;
}

std::string ArchEvalManager::bitgen_qor_report_file() const {
  std::filesystem::path ret =
    bitgen_project_home_dir() / std::filesystem::path(BITGEN_QOR_REPORT_FILENAME_);
  return ret.string();
}

std::vector<std::string> ArchEvalManager::bitgen_result_files(const bool& verbose) const {
  std::vector<std::string> ret;
  std::vector<e_file_type> ftypes = {
    e_file_type::OUT_BLIF,           e_file_type::STAT_JSON,
    e_file_type::VPR_NET_FILE,       e_file_type::VPR_PLACE_FILE,
    e_file_type::VPR_ROUTE_FILE,     e_file_type::BITSTREAM_TEXT_FILE,
    e_file_type::BITSTREAM_XML_FILE, e_file_type::BITSTREAM_PIN_MAP_FILE};
  for (auto st_id : bm_mgr_->suites()) {
    for (auto bm_id : bm_mgr_->benchmarks(st_id)) {
      for (auto curr_corner : corners()) {
        for (auto f_type : ftypes) {
          std::string result_file = bitgen_file(st_id, bm_id, f_type, curr_corner, verbose);
          ret.push_back(result_file);
        }
      }
    }
  }
  /* Add QoR report file */
  ret.push_back(bitgen_qor_report_file());
  return ret;
}

std::string ArchEvalManager::bitgen_md5_yaml_file() const {
  std::filesystem::path ret = bitgen_project_home_dir() / std::filesystem::path(MD5_FILENAME_);
  return ret.string();
}

int ArchEvalManager::save_bitgen_results(const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  FileMd5Manager md5_mgr;
  for (auto fpath : bitgen_result_files(verbose)) {
    /* TODO: Need to find a way how to identify which corner has been covered by bitgen!!! */
    if (!std::filesystem::exists(fpath)) {
      continue;
    }
    md5_mgr.add_file(fpath);
  }
  status = md5_mgr.gen_md5(verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Md5 generation failed when saving results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = md5_mgr.write_md5_to_yaml_file(bitgen_md5_yaml_file(), verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Export Md5 to files failed when saving results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return status;
}

int ArchEvalManager::check_bitgen_results(const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Check if the md5 file exists */
  if (!std::filesystem::exists(bitgen_md5_yaml_file())) {
    VTR_LOG_ERROR("Required Md5 file does not exist! Unable to check results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  FileMd5Manager md5_mgr;
  status = md5_mgr.load_md5_from_yaml_file(bitgen_md5_yaml_file(), verbose);
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

int ArchEvalManager::report_bitgen_qor_to_csv_file(const std::string& fname,
                                                   const std::vector<std::string>& corners) const {
  rapidcsv::Document csvFile;

  size_t row_id = 0;
  std::vector<std::string> header_row = report_bitgen_csv_header_row();
  csvFile.SetRow(row_id, header_row);
  row_id++;

  for (const BenchmarkId& bm_id : bitgen_benchmarks()) {
    if (bm_mgr_->is_benchmark_disabled(bm_id)) {
      continue;
    }
    for (std::string corner : corners) {
      std::vector<std::string> data_row = report_bitgen_csv_data_row(bm_id, corner);
      csvFile.SetRow(row_id, data_row);
      row_id++;
    }
  }

  std::filesystem::path rpt_fpath(fname);
  openfpga::create_directory(rpt_fpath.parent_path().string(), true, false);
  csvFile.Save(fname);
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::report_pre_arch_qor_to_csv_file(const std::string& fname) const {
  rapidcsv::Document csvFile;

  size_t row_id = 0;
  std::vector<std::string> header_row = report_sweep_k_csv_header_row();
  csvFile.SetRow(row_id, header_row);
  row_id++;

  for (const BenchmarkId& bm_id : pre_arch_benchmarks()) {
    if (bm_mgr_->is_benchmark_disabled(bm_id)) {
      continue;
    }
    for (size_t k : pre_arch_k_range()) {
      std::vector<std::string> data_row = report_sweep_k_csv_data_row(bm_id, k);
      csvFile.SetRow(row_id, data_row);
      row_id++;
    }
  }

  std::filesystem::path rpt_fpath(fname);
  openfpga::create_directory(rpt_fpath.parent_path().string(), true, false);
  csvFile.Save(fname);
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchEvalManager::gen_builtin_report_bitgen_qor(const std::vector<std::string>& corners) const {
  return report_bitgen_qor_to_csv_file(bitgen_qor_report_file(), corners);
}

int ArchEvalManager::gen_report_bitgen_qor(const std::string& ext_fname,
                                           const std::vector<std::string>& corners,
                                           const bool& verbose) const {
  size_t num_err = 0;
  int status = gen_builtin_report_bitgen_qor(corners);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  num_err += copy_file_with_mkdir(bitgen_qor_report_file(), ext_fname, false, verbose);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arcxpress
