/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML of project file to the associated
 * data structures
 *******************************************************************/
#include <filesystem>
#include <string>

/* Headers from pugi XML library */
#include "pugixml.hpp"
#include "pugixml_util.hpp"

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_time.h"

/* Headers from libarchfpga */
#include "arch_error.h"
#include "command_exit_codes.h"
#include "pugixml_util.hpp"
#include "read_xml_tapeout_manager.h"
#include "read_xml_tapeout_manager_arch_template.h"
#include "read_xml_util.h"
#include "tapeout_manager_xml_constants.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * Parse XML codes of a <stamp> to an object of ProjectManager
 *******************************************************************/
static void read_xml_stamp(pugi::xml_node& xml_stamp, const pugiutil::loc_data& loc_data,
                           TapeoutManager& to_mgr, const std::string& fname) {
  pugi::xml_node xml_author = get_single_child(xml_stamp, XML_TAPEOUT_AUTHOR_NODE_NAME, loc_data);
  std::string author_str = std::string(xml_author.child_value());

  pugi::xml_node xml_dates = get_single_child(xml_stamp, XML_TAPEOUT_DATE_NODE_NAME, loc_data);
  pugi::xml_node xml_created_date =
    get_single_child(xml_dates, XML_TAPEOUT_CREATE_DATE_NODE_NAME, loc_data);
  std::string date_created_str = std::string(xml_created_date.child_value());

  pugi::xml_node xml_last_modified_date =
    get_single_child(xml_dates, XML_TAPEOUT_LAST_MODIFIED_DATE_NODE_NAME, loc_data);
  std::string date_last_modified_str = std::string(xml_last_modified_date.child_value());

  pugi::xml_node xml_tool_version =
    get_single_child(xml_stamp, XML_TAPEOUT_TOOL_VERSION_NODE_NAME, loc_data);
  std::string tool_version_str = std::string(xml_tool_version.child_value());

  to_mgr.set_author(author_str);
  to_mgr.set_date_created(date_created_str);
  to_mgr.set_date_last_modified(date_last_modified_str);
  to_mgr.set_tool_version(tool_version_str);
}

static int read_xml_step_run_dep(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                                 TapeoutManager& to_mgr, const std::string& step_name,
                                 const std::string& curr_run) {
  int num_err = 0;
  for (pugi::xml_node xml_dep : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_dep.name() != std::string(XML_TAPEOUT_STEP_RUNS_RUN_DEP_NODE_NAME)) {
      continue;
    }
    /* Parse each dependency */
    std::string dep_step =
      get_attribute(xml_dep, XML_TAPEOUT_STEP_RUNS_RUN_DEP_ATTRIBUTE_STEP, loc_data).as_string();
    std::string dep_run =
      get_attribute(xml_dep, XML_TAPEOUT_STEP_RUNS_RUN_DEP_ATTRIBUTE_RUN, loc_data).as_string();
    bool op_succeed = to_mgr.add_step_run_dep(step_name, curr_run, dep_step, dep_run);
    if (!op_succeed) {
      num_err++;
      break;
    }
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_step_run_substeps(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                                      TapeoutManager& to_mgr, const std::string& step_name,
                                      const std::string& curr_run) {
  int num_err = 0;
  for (pugi::xml_node xml_substep : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_substep.name() != std::string(XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_NODE_NAME)) {
      continue;
    }
    /* Parse each substep */
    std::string substep =
      get_attribute(xml_substep, XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_ATTRIBUTE_NAME, loc_data)
        .as_string();
    std::string status =
      get_attribute(xml_substep, XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_ATTRIBUTE_STATUS, loc_data)
        .as_string();
    bool op_succeed = to_mgr.set_substep_status(step_name, curr_run, substep, status);
    if (!op_succeed) {
      num_err++;
      break;
    }
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_step(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                         TapeoutManager& to_mgr) {
  int num_err = 0;
  std::string step_name =
    get_attribute(xml_root, XML_TAPEOUT_STEP_ATTRIBUTE_NAME, loc_data).as_string();

  pugi::xml_node xml_runs = get_single_child(xml_root, XML_TAPEOUT_STEP_RUNS_NODE_NAME, loc_data,
                                             pugiutil::ReqOpt::OPTIONAL);
  if (!xml_runs) {
    return openfpga::CMD_EXEC_SUCCESS;
  }
  std::string latest_run =
    get_attribute(xml_runs, XML_TAPEOUT_STEP_RUNS_ATTRIBUTE_LATEST, loc_data).as_string();
  to_mgr.set_step_latest_run(step_name, latest_run);

  for (pugi::xml_node xml_run : xml_runs.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_run.name() != std::string(XML_TAPEOUT_STEP_RUNS_RUN_NODE_NAME)) {
      bad_tag(xml_runs, loc_data, xml_run, {XML_TAPEOUT_STEP_RUNS_RUN_NODE_NAME});
    }
    /* Parse each run */
    std::string curr_run =
      get_attribute(xml_run, XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_ID, loc_data).as_string();
    bool op_succeed = to_mgr.add_step_run(step_name, curr_run);
    if (!op_succeed) {
      num_err++;
      break;
    }

    std::string status_str =
      get_attribute(xml_run, XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_STATUS, loc_data).as_string();
    op_succeed = to_mgr.set_step_run_status(step_name, curr_run, status_str);
    if (!op_succeed) {
      num_err++;
      break;
    }

    bool frozen =
      get_attribute(xml_run, XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_FROZEN, loc_data).as_bool();
    op_succeed = to_mgr.set_step_run_frozen(step_name, curr_run, frozen);
    if (!op_succeed) {
      num_err++;
      break;
    }

    /* Sanity checks */
    for (pugi::xml_node xml_dep : xml_run.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_dep.name() != std::string(XML_TAPEOUT_STEP_RUNS_RUN_DEP_NODE_NAME) &&
          xml_dep.name() != std::string(XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_NODE_NAME)) {
        bad_tag(
          xml_run, loc_data, xml_dep,
          {XML_TAPEOUT_STEP_RUNS_RUN_DEP_NODE_NAME, XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_NODE_NAME});
      }
    }

    /* Parse dependency */
    int status = read_xml_step_run_dep(xml_run, loc_data, to_mgr, step_name, curr_run);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
    /* Parse substeps */
    status = read_xml_step_run_substeps(xml_run, loc_data, to_mgr, step_name, curr_run);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_corner_map(pugi::xml_node& xml_corner_map, const pugiutil::loc_data& loc_data,
                               TapeoutManager& to_mgr) {
  int num_err = 0;

  std::string default_corner =
    get_attribute(xml_corner_map, XML_TAPEOUT_CORNERMAP_ATTRIBUTE_DEFAULT, loc_data,
                  pugiutil::ReqOpt::OPTIONAL)
      .as_string();
  to_mgr.set_default_corner(default_corner);

  for (pugi::xml_node xml_corner : xml_corner_map.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_corner.name() == std::string(XML_TAPEOUT_CORNER_DISABLED_NODE_NAME)) {
      std::string corner_name = std::string(xml_corner.child_value());
      int status = to_mgr.add_disabled_corner(corner_name);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                      XML_TAPEOUT_CORNER_DISABLED_NODE_NAME);
        num_err++;
      }
    } else if (xml_corner.name() == std::string(XML_TAPEOUT_CORNERMAP_CORNER_NODE_NAME)) {
      /* Parse data */
      std::string corner_name =
        get_attribute(xml_corner, XML_TAPEOUT_CORNERMAP_CORNER_ATTRIBUTE_NAME, loc_data)
          .as_string();
      /* Sclib, spef, and arch timing data is in a dedicated XML node */
      pugi::xml_node xml_sclib =
        get_single_child(xml_corner, XML_TAPEOUT_CORNERMAP_CORNER_SCLIB_NODE_NAME, loc_data);
      pugi::xml_node xml_spef =
        get_single_child(xml_corner, XML_TAPEOUT_CORNERMAP_CORNER_SPEF_NODE_NAME, loc_data);
      to_mgr.add_corner_tag(corner_name, xml_sclib.child_value(), xml_spef.child_value());
    } else {
      bad_tag(xml_corner, loc_data, xml_corner_map,
              {XML_TAPEOUT_CORNERMAP_CORNER_NODE_NAME, XML_TAPEOUT_CORNER_DISABLED_NODE_NAME});
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

// Add a new helper function to handle status nodes
static int read_xml_benchmark_status(pugi::xml_node& xml_status_node,
                                     const pugiutil::loc_data& loc_data, TapeoutManager& to_mgr) {
  int num_err = 0;

  // Parse attributes of the status node
  std::string benchmark_name =
    get_attribute(xml_status_node, XML_TAPEOUT_BENCHMARK_STATUS_ATTRIBUTE_BENCHMARK, loc_data)
      .as_string();
  std::string run_name =
    get_attribute(xml_status_node, XML_TAPEOUT_BENCHMARK_STATUS_ATTRIBUTE_RUN, loc_data)
      .as_string();
  std::string corner_name =
    get_attribute(xml_status_node, XML_TAPEOUT_BENCHMARK_STATUS_ATTRIBUTE_CORNER, loc_data)
      .as_string();

  // Process child nodes of the status node, including custom_vpr_constraint_file
  for (pugi::xml_node xml_child : xml_status_node.children()) {
    if (std::string(xml_child.name()) ==
        std::string(XML_TAPEOUT_CUSTOM_VPR_CONSTRAINT_FILE_NODE_NAME)) {
      // If constraint handling is needed, logic can be added here
      std::string file_path = std::string(xml_child.child_value());
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_boundary_timing(pugi::xml_node& xml_bdy_timing,
                                    const pugiutil::loc_data& loc_data, TapeoutManager& to_mgr) {
  int num_err = 0;

  for (pugi::xml_node xml_file : xml_bdy_timing.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() == std::string(XML_TAPEOUT_BOUNDARY_TIMING_FILE_NODE_NAME)) {
      std::string corner_name =
        get_attribute(xml_file, XML_TAPEOUT_BOUNDARY_TIMING_FILE_ATTR_CORNER, loc_data).as_string();
      std::string fpath = std::string(xml_file.child_value());
      to_mgr.add_boundary_timing_file(corner_name, fpath);
    } else {
      bad_tag(xml_file, loc_data, xml_bdy_timing, {XML_TAPEOUT_BOUNDARY_TIMING_FILE_NODE_NAME});
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_arch_settings(pugi::xml_node& xml_arch_settings,
                                  const pugiutil::loc_data& loc_data, TapeoutManager& to_mgr) {
  int num_err = 0;

  for (pugi::xml_node xml_run : xml_arch_settings.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_run.name() != std::string(XML_TAPEOUT_ARCH_SETTINGS_RUN_NODE_NAME)) {
      bad_tag(xml_arch_settings, loc_data, xml_run, {XML_TAPEOUT_ARCH_SETTINGS_RUN_NODE_NAME});
    }
    /* Parse each run */
    std::string curr_run =
      get_attribute(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_ID, loc_data).as_string();
    /* Parse settings one by one */
    /* Core dimension */
    pugi::xml_node xml_core_h =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_CORE_HEIGHT_NODE_NAME, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    pugi::xml_node xml_core_w =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_CORE_WIDTH_NODE_NAME, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    if ((xml_core_h && !xml_core_w) || (!xml_core_h && xml_core_w)) {
      VTR_LOG_ERROR(
        "Both height and width of core dimension must be specified. Arch settings at run '%s' have "
        "fatal errors \n",
        curr_run.c_str());
      num_err++;
      break;
    }
    if (xml_core_h && xml_core_w) {
      int h_val = get_attribute(xml_core_h, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE, loc_data)
                    .as_int(0);
      int w_val = get_attribute(xml_core_w, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE, loc_data)
                    .as_int(0);
      to_mgr.set_core_dimension(curr_run, h_val, w_val, true, false);
    }
    /* Sync dp dimension */
    pugi::xml_node xml_sync_dp_dim =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_SYNC_DP_DIMENSION_NODE_NAME, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    if (xml_sync_dp_dim) {
      bool val =
        get_attribute(xml_sync_dp_dim, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE, loc_data)
          .as_bool(false);
      to_mgr.set_sync_dp_dimension(curr_run, val, true, false);
    }
    /* num of clocks */
    pugi::xml_node xml_num_clks =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_CLOCKS_NODE_NAME, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    if (xml_num_clks) {
      int val = get_attribute(xml_num_clks, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE, loc_data)
                  .as_int(0);
      to_mgr.set_num_clocks(curr_run, val, true, false);
    }
    /* num of clocks */
    pugi::xml_node xml_num_rsts =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_RESETS_NODE_NAME, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    if (xml_num_rsts) {
      int val = get_attribute(xml_num_rsts, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE, loc_data)
                  .as_int(0);
      to_mgr.set_num_resets(curr_run, val, true, false);
    }
    /* num of programming clocks */
    pugi::xml_node xml_num_pclks =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_PCLOCKS_NODE_NAME, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    if (xml_num_pclks) {
      int val =
        get_attribute(xml_num_pclks, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE, loc_data)
          .as_int(0);
      to_mgr.set_num_prog_clocks(curr_run, val, true, false);
    }
    /* num of configuration regions */
    pugi::xml_node xml_num_config_regions =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_CONFIG_REGIONS_NODE_NAME,
                       loc_data, pugiutil::ReqOpt::OPTIONAL);
    if (xml_num_config_regions) {
      int val = get_attribute(xml_num_config_regions, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE,
                              loc_data)
                  .as_int(0);
      to_mgr.set_num_config_regions(curr_run, val, true, false);
    }
    /* Number of I/Os */
    pugi::xml_node xml_num_ios =
      get_single_child(xml_run, XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NODE_NAME, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    if (xml_num_ios) {
      for (pugi::xml_node xml_num_io : xml_num_ios.children()) {
        /* Error out if the XML child has an invalid name! */
        if (xml_num_io.name() !=
            std::string(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_NODE_NAME)) {
          bad_tag(xml_num_ios, loc_data, xml_num_io,
                  {XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_NODE_NAME});
        }
        std::string side_str =
          get_attribute(xml_num_io, XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_ATTRIBUTE_SIDE,
                        loc_data)
            .as_string();
        std::string type_str =
          get_attribute(xml_num_io, XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_ATTRIBUTE_TYPE,
                        loc_data)
            .as_string();
        int val = get_attribute(xml_num_io, XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE, loc_data)
                    .as_int(0);
        to_mgr.set_io_num(curr_run, side_str, type_str, val, true, false);
      }
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_tapeout_manager(const char* fname, TapeoutManager& to_mgr) {
  vtr::ScopedStartFinishTimer timer("Read tapeout project file");
  /* Check file existence */
  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File does not exist at location: %s!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t num_err = 0;

  to_mgr.clear();
  to_mgr.set_file(std::string(fname));

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_TAPEOUT_ROOT_NODE_NAME, loc_data);

    /* Parse project-level info */
    to_mgr.set_name(get_attribute(xml_root, XML_TAPEOUT_ATTRIBUTE_NAME, loc_data).as_string());
    to_mgr.set_family(get_attribute(xml_root, XML_TAPEOUT_ATTRIBUTE_FAMILY, loc_data).as_string());
    bool curr_status =
      to_mgr.set_path(get_attribute(xml_root, XML_TAPEOUT_ATTRIBUTE_PATH, loc_data).as_string());
    if (!curr_status) {
      num_err++;
    }

    /* Parse stamps */
    pugi::xml_node xml_stamp_root =
      get_single_child(xml_root, XML_TAPEOUT_STAMP_ROOT_NODE_NAME, loc_data);
    read_xml_stamp(xml_stamp_root, loc_data, to_mgr, std::string(fname));

    /* Parse pdk path */
    pugi::xml_node xml_pdk_path = get_single_child(xml_root, XML_TAPEOUT_PDK_NODE_NAME, loc_data);
    std::string pdk_path = std::string(xml_pdk_path.child_value());
    to_mgr.set_pdk_config(pdk_path);

    /* Parse corner map */
    pugi::xml_node xml_corner_map =
      get_single_child(xml_root, XML_TAPEOUT_CORNERMAP_NODE_NAME, loc_data);
    int corner_map_status = read_xml_corner_map(xml_corner_map, loc_data, to_mgr);
    if (corner_map_status == openfpga::CMD_EXEC_FATAL_ERROR) {
      VTR_LOG_ERROR("Parsing node '%s' failed with errors\n", XML_TAPEOUT_CORNERMAP_NODE_NAME);
      num_err++;
    }
    /* Parse boundary timing */
    pugi::xml_node xml_bdy_timing = get_single_child(
      xml_root, XML_TAPEOUT_BOUNDARY_TIMING_NODE_NAME, loc_data, pugiutil::ReqOpt::OPTIONAL);
    if (xml_bdy_timing) {
      int bdy_timing_status = read_xml_boundary_timing(xml_bdy_timing, loc_data, to_mgr);
      if (bdy_timing_status == openfpga::CMD_EXEC_FATAL_ERROR) {
        VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                      XML_TAPEOUT_BOUNDARY_TIMING_NODE_NAME);
        num_err++;
      }
    }

    /* Parse arch template file  */
    pugi::xml_node xml_arch_tmpl =
      get_single_child(xml_root, XML_TAPEOUT_ARCH_TEMPLATE_FILE_NODE_NAME, loc_data);
    std::string xml_arch_tmpl_file = std::string(xml_arch_tmpl.child_value());
    to_mgr.set_arch_template_file(xml_arch_tmpl_file);

    /* Parse benchmark path */
    pugi::xml_node xml_bm_path =
      get_single_child(xml_root, XML_TAPEOUT_BENCHMARK_NODE_NAME, loc_data);
    std::string default_bm_name =
      get_attribute(xml_bm_path, XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT, loc_data).as_string();
    to_mgr.set_default_benchmark_name(default_bm_name);
    std::string default4clk_sweep_bm_name =
      get_attribute(xml_bm_path, XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT_FOR_CLOCK_SWEEP, loc_data)
        .as_string();
    to_mgr.set_default4clk_sweep_benchmark_name(default4clk_sweep_bm_name);
    std::string default4rst_sweep_bm_name =
      get_attribute(xml_bm_path, XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT_FOR_RESET_SWEEP, loc_data)
        .as_string();
    to_mgr.set_default4rst_sweep_benchmark_name(default4rst_sweep_bm_name);
    for (pugi::xml_node xml_file : xml_bm_path.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_file.name() == std::string(XML_TAPEOUT_BENCHMARK_FILE_NODE_NAME)) {
        std::string bm_path = std::string(xml_file.child_value());
        bool status = to_mgr.add_benchmark_config(bm_path);
        if (!status) {
          VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                        XML_TAPEOUT_BENCHMARK_FILE_NODE_NAME);
          num_err++;
        }
      } else if (xml_file.name() == std::string(XML_TAPEOUT_BENCHMARK_DISABLED_NODE_NAME)) {
        std::string bm_name = std::string(xml_file.child_value());
        int status = to_mgr.add_disabled_benchmark(bm_name);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                        XML_TAPEOUT_BENCHMARK_DISABLED_NODE_NAME);
          num_err++;
        }
      } else if (xml_file.name() == std::string(XML_TAPEOUT_BENCHMARK_STATUS_NODE_NAME)) {
        // Add support for status nodes
        int status = read_xml_benchmark_status(xml_file, loc_data, to_mgr);
        if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
          VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                        XML_TAPEOUT_BENCHMARK_STATUS_NODE_NAME);
          num_err++;
        }
      } else {
        bad_tag(xml_bm_path, loc_data, xml_file,
                {XML_TAPEOUT_BENCHMARK_FILE_NODE_NAME, XML_TAPEOUT_BENCHMARK_DISABLED_NODE_NAME,
                 XML_TAPEOUT_BENCHMARK_STATUS_NODE_NAME});
      }
    }

    /* Parse steps */
    for (pugi::xml_node xml_step : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_step.name() == std::string(XML_TAPEOUT_STEP_NODE_NAME)) {
        int status = read_xml_step(xml_step, loc_data, to_mgr);
        if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
          VTR_LOG_ERROR("Parsing node '%s' failed with errors\n", XML_TAPEOUT_STEP_NODE_NAME);
          num_err++;
        }
      }
    }
    /* Parse arch settings */
    pugi::xml_node xml_arch_settings = get_single_child(
      xml_root, XML_TAPEOUT_ARCH_SETTINGS_NODE_NAME, loc_data, pugiutil::ReqOpt::OPTIONAL);
    if (xml_arch_settings) {
      int arch_setting_status = read_xml_arch_settings(xml_arch_settings, loc_data, to_mgr);
      if (arch_setting_status == openfpga::CMD_EXEC_FATAL_ERROR) {
        VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                      XML_TAPEOUT_ARCH_SETTINGS_NODE_NAME);
        num_err++;
      }
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname, e.line(), "%s", e.what());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Parse the addition file which is defined as a child of this file */
  int arch_tmpl_status =
    read_xml_tapeout_manager_arch_template(to_mgr.arch_template_file(false, false).c_str(), to_mgr);
  if (arch_tmpl_status != openfpga::CMD_EXEC_SUCCESS) {
    num_err++;
  }

  if (num_err) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arcxpress
