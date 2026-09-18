/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "arch_gen_circuit_model_utils.h"
#include "command_exit_codes.h"
#include "openfpga_arch_xml_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arkangel {

int add_xml_openfpga_arch_circuit_model_global_input(pugi::xml_node& cmdl_node,
                                                     const std::string& prefix, const size_t& size,
                                                     const std::string& lib_name) {
  int num_err = 0;
  pugi::xml_node port_node = cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = prefix.c_str();
  if (!lib_name.empty()) {
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) =
      lib_name.c_str();
  }
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
    std::to_string(size).c_str();
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL) = "true";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL) = "0";
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_xml_openfpga_arch_circuit_model_global_output(pugi::xml_node& cmdl_node,
                                                      const std::string& prefix, const size_t& size,
                                                      const std::string& lib_name) {
  int num_err = 0;
  pugi::xml_node port_node = cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = prefix.c_str();
  if (!lib_name.empty()) {
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) =
      lib_name.c_str();
  }
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
    std::to_string(size).c_str();
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL) = "true";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_IO) = "true";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL) = "0";

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_xml_openfpga_arch_circuit_model_mode_port(pugi::xml_node& cmdl_node,
                                                  const std::string& prefix, const size_t& size,
                                                  const std::string& lib_name) {
  int num_err = 0;
  pugi::xml_node port_node = cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "sram";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = prefix.c_str();
  if (!lib_name.empty()) {
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) =
      lib_name.c_str();
  }
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
    std::to_string(size).c_str();
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_MODE_SELECT) = "true";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_CIRCUIT_MODEL_NAME) =
    "EFPGA_CCFF";
  port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL) = "0";
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arkangel
