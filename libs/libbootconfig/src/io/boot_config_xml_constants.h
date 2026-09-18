#pragma once

#include <array>

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

constexpr const char* XML_BOOTCONFIG_ROOT_NODE_NAME = "boot_config";

constexpr const char* XML_BOOTCONFIG_STAMP_ROOT_NODE_NAME = "stamp";
constexpr const char* XML_BOOTCONFIG_AUTHOR_NODE_NAME = "author";
constexpr const char* XML_BOOTCONFIG_DATE_NODE_NAME = "dates";
constexpr const char* XML_BOOTCONFIG_CREATE_DATE_NODE_NAME = "created";
constexpr const char* XML_BOOTCONFIG_LAST_MODIFIED_DATE_NODE_NAME = "last_modified";
constexpr const char* XML_BOOTCONFIG_TOOL_VERSION_NODE_NAME = "tool_version";

constexpr const char* XML_BOOTCONFIG_BOOT_TYPE_NODE_NAME = "boot_type";
constexpr const char* XML_BOOTCONFIG_BIN_ROOT_NODE_NAME = "bin_relpaths";

/* Common syntax used by design, cocotb and constraints */
constexpr const char* XML_BOOTCONFIG_FILE_NODE_NAME = "file";
constexpr const char* XML_BOOTCONFIG_FILE_ATTRIBUTE_TYPE = "type";

}  // End of namespace arkangel
