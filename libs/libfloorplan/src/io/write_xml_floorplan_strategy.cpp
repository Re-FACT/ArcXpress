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
#include "floorplan_strategy_xml_constants.h"
#include "openfpga_digest.h"
#include "pugixml.hpp"
#include "side_manager.h"
#include "write_xml_floorplan_strategy.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * A writer to output a PDK manager object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_floorplan_strategy(const char* fname, const FloorplanStrategy& flrpln_strtg) {
  vtr::ScopedStartFinishTimer timer("Write floorplan strategy to file");

  int status = openfpga::CMD_EXEC_SUCCESS;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_FLOORPLANSTRATEGY_ROOT_NODE_NAME);

  pugi::xml_node bcw_node = root_node.append_child(XML_FLOORPLANSTRATEGY_NODE_BOUNDARY_CELL_WIDTH);
  bcw_node.text().set(std::to_string(flrpln_strtg.boundary_cell_width()).c_str());

  pugi::xml_node hwr_node = root_node.append_child(XML_FLOORPLANSTRATEGY_NODE_HEIGHT_WIDTH_RATIO);
  hwr_node.text().set(std::to_string(flrpln_strtg.height_width_ratio()).c_str());

  pugi::xml_node tutil_node = root_node.append_child(XML_FLOORPLANSTRATEGY_NODE_TILE_UTILIZATION);
  tutil_node.append_attribute(XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_ATTR_DEFAULT) =
    std::to_string(flrpln_strtg.tile_default_utilization()).c_str();
  for (std::string tile : flrpln_strtg.tiles()) {
    pugi::xml_node tutil_tnode =
      tutil_node.append_child(XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_NODE_TILE);
    tutil_tnode.append_attribute(XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_TILE_ATTR_NAME) =
      tile.c_str();
    tutil_tnode.text().set(std::to_string(flrpln_strtg.tile_utilization(tile)).c_str());
  }

  pugi::xml_node tgap_node = root_node.append_child(XML_FLOORPLANSTRATEGY_NODE_TILE_GAP);
  pugi::xml_node tgap_hnode =
    tgap_node.append_child(XML_FLOORPLANSTRATEGY_TILE_GAP_NODE_HORIZENTAL);
  tgap_hnode.text().set(std::to_string(flrpln_strtg.tile_horizental_gap()).c_str());
  pugi::xml_node tgap_vnode = tgap_node.append_child(XML_FLOORPLANSTRATEGY_TILE_GAP_NODE_VERTICAL);
  tgap_vnode.text().set(std::to_string(flrpln_strtg.tile_vertical_gap()).c_str());

  pugi::xml_node sgap_node = root_node.append_child(XML_FLOORPLANSTRATEGY_NODE_SIDE_GAP);
  for (e_side curr_side : flrpln_strtg.sides()) {
    SideManager s_mgr(curr_side);
    pugi::xml_node sgap_snode = sgap_node.append_child(s_mgr.c_str());
    sgap_snode.text().set(std::to_string(flrpln_strtg.side_gap(curr_side)).c_str());
  }

  out_xml.save_file(fname);

  return status;
}

}  // End of namespace arcxpress
