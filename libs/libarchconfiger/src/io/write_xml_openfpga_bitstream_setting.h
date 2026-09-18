#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

int write_xml_openfpga_bitstream_setting(const char* fname, const ArchGen& arch_gen);

}  // End of namespace arkangel
