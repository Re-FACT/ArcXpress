#pragma once

#include <string>

#include "pugixml.hpp"

namespace arcxpress {

int blif_param2vpr_constraint(const std::string& blif_fpath, const std::string& arch_fpath,
                              pugi::xml_node& vpr_constr_xml_root, const bool& verbose);

}  // namespace arcxpress
