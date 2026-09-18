#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "qorso_strategy.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int read_xml_qorso_strategy(const char* fname, QorsoStrategy& qs_strtg, const bool& append,
                            const bool& verbose);

}  // End of namespace arkangel
