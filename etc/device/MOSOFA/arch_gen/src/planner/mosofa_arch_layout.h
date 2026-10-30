#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace MOSOFA {  // Begin namespace MOSOFA

/* pb_type name, circuit model name */
typedef std::tuple<std::string, std::string> PhyPbInfo;

int create_vpr_arch_layout(arcxpress::ArchGen& arch_gen,
                         const int& height, const int& width,
                         const int& num_clks, const int& num_rsts,
                         const std::map<e_side, int>& num_inputs,
                         const std::map<e_side, int>& num_outputs,
                         const int& num_pclks, const int& num_regions,
                         const bool& sync_dp_dimension);

}  // End of namespace MOSOFA
