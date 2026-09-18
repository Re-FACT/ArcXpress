#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "floorplan_strategy.h"
#include "tile_qor.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int run_floorplan_flow(const FloorplanStrategy& flrpln_strtg, TileQor& tile_qor,
                       const std::string& dom_tile_name, const bool& verbose);
}  // End of namespace arkangel
