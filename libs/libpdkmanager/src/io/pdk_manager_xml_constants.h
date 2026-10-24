#ifndef PDK_MANAGER_XML_CONSTANTS_H
#define PDK_MANAGER_XML_CONSTANTS_H

#include <array>

/* Constants required by XML parser */

namespace arcxpress {  // Begin namespace arcxpress

constexpr const char* XML_PDK_ROOT_NODE_NAME = "pdk";
constexpr const char* XML_PDK_ATTRIBUTE_NAME = "name";

constexpr const char* XML_PDK_STAMP_ROOT_NODE_NAME = "stamp";
constexpr const char* XML_PDK_AUTHOR_NODE_NAME = "author";
constexpr const char* XML_PDK_DATE_NODE_NAME = "dates";
constexpr const char* XML_PDK_CREATE_DATE_NODE_NAME = "created";
constexpr const char* XML_PDK_LAST_MODIFIED_DATE_NODE_NAME = "last_modified";
constexpr const char* XML_PDK_TOOL_VERSION_NODE_NAME = "tool_version";

constexpr const char* XML_PDK_NODE_ROOT = "root";
constexpr const char* XML_PDK_NODE_SHRINK_FACTOR = "shrink_factor";
constexpr const char* XML_PDK_NODE_SCLIBS = "sclibs";
constexpr const char* XML_PDK_SCLIBS_NODE_SCLIB = "sclib";
constexpr const char* XML_PDK_SCLIB_ATTRIBUTE_NAME = "name";
constexpr const char* XML_PDK_SCLIB_ATTRIBUTE_HEIGHT = "height";
constexpr const char* XML_PDK_SCLIB_ATTRIBUTE_CPP = "cpp";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_PROCESS = "process";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_VDD = "vdd";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_TEMP = "temp";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_MODEL = "model";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_TAG = "tag";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_METALSTACK = "metal_stack";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_CORNER = "corner";

constexpr const char* XML_PDK_NODE_TECHFILE = "techfile";

constexpr const char* XML_PDK_NODE_DESIGN = "design";
constexpr const char* XML_PDK_NODE_DESIGN_TB_MODULE_POSTFIX = "testbench_module_postfix";
constexpr const char* XML_PDK_NODE_DESIGN_DUT_MODULE_POSTFIX = "dut_module_postfix";

constexpr const char* XML_PDK_NODE_RCCORNERS = "rc_corners";
constexpr const char* XML_PDK_NODE_RCCORNERS_CORNER = "rc_corner";
constexpr const char* XML_PDK_NODE_RCCORNERS_CORNER_ATTRIBUTE_TAG = "tag";

/* Common syntax used by design, cocotb and constraints */
constexpr const char* XML_PDK_FILE_NODE_NAME = "file";
constexpr const char* XML_PDK_FILE_ATTRIBUTE_TYPE = "type";

}  // End of namespace arcxpress

#endif
