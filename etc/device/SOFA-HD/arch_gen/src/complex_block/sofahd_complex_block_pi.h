#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace SofaHD {  // Begin namespace SofaHD

int assemble_pi_complex_block(arcxpress::ArchGen& arch_gen, const bool& verbose);

}  // namespace SofaHD
