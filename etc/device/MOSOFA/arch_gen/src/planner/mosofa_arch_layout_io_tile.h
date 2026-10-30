#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_arch_layout.h"
#include "mosofa_arch_planner.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int add_io_tiles_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                    const ArchPlanner& arch_plan);

}  // End of namespace MOSOFA
