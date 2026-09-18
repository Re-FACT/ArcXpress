#pragma once

#include <string>

#include "global_port_loc_map.h"

namespace arkangel {
int write_xml_clk_loc_map(const std::string& ckbuf_loc_map_file,
                          const GlobalPortLoc& global_port_loc);
}  // namespace arkangel
