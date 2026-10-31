#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int add_pi_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout);

int add_po_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout);

}  // namespace MOSOFA
