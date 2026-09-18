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

int floorplan_io_physical_location(const FloorplanStrategy& flrpln_strtg,
                                   const std::string& io_coord_fpath,
                                   const std::string& io_metal_layer, TileQor& tile_qor,
                                   const bool& verbose);

int floorplan_tile_pin_physical_location(const FloorplanStrategy& flrpln_strtg,
                                         const std::string& tile_pinloc_fpath,
                                         const std::string& tile_pin_metal_layer, TileQor& tile_qor,
                                         const bool& verbose);

}  // End of namespace arkangel
