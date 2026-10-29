/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

/* Headers from pin constraint library */
#include "boot_config_xml_constants.h"
#include "openfpga_digest.h"
#include "pugixml.hpp"
#include "write_xml_boot_config.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * Only the time stamp of the first file that is read in will be outputted!
 * TODO: Consider to aggregate all the authors. Use the earliest created date.
 * Use the latest modified date.
 *******************************************************************/
static int write_xml_stamps(pugi::xml_node& root_node, const BootConfig& boot_cfg) {
  int err_code = 0;
  pugi::xml_node stmp_node = root_node.append_child(XML_BOOTCONFIG_STAMP_ROOT_NODE_NAME);

  pugi::xml_node author_node = stmp_node.append_child(XML_BOOTCONFIG_AUTHOR_NODE_NAME);
  author_node.text().set(boot_cfg.author().c_str());

  pugi::xml_node dates_node = stmp_node.append_child(XML_BOOTCONFIG_DATE_NODE_NAME);
  pugi::xml_node created_node = dates_node.append_child(XML_BOOTCONFIG_CREATE_DATE_NODE_NAME);
  created_node.text().set(boot_cfg.date_created().c_str());

  pugi::xml_node modified_node =
    dates_node.append_child(XML_BOOTCONFIG_LAST_MODIFIED_DATE_NODE_NAME);
  modified_node.text().set(boot_cfg.date_last_modified().c_str());

  pugi::xml_node ver_node = stmp_node.append_child(XML_BOOTCONFIG_TOOL_VERSION_NODE_NAME);
  ver_node.text().set(boot_cfg.tool_version().c_str());

  return err_code;
}

static int write_xml_bin_relpath(pugi::xml_node& root_node, const BootConfig& boot_cfg,
                                 const BootConfig::e_bin_type& ftype) {
  int err_code = 0;
  pugi::xml_node f_node = root_node.append_child(XML_BOOTCONFIG_FILE_NODE_NAME);
  f_node.append_attribute(XML_BOOTCONFIG_FILE_ATTRIBUTE_TYPE) =
    boot_cfg.bin_type2str(ftype).c_str();
  f_node.text().set(boot_cfg.bin_relpath(ftype).c_str());

  return err_code;
}

/********************************************************************
 * A writer to output a PDK manager object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_boot_config(const char* fname, const BootConfig& boot_cfg) {
  vtr::ScopedStartFinishTimer timer("Write boot configuration to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_BOOTCONFIG_ROOT_NODE_NAME);

  /* Output time stamp */
  err_code += write_xml_stamps(root_node, boot_cfg);

  /* Output boot type */
  pugi::xml_node type_node = root_node.append_child(XML_BOOTCONFIG_BOOT_TYPE_NODE_NAME);
  type_node.text().set(boot_cfg.boot_type_str().c_str());

  /* Output bin_relpath */
  pugi::xml_node binrelpath_node = root_node.append_child(XML_BOOTCONFIG_BIN_ROOT_NODE_NAME);
  for (auto ftype : boot_cfg.bin_types()) {
    err_code += write_xml_bin_relpath(binrelpath_node, boot_cfg, ftype);
  }

  out_xml.save_file(fname);

  return err_code;
}

}  // End of namespace arcxpress
