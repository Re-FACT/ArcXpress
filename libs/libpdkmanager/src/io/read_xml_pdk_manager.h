#ifndef READ_XML_PDK_MANAGER_H
#define READ_XML_PDK_MANAGER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "pdk_manager.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int read_xml_pdk_manager(const char* fname, PdkManager& pdk_mgr);

}  // End of namespace arcxpress

#endif
