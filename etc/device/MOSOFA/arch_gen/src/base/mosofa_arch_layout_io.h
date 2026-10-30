#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace Sofa {  // Begin namespace Sofa

int add_pio_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout);

}  // namespace Sofa
