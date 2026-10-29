#pragma once

#include <iostream>

#include "global_port_loc_map.h"

namespace arcxpress {
int set_up_global_ports(std::string clock_loc_map_file, GlobalPortLoc& global_port_loc);
}  // namespace arcxpress