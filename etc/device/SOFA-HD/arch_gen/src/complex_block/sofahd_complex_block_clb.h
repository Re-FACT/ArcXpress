#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace alkaid {  // Begin namespace alkaidC

int assemble_clb_complex_block(arkangel::ArchGen& arch_gen, const bool& verbose);

}  // namespace alkaid
