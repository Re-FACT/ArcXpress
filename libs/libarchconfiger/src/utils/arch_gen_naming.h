#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <map>
#include <string>
#include <tuple>

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

std::string gen_pb_mode_name(const std::string& parent_pb, const std::string& child_pb);

std::string gen_default_mode_name(const std::string& parent_pb);

std::string gen_physical_mode_name(const std::string& parent_pb);

std::string gen_ioff_pack_pattern_name(const std::string& ff_pb_name,
                                       const std::string& ff_port_name);

}  // End of namespace arkangel
