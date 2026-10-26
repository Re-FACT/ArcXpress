#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "floorplan_strategy.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int read_xml_floorplan_strategy(const char* fname, FloorplanStrategy& flrpln_strtg);

}  // End of namespace arcxpress
