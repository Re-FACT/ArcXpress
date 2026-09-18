#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "vpr_arch_layout.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

int add_full_capable_direct_connections(VprArchLayout& va_layout, const std::string& clb_tile,
                                        const std::string& clb_eq_input_port_prefix,
                                        const std::string& clb_non_eq_input_port_prefix,
                                        const std::string& clb_output_port, const size_t& lut_size,
                                        const size_t& num_fle, const size_t& num_outputs);

}  // namespace arkangel
