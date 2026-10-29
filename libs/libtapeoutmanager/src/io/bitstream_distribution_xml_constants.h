#pragma once

#include <array>

/* Constants required by XML parser */

namespace arcxpress {  // Begin namespace arcxpress

/* Project file syntax */
constexpr const char* XML_BITSTREAM_DISTRIBUTION_ROOT_NODE_NAME = "bitstream_distribution";
constexpr const char* XML_BITSTREAM_DISTRIBUTION_NODE_REGIONS = "regions";
constexpr const char* XML_BITSTREAM_DISTRIBUTION_NODE_BLOCKS = "blocks";
constexpr const char* XML_BITSTREAM_DISTRIBUTION_NODE_REGION = "region";
constexpr const char* XML_BITSTREAM_DISTRIBUTION_REGION_ATTR_ID = "id";
constexpr const char* XML_BITSTREAM_DISTRIBUTION_REGION_ATTR_NUM_OF_BITS = "number_of_bits";
constexpr const char* XML_BITSTREAM_DISTRIBUTION_NODE_BLOCK = "block";
constexpr const char* XML_BITSTREAM_DISTRIBUTION_BLOCK_ATTR_NUM_OF_BITS = "number_of_bits";

}  // End of namespace arcxpress
