#ifndef WRITE_XML_PDK_MANAGER_H
#define WRITE_XML_PDK_MANAGER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "pdk_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

int write_xml_pdk_manager(const char* fname, const PdkManager& pdk_mgr);

int write_xml_pdk_config_for_synth(const char* fname, const PdkManager& pdk_mgr);

int write_xml_pdk_config_for_qor_extraction(const char* fname, const PdkManager& pdk_mgr,
                                            const std::map<std::string, std::string>& corner_map);

}  // End of namespace arkangel

#endif
