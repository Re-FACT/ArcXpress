#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "floorplan_strategy.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int read_xml_floorplan_strategy(const char* fname, FloorplanStrategy& flrpln_strtg);

}  // End of namespace arkangel
