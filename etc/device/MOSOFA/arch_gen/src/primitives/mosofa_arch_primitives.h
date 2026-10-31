#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int create_vpr_arch_primitives(arcxpress::ArchGen& arch_gen);

}  // End of namespace MOSOFA
