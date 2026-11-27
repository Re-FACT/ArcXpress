#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

int assemble_ckbuf_complex_block(arkangel::ArchGen& arch_gen, const bool& verbose);

}  // end namespace MOSOFA
