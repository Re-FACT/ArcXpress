#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "io_coord_data.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int read_xml_io_coordinates(const char* fname, IoCoordData& io_coord, const bool& verbose);

}  // End of namespace arkangel
