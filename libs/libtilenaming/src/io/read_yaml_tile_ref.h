#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "tile_ref.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int read_yaml_tile_ref(const char* fname, TileRef& tile_ref, const bool& verbose);

}  // End of namespace arcxpress
