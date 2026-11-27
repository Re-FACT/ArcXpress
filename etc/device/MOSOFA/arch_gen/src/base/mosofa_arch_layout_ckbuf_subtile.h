#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "mosofa_arch_planner.h"
#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int add_ckbuf_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout);

int add_ckbuf_subtile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                       const ArchPlanner& arch_planner,
                                       const std::string& tile_name);

}  // End of namespace MOSOFA
