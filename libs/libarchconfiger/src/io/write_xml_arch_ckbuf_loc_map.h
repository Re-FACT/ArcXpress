#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"
#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

int write_xml_arch_ckbuf_loc_map(const char* fname, const ArchGen& arch_gen,
                                 const VprArchLayout::e_fabric_type& fb_type);

}  // End of namespace arkangel
