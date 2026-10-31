#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"
#include "arch_gen_input_params.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

/* pb_type name, circuit model name */
typedef std::tuple<std::string, std::string> PhyPbInfo;

int create_vpr_arch_layout(arcxpress::ArchGen& arch_gen,
                         const arcxpress::ArchGenInputParams& arch_params);

}  // End of namespace MOSOFA
