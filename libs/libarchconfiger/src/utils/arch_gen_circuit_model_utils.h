#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "pugixml.hpp"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

int add_xml_openfpga_arch_circuit_model_global_input(pugi::xml_node& root_node,
                                                     const std::string& prefix, const size_t& size,
                                                     const std::string& lib_name = std::string());
int add_xml_openfpga_arch_circuit_model_global_output(pugi::xml_node& root_node,
                                                      const std::string& prefix, const size_t& size,
                                                      const std::string& lib_name = std::string());
int add_xml_openfpga_arch_circuit_model_mode_port(pugi::xml_node& root_node,
                                                  const std::string& prefix, const size_t& size,
                                                  const std::string& lib_name = std::string());

}  // End of namespace arkangel
