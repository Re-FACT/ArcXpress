#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "tile_ref.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int read_yaml_tile_ref(const char* fname, TileRef& tile_ref, const bool& verbose);

}  // End of namespace arkangel
