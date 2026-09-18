#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "tapeout_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int read_xml_tapeout_manager_arch_template(const char* fname, TapeoutManager& to_mgr);

}  // End of namespace arkangel
