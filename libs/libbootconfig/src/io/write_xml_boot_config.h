#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "boot_config.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arcxpress {  // Begin namespace arcxpress

int write_xml_boot_config(const char* fname, const BootConfig& boot_cfg);

}  // End of namespace arcxpress
