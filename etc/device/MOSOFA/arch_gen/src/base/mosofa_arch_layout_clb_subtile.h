#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "vpr_arch_layout.h"
#include "mosofa_arch_planner.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int add_clb_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                       const ArchPlanner& arch_planner);

int add_clb_subtile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                       const ArchPlanner& arch_planner,
                                       const std::string& tile_name);

}  // End of namespace MOSOFA
