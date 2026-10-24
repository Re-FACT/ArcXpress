#pragma once

/* Constants required by XML parser */

namespace arcxpress {  // Begin namespace arcxpress

constexpr const char* XML_PINTABLENAMERULES_ROOT_NODE_NAME = "pin_table_naming_rules";

constexpr const char* XML_PINTABLENAMERULES_NODE_SIDERULE = "side_rule";

constexpr const char* XML_SIDERULE_ATTR_SIDE = "side";
constexpr const char* XML_SIDERULE_ATTR_STARTX = "startx";
constexpr const char* XML_SIDERULE_ATTR_STARTY = "starty";
constexpr const char* XML_SIDERULE_ATTR_ENDX = "endx";
constexpr const char* XML_SIDERULE_ATTR_ENDY = "endy";

}  // End of namespace arcxpress
