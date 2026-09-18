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

int floorplan_signal_distribution_network(const FloorplanStrategy& flrpln_strtg, TileQor& tile_qor,
                                          const bool& verbose);
}  // End of namespace arkangel
