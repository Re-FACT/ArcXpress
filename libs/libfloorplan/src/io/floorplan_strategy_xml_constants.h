#pragma once

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

constexpr const char* XML_FLOORPLANSTRATEGY_ROOT_NODE_NAME = "floorplan_strategies";
constexpr const char* XML_FLOORPLANSTRATEGY_NODE_BOUNDARY_CELL_WIDTH = "boundary_cell_width";
constexpr const char* XML_FLOORPLANSTRATEGY_NODE_HEIGHT_WIDTH_RATIO = "height_width_ratio";
constexpr const char* XML_FLOORPLANSTRATEGY_NODE_TILE_UTILIZATION = "tile_utilization";
constexpr const char* XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_ATTR_DEFAULT = "default";
constexpr const char* XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_NODE_TILE = "tile";
constexpr const char* XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_TILE_ATTR_NAME = "name";
constexpr const char* XML_FLOORPLANSTRATEGY_NODE_TILE_GAP = "tile_gap";
constexpr const char* XML_FLOORPLANSTRATEGY_TILE_GAP_NODE_HORIZENTAL = "horizental";
constexpr const char* XML_FLOORPLANSTRATEGY_TILE_GAP_NODE_VERTICAL = "vertical";
constexpr const char* XML_FLOORPLANSTRATEGY_NODE_SIDE_GAP = "side_gap";
// Metal stacks
constexpr const char* XML_FLOORPLANSTRATEGY_NODE_METAL_STACK = "metal_stack";
constexpr const char* XML_FLOORPLANSTRATEGY_METAL_STACK_NODE_LAYER = "layer";
constexpr const char* XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_NAME = "name";
constexpr const char* XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_LENGTH = "length";
constexpr const char* XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_WIDTH = "width";
constexpr const char* XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_SPACING = "spacing";
// Signal distribution network
constexpr const char* XML_FLOORPLANSTRATEGY_NODE_SIGNAL_DISTRIBUTION_NETWORK =
  "signal_distribution_network";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NODE_NETWORK = "network";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_PORT = "port";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_TYPE = "type";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_BUFCELL =
  "bufcell";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_DEPTH =
  "depth";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_NODE_ENTRY =
  "entry";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ENTRY_ATTR_X = "x";
constexpr const char* XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ENTRY_ATTR_Y = "y";

}  // End of namespace arkangel
