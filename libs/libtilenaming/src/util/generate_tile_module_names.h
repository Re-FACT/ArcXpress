#pragma once

#include <string>

/* namespace arcxpress begins */
namespace arcxpress {

int generate_tile_module_names(const std::string& i_module_names_fname,
                               const std::string& i_fabric_hie_fname,
                               const std::string& i_tile_ref_fname, const std::string& o_tile_fname,
                               const bool& verbose);

}  // namespace arcxpress
