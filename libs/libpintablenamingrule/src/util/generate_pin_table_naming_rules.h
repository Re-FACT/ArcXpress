#pragma once

#include <string>

/* namespace arkangel begins */
namespace arkangel {

int generate_pin_table_naming_rules(const std::string& io_coord_fname, const std::string& out_fname,
                                    const bool& verbose);

}  // namespace arkangel
