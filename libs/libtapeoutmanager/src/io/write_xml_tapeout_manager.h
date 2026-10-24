#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "tapeout_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arcxpress {  // Begin namespace arcxpress

int write_xml_tapeout_manager(const char* fname, const TapeoutManager& to_mgr,
                              const bool& keep_reserved_word);

}  // End of namespace arcxpress
