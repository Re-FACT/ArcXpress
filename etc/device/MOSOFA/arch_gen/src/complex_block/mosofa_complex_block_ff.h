#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_gen.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace SofaHD {  // Begin namespace SofaHD

int wire_ff_pb_type_mode(arcxpress::ArchGen& arch_gen, const std::string& ff_mode,
                         const std::string& ff_pb_name, const bool& add_ipack_pattern,
                         const bool& add_opack_pattern);

int bind_ff_pb_type_to_physical(arcxpress::ArchGen& arch_gen, const std::string& ff_pb_tag,
                                const std::string& ff_pb_type, const std::string& phy_pb_type_name,
                                const bool& verbose);

int add_ff_pb_type(arcxpress::ArchGen& arch_gen, const std::string& ff_pb, const bool& verbose);

int add_sofahd_builtin_latch_primitive_pb_type(arcxpress::ArchGen& arch_gen, const bool& verbose);

}  // namespace SofaHD
