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
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "pin_table_naming_rules_xml_constants.h"
#include "pugixml.hpp"
#include "write_xml_pin_table_naming_rules.h"

namespace arkangel {  // Begin namespace arkangel

static int write_xml_borderline(pugi::xml_node& root_node, const IoCoordData& io_coord,
                                const e_side& b_side, const IoBorderlineId& b_idx) {
  int err_code = 0;
  pugi::xml_node f_node = root_node.append_child(XML_PINTABLENAMERULES_NODE_SIDERULE);
  f_node.append_attribute(XML_SIDERULE_ATTR_SIDE) = io_coord.side2str(b_side).c_str();

  f_node.append_attribute(XML_SIDERULE_ATTR_STARTX) =
    std::to_string(io_coord.start_point(b_side, b_idx).x()).c_str();
  f_node.append_attribute(XML_SIDERULE_ATTR_STARTY) =
    std::to_string(io_coord.start_point(b_side, b_idx).y()).c_str();

  f_node.append_attribute(XML_SIDERULE_ATTR_ENDX) =
    std::to_string(io_coord.end_point(b_side, b_idx).x()).c_str();
  f_node.append_attribute(XML_SIDERULE_ATTR_ENDY) =
    std::to_string(io_coord.end_point(b_side, b_idx).y()).c_str();

  return err_code;
}

/********************************************************************
 * A writer to output a PDK manager object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_pin_table_naming_rules(const char* fname, const IoCoordData& io_coord,
                                     const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Write pin table naming rules to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_PINTABLENAMERULES_ROOT_NODE_NAME);

  /* Output borders */
  size_t border_cnt = 0;
  for (e_side b_side : io_coord.borderline_sides()) {
    for (auto b_idx : io_coord.borderlines(b_side)) {
      err_code += write_xml_borderline(root_node, io_coord, b_side, b_idx);
      border_cnt++;
    }
  }
  VTR_LOGV(verbose, "Output %lu borderlines\n", border_cnt);

  std::filesystem::path fpath = std::string(fname);
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  out_xml.save_file(fname);

  return err_code ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arkangel
