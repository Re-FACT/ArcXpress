#include "dv_manager.h"

#include <yaml-cpp/yaml.h>

#include "arch_aggregator.h"
#include "arkangel_constants.h"
#include "arkangel_file_os.h"
#include "arkangel_sys_env.h"
#include "cocotb_makefile_utils.h"
#include "command_exit_codes.h"
#include "file_md5_manager.h"
#include "openfpga_digest.h"
#include "verilog_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

DvManager::DvManager(const TapeoutManager& to_mgr, const std::string& run_name,
                     const ArchEvalManager& ae_mgr, const NetlistDevManager& nd_mgr) {
  init(to_mgr.path(), to_mgr.name(), to_mgr.family(), run_name, ae_mgr, nd_mgr,
       to_mgr.openfpga_error_conf_file(), to_mgr.frontend_cell_lib_path(), to_mgr.python_bin_path(),
       to_mgr.python_site_package_path());
}

void DvManager::init(const std::string& root_path, const std::string& proj_name,
                     const std::string& family_name, const std::string& run_name,
                     const ArchEvalManager& ae_mgr, const NetlistDevManager& nd_mgr,
                     const std::string& openfpga_error_conf_file, const std::string& cell_lib_path,
                     const std::string& py_bin_path, const std::string& py_site_packages_path) {
  root_path_ = root_path;
  proj_name_ = proj_name;
  family_name_ = family_name;
  run_name_ = run_name;
  is_valid_ = false;
  ae_mgr_ = &ae_mgr;
  nd_mgr_ = &nd_mgr;
  cell_lib_path_ = cell_lib_path;

  PYTHON_COMMAND_ = py_bin_path;
  PYTHON_SITE_PACKAGES_PATH_ = py_site_packages_path;

  VPR_BLIF_VAR_NAME_ = "VPR_TESTBENCH_BLIF";
  VPR_FABRIC_VAR_NAME_ = "OPENFPGA_VPR_DEVICE_LAYOUT";
  VPR_ROUTE_CHAN_W_VAR_NAME_ = "OPENFPGA_VPR_ROUTE_CHAN_WIDTH";
  OPENFPGA_PYTHON_EXEC_VAR_NAME_ = "PYTHON_EXEC";
  OPENFPGA_BUS_GROUP_GEN_EXEC_VAR_NAME_ = "BUS_GROUP_GEN_BIN";

  SCRIPT_TYPE_STRING_ = {"preconfig_tb_gen", "full_tb_gen"};
  FILE_NAMES_ = {"post_synth.blif",
                 "post_synth_rewrite.v",
                 "post_synth.v",
                 "vpr_arch.xml",
                 "openfpga_arch.xml",
                 "openfpga_global_network.xml",
                 "design_variables.yml", /* Placeholder only */
                 "openfpga_sim_setting.xml",
                 "openfpga_bitstream_setting.xml",
                 "tile_config.xml",
                 "gclk_pcf.xml",
                 "vpr_dummy.sdc",
                 "pcf_dummy.pcf",
                 "final_gclk_pcf.xml",
                 "ckbuf_loc_map.xml",
                 "final_gclk_pcf4dv.xml",
                 "fabric_key.xml",
                 "module_names.xml",
                 "rr_graph.bin",
                 "placement_delay_lookup.bin",
                 "router_lookahead.bin",
                 "vpr.net",
                 "vpr.place",
                 "vpr.route",
                 "base_bitstream.xml",
                 "fabric_bitstream.bit",
                 "fabric_bitstream.xml",
                 "PinMapping.xml",
                 "preconfig_tb.v",
                 "full_tb.v",
                 "bus_group.xml",
                 "bgg_task.yaml",
                 "arch_timing_data_file",
                 "cocotb_result_waveform_fsdb_file"};
  VAR_NAMES_ = {"BLIF_FILE",
                "REFERENCE_VERILOG_REWRITE",
                "REFERENCE_VERILOG",
                "VPR_ARCH_FILE",
                "OPENFPGA_ARCH_FILE",
                "OPENFPGA_CLOCK_ARCH_FILE",
                "DESIGN_VAR_YAML", /* Placeholder only */
                "OPENFPGA_SIM_SETTING_FILE",
                "OPENFPGA_BITSTREAM_SETTING_FILE",
                "OPENFPGA_TILE_CONFIG_FILE",
                "OPENFPGA_PIN_CONSTRAINTS_FILE",
                "OPENFPGA_VPR_SDC",
                "OPENFPGA_PCF",
                "OPENFPGA_FINAL_VPR_PIN_CONSTRAINTS_FILE",
                "OPENFPGA_CKBUF_LOC_MAP_FILE",
                "OPENFPGA_FINAL_PIN_CONSTRAINTS_FILE",
                "EXTERNAL_FABRIC_KEY_FILE",
                "OPENFPGA_RENAME_MODULE_FILE",
                "OPENFPGA_VPR_RR_GRAPH_FILE",
                "OPENFPGA_VPR_PLACEMENT_DELAY_LOOKUP_FILE",
                "OPENFPGA_VPR_ROUTER_LOOKAHEAD_FILE",
                "VPR_NET_FILE",
                "VPR_PLACE_FILE",
                "VPR_ROUTE_FILE",
                "OPENFPGA_BASE_BITSTREAM_XML_FILE",
                "OPENFPGA_BITSTREAM_BIT_FILE",
                "OPENFPGA_BITSTREAM_XML_FILE",
                "OPENFPGA_BITSTREAM_PIN_MAP_FILE",
                "OPENFPGA_PRECONFIG_TESTBENCH_DIR",
                "OPENFPGA_FULL_TESTBENCH_DIR",
                "OPENFPGA_BUS_GROUP_FILE",
                "OPENFPGA_BUS_GROUP_TASK_YAML",
                "arch_timing_data_file",
                "COCOTB_RESULT_WAVEFORM_FSDB"};

  NETLIST_TYPE_STRING_ = {"rtl", "gl", "pl"};
  COCOTB_FILE_TYPE_STRING_ = {"cocotb_testbench", "cocotb_makefile", "bitstream_base",
                              "bitstream_value", "cocotb_util"};
  /* Must be consistent with the cocotb utils */
  SIMULATOR_TYPE_STRING_ = {"synopsys_vcs", "mentor_modelsim", "mentor_questa"};
  WAVEFORM_TYPE_STRING_ = {"none", "fsdb", "vcd"};

  DV_SUBDIR_NAME_ = "dv";
  ARCH_DIR_NAME_ = "arch";
  NETLIST_DIR_NAME_ = "netlist";
  DV_DIR_NAME_ = "dv";
  RPT_DIR_NAME_ = "reports";
  RPT_TIMING_DIR_NAME_ = "benchmark_timing";
  RPT_POWER_DIR_NAME_ = "benchmark_power";
  RPT_SUMMARY_DIR_NAME_ = "summary";
  PRECONFIG_TB_DIR_NAME_ = "preconfig_testbench";
  PRECONFIG_TB_TASK_DIR_NAME_ = "tasks";
  FULL_TB_DIR_NAME_ = "full_testbench";
  OPENFPGA_RUNDIR_NAME_ = "openfpga_tasks";

  OPENFPGA_LOG_FILENAME_ = "openfpga.log";
  OPENFPGA_SCRIPT_FILENAME_ = "run.openfpga";
  MD5_FILENAME_ = "results_md5.yaml";
  COCOTB_TASK_FILENAME_POSTFIX_ = "_task.yaml";
  COCOTB_BASE_BITSTREAM_XML_FILENAME_ = "fabric_bitstream_base.xml";
  ARCH_TIMING_FILE_PREFIX_ = "arch_timing_";
  ARCH_TIMING_FILE_POSTFIX_ = ".yaml";

  if (openfpga::CMD_EXEC_SUCCESS !=
      openfpga_log_checker_.load_keywords_from_xml(openfpga_error_conf_file, false)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::vector<DvManager::e_netlist_type> DvManager::netlist_types() const {
  return {DvManager::e_netlist_type::RTL, DvManager::e_netlist_type::GL,
          DvManager::e_netlist_type::PL};
}

std::filesystem::path DvManager::project_home_dir() const {
  std::filesystem::path proj_home = std::filesystem::path(root_path_);
  std::filesystem::path subdir =
    std::string(proj_name_) + std::string("_") + std::string(DV_SUBDIR_NAME_);
  proj_home = proj_home / subdir / std::filesystem::path(run_name_);
  return proj_home;
}

std::string DvManager::file(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                            const e_file_type& f_type, const std::string& corner,
                            const bool& verbose) const {
  check_valid_file_type(f_type);
  std::filesystem::path proj_home = project_home_dir();
  std::filesystem::path ret;
  switch (f_type) {
    case e_file_type::VPR_ARCH:
    case e_file_type::OPENFPGA_ARCH:
    case e_file_type::OPENFPGA_GNTWK_ARCH:
    case e_file_type::DESIGN_VAR:
    case e_file_type::FINAL_FABRIC_KEY:
    case e_file_type::CRAFTED_MODULE_NAME:
    case e_file_type::OPENFPGA_BITSTREAM_SETTING:
    case e_file_type::OPENFPGA_CKBUF_LOC_MAP:
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::RRG:
    case e_file_type::PDL:
    case e_file_type::RLA:
      /* Device data that contains timing data are corner dependent */
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_) /
            std::filesystem::path(corner + "_" + FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::OPENFPGA_SIM_SETTING:
    case e_file_type::OPENFPGA_TILE_CONFIG:
    case e_file_type::OPENFPGA_PCF_XML:
    case e_file_type::VPR_SDC:
    case e_file_type::OPENFPGA_PCF:
    case e_file_type::BASE_BITSTREAM_XML_FILE:
      ret = std::filesystem::path(src_openfpga_setting_files_.at(f_type));
      break;
    case e_file_type::OUT_BLIF:
    case e_file_type::OUT_VERILOG_REWRITE:
    case e_file_type::OUT_VERILOG:
    case e_file_type::VPR_NET_FILE:
    case e_file_type::VPR_PLACE_FILE:
    case e_file_type::VPR_ROUTE_FILE:
    case e_file_type::OPENFPGA_FINAL_PCF_XML:
    case e_file_type::OPENFPGA_FINAL_PCF_XML4DV:
      ret = proj_home /
            std::filesystem::path(ae_mgr_->vpr_report_output_dir(st_id, bm_id, corner)) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::BITSTREAM_TEXT_FILE:
    case e_file_type::BITSTREAM_XML_FILE:
    case e_file_type::BITSTREAM_PIN_MAP_FILE:
      ret = dv_dir() / std::filesystem::path(ae_mgr_->bitstream_output_dir(st_id, bm_id, corner)) /
            std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    /* Preconfigure testbench file depends on openfpga's naming rules */
    case e_file_type::PRECONFIG_TESTBENCH_FILE: {
      std::string preconfig_tb_fname = ae_mgr_->benchmark_manager()->top_module(bm_id) +
                                       std::string(FORMAL_VERIFICATION_VERILOG_FILE_POSTFIX);
      ret = preconfig_sim_runtime_dir() /
            std::filesystem::path(output_subdir(st_id, bm_id, corner)) /
            std::filesystem::path(preconfig_tb_fname);
      break;
    }
    /* Full testbench file depends on openfpga's naming rule */
    case e_file_type::FULL_TESTBENCH_FILE: {
      std::string full_tb_fname = ae_mgr_->benchmark_manager()->top_module(bm_id) +
                                  std::string(AUTOCHECK_TOP_TESTBENCH_VERILOG_FILE_POSTFIX);
      ret = full_sim_runtime_dir() / std::filesystem::path(output_subdir(st_id, bm_id, corner)) /
            std::filesystem::path(full_tb_fname);
      break;
    }
    /* Bus group file from benchmark */
    case e_file_type::BUS_GROUP_FILE:
      ret = std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::BUS_GROUP_TASK_FILE:
      ret = std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
      break;
    case e_file_type::ARCH_TIMING: {
      ret = proj_home / std::filesystem::path(ARCH_DIR_NAME_);
      std::string l_fname =
        std::string(ARCH_TIMING_FILE_PREFIX_) + corner + std::string(ARCH_TIMING_FILE_POSTFIX_);
      ret = ret / std::filesystem::path(l_fname);
      break;
    }
    case e_file_type::WAVEFORM_FSDB: {
      std::string file_name = ae_mgr_->benchmark_manager()->benchmark_name(bm_id) +
                              std::string(".") + waveform_type2str(e_waveform_type::FSDB);
      ret = cocotb_runtime_file(st_id, bm_id, corner, file_name);
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid file type!\n");
  }
  return ret.string();
}

std::string DvManager::openfpga_runtime_dir(const e_script_type& runtime_type,
                                            const BenchmarkId& bm_id, const std::string& corner,
                                            const bool& verbose) const {
  std::filesystem::path proj_home = project_home_dir();
  std::filesystem::path ret =
    proj_home / std::filesystem::path(OPENFPGA_RUNDIR_NAME_) /
    std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
    std::filesystem::path(ae_mgr_->benchmark_manager()->benchmark_name(bm_id)) /
    std::filesystem::path(corner);
  return ret.string();
}

std::string DvManager::openfpga_runtime_file(const DvManager::e_file_type& f_type,
                                             const e_script_type& runtime_type,
                                             const BenchmarkId& bm_id, const std::string& corner,
                                             const bool& verbose) const {
  check_valid_file_type(f_type);
  if (f_type == e_file_type::VPR_ARCH || f_type == e_file_type::OPENFPGA_ARCH) {
    std::filesystem::path ret = openfpga_runtime_dir(runtime_type, bm_id, corner, verbose);
    ret = ret / std::filesystem::path(FILE_NAMES_[size_t(f_type)]);
    return ret.string();
  }
  return "";
}

std::filesystem::path DvManager::dv_dir() const {
  std::filesystem::path proj_home = project_home_dir();
  return proj_home / std::filesystem::path(DV_DIR_NAME_);
}

std::filesystem::path DvManager::netlist_dir(const e_netlist_type& nlist_type) const {
  std::filesystem::path proj_home = project_home_dir();
  return proj_home / std::filesystem::path(NETLIST_DIR_NAME_) /
         std::filesystem::path(NETLIST_TYPE_STRING_[size_t(nlist_type)]);
}

std::filesystem::path DvManager::output_subdir(const BenchmarkSuiteId& st_id,
                                               const BenchmarkId& bm_id,
                                               const std::string& corner) const {
  std::filesystem::path ret;
  ret = std::filesystem::path(ae_mgr_->benchmark_manager()->suite_name(st_id)) /
        std::filesystem::path(ae_mgr_->benchmark_manager()->benchmark_name(bm_id)) /
        std::filesystem::path(corner);
  return ret;
}

std::filesystem::path DvManager::preconfig_sim_runtime_dir() const {
  return dv_dir() / std::filesystem::path(PRECONFIG_TB_DIR_NAME_);
}

std::filesystem::path DvManager::full_sim_runtime_dir() const {
  return dv_dir() / std::filesystem::path(FULL_TB_DIR_NAME_) / ae_mgr_->bitstream_dir().filename();
}

std::filesystem::path DvManager::bitstream_dir() const {
  return dv_dir() / ae_mgr_->bitstream_dir().filename();
}

std::filesystem::path DvManager::cocotb_task_file(const BenchmarkSuiteId& st_id,
                                                  const std::string& corner) const {
  std::string fname = corner + "_" + ae_mgr_->benchmark_manager()->suite_name(st_id) +
                      std::string(COCOTB_TASK_FILENAME_POSTFIX_);
  return preconfig_sim_runtime_dir() / std::filesystem::path(PRECONFIG_TB_TASK_DIR_NAME_) /
         std::filesystem::path(fname);
}

std::string DvManager::cocotb_runtime_netlist_dir(const BenchmarkSuiteId& st_id,
                                                  const BenchmarkId& bm_id,
                                                  const std::string& corner,
                                                  const e_netlist_type& nlist_type) const {
  std::filesystem::path ret = preconfig_sim_runtime_dir() / output_subdir(st_id, bm_id, corner) /
                              std::filesystem::path(netlist_type2str(nlist_type));
  return ret.string();
}

std::string DvManager::cocotb_runtime_file(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                           const std::string& corner,
                                           const std::filesystem::path& fname) const {
  std::filesystem::path ret =
    preconfig_sim_runtime_dir() / output_subdir(st_id, bm_id, corner) / fname;
  return ret.string();
}

int DvManager::init_dir(const bool& verbose) const {
  int num_err = 0;
  std::filesystem::path proj_home = project_home_dir();
  /* Link arch files */
  std::filesystem::path src_arch_dir = ae_mgr_->arch_dir();
  std::filesystem::path des_arch_dir = proj_home / std::filesystem::path(ARCH_DIR_NAME_);
  num_err += copy_file_with_mkdir(src_arch_dir.string(), des_arch_dir.string(), true, verbose);

  /* Link report directory */
  std::filesystem::path src_rpt_dir = ae_mgr_->report_dir();
  std::filesystem::path des_rpt_dir = proj_home / ae_mgr_->report_dir().filename();
  num_err += copy_file_with_mkdir(src_rpt_dir.string(), des_rpt_dir.string(), true, verbose);

  /* Link bitstream directory */
  openfpga::create_directory(dv_dir().string(), true, verbose);
  std::filesystem::path src_bitstream_dir = ae_mgr_->bitstream_dir();
  std::filesystem::path des_bitstream_dir = bitstream_dir();
  num_err +=
    copy_file_with_mkdir(src_bitstream_dir.string(), des_bitstream_dir.string(), true, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::copy_file_with_mkdir(const std::string& src_file, const std::string& des_file,
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

int DvManager::generate_preconfig_testbench(const std::string& openfpga_bin_path,
                                            const std::string& bgg_bin_path,
                                            const std::string& corner, const bool& verbose) {
  int num_err = 0;
  for (auto st_id : ae_mgr_->benchmark_manager()->suites()) {
    VTR_LOG("Running bitstream generation for suite '%s'...\n",
            ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    for (auto bm_id : ae_mgr_->benchmark_manager()->benchmarks(st_id)) {
      if (ae_mgr_->benchmark_manager()->is_benchmark_disabled(bm_id)) {
        VTR_LOGV(verbose, "Bypass benchmark '%s' as it is disabled by user\n",
                 ae_mgr_->benchmark_manager()->benchmark_name(bm_id).c_str());
        continue;
      }
      num_err +=
        run_preconfig_tb_gen(st_id, bm_id, openfpga_bin_path, bgg_bin_path, corner, verbose);
    }
    VTR_LOG("Done\n");
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::init_runtime_dir(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                const e_script_type& runtime_type, const std::string& corner,
                                const bool& verbose) const {
  int num_err = 0;

  ArchAggregator arch_agg(file(st_id, bm_id, e_file_type::DESIGN_VAR, corner, verbose), verbose);
  arch_agg.load_variables(file(st_id, bm_id, e_file_type::ARCH_TIMING, corner, verbose), false,
                          false, verbose);
  /* Generate the final vpr and openfpga arch file */
  std::string input_vpr_arch = file(st_id, bm_id, e_file_type::VPR_ARCH, corner, verbose);
  std::string runtime_vpr_arch =
    openfpga_runtime_file(e_file_type::VPR_ARCH, runtime_type, bm_id, corner, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_vpr_dirpath = runtime_vpr_arch;
  openfpga::create_directory(runtime_vpr_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_vpr_arch, runtime_vpr_arch, verbose);

  /* Generate the final vpr and openfpga arch file */
  std::string input_openfpga_arch = file(st_id, bm_id, e_file_type::OPENFPGA_ARCH, corner, verbose);
  std::string runtime_openfpga_arch =
    openfpga_runtime_file(e_file_type::OPENFPGA_ARCH, runtime_type, bm_id, corner, verbose);
  /* Create directory for des */
  std::filesystem::path runtime_openfpga_dirpath = runtime_openfpga_arch;
  openfpga::create_directory(runtime_openfpga_dirpath.parent_path().string(), true, verbose);
  num_err += arch_agg.aggregate(input_openfpga_arch, runtime_openfpga_arch, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

void DvManager::set_route_chan_width(const size_t& val) { route_chan_width_ = val; }
void DvManager::set_fabric_name(const std::string& val) { fabric_name_ = val; }

std::string DvManager::openfpga_script_file(const e_script_type& runtime_type) const {
  return std::filesystem::path(src_script_files_[size_t(runtime_type)]).string();
}

std::string DvManager::openfpga_runtime_script_file(const BenchmarkId& bm_id,
                                                    const e_script_type& runtime_type,
                                                    const std::string& corner,
                                                    const bool& verbose) const {
  std::filesystem::path proj_home = project_home_dir();
  std::filesystem::path ret =
    proj_home / std::filesystem::path(OPENFPGA_RUNDIR_NAME_) /
    std::filesystem::path(SCRIPT_TYPE_STRING_[size_t(runtime_type)]) /
    std::filesystem::path(ae_mgr_->benchmark_manager()->benchmark_name(bm_id)) /
    std::filesystem::path(corner) / std::filesystem::path(OPENFPGA_SCRIPT_FILENAME_);

  return ret.string();
}

int DvManager::init_runtime_script(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                   const e_script_type& runtime_type, const std::string& bgg_bin,
                                   const std::string& corner, const bool& verbose) const {
  int num_err = 0;
  /* Generate the final openfpga script file */
  ArchAggregator script_agg;
  /* Add variables. */
  for (size_t itype = 0; itype < size_t(e_file_type::NUM_TYPES); ++itype) {
    e_file_type curr_type = static_cast<e_file_type>(itype);
    if (curr_type == e_file_type::VPR_ARCH || curr_type == e_file_type::OPENFPGA_ARCH) {
      std::string rt_file = openfpga_runtime_file(curr_type, runtime_type, bm_id, corner, verbose);
      script_agg.add_variable(VAR_NAMES_[itype], rt_file);
      continue;
    }
    if (curr_type == e_file_type::DESIGN_VAR) {
      continue;
    }
    script_agg.add_variable(VAR_NAMES_[itype], file(st_id, bm_id, curr_type, corner, verbose));
  }
  /* Add other variables, which are related to flow control. i.e., layout, channel width etc. */
  script_agg.add_variable(VPR_FABRIC_VAR_NAME_, fabric_name_);
  script_agg.add_variable(VPR_ROUTE_CHAN_W_VAR_NAME_, route_chan_width_);
  /* Add benchmark info */
  script_agg.add_variable(VPR_BLIF_VAR_NAME_,
                          file(st_id, bm_id, e_file_type::OUT_BLIF, corner, verbose));
  for (auto vpr_file_type :
       {e_file_type::VPR_NET_FILE, e_file_type::VPR_PLACE_FILE, e_file_type::VPR_ROUTE_FILE}) {
    script_agg.add_variable(VAR_NAMES_[size_t(vpr_file_type)],
                            file(st_id, bm_id, vpr_file_type, corner, verbose));
  }
  for (auto f_id : ae_mgr_->benchmark_manager()->constraints_by_type(
         bm_id, BenchmarkManager::e_file_type::SDC)) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::VPR_SDC)],
                            ae_mgr_->benchmark_manager()->constraint_file(bm_id, f_id));
  }
  for (auto f_id : ae_mgr_->benchmark_manager()->constraints_by_type(
         bm_id, BenchmarkManager::e_file_type::PCF_GCLK)) {
    script_agg.add_variable(VAR_NAMES_[size_t(e_file_type::OPENFPGA_PCF_XML)],
                            ae_mgr_->benchmark_manager()->constraint_file(bm_id, f_id));
  }
  /* Use the final PCF generated during arch eval step */
  for (auto runtime_pcf_file_type : {e_file_type::OPENFPGA_FINAL_PCF_XML4DV}) {
    script_agg.add_variable(VAR_NAMES_[size_t(runtime_pcf_file_type)],
                            file(st_id, bm_id, runtime_pcf_file_type, corner, verbose));
  }
  /* Special for preconfig/full tesbtenches, use a dirpath other than filename. This is restricted
   * by OpenFPGA */
  for (auto tb_file_type :
       {e_file_type::PRECONFIG_TESTBENCH_FILE, e_file_type::FULL_TESTBENCH_FILE}) {
    std::filesystem::path fdir =
      std::filesystem::path(file(st_id, bm_id, tb_file_type, corner, verbose)).parent_path();
    script_agg.add_variable(VAR_NAMES_[size_t(tb_file_type)], fdir.string());
  }
  /* Bus group file related */
  script_agg.add_variable(OPENFPGA_PYTHON_EXEC_VAR_NAME_, PYTHON_COMMAND_);
  script_agg.add_variable(OPENFPGA_BUS_GROUP_GEN_EXEC_VAR_NAME_, bgg_bin);

  /* Do aggregation here */
  std::string input_openfpga_script = openfpga_script_file(runtime_type);
  std::string runtime_openfpga_script =
    openfpga_runtime_script_file(bm_id, runtime_type, corner, verbose);
  num_err += script_agg.aggregate(input_openfpga_script, runtime_openfpga_script, verbose);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::write_bus_group_runtime_task_yaml(const BenchmarkSuiteId& st_id,
                                                 const BenchmarkId& bm_id,
                                                 const e_script_type& runtime_type,
                                                 const std::string& corner,
                                                 const bool& verbose) const {
  int num_err = 0;

  YAML::Node config;
  std::string root_tag = "task";
  std::string src_tag = "source";
  std::string src_name_tag = "name";
  std::string tm_tag = "top_module";
  std::string bgf_tag = "bus_group_file";
  /* Add post-synth verilog after rewrite */
  YAML::Node src_node;
  src_node[src_name_tag] = file(st_id, bm_id, e_file_type::OUT_VERILOG_REWRITE, corner, verbose);
  config[root_tag][src_tag].push_back(src_node);
  /* Add top module */
  config[root_tag][tm_tag] = ae_mgr_->benchmark_manager()->top_module(bm_id);
  /* Add output bus_group file */
  config[root_tag][bgf_tag] = file(st_id, bm_id, e_file_type::BUS_GROUP_FILE, corner, verbose);

  /* YAML writer */
  std::filesystem::path runtime_task_fpath =
    openfpga_runtime_script_file(bm_id, runtime_type, corner, verbose);
  runtime_task_fpath =
    runtime_task_fpath.parent_path() /
    std::filesystem::path(file(st_id, bm_id, e_file_type::BUS_GROUP_TASK_FILE, corner, verbose));
  openfpga::create_directory(runtime_task_fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(runtime_task_fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Bus group task data written to '%s'\n", runtime_task_fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", runtime_task_fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::runtime_exec_openfpga(const BenchmarkId& bm_id, const e_script_type& runtime_type,
                                     const std::string& openfpga_bin_path,
                                     const std::string& corner, const bool& verbose) {
  int num_err = 0;
  std::string runtime_openfpga_script =
    openfpga_runtime_script_file(bm_id, runtime_type, corner, verbose);
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

int DvManager::check_result_files(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                  const std::vector<e_file_type>& f_types_to_check,
                                  const std::string& corner, const bool& verbose) const {
  int num_err = 0;
  for (e_file_type f_type_to_check : f_types_to_check) {
    std::string chk_fpath = file(st_id, bm_id, f_type_to_check, corner, verbose);
    if (!std::filesystem::exists(chk_fpath)) {
      VTR_LOG_ERROR("Expected resulting file %s is not generated:\n\t%s\n",
                    FILE_NAMES_[size_t(f_type_to_check)].c_str(), chk_fpath.c_str());
      num_err++;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::run_preconfig_tb_gen(const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                    const std::string& openfpga_bin_path,
                                    const std::string& bgg_bin_path, const std::string& corner,
                                    const bool& verbose) {
  int num_err = 0;
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  e_script_type runtime_type = e_script_type::OPENFPGA_SCRIPT_PRECONFIGTB;

  /* Generate architecture runtime files */
  status = init_runtime_dir(st_id, bm_id, runtime_type, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Generate openfpga runtime scripts */
  status = init_runtime_script(st_id, bm_id, runtime_type, bgg_bin_path, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Generate bus group task file */
  status = write_bus_group_runtime_task_yaml(st_id, bm_id, runtime_type, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Run OpenFPGA */
  status = runtime_exec_openfpga(bm_id, runtime_type, openfpga_bin_path, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Check result files */
  std::vector<e_file_type> of_f_types_to_check = {e_file_type::PRECONFIG_TESTBENCH_FILE};
  status = check_result_files(st_id, bm_id, of_f_types_to_check, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::generate_testbench(const std::string& openfpga_bin_path,
                                  const std::string& bgg_bin_path, const std::string& corner,
                                  const bool& verbose) {
  /* Initializing directory */
  VTR_LOG("Initializing design verification workspace...\n");
  init_dir(verbose);
  VTR_LOG("Done\n");

  /* generate preconfig testbenches */
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  VTR_LOG("Generating preconfigured testbenches");
  status = generate_preconfig_testbench(openfpga_bin_path, bgg_bin_path, corner, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* TODO: generate full testbenches */

  return openfpga::CMD_EXEC_SUCCESS;
}

bool DvManager::add_openfpga_script(const DvManager::e_script_type& f_type,
                                    const std::string& f_name) {
  if (!valid_script_type(f_type)) {
    return false;
  }
  src_script_files_[size_t(f_type)] = f_name;
  return true;
}

bool DvManager::add_yosys_script(const DvManager::e_script_type& f_type,
                                 const std::string& f_name) {
  if (!valid_script_type(f_type)) {
    return false;
  }
  src_script_files_[size_t(f_type)] = f_name;
  return true;
}

bool DvManager::add_file(const DvManager::e_file_type& f_type, const std::string& f_name) {
  if (!valid_file_type(f_type)) {
    return false;
  }
  switch (f_type) {
    case DvManager::e_file_type::OPENFPGA_SIM_SETTING:
    case DvManager::e_file_type::OPENFPGA_BITSTREAM_SETTING:
    case DvManager::e_file_type::OPENFPGA_TILE_CONFIG:
    case DvManager::e_file_type::OPENFPGA_PCF_XML:
    case DvManager::e_file_type::VPR_SDC:
    case DvManager::e_file_type::BASE_BITSTREAM_XML_FILE:
    case DvManager::e_file_type::OPENFPGA_PCF: {
      src_openfpga_setting_files_[f_type] = f_name;
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid file type!\n");
      return false;
  }
  return true;
}

bool DvManager::required_script_type(const TapeoutManager::e_script_type& f_type) const {
  return valid_script_type(convert_script_type(f_type));
}

DvManager::e_script_type DvManager::convert_script_type(
  const TapeoutManager::e_script_type& to_f_type) const {
  switch (to_f_type) {
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_PRECONFIGTB:
      return e_script_type::OPENFPGA_SCRIPT_PRECONFIGTB;
    case TapeoutManager::e_script_type::OPENFPGA_SCRIPT_FULLTB:
      return e_script_type::OPENFPGA_SCRIPT_FULLTB;
    case TapeoutManager::e_script_type::YOSYS_SCRIPT_REWRITE:
      return e_script_type::YOSYS_SCRIPT_REWRITE;
    default:
      return e_script_type::NUM_TYPES;
  }
  return e_script_type::NUM_TYPES;
}

bool DvManager::required_file_type(const TapeoutManager::e_arch_file_type& f_type) const {
  return valid_file_type(convert_file_type(f_type));
}

bool DvManager::valid_script_type(const DvManager::e_script_type& f_type) const {
  return f_type != DvManager::e_script_type::NUM_TYPES;
}

bool DvManager::valid_file_type(const DvManager::e_file_type& f_type) const {
  return f_type != DvManager::e_file_type::NUM_TYPES;
}

void DvManager::check_valid_file_type(const DvManager::e_file_type& f_type) const {
  if (!valid_file_type(f_type)) {
    VTR_LOG_ERROR("Invalid file type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

DvManager::e_file_type DvManager::convert_file_type(
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

int DvManager::write_cocotb_task_file(const BenchmarkSuiteId& st_id,
                                      const std::string& cocotb_force_bitstrm_bin_path,
                                      const std::string& corner, size_t& cnt,
                                      const bool& verbose) const {
  int num_err = 0;

  /* YAML constants */
  std::string group_tag = "group";
  std::string fabric_tag = "fabric";
  std::string testcase_tag = "testcases";
  std::string tc_name_tag = "name";
  std::string tc_files_tag = "files";
  std::string tc_file_name_tag = "name";
  std::string tc_file_type_tag = "type";
  std::string tc_file_src_tag = "src";
  std::string tc_file_des_tag = "des";
  std::string tc_mkfname = "makefile";
  std::string tc_cocotb_tb_name = "testbench";
  std::string tc_bitstream_base_fname = "bitstream_base";
  std::string tc_bitstream_val_fname = "bitstream_value";
  std::string tc_cocotb_util_fname = "cocotb_force";

  /* Create YAML data */
  YAML::Node config;
  config[group_tag] = ae_mgr_->benchmark_manager()->suite_name(st_id);
  config[fabric_tag] = fabric_name_;
  /* Initialize */
  cnt = 0;
  for (auto bm_id : ae_mgr_->benchmark_manager()->benchmarks(st_id)) {
    if (ae_mgr_->benchmark_manager()->is_benchmark_disabled(bm_id)) {
      VTR_LOGV(verbose, "Bypass benchmark '%s' as it is disabled by user\n",
               ae_mgr_->benchmark_manager()->benchmark_name(bm_id).c_str());
      continue;
    }
    /* For each benchmark, a dedicated node tree is requird */
    YAML::Node tc_node;
    tc_node[tc_name_tag] = ae_mgr_->benchmark_manager()->benchmark_name(bm_id);
    /* Add all required files */
    /* RTL, GL, PL netlists */
    for (e_netlist_type nlist_type : netlist_types()) {
      YAML::Node tc_fnode;
      tc_fnode[tc_file_name_tag] = netlist_type2str(nlist_type);
      tc_fnode[tc_file_type_tag] = netlist_type2str(nlist_type);
      tc_fnode[tc_file_src_tag] = netlist_dir(nlist_type).string();
      tc_fnode[tc_file_des_tag] = cocotb_runtime_netlist_dir(st_id, bm_id, corner, nlist_type);
      tc_node[tc_files_tag].push_back(tc_fnode);
    }
    /* cocotb makefile */
    {
      YAML::Node tc_fnode;
      tc_fnode[tc_file_name_tag] = tc_mkfname;
      tc_fnode[tc_file_type_tag] = cocotb_file_type2str(e_cocotb_file_type::MAKEFILE);
      std::string src_fname = ae_mgr_->benchmark_manager()->cocotb_makefile(bm_id);
      tc_fnode[tc_file_src_tag] = src_fname;
      tc_fnode[tc_file_des_tag] =
        cocotb_runtime_file(st_id, bm_id, corner, std::filesystem::path(src_fname).filename());
      tc_node[tc_files_tag].push_back(tc_fnode);
    }
    /* cocotb testbench */
    {
      YAML::Node tc_fnode;
      tc_fnode[tc_file_name_tag] = tc_cocotb_tb_name;
      tc_fnode[tc_file_type_tag] = cocotb_file_type2str(e_cocotb_file_type::TOP);
      std::vector<BenchmarkCocotbSourceId> constr_f =
        ae_mgr_->benchmark_manager()->cocotb_sources_by_type(
          bm_id, BenchmarkManager::e_file_type::COCOTB_TOP);
      if (constr_f.empty()) {
        VTR_LOG_ERROR("Benchmark '%s' has not valid cocotb testbench!\n",
                      ae_mgr_->benchmark_manager()->benchmark_name(bm_id).c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      if (constr_f.size() > 1) {
        VTR_LOG_ERROR("Benchmark '%s' has more than 1 cocotb testbench!\n",
                      ae_mgr_->benchmark_manager()->benchmark_name(bm_id).c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      std::string src_fname = ae_mgr_->benchmark_manager()->cocotb_file(bm_id, constr_f[0]);
      tc_fnode[tc_file_src_tag] = src_fname;
      tc_fnode[tc_file_des_tag] =
        cocotb_runtime_file(st_id, bm_id, corner, std::filesystem::path(src_fname).filename());
      tc_node[tc_files_tag].push_back(tc_fnode);
    }
    /* base bitstream */
    {
      YAML::Node tc_fnode;
      tc_fnode[tc_file_name_tag] = tc_bitstream_base_fname;
      tc_fnode[tc_file_type_tag] = cocotb_file_type2str(e_cocotb_file_type::BITSTREAM_BASE);
      std::string src_fname =
        file(st_id, bm_id, e_file_type::BASE_BITSTREAM_XML_FILE, corner, verbose);
      tc_fnode[tc_file_src_tag] = src_fname;
      /* The name change is for a hardcoded file name in cocotb utility script. Should update the
       * cocotb utility script to be flexible */
      tc_fnode[tc_file_des_tag] =
        cocotb_runtime_file(st_id, bm_id, corner, COCOTB_BASE_BITSTREAM_XML_FILENAME_);
      tc_node[tc_files_tag].push_back(tc_fnode);
    }
    /* bitstream value file */
    {
      YAML::Node tc_fnode;
      tc_fnode[tc_file_name_tag] = tc_bitstream_val_fname;
      tc_fnode[tc_file_type_tag] = cocotb_file_type2str(e_cocotb_file_type::BITSTREAM_VALUE);
      std::string src_fname = file(st_id, bm_id, e_file_type::BITSTREAM_XML_FILE, corner, verbose);
      tc_fnode[tc_file_src_tag] = src_fname;
      tc_fnode[tc_file_des_tag] =
        cocotb_runtime_file(st_id, bm_id, corner, std::filesystem::path(src_fname).filename());
      tc_node[tc_files_tag].push_back(tc_fnode);
    }
    /* force_bitstream util */
    {
      YAML::Node tc_fnode;
      tc_fnode[tc_file_name_tag] = tc_cocotb_util_fname;
      tc_fnode[tc_file_type_tag] = cocotb_file_type2str(e_cocotb_file_type::UTIL);
      std::string src_fname = cocotb_force_bitstrm_bin_path;
      tc_fnode[tc_file_src_tag] = src_fname;
      tc_fnode[tc_file_des_tag] =
        cocotb_runtime_file(st_id, bm_id, corner, std::filesystem::path(src_fname).filename());
      tc_node[tc_files_tag].push_back(tc_fnode);
    }

    /* Append */
    config[testcase_tag].push_back(tc_node);
    cnt++;
  }

  if (cnt == 0) {
    VTR_LOG(
      "Bypass benchmark suite '%s' for design verification, as it is fully disabled by user\n",
      ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
  }

  /* YAML writer */
  std::filesystem::path runtime_task_fpath = cocotb_task_file(st_id, corner);
  openfpga::create_directory(runtime_task_fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(runtime_task_fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "Cocotb task data written to '%s'\n", runtime_task_fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", runtime_task_fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::run_cocotb_setup(const BenchmarkSuiteId& st_id,
                                const std::string& cocotb_util_bin_path, const size_t& max_jobs,
                                const size_t& new_thread_wait_time,
                                const size_t& sim_start_wait_time, const std::string& corner,
                                const bool& verbose) const {
  if (openfpga::CMD_EXEC_SUCCESS !=
      set_system_env_var(PYTHON_PATH_ENV_VAR_NAME, PYTHON_SITE_PACKAGES_PATH_, false)) {
    VTR_LOG_ERROR("Failed to set PYTHONPATH for cocotb setup.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string py_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + cocotb_util_bin_path);
  py_cmd += std::string(" --config ") + cocotb_task_file(st_id, corner).string();
  py_cmd += std::string(" --jobs ") + std::to_string(max_jobs);
  py_cmd += std::string(" --setup ");
  py_cmd += std::string(" --new_thread_wait_time ") + std::to_string(new_thread_wait_time);
  py_cmd += std::string(" --sim_start_wait_time ") + std::to_string(sim_start_wait_time);

  VTR_LOGV(verbose, "Running command:\n\t%s\n", py_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = preconfig_sim_runtime_dir();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR("No process is available to run cocotb setup for benchmark suite '%s'!\n",
                  ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(py_cmd.c_str())) {
    VTR_LOG_ERROR("Cocotb setup run failed with errors for benchmark suite '%s'!\n",
                  ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::setup_cocotb(const std::string& cocotb_util_bin_path,
                            const std::string& cocotb_force_bitstrm_bin_path,
                            const size_t& max_jobs, const size_t& new_thread_wait_time,
                            const size_t& sim_start_wait_time, const std::string& corner,
                            const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  for (auto st_id : ae_mgr_->benchmark_manager()->suites()) {
    VTR_LOG("Generating cocotb task for suite '%s'...\n",
            ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    size_t cnt = 0;
    status = write_cocotb_task_file(st_id, cocotb_force_bitstrm_bin_path, corner, cnt, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
    if (cnt == 0) {
      continue;
    }
    VTR_LOG("Setup cocotb workspace for suite '%s'...\n",
            ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    status = run_cocotb_setup(st_id, cocotb_util_bin_path, max_jobs, new_thread_wait_time,
                              sim_start_wait_time, corner, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::run_cocotb_preconfig_task(
  const BenchmarkSuiteId& st_id, const std::string& cocotb_util_bin_path,
  const std::string& cocotb_makefile_incl_fpath, const DvManager::e_netlist_type& nlist_type,
  const DvManager::e_simulator_type& sim_type, const DvManager::e_waveform_type& wfm_type,
  const size_t& max_jobs, const size_t& new_thread_wait_time, const size_t& sim_start_wait_time,
  const size_t& log_check_wait_time, const std::string& corner, const std::string& benchmark_name,
  const bool& verbose) const {
  if (openfpga::CMD_EXEC_SUCCESS !=
      set_system_env_var(PYTHON_PATH_ENV_VAR_NAME, PYTHON_SITE_PACKAGES_PATH_, false)) {
    VTR_LOG_ERROR("Failed to set PYTHONPATH for cocotb preconfig simulation.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string py_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + cocotb_util_bin_path);
  py_cmd += std::string(" --config ") + cocotb_task_file(st_id, corner).string();
  py_cmd += std::string(" --jobs ") + std::to_string(max_jobs);
  py_cmd += std::string(" --new_thread_wait_time ") + std::to_string(new_thread_wait_time);
  py_cmd += std::string(" --sim_start_wait_time ") + std::to_string(sim_start_wait_time);
  py_cmd += std::string(" --log_check_wait_time ") + std::to_string(log_check_wait_time);
  py_cmd += std::string(" --netlist_type ") + netlist_type2str(nlist_type);
  py_cmd += std::string(" --simulator ") + simulator_type2str(sim_type);
  py_cmd += std::string(" --dump_waveform ") + waveform_type2str(wfm_type);
  py_cmd += std::string(" --params MAKEFILE_INC_HOME:") +
            std::filesystem::path(cocotb_makefile_incl_fpath).parent_path().string() +
            std::string(",CELL_LIB_HOME:") + std::filesystem::path(cell_lib_path_).string();
  py_cmd += std::string(" --clean_previous_run ");
  if (!benchmark_name.empty()) {
    py_cmd += std::string(" --task ") + benchmark_name;
  }

  VTR_LOGV(verbose, "Running command:\n\t%s\n", py_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = preconfig_sim_runtime_dir();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR(
      "No process is available to run cocotb preconfigured simulation for benchmark suite '%s'!\n",
      ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(py_cmd.c_str())) {
    VTR_LOG_ERROR(
      "Cocotb preconfigured simulation run failed with errors for benchmark suite '%s'!\n",
      ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::run_cocotb_preconfig_task_benchmark(
  const BenchmarkSuiteId& st_id, const std::string& benchmark_name,
  const std::string& cocotb_util_bin_path, const std::string& cocotb_makefile_incl_fpath,
  const DvManager::e_netlist_type& nlist_type, const DvManager::e_simulator_type& sim_type,
  const DvManager::e_waveform_type& wfm_type, const size_t& max_jobs,
  const size_t& new_thread_wait_time, const size_t& sim_start_wait_time,
  const size_t& log_check_wait_time, const std::string& corner, const bool& verbose) const {
  if (openfpga::CMD_EXEC_SUCCESS !=
      set_system_env_var(PYTHON_PATH_ENV_VAR_NAME, PYTHON_SITE_PACKAGES_PATH_, false)) {
    VTR_LOG_ERROR("Failed to set PYTHONPATH for cocotb benchmark simulation.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string py_cmd(std::string(PYTHON_COMMAND_) + std::string(" ") + cocotb_util_bin_path);
  py_cmd += std::string(" --config ") + cocotb_task_file(st_id, corner).string();
  py_cmd += std::string(" --jobs ") + std::to_string(max_jobs);
  py_cmd += std::string(" --task ") + benchmark_name;
  py_cmd += std::string(" --new_thread_wait_time ") + std::to_string(new_thread_wait_time);
  py_cmd += std::string(" --sim_start_wait_time ") + std::to_string(sim_start_wait_time);
  py_cmd += std::string(" --log_check_wait_time ") + std::to_string(log_check_wait_time);
  py_cmd += std::string(" --netlist_type ") + netlist_type2str(nlist_type);
  py_cmd += std::string(" --simulator ") + simulator_type2str(sim_type);
  py_cmd += std::string(" --dump_waveform ") + waveform_type2str(wfm_type);
  py_cmd += std::string(" --params MAKEFILE_INC_HOME:") +
            std::filesystem::path(cocotb_makefile_incl_fpath).parent_path().string();
  py_cmd += std::string(" --clean_previous_run ");

  VTR_LOGV(verbose, "Running command:\n\t%s\n", py_cmd.c_str());

  /* Change directory */
  std::filesystem::path curr_dir = std::filesystem::current_path();
  std::filesystem::path runtime_dir = preconfig_sim_runtime_dir();
  std::filesystem::current_path(runtime_dir);
  VTR_LOGV(verbose, "Change to runtime directory\n\t%s\n", runtime_dir.c_str());

  if (!system(NULL)) {
    VTR_LOG_ERROR(
      "No process is available to run cocotb preconfigured simulation for benchmark '%s'!\n",
      benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (system(py_cmd.c_str())) {
    VTR_LOG_ERROR("Cocotb preconfigured simulation run failed with errors for benchmark '%s'!\n",
                  benchmark_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOG("Done\n");

  /* FIXME: check the return code, log file to validate if the run is a success or not !!! */
  /* Restore directory */
  std::filesystem::current_path(curr_dir);
  VTR_LOGV(verbose, "Restore to project home directory\n\t%s\n", curr_dir.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::run_preconfig_cocotb(const std::string& cocotb_util_bin_path,
                                    const std::string& cocotb_makefile_incl_fpath,
                                    const DvManager::e_netlist_type& nlist_type,
                                    const DvManager::e_simulator_type& sim_type,
                                    const DvManager::e_waveform_type& wfm_type,
                                    const size_t& max_jobs, const size_t& new_thread_wait_time,
                                    const size_t& sim_thread_wait_time,
                                    const size_t& log_check_wait_time, const std::string& corner,
                                    const std::string& benchmark_name, const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Error out if not specified */
  BenchmarkId bm_id;
  BenchmarkSuiteId bm_st_id;
  if (!benchmark_name.empty()) {
    bm_id = ae_mgr_->benchmark_manager()->find_benchmark_by_name(benchmark_name);
    bm_st_id = ae_mgr_->benchmark_manager()->find_benchmark_suite(benchmark_name);
    if (!ae_mgr_->benchmark_manager()->valid_benchmark_id(bm_id)) {
      VTR_LOG_ERROR("Invalid benchmark name '%s' which is not defined\n", benchmark_name.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    if (ae_mgr_->benchmark_manager()->is_benchmark_disabled(bm_id)) {
      VTR_LOG_ERROR("Invalid benchmark name '%s' which is disabled\n", benchmark_name.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  for (auto st_id : ae_mgr_->benchmark_manager()->suites()) {
    if (!benchmark_name.empty() && st_id != bm_st_id) {
      VTR_LOGV(verbose, "Skip suite as specified benchmark '%s' is not in it\n",
               benchmark_name.c_str());
      continue;
    }
    VTR_LOG("Run preconfigured simulation for suite '%s'...\n",
            ae_mgr_->benchmark_manager()->suite_name(st_id).c_str());
    status = run_cocotb_preconfig_task(st_id, cocotb_util_bin_path, cocotb_makefile_incl_fpath,
                                       nlist_type, sim_type, wfm_type, max_jobs,
                                       new_thread_wait_time, sim_thread_wait_time,
                                       log_check_wait_time, corner, benchmark_name, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOG("Done\n");
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int DvManager::run_preconfig_cocotb_benchmark(
  const std::string& cocotb_util_bin_path, const std::string& cocotb_makefile_incl_fpath,
  const BenchmarkSuiteId& st_id, const std::string& benchmark_name,
  const DvManager::e_netlist_type& nlist_type, const DvManager::e_simulator_type& sim_type,
  const DvManager::e_waveform_type& wfm_type, const size_t& max_jobs,
  const size_t& new_thread_wait_time, const size_t& sim_thread_wait_time,
  const size_t& log_check_wait_time, const std::string& corner, const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  VTR_LOG("Run preconfigured simulation for benchmark '%s'...\n", benchmark_name.c_str());
  status = run_cocotb_preconfig_task_benchmark(
    st_id, benchmark_name, cocotb_util_bin_path, cocotb_makefile_incl_fpath, nlist_type, sim_type,
    wfm_type, max_jobs, new_thread_wait_time, sim_thread_wait_time, log_check_wait_time, corner,
    verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  return openfpga::CMD_EXEC_SUCCESS;
}

bool DvManager::valid_netlist_type(const DvManager::e_netlist_type& nlist_type) const {
  return nlist_type != DvManager::e_netlist_type::NUM_TYPES;
}

void DvManager::check_valid_netlist_type(const DvManager::e_netlist_type& nlist_type) const {
  if (!valid_netlist_type(nlist_type)) {
    VTR_LOG_ERROR("Invalid netlist type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

DvManager::e_netlist_type DvManager::str2netlist_type(const std::string& str,
                                                      const bool& verbose) const {
  for (int itype = size_t(DvManager::e_netlist_type::RTL);
       itype != size_t(DvManager::e_netlist_type::NUM_TYPES); ++itype) {
    if (str == std::string(NETLIST_TYPE_STRING_[itype])) {
      return static_cast<DvManager::e_netlist_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of netlist file! Expect %s\n",
                 netlist_type_all2str().c_str());
  return DvManager::e_netlist_type::NUM_TYPES;
}

std::string DvManager::netlist_type2str(const DvManager::e_netlist_type& type,
                                        const bool& verbose) const {
  if (!valid_netlist_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of netlist file! Expect %s\n",
                   netlist_type_all2str().c_str());
    return std::string();
  }
  return std::string(NETLIST_TYPE_STRING_[size_t(type)]);
}

std::string DvManager::netlist_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(DvManager::e_netlist_type::RTL);
       itype != size_t(DvManager::e_netlist_type::NUM_TYPES); ++itype) {
    full_types += std::string(NETLIST_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool DvManager::valid_cocotb_file_type(const DvManager::e_cocotb_file_type& nlist_type) const {
  return nlist_type != DvManager::e_cocotb_file_type::NUM_TYPES;
}

void DvManager::check_valid_cocotb_file_type(
  const DvManager::e_cocotb_file_type& nlist_type) const {
  if (!valid_cocotb_file_type(nlist_type)) {
    VTR_LOG_ERROR("Invalid netlist type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

DvManager::e_cocotb_file_type DvManager::str2cocotb_file_type(const std::string& str,
                                                              const bool& verbose) const {
  for (int itype = size_t(DvManager::e_cocotb_file_type::TOP);
       itype != size_t(DvManager::e_cocotb_file_type::NUM_TYPES); ++itype) {
    if (str == std::string(COCOTB_FILE_TYPE_STRING_[itype])) {
      return static_cast<DvManager::e_cocotb_file_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of cocotb file! Expect %s\n",
                 cocotb_file_type_all2str().c_str());
  return DvManager::e_cocotb_file_type::NUM_TYPES;
}

std::string DvManager::cocotb_file_type2str(const DvManager::e_cocotb_file_type& type,
                                            const bool& verbose) const {
  if (!valid_cocotb_file_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of cocotb file! Expect %s\n",
                   cocotb_file_type_all2str().c_str());
    return std::string();
  }
  return std::string(COCOTB_FILE_TYPE_STRING_[size_t(type)]);
}

std::string DvManager::cocotb_file_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(DvManager::e_cocotb_file_type::TOP);
       itype != size_t(DvManager::e_cocotb_file_type::NUM_TYPES); ++itype) {
    full_types += std::string(COCOTB_FILE_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool DvManager::init_netlist_source_dir(const bool& verbose) {
  int num_err = 0;
  std::filesystem::path prj_home = project_home_dir();
  for (NetlistDevManager::e_netlist_type nlist_type : nd_mgr_->netlist_types()) {
    if (!required_netlist_type(nlist_type)) {
      continue;
    }
    auto dv_nlist_type = convert_netlist_type(nlist_type);
    std::string nlist_src = nd_mgr_->netlist_release_dir(nlist_type);
    std::string nlist_des = netlist_dir(dv_nlist_type).string();
    /* Create symbolic links */
    num_err += copy_file_with_mkdir(nlist_src, nlist_des, true, verbose);
  }
  /* Create a fake post-layout directory. TODO: Consider a real one once PD is integrated */
  { openfpga::create_directory(netlist_dir(e_netlist_type::PL).string(), true, verbose); }
  return num_err ? false : true;
}

bool DvManager::required_netlist_type(const NetlistDevManager::e_netlist_type& f_type) const {
  return valid_netlist_type(convert_netlist_type(f_type));
}

DvManager::e_netlist_type DvManager::convert_netlist_type(
  const NetlistDevManager::e_netlist_type& nd_f_type) const {
  switch (nd_f_type) {
    case NetlistDevManager::e_netlist_type::RTL:
      return e_netlist_type::RTL;
    case NetlistDevManager::e_netlist_type::GL:
      return e_netlist_type::GL;
    default:
      return e_netlist_type::NUM_TYPES;
  }
  return e_netlist_type::NUM_TYPES;
}

TapeoutManager::e_substep_name DvManager::convert_netlist_type_to_substep_name(
  const DvManager::e_netlist_type& nlist_type) const {
  switch (nlist_type) {
    case DvManager::e_netlist_type::RTL:
      return TapeoutManager::e_substep_name::DV_RTL_PRECONFIG;
    case DvManager::e_netlist_type::GL:
      return TapeoutManager::e_substep_name::DV_GL_PRECONFIG;
    case DvManager::e_netlist_type::PL:
      return TapeoutManager::e_substep_name::DV_PL_PRECONFIG;
    default:
      return TapeoutManager::e_substep_name::NUM_TYPES;
  }
  return TapeoutManager::e_substep_name::NUM_TYPES;
}

bool DvManager::valid_simulator_type(const DvManager::e_simulator_type& nlist_type) const {
  return nlist_type != DvManager::e_simulator_type::NUM_TYPES;
}

void DvManager::check_valid_simulator_type(const DvManager::e_simulator_type& nlist_type) const {
  if (!valid_simulator_type(nlist_type)) {
    VTR_LOG_ERROR("Invalid simulator type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

DvManager::e_simulator_type DvManager::str2simulator_type(const std::string& str,
                                                          const bool& verbose) const {
  for (int itype = size_t(DvManager::e_simulator_type::VCS);
       itype != size_t(DvManager::e_simulator_type::NUM_TYPES); ++itype) {
    if (str == std::string(SIMULATOR_TYPE_STRING_[itype])) {
      return static_cast<DvManager::e_simulator_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of simulator! Expect %s\n",
                 simulator_type_all2str().c_str());
  return DvManager::e_simulator_type::NUM_TYPES;
}

std::string DvManager::simulator_type2str(const DvManager::e_simulator_type& type,
                                          const bool& verbose) const {
  if (!valid_simulator_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of simulator! Expect %s\n",
                   simulator_type_all2str().c_str());
    return std::string();
  }
  return std::string(SIMULATOR_TYPE_STRING_[size_t(type)]);
}

std::string DvManager::simulator_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(DvManager::e_simulator_type::VCS);
       itype != size_t(DvManager::e_simulator_type::NUM_TYPES); ++itype) {
    full_types += std::string(SIMULATOR_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool DvManager::valid_waveform_type(const DvManager::e_waveform_type& nlist_type) const {
  return nlist_type != DvManager::e_waveform_type::NUM_TYPES;
}

void DvManager::check_valid_waveform_type(const DvManager::e_waveform_type& nlist_type) const {
  if (!valid_waveform_type(nlist_type)) {
    VTR_LOG_ERROR("Invalid waveform type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

DvManager::e_waveform_type DvManager::str2waveform_type(const std::string& str,
                                                        const bool& verbose) const {
  for (int itype = size_t(DvManager::e_waveform_type::NONE);
       itype != size_t(DvManager::e_waveform_type::NUM_TYPES); ++itype) {
    if (str == std::string(WAVEFORM_TYPE_STRING_[itype])) {
      return static_cast<DvManager::e_waveform_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of waveform! Expect %s\n", waveform_type_all2str().c_str());
  return DvManager::e_waveform_type::NUM_TYPES;
}

std::string DvManager::waveform_type2str(const DvManager::e_waveform_type& type,
                                         const bool& verbose) const {
  if (!valid_waveform_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of waveform! Expect %s\n",
                   waveform_type_all2str().c_str());
    return std::string();
  }
  return std::string(WAVEFORM_TYPE_STRING_[size_t(type)]);
}

std::string DvManager::waveform_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(DvManager::e_waveform_type::NONE);
       itype != size_t(DvManager::e_waveform_type::NUM_TYPES); ++itype) {
    full_types += std::string(WAVEFORM_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::vector<std::string> DvManager::result_files(const bool& verbose) const {
  std::vector<std::string> ret;
  /* Preconfigured testbenches */
  for (auto st_id : ae_mgr_->benchmark_manager()->suites()) {
    for (auto bm_id : ae_mgr_->benchmark_manager()->benchmarks(st_id)) {
      for (auto corner : ae_mgr_->corners()) {
        std::string tb_file =
          file(st_id, bm_id, e_file_type::PRECONFIG_TESTBENCH_FILE, corner, verbose);
        /* TODO: Need to find a way how to identify which corner has been covered by bitgen!!! */
        if (!std::filesystem::exists(tb_file)) {
          continue;
        }
        ret.push_back(tb_file);
      }
    }
  }
  /* Cocotb task files */
  for (auto st_id : ae_mgr_->benchmark_manager()->suites()) {
    for (auto corner : ae_mgr_->corners()) {
      /* TODO: Need to find a way how to identify which corner has been covered by bitgen!!! */
      std::string cctb_task_file = cocotb_task_file(st_id, corner).string();
      if (!std::filesystem::exists(cctb_task_file)) {
        continue;
      }
      ret.push_back(cctb_task_file);
    }
  }
  return ret;
}

std::string DvManager::md5_yaml_file() const {
  std::filesystem::path fpath = project_home_dir() / std::filesystem::path(MD5_FILENAME_);
  return fpath.string();
}

int DvManager::save_results(const bool& verbose) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  FileMd5Manager md5_mgr;
  for (auto fpath : result_files(verbose)) {
    /* TODO: Only success step require all the files exist. Also required file should be more
     * detailed by each step! And this check should depend on substep success */
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
  status = md5_mgr.write_md5_to_yaml_file(md5_yaml_file(), verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Export Md5 to files failed when saving results\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return status;
}

int DvManager::check_results(const bool& verbose) const {
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

std::string DvManager::report_timing_dirpath(const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(RPT_DIR_NAME_) /
                                  std::filesystem::path(RPT_TIMING_DIR_NAME_) /
                                  std::filesystem::path(NETLIST_TYPE_STRING_[size_t(nlist_type)]);
  return dirpath.string();
}

std::string DvManager::report_power_dirpath(const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(RPT_DIR_NAME_) /
                                  std::filesystem::path(RPT_POWER_DIR_NAME_) /
                                  std::filesystem::path(NETLIST_TYPE_STRING_[size_t(nlist_type)]);
  return dirpath.string();
}

std::string DvManager::ppa_summary_dirpath(const e_netlist_type& nlist_type) const {
  std::filesystem::path dirpath = project_home_dir() / std::filesystem::path(RPT_DIR_NAME_) /
                                  std::filesystem::path(RPT_SUMMARY_DIR_NAME_) /
                                  std::filesystem::path(NETLIST_TYPE_STRING_[size_t(nlist_type)]);
  return dirpath.string();
}

}  // End namespace arcxpress
