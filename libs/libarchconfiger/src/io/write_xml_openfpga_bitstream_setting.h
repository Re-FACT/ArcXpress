#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arcxpress {  // Begin namespace arcxpress

int write_xml_openfpga_bitstream_setting(const char* fname, const ArchGen& arch_gen);

}  // End of namespace arcxpress
