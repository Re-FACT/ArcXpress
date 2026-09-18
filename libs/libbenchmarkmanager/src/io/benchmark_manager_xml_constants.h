#ifndef BENCHMARK_MANAGER_XML_CONSTANTS_H
#define BENCHMARK_MANAGER_XML_CONSTANTS_H

#include <array>

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

constexpr const char* XML_SUITE_ROOT_NODE_NAME = "benchmark_suite";

constexpr const char* XML_SUITE_STAMP_ROOT_NODE_NAME = "stamp";
constexpr const char* XML_SUITE_AUTHOR_NODE_NAME = "author";
constexpr const char* XML_SUITE_DATE_NODE_NAME = "dates";
constexpr const char* XML_SUITE_CREATE_DATE_NODE_NAME = "created";
constexpr const char* XML_SUITE_LAST_MODIFIED_DATE_NODE_NAME = "last_modified";
constexpr const char* XML_SUITE_TOOL_VERSION_NODE_NAME = "tool_version";

constexpr const char* XML_SUITE_BENCHMARK_ROOT_NODE_NAME = "benchmark";
constexpr const char* XML_SUITE_BENCHMARK_ATTRIBUTE_NAME = "name";
constexpr const char* XML_SUITE_BENCHMARK_ATTRIBUTE_SUITE = "suite";
constexpr const char* XML_SUITE_BENCHMARK_TOPMODULE_NODE_NAME = "top_module";
constexpr const char* XML_SUITE_BENCHMARK_DESIGN_NODE_NAME = "design";
constexpr const char* XML_SUITE_BENCHMARK_COCOTB_NODE_NAME = "cocotb";
constexpr const char* XML_SUITE_BENCHMARK_CONSTRAINTS_NODE_NAME = "constraints";

/* Common syntax used by design, cocotb and constraints */
constexpr const char* XML_SUITE_BENCHMARK_FILE_NODE_NAME = "file";
constexpr const char* XML_SUITE_BENCHMARK_FILE_ATTRIBUTE_TYPE = "type";

}  // End of namespace arkangel

#endif
