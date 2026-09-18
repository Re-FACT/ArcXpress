#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "tile_ref.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int read_yaml_tile_types(const char* fname, TileRef& tile_ref, const bool& verbose);

int read_yaml_tile_subblocks(const char* fname, std::vector<std::string>& subblks,
                             const bool& verbose);

}  // End of namespace arkangel
