#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "mosofa_arch_planner.h"
#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int add_io_tiles_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                    const ArchPlanner& arch_plan);

}  // End of namespace MOSOFA
