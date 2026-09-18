#pragma once

#include <array>

/* Constants required by XML parser */
// <vpr_constraints>
//   <partition_list>
//      <partition name=”int_clk_ckbuf”>

//          <add_region xlow=”0” ylow=”1” x_high=”0” yhigh=”1” subtile=”5”/>

//          <add_atom name_pattern=”int_clk_ckbuf”/>

//      </partition>
//    </partition_list>
// </vpr_constraints>

namespace arkangel {  // Begin namespace arkangel
// For final vpr constriant file
constexpr const char* XML_VPR_CONSTRAINT_FILE_ROOT_NAME = "vpr_constraints";
constexpr const char* XML_VPR_CONSTRAINT_FILE_SUBROOT_NAME = "partition_list";
constexpr const char* XML_VPR_CONSTRAINT_FILE_SUBROOT_GLOBAL_ROUTE_CONSTRAINTS_NAME =
  "global_route_constraints";
constexpr const char* XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL = "set_global_signal";
constexpr const char* XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_NAME = "name";
constexpr const char* XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_ROUTE_MODEL = "route_model";
constexpr const char* XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_ROUTE_MODEL_IDEAL = "ideal";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE = "partition";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_NAME = "name";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION = "add_region";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_X_LOW = "x_low";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_X_HIGH = "x_high";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_Y_LOW = "y_low";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_Y_HIGH = "y_high";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_SUBTILE = "subtile";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM = "add_atom";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM_NAME_PATTERN = "name_pattern";
constexpr const char* XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM_IS_REGEX = "is_regex";
constexpr const char* XML_VPR_CONSTRAINT_FILE_BOOL_TRUE = "true";
constexpr const char* XML_VPR_CONSTRAINT_FILE_BOOL_FALSE = "false";

// For cell map
constexpr const char* XML_CELL_MAP_FILE_INSTANCE_ROOT = "ckbuf_cell_map";
constexpr const char* XML_CELL_MAP_FILE_INSTANCE_NODE = "ckbuf";
constexpr const char* XML_CELL_MAP_FILE_INSTANCE_NODE_INPUT_NET = "input_net";
constexpr const char* XML_CELL_MAP_FILE_INSTANCE_NODE_CELL = "cell";
constexpr const char* XML_CELL_MAP_FILE_INSTANCE_NODE_TYPE = "type";

// For clock location map
constexpr const char* XML_CLK_LOC_MAP_FILE_INSTANCE_ROOT = "ckbuf_location_map";
constexpr const char* XML_CLK_LOC_MAP_FILE_INSTANCE_NODE = "ckbuf";
constexpr const char* XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_PIN = "pin";
constexpr const char* XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_TYPE = "type";
constexpr const char* XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_X = "x";
constexpr const char* XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_Y = "y";
constexpr const char* XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_Z = "z";

}  // End of namespace arkangel
