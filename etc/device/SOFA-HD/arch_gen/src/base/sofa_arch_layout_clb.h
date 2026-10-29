#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace Sofa {  // Begin namespace Sofa

int add_clb_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout);

int add_ckbuf_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout);

int add_clb_clb_tile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                        const std::string& tile_name);

}  // End of namespace Sofa
