/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

/* Headers from pin constraint library */
#include "pugixml.hpp"
#include "tapeout_manager_xml_constants.h"
#include "write_xml_tapeout_manager.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * Only the time stamp of the first file that is read in will be outputted!
 * TODO: Consider to aggregate all the authors. Use the earliest created date.
 * Use the latest modified date.
 *******************************************************************/
static int write_xml_tapeout_stamps(pugi::xml_node& root_node, const TapeoutManager& to_mgr) {
  int err_code = 0;
  pugi::xml_node stmp_node = root_node.append_child(XML_TAPEOUT_STAMP_ROOT_NODE_NAME);

  pugi::xml_node author_node = stmp_node.append_child(XML_TAPEOUT_AUTHOR_NODE_NAME);
  author_node.text().set(to_mgr.author().c_str());

  pugi::xml_node dates_node = stmp_node.append_child(XML_TAPEOUT_DATE_NODE_NAME);
  pugi::xml_node created_node = dates_node.append_child(XML_TAPEOUT_CREATE_DATE_NODE_NAME);
  created_node.text().set(to_mgr.date_created().c_str());

  pugi::xml_node modified_node = dates_node.append_child(XML_TAPEOUT_LAST_MODIFIED_DATE_NODE_NAME);
  modified_node.text().set(to_mgr.date_last_modified().c_str());

  pugi::xml_node ver_node = stmp_node.append_child(XML_TAPEOUT_TOOL_VERSION_NODE_NAME);
  ver_node.text().set(to_mgr.tool_version().c_str());

  return err_code;
}

int write_xml_step_run(pugi::xml_node& root_node, const TapeoutManager& to_mgr,
                       const TapeoutManager::e_step_name& step, const std::string& run) {
  int err_code = 0;
  pugi::xml_node run_node = root_node.append_child(XML_TAPEOUT_STEP_RUNS_RUN_NODE_NAME);
  run_node.append_attribute(XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_ID) = run.c_str();
  run_node.append_attribute(XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_STATUS) =
    to_mgr.step_run_status_str(step, run).c_str();
  run_node.append_attribute(XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_FROZEN) =
    to_mgr.step_run_frozen(step, run);

  for (TapeoutManager::e_substep_name substep : to_mgr.substeps(step, run)) {
    pugi::xml_node substep_node =
      run_node.append_child(XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_NODE_NAME);
    substep_node.append_attribute(XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_ATTRIBUTE_NAME) =
      to_mgr.substep_name2str(substep).c_str();
    substep_node.append_attribute(XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_ATTRIBUTE_STATUS) =
      to_mgr.substep_status_str(step, run, substep).c_str();
  }

  for (TapeoutManager::e_step_name dep : to_mgr.step_run_deps(step, run)) {
    pugi::xml_node dep_node = run_node.append_child(XML_TAPEOUT_STEP_RUNS_RUN_DEP_NODE_NAME);
    dep_node.append_attribute(XML_TAPEOUT_STEP_RUNS_RUN_DEP_ATTRIBUTE_STEP) =
      to_mgr.step_name2str(dep).c_str();
    dep_node.append_attribute(XML_TAPEOUT_STEP_RUNS_RUN_DEP_ATTRIBUTE_RUN) =
      to_mgr.step_run_dep_run(step, run, dep).c_str();
  }

  return err_code;
}

static int write_xml_corner_map(pugi::xml_node& flset_node, const TapeoutManager& to_mgr) {
  int err_code = 0;
  pugi::xml_node corner_map_node = flset_node.append_child(XML_TAPEOUT_CORNERMAP_NODE_NAME);
  corner_map_node.append_attribute(XML_TAPEOUT_CORNERMAP_ATTRIBUTE_DEFAULT) =
    to_mgr.default_corner().c_str();

  for (std::string tag : to_mgr.user_defined_corner_tags()) {
    pugi::xml_node corner_node =
      corner_map_node.append_child(XML_TAPEOUT_CORNERMAP_CORNER_NODE_NAME);
    corner_node.append_attribute(XML_TAPEOUT_CORNERMAP_CORNER_ATTRIBUTE_NAME) = tag.c_str();

    pugi::xml_node sclib_node =
      corner_node.append_child(XML_TAPEOUT_CORNERMAP_CORNER_SCLIB_NODE_NAME);
    sclib_node.text().set(
      to_mgr.user_defined_corner_tag(tag, TapeoutManager::e_corner_tag_type::SCLIB, true).c_str());

    pugi::xml_node spef_node =
      corner_node.append_child(XML_TAPEOUT_CORNERMAP_CORNER_SPEF_NODE_NAME);
    spef_node.text().set(
      to_mgr.user_defined_corner_tag(tag, TapeoutManager::e_corner_tag_type::SPEF, true).c_str());
  }

  for (std::string disabled_corner : to_mgr.disabled_corners()) {
    pugi::xml_node disabled_corner_node =
      corner_map_node.append_child(XML_TAPEOUT_CORNER_DISABLED_NODE_NAME);
    disabled_corner_node.text().set(disabled_corner.c_str());
  }

  return err_code;
}

static int write_xml_boundary_timing_files(pugi::xml_node& flset_node,
                                           const TapeoutManager& to_mgr) {
  int err_code = 0;
  pugi::xml_node bdy_timing_node = flset_node.append_child(XML_TAPEOUT_BOUNDARY_TIMING_NODE_NAME);
  for (auto corner : to_mgr.boundary_timing_corners()) {
    pugi::xml_node f_node =
      bdy_timing_node.append_child(XML_TAPEOUT_BOUNDARY_TIMING_FILE_NODE_NAME);
    f_node.append_attribute(XML_TAPEOUT_BOUNDARY_TIMING_FILE_ATTR_CORNER) = corner.c_str();
    f_node.text().set(to_mgr.boundary_timing_file(corner, true).c_str());
  }

  return err_code;
}

static int write_xml_arch_settings(pugi::xml_node& flset_node, const TapeoutManager& to_mgr) {
  int err_code = 0;

  ArchIoConstraints arch_io_constr;
  pugi::xml_node arch_settings_node = flset_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_NODE_NAME);
  /* Arch settings applied to each arch run */
  for (std::string& run_id : to_mgr.step_runs(TapeoutManager::e_step_name::ARCH)) {
    pugi::xml_node run_node =
      arch_settings_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_NODE_NAME);
    run_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_ID) = run_id.c_str();
    if (to_mgr.core_height(run_id)) {
      pugi::xml_node curr_node =
        run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_CORE_HEIGHT_NODE_NAME);
      curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
        std::to_string(to_mgr.core_height(run_id)).c_str();
    }
    if (to_mgr.core_width(run_id)) {
      pugi::xml_node curr_node =
        run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_CORE_WIDTH_NODE_NAME);
      curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
        std::to_string(to_mgr.core_width(run_id)).c_str();
    }
    if (to_mgr.sync_dp_dimension(run_id)) {
      pugi::xml_node curr_node =
        run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_SYNC_DP_DIMENSION_NODE_NAME);
      curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
        to_mgr.sync_dp_dimension(run_id) ? "true" : "false";
    }
    if (to_mgr.num_clocks(run_id)) {
      pugi::xml_node curr_node =
        run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_CLOCKS_NODE_NAME);
      curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
        std::to_string(to_mgr.num_clocks(run_id)).c_str();
    }
    if (to_mgr.num_resets(run_id)) {
      pugi::xml_node curr_node =
        run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_RESETS_NODE_NAME);
      curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
        std::to_string(to_mgr.num_resets(run_id)).c_str();
    }
    if (to_mgr.num_prog_clocks(run_id)) {
      pugi::xml_node curr_node =
        run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_PCLOCKS_NODE_NAME);
      curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
        std::to_string(to_mgr.num_prog_clocks(run_id)).c_str();
    }
    if (to_mgr.num_config_regions(run_id)) {
      pugi::xml_node curr_node =
        run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_CONFIG_REGIONS_NODE_NAME);
      curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
        std::to_string(to_mgr.num_config_regions(run_id)).c_str();
    }
    pugi::xml_node num_ios_node =
      run_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NODE_NAME);
    for (e_side curr_side : TOTAL_2D_SIDES) {
      for (auto curr_type : arch_io_constr.io_directions()) {
        if (-1 != to_mgr.io_num(run_id, curr_side, curr_type)) {
          pugi::xml_node curr_node =
            num_ios_node.append_child(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_NODE_NAME);
          curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_ATTRIBUTE_SIDE) =
            arch_io_constr.side2str(curr_side).c_str();
          curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_ATTRIBUTE_TYPE) =
            arch_io_constr.io_direction2str(curr_type).c_str();
          curr_node.append_attribute(XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE) =
            std::to_string(to_mgr.io_num(run_id, curr_side, curr_type)).c_str();
        }
      }
    }
  }

  return err_code;
}

/********************************************************************
 * A writer to output a strategy manager object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_tapeout_manager(const char* fname, const TapeoutManager& to_mgr,
                              const bool& keep_reserved_word) {
  vtr::ScopedStartFinishTimer timer("Write tapeout project data to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_TAPEOUT_ROOT_NODE_NAME);
  root_node.append_attribute(XML_TAPEOUT_ATTRIBUTE_NAME) = to_mgr.name().c_str();
  root_node.append_attribute(XML_TAPEOUT_ATTRIBUTE_FAMILY) = to_mgr.family().c_str();
  root_node.append_attribute(XML_TAPEOUT_ATTRIBUTE_PATH) = to_mgr.path().c_str();

  /* Output time stamp */
  err_code += write_xml_tapeout_stamps(root_node, to_mgr);

  /* Output pdk config path */
  pugi::xml_node pdk_node = root_node.append_child(XML_TAPEOUT_PDK_NODE_NAME);
  pdk_node.text().set(to_mgr.pdk_config(keep_reserved_word).c_str());

  /* Output benchmark config path */
  pugi::xml_node bm_node = root_node.append_child(XML_TAPEOUT_BENCHMARK_NODE_NAME);
  bm_node.append_attribute(XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT) =
    to_mgr.default_benchmark_name().c_str();
  bm_node.append_attribute(XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT_FOR_CLOCK_SWEEP) =
    to_mgr.default4clk_sweep_benchmark_name().c_str();
  bm_node.append_attribute(XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT_FOR_RESET_SWEEP) =
    to_mgr.default4rst_sweep_benchmark_name().c_str();
  for (std::string bm_file : to_mgr.benchmark_configs(keep_reserved_word)) {
    pugi::xml_node bm_file_node = bm_node.append_child(XML_TAPEOUT_BENCHMARK_FILE_NODE_NAME);
    bm_file_node.text().set(bm_file.c_str());
  }
  for (std::string bm_name : to_mgr.disabled_benchmarks()) {
    pugi::xml_node bm_disabled_node =
      bm_node.append_child(XML_TAPEOUT_BENCHMARK_DISABLED_NODE_NAME);
    bm_disabled_node.text().set(bm_name.c_str());
  }

  /* Output flow settings */
  err_code += write_xml_corner_map(root_node, to_mgr);

  err_code += write_xml_boundary_timing_files(root_node, to_mgr);

  /* Output arch template config path */
  pugi::xml_node arch_tmpl_node = root_node.append_child(XML_TAPEOUT_ARCH_TEMPLATE_FILE_NODE_NAME);
  arch_tmpl_node.text().set(to_mgr.arch_template_file(keep_reserved_word).c_str());

  /* Output arch settings */
  err_code += write_xml_arch_settings(root_node, to_mgr);

  /* Output steps */
  for (TapeoutManager::e_step_name curr_step : to_mgr.steps()) {
    pugi::xml_node step_node = root_node.append_child(XML_TAPEOUT_STEP_NODE_NAME);
    step_node.append_attribute(XML_TAPEOUT_STEP_ATTRIBUTE_NAME) =
      to_mgr.step_name2str(curr_step).c_str();
    /* Bypass those have no runs */
    if (to_mgr.step_runs(curr_step).empty()) {
      continue;
    }
    pugi::xml_node runs_node = step_node.append_child(XML_TAPEOUT_STEP_RUNS_NODE_NAME);
    runs_node.append_attribute(XML_TAPEOUT_STEP_RUNS_ATTRIBUTE_LATEST) =
      to_mgr.step_latest_run(curr_step).c_str();
    for (std::string& run_id : to_mgr.step_runs(curr_step)) {
      err_code += write_xml_step_run(runs_node, to_mgr, curr_step, run_id);
    }
  }

  // Find or create the benchmarks node to add constraint information to
  pugi::xml_node benchmarks_node = root_node.child("benchmarks");
  if (benchmarks_node.empty()) {
    benchmarks_node = root_node.append_child("benchmarks");
  }

  // Output constraint information - combine with pin utilization data to avoid duplicate status
  // nodes Group all status information by benchmark, run, and corner to avoid duplicate status
  // nodes
  std::map<std::tuple<std::string, std::string, std::string>, std::vector<std::string>>
    constraint_files_map;
  auto all_keys = to_mgr.get_all_constraint_keys();
  for (const auto& key : all_keys) {
    auto files = to_mgr.get_vpr_constraint_files(key.benchmark, key.run, key.corner);
    if (!files.empty()) {
      constraint_files_map[{key.benchmark, key.run, key.corner}] = files;
    }
  }

  // Group custom pin utilization data by benchmark, run, and corner
  std::map<std::tuple<std::string, std::string, std::string>,
           std::map<std::string, TapeoutManager::CustomPinUtilization>>
    pin_utilization_map;
  for (const auto& [benchmark_name, runs_map] : to_mgr.get_custom_pin_utilization_nested()) {
    for (const auto& [run_name, corners_map] : runs_map) {
      for (const auto& [corner_name, tile_utilization] : corners_map) {
        pin_utilization_map[{benchmark_name, run_name, corner_name}] =
          tile_utilization.tile_utilizations;
      }
    }
  }

  // Combine both types of data into single status nodes
  std::set<std::tuple<std::string, std::string, std::string>> all_combinations;

  // Add all combinations from constraint files
  for (const auto& [key, files] : constraint_files_map) {
    all_combinations.insert(key);
  }

  // Add all combinations from pin utilization data
  for (const auto& [key, util_data] : pin_utilization_map) {
    all_combinations.insert(key);
  }

  // Create status nodes combining both types of data
  for (const auto& key : all_combinations) {
    const std::string& benchmark_name = std::get<0>(key);
    const std::string& run_name = std::get<1>(key);
    const std::string& corner_name = std::get<2>(key);

    pugi::xml_node status_node = benchmarks_node.append_child("status");
    status_node.append_attribute("benchmark") = benchmark_name.c_str();
    status_node.append_attribute("run") = run_name.c_str();
    status_node.append_attribute("corner") = corner_name.c_str();

    // Add constraint files if they exist for this combination
    auto constraint_it = constraint_files_map.find(key);
    if (constraint_it != constraint_files_map.end()) {
      for (const std::string& file : constraint_it->second) {
        pugi::xml_node constraint_node = status_node.append_child("custom_vpr_constraint_file");

        std::filesystem::path file_path(file);
        std::string final_path = file;

        if (file_path.is_relative()) {
          std::filesystem::path abs_path = std::filesystem::absolute(file_path);
          final_path = abs_path.string();
        } else {
          final_path = file_path.lexically_normal().string();
        }

        constraint_node.text().set(final_path.c_str());
      }
    }

    // Add pin utilization data if it exists for this combination
    auto util_it = pin_utilization_map.find(key);
    if (util_it != pin_utilization_map.end()) {
      for (const auto& [tile_name, util_data] : util_it->second) {
        pugi::xml_node tile_node = status_node.append_child("tile");
        tile_node.append_attribute("name") = tile_name.c_str();

        pugi::xml_node input_util_node = tile_node.append_child("input_pin_util");
        input_util_node.text().set(std::to_string(util_data.input_pin_util).c_str());

        pugi::xml_node output_util_node = tile_node.append_child("output_pin_util");
        output_util_node.text().set(std::to_string(util_data.output_pin_util).c_str());
      }
    }
  }

  out_xml.save_file(fname);

  return err_code;
}

}  // End of namespace arcxpress
