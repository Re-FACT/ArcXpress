#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "fabric_synth_strategy.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int read_xml_synth_strategy(const char* fname, FabricSynthStrategy& syn_strtg, const bool& verbose);

}  // End of namespace arcxpress
