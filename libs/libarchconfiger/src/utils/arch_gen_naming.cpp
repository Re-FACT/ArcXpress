/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
#include "arch_gen_naming.h"

#include <string>

namespace arcxpress {

std::string gen_pb_mode_name(const std::string& parent_pb, const std::string& child_pb) {
  return parent_pb + std::string("_mode_") + child_pb;
}

std::string gen_default_mode_name(const std::string& parent_pb) {
  return parent_pb + std::string("_default_mode");
}

std::string gen_physical_mode_name(const std::string& parent_pb) {
  return parent_pb + std::string("_physical_mode");
}

std::string gen_ioff_pack_pattern_name(const std::string& ff_pb_name,
                                       const std::string& ff_port_name) {
  return std::string("pack-") + ff_pb_name + std::string("-") + ff_port_name;
}

}  // End of namespace arcxpress
