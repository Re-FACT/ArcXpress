#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "tapeout_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int read_xml_tapeout_manager(const char* fname, TapeoutManager& to_mgr);

}  // End of namespace arcxpress
