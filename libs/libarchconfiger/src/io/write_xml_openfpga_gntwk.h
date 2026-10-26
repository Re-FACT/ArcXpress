#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"
#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arcxpress {  // Begin namespace arcxpress

int write_xml_openfpga_gntwk(const char* fname, const ArchGen& arch_gen,
                             const VprArchLayout::e_fabric_type& fb_type);

}  // End of namespace arcxpress
