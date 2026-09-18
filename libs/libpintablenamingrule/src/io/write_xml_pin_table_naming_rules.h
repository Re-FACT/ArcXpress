#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "io_coord_data.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

int write_xml_pin_table_naming_rules(const char* fname, const IoCoordData& io_coord,
                                     const bool& verbose);

}  // End of namespace arkangel
