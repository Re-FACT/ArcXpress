#pragma once

#include <array>

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

/* Project file syntax */
constexpr const char* XML_TAPEOUT_ROOT_NODE_NAME = "tapeout";
constexpr const char* XML_TAPEOUT_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_ATTRIBUTE_FAMILY = "family";
constexpr const char* XML_TAPEOUT_ATTRIBUTE_PATH = "path";

constexpr const char* XML_TAPEOUT_STAMP_ROOT_NODE_NAME = "stamp";
constexpr const char* XML_TAPEOUT_AUTHOR_NODE_NAME = "author";
constexpr const char* XML_TAPEOUT_DATE_NODE_NAME = "dates";
constexpr const char* XML_TAPEOUT_CREATE_DATE_NODE_NAME = "created";
constexpr const char* XML_TAPEOUT_LAST_MODIFIED_DATE_NODE_NAME = "last_modified";
constexpr const char* XML_TAPEOUT_TOOL_VERSION_NODE_NAME = "tool_version";

constexpr const char* XML_TAPEOUT_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_BENCHMARK_NODE_NAME = "benchmarks";
constexpr const char* XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT = "default";
constexpr const char* XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT_FOR_CLOCK_SWEEP =
  "default_for_clock_sweep";
constexpr const char* XML_TAPEOUT_BENCHMARK_ATTRIBUTE_DEFAULT_FOR_RESET_SWEEP =
  "default_for_reset_sweep";
constexpr const char* XML_TAPEOUT_BENCHMARK_FILE_NODE_NAME = "file";
constexpr const char* XML_TAPEOUT_BENCHMARK_DISABLED_NODE_NAME = "disabled";

constexpr const char* XML_TAPEOUT_CORNERMAP_NODE_NAME = "corner_map";
constexpr const char* XML_TAPEOUT_CORNERMAP_ATTRIBUTE_DEFAULT = "default";
constexpr const char* XML_TAPEOUT_CORNERMAP_CORNER_NODE_NAME = "corner";
constexpr const char* XML_TAPEOUT_CORNERMAP_CORNER_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_CORNERMAP_CORNER_SCLIB_NODE_NAME = "sclib";
constexpr const char* XML_TAPEOUT_CORNERMAP_CORNER_SPEF_NODE_NAME = "spef";
constexpr const char* XML_TAPEOUT_CORNER_DISABLED_NODE_NAME = "disabled";

constexpr const char* XML_TAPEOUT_BOUNDARY_TIMING_NODE_NAME = "boundary_timing";
constexpr const char* XML_TAPEOUT_BOUNDARY_TIMING_FILE_NODE_NAME = "file";
constexpr const char* XML_TAPEOUT_BOUNDARY_TIMING_FILE_ATTR_CORNER = "corner";

constexpr const char* XML_TAPEOUT_ARCH_TEMPLATE_FILE_NODE_NAME = "arch_template_file";

constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_NODE_NAME = "arch_settings";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NODE_NAME = "run";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_ID = "id";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_ATTRIBUTE_VALUE = "value";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_CORE_HEIGHT_NODE_NAME = "core_height";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_CORE_WIDTH_NODE_NAME = "core_width";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_SYNC_DP_DIMENSION_NODE_NAME =
  "sync_dp_dimension";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_CLOCKS_NODE_NAME = "num_clocks";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_RESETS_NODE_NAME = "num_resets";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_PCLOCKS_NODE_NAME = "num_pclocks";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_CONFIG_REGIONS_NODE_NAME =
  "num_config_regions";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NODE_NAME = "num_ios";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_NODE_NAME = "num_io";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_ATTRIBUTE_SIDE = "side";
constexpr const char* XML_TAPEOUT_ARCH_SETTINGS_RUN_NUM_IOS_NUM_IO_ATTRIBUTE_TYPE = "type";

constexpr const char* XML_TAPEOUT_STEP_NODE_NAME = "step";
constexpr const char* XML_TAPEOUT_STEP_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_STEP_RUNS_NODE_NAME = "runs";
constexpr const char* XML_TAPEOUT_STEP_RUNS_ATTRIBUTE_LATEST = "latest_id";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_NODE_NAME = "run";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_ID = "id";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_STATUS = "status";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_ATTRIBUTE_FROZEN = "frozen";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_NODE_NAME = "substep";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_SUBSTEP_ATTRIBUTE_STATUS = "status";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_DEP_NODE_NAME = "dep";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_DEP_ATTRIBUTE_STEP = "step";
constexpr const char* XML_TAPEOUT_STEP_RUNS_RUN_DEP_ATTRIBUTE_RUN = "run";

// Add constants for status and constraint file nodes
constexpr const char* XML_TAPEOUT_BENCHMARK_STATUS_NODE_NAME = "status";
constexpr const char* XML_TAPEOUT_BENCHMARK_STATUS_ATTRIBUTE_BENCHMARK = "benchmark";
constexpr const char* XML_TAPEOUT_BENCHMARK_STATUS_ATTRIBUTE_RUN = "run";
constexpr const char* XML_TAPEOUT_BENCHMARK_STATUS_ATTRIBUTE_CORNER = "corner";
constexpr const char* XML_TAPEOUT_CUSTOM_VPR_CONSTRAINT_FILE_NODE_NAME =
  "custom_vpr_constraint_file";
}  // End of namespace arkangel
