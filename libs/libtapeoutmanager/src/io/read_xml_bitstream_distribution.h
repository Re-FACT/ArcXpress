#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "bitstream_distribution.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int read_xml_bitstream_distribution(const std::string& fname, BitstreamDistribution& bit_dist);

}  // End of namespace arkangel
