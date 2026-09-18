#pragma once

#include <array>

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

/* eFPGA architecture template file syntax */
constexpr const char* XML_TAPEOUT_ARCH_ROOT_NODE_NAME = "efpga_arch_template";

constexpr const char* XML_TAPEOUT_FLOWSET_NODE_NAME = "flow_settings";
constexpr const char* XML_TAPEOUT_FLOWSET_SYNTH_NODE_NAME = "synth";
constexpr const char* XML_TAPEOUT_FLOWSET_SYNTH_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_FLOWSET_SYNTH_PDK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_FLOWSET_SYNTH_SDC_NODE_NAME = "sdc";
constexpr const char* XML_TAPEOUT_FLOWSET_SYNTH_STRATEGY_NODE_NAME = "strategy";
constexpr const char* XML_TAPEOUT_FLOWSET_FLOORPLAN_NODE_NAME = "floorplan";
constexpr const char* XML_TAPEOUT_FLOWSET_FLOORPLAN_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_FLOWSET_FLOORPLAN_PDK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_FLOWSET_FLOORPLAN_STRATEGY_NODE_NAME = "strategy";
constexpr const char* XML_TAPEOUT_FLOWSET_CCFFMAXFANOUT_NODE_NAME = "ccff_max_fanout";
constexpr const char* XML_TAPEOUT_FLOWSET_QORSO_NODE_NAME = "qorso";
constexpr const char* XML_TAPEOUT_FLOWSET_QORSO_TASK_NODE_NAME = "task";
constexpr const char* XML_TAPEOUT_FLOWSET_QORSO_TASK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_FLOWSET_QORSO_TASK_ATTRIBUTE_TYPE = "type";
constexpr const char* XML_TAPEOUT_FLOWSET_QORSO_TASK_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_FLOWSET_QORSO_TASK_PDK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_FLOWSET_QORSO_TASK_CONFIG_NODE_NAME = "config";
constexpr const char* XML_TAPEOUT_NETLIST_FLAGS_NODE_NAME = "netlist_flags";
constexpr const char* XML_TAPEOUT_NETLIST_FLAGS_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_NETLIST_FLAGS_PDK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_NETLIST_FLAGS_FLAG_NODE_NAME = "flag";
constexpr const char* XML_TAPEOUT_NETLIST_FLAGS_FLAG_ATTRIBUTE_NAME = "name";

constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_NODE_NAME = "builtin_corner_map";
constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_PDK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_PDK_ATTRIBUTE_DEFAULT = "default";
constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_NODE_NAME = "corner";
constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_SCLIB_NODE_NAME = "sclib";
constexpr const char* XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_SPEF_NODE_NAME = "spef";

constexpr const char* XML_TAPEOUT_ARCH_NODE_NAME = "arch_template";
constexpr const char* XML_TAPEOUT_ARCH_ATTRIBUTE_W = "route_chan_width";
constexpr const char* XML_TAPEOUT_ARCH_ARCH_GENERATOR_NODE_NAME = "arch_generator";
constexpr const char* XML_TAPEOUT_ARCH_TIMING_NODE_NAME = "arch_timing";
constexpr const char* XML_TAPEOUT_ARCH_TIMING_ATTRIBUTE_PDK_TEMPLATE = "pdk_template";
constexpr const char* XML_TAPEOUT_ARCH_TIMING_ATTRIBUTE_FILE_TEMPLATE = "file_template";
constexpr const char* XML_TAPEOUT_ARCH_TIMING_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_ARCH_TIMING_PDK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_ARCH_DP_NODE_NAME = "dp";
constexpr const char* XML_TAPEOUT_ARCH_ULTIMATE_NODE_NAME = "ultimate";
constexpr const char* XML_TAPEOUT_ARCH_OPENFPGASETTING_NODE_NAME = "openfpga_settings";
constexpr const char* XML_TAPEOUT_ARCH_OPENFPGASCRIPT_NODE_NAME = "openfpga_shell_scripts";
constexpr const char* XML_TAPEOUT_ARCH_YOSYSSCRIPT_NODE_NAME = "yosys_scripts";
constexpr const char* XML_TAPEOUT_ARCH_IP_NODE_NAME = "ips";
constexpr const char* XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_NODE_NAME = "netlist_makeup_key";
constexpr const char* XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_ATTRIBUTE_ROOT = "root";
constexpr const char* XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_NODE_NAME = "pdk";
constexpr const char* XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_NETLIST_MAKEUP_CONFIG_NODE_NAME =
  "netlist_makeup_config";
constexpr const char* XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_CCFF_MAKEUP_BUF_CONFIG_NODE_NAME =
  "ccff_makeup_buf_config";
constexpr const char* XML_TAPEOUT_ARCH_FILE_NODE_NAME = "file";
constexpr const char* XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE = "type";
constexpr const char* XML_TAPEOUT_ARCH_ATTRIBUTE_FORMAT = "format";

constexpr const char* XML_TAPEOUT_ARCH_OPENFPGA_STATS_CONF_NODE_NAME = "openfpga_stats_conf";
constexpr const char* XML_TAPEOUT_ARCH_OPENFPGA_ERROR_CONF_NODE_NAME = "openfpga_error_conf";
constexpr const char* XML_TAPEOUT_ARCH_YOSYS_ERROR_CONF_NODE_NAME = "yosys_error_conf";

constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_NODE_NAME = "io_settings";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_NODE_NAME = "num_io_per_tile";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_TYPE = "type";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_SIDE = "side";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_MSB = "msb";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_MAX = "max";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_IO_TYPE_NODE_NAME = "io_type";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_IO_TYPE_ATTRIBUTE_NAME = "name";
constexpr const char* XML_TAPEOUT_ARCH_IO_SETTINGS_IO_TYPE_ATTRIBUTE_SIDE = "side";

}  // End of namespace arkangel
