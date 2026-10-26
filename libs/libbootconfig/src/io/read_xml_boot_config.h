#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "boot_config.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int read_xml_boot_config(const char* fname, BootConfig& boot_cfg);

}  // End of namespace arcxpress
