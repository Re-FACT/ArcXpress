#pragma once

#include <iostream>

#include "global_port_loc_map.h"

namespace arkangel {
int set_up_global_ports(std::string clock_loc_map_file, GlobalPortLoc& global_port_loc);
}  // namespace arkangel