#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "vpr_arch_layout.h"
#include "sofahd_arch_planner.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace SofaHD {  // Begin namespace SofaHD

int add_clb_tiles_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                    const SofaHD::ArchPlanner& arch_plan);

}  // End of namespace SofaHD
