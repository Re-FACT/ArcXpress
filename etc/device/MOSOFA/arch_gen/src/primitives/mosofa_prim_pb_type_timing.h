#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace SofaHD {  // Begin namespace SofaHD

int add_primitive_pb_type_timing(arcxpress::ArchGen& arch_gen, const bool& verbose);

}  // namespace SofaHD
