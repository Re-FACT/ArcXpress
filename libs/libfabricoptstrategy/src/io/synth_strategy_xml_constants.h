#pragma once

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

constexpr const char* XML_SYNTHSTRATEGY_ROOT_NODE_NAME = "synth_strategies";

constexpr const char* XML_SYNTHSTRATEGY_NODE_STRATEGY = "strategy";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_ATTR_NAME = "name";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_ATTR_DEFAULT = "default";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_NODE_DESIGNS = "designs";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_DESIGNS_NODE_PATTERN = "pattern";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_NODE_TARGETLIB = "target_lib";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_TARGETLIB_NODE_LIB = "lib";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_NODE_COMPILE = "compile";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_COMPILE_ATTR_TYPE = "type";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_COMPILE_ATTR_OPTIMIZATION = "optimization";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_COMPILE_ATTR_EFFORT = "effort";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_NODE_SDC = "sdc";
constexpr const char* XML_SYNTHSTRATEGY_STRATEGY_SDC_NODE_FILE = "file";

}  // End of namespace arkangel
