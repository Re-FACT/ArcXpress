#pragma once

#include <fstream>

#include "ckbuf_cell_info.h"
#include "global_port_loc_map.h"
#include "pin_constraints.h"
#include "pugixml.hpp"

namespace arcxpress {
void write_xml_vpr_constraints_file(pugi::xml_node& vpr_constr_xml_root,
                                    const CkbufInfo& ckbuf_info,
                                    const GlobalPortLoc& global_port_loc,
                                    const PinConstraints& pcf_info);
}  // namespace arcxpress
