#pragma once

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

constexpr const char* XML_QORSOSTRATEGY_ROOT_NODE_NAME = "qorso_strategies";

constexpr const char* XML_QORSOSTRATEGY_NODE_GLOBAL_VAR = "global_variables";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_NODE_SWEEP = "sweep";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_NAME = "name";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_START = "start";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_END = "end";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_STEP = "step";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_NODE_CONSTANT = "constant";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_CONSTANT_ATTR_NAME = "name";
constexpr const char* XML_QORSOSTRATEGY_GLOBAL_VAR_CONSTANT_ATTR_VALUE = "value";

constexpr const char* XML_QORSOSTRATEGY_NODE_STRATEGY = "strategy";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_ATTR_NAME = "name";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_ATTR_TYPE = "type";

constexpr const char* XML_QORSOSTRATEGY_STRATEGY_NODE_SUMMARY = "summary";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_SUMMARY_ATTR_TIMING_DATA = "timing_data";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_SUMMARY_ATTR_DATA_TYPE = "data_type";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_SUMMARY_ATTR_SCALE_FACTOR = "scale_factor";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_NODE_VARIABLE = "variable";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_VARIABLE_ATTR_NAME = "name";

constexpr const char* XML_QORSOSTRATEGY_STRATEGY_NODE_TASK = "task";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_ATTR_NAME = "name";

constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_DESIGNS = "designs";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_DESIGNS_ATTR_AUTO_INFER_FROM_TILE_TYPE =
  "auto_infer_from_tile_type";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_DESIGNS_NODE_DESIGN = "design";

/* Timing task */
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_PATH = "path";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_MAX_DELAY = "max_delay";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_MIN_DELAY = "min_delay";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_DELAY_TYPE = "delay_type";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_CLOCK_GROUP = "clock_group";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_NWORST = "nworst";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_MAX_PATHS = "max_paths";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_FROM_PINS = "from_pins";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_TO_PINS = "to_pins";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_THRU_PINS = "thru_pins";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_PIN = "pin";

/* Power task */
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_METHOD = "method";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_APPVAR = "app_variables";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_SDCVAR = "sdc_variables";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_REPORT_COMMAND = "report_command";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_REPORT_COMMAND_ATTR_REMOVE_TIME_STAMP =
  "remove_time_stamp";

constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_SDC = "sdc";
constexpr const char* XML_QORSOSTRATEGY_STRATEGY_TASK_SDC_NODE_FILE = "file";

}  // End of namespace arkangel
