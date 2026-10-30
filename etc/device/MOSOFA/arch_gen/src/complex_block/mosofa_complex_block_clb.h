#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"
#include "mosofa_arch_planner.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int assemble_clb_complex_block(arcxpress::ArchGen& arch_gen, 
                               const ArchPlanner& arch_planner,
                               const bool& verbose);

}  // namespace MOSOFA
