/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML of project file to the associated
 * data structures
 *******************************************************************/
#include <string>

/* Headers from pugi XML library */
#include "pugixml.hpp"
#include "pugixml_util.hpp"

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_time.h"

/* Headers from libarchfpga */
#include "arch_error.h"
#include "boot_config_xml_constants.h"
#include "command_exit_codes.h"
#include "pugixml_util.hpp"
#include "read_xml_boot_config.h"
#include "read_xml_util.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * Parse XML codes of a <stamp> to an object of ProjectManager
 *******************************************************************/
static void read_xml_stamp(pugi::xml_node& xml_stamp, const pugiutil::loc_data& loc_data,
                           BootConfig& boot_cfg, const std::string& fname) {
  pugi::xml_node xml_author =
    get_single_child(xml_stamp, XML_BOOTCONFIG_AUTHOR_NODE_NAME, loc_data);
  std::string author_str = std::string(xml_author.child_value());

  pugi::xml_node xml_dates = get_single_child(xml_stamp, XML_BOOTCONFIG_DATE_NODE_NAME, loc_data);
  pugi::xml_node xml_created_date =
    get_single_child(xml_dates, XML_BOOTCONFIG_CREATE_DATE_NODE_NAME, loc_data);
  std::string date_created_str = std::string(xml_created_date.child_value());

  pugi::xml_node xml_last_modified_date =
    get_single_child(xml_dates, XML_BOOTCONFIG_LAST_MODIFIED_DATE_NODE_NAME, loc_data);
  std::string date_last_modified_str = std::string(xml_last_modified_date.child_value());

  pugi::xml_node xml_tool_version =
    get_single_child(xml_stamp, XML_BOOTCONFIG_TOOL_VERSION_NODE_NAME, loc_data);
  std::string tool_version_str = std::string(xml_tool_version.child_value());

  boot_cfg.set_author(author_str);
  boot_cfg.set_date_created(date_created_str);
  boot_cfg.set_date_last_modified(date_last_modified_str);
  boot_cfg.set_tool_version(tool_version_str);
}

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_bin_relpath(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                                BootConfig& boot_cfg) {
  for (pugi::xml_node xml_source : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_source.name() != std::string(XML_BOOTCONFIG_FILE_NODE_NAME)) {
      bad_tag(xml_root, loc_data, xml_source, {XML_BOOTCONFIG_FILE_NODE_NAME});
    }
    std::string ftype =
      get_attribute(xml_source, XML_BOOTCONFIG_FILE_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string fname = std::string(xml_source.child_value());
    boot_cfg.set_bin_relpath(ftype, fname);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_boot_config(const char* fname, BootConfig& boot_cfg) {
  vtr::ScopedStartFinishTimer timer("Read Boot Configuration File");
  size_t num_err = 0;

  boot_cfg.clear();

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_BOOTCONFIG_ROOT_NODE_NAME, loc_data);

    /* Parse stamps */
    pugi::xml_node xml_stamp_root =
      get_single_child(xml_root, XML_BOOTCONFIG_STAMP_ROOT_NODE_NAME, loc_data);
    read_xml_stamp(xml_stamp_root, loc_data, boot_cfg, std::string(fname));
    /* Read boot type */
    pugi::xml_node xml_boottype =
      get_single_child(xml_root, XML_BOOTCONFIG_BOOT_TYPE_NODE_NAME, loc_data);
    boot_cfg.set_boot_type(std::string(xml_boottype.child_value()));
    /* Parse tech files */
    pugi::xml_node xml_binrelpath =
      get_single_child(xml_root, XML_BOOTCONFIG_BIN_ROOT_NODE_NAME, loc_data);
    int status = read_xml_bin_relpath(xml_binrelpath, loc_data, boot_cfg);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname, e.line(), "%s", e.what());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  if (num_err) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arcxpress
