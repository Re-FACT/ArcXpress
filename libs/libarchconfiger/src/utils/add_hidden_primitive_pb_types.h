#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arcxpress {  // Begin namespace arcxpress

int add_builtin_pi_primitive_pb_type(arkangel::ArchGen& arch_gen, const std::string& pb_name,
                                     const std::string& mode_bits, const bool& verbose);
int add_builtin_po_primitive_pb_type(arkangel::ArchGen& arch_gen, const std::string& pb_name,
                                     const bool& verbose);
int add_builtin_latch_primitive_pb_type(arkangel::ArchGen& arch_gen, const std::string& pb_name,
                                        const bool& verbose);
int add_builtin_lut_primitive_pb_type(arkangel::ArchGen& arch_gen, const short& lut_size,
                                      const std::string& mode_bits, const bool& verbose);

}  // End of namespace arcxpress
