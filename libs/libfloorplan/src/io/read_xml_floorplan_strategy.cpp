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
#include "command_exit_codes.h"
#include "floorplan_strategy_xml_constants.h"
#include "pugixml_util.hpp"
#include "read_xml_floorplan_strategy.h"
#include "read_xml_util.h"
#include "side_manager.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

static int read_xml_boundary_cell_width(pugi::xml_node& xml_node,
                                        const pugiutil::loc_data& loc_data,
                                        FloorplanStrategy& flrpln_strtg) {
  std::string val = std::string(xml_node.child_value());
  flrpln_strtg.set_boundary_cell_width(val);
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_height_width_ratio(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                       FloorplanStrategy& flrpln_strtg) {
  float val = std::stof(std::string(xml_node.child_value()));
  flrpln_strtg.set_height_width_ratio(val);
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_tile_utilization(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                     FloorplanStrategy& flrpln_strtg) {
  float default_util =
    get_attribute(xml_node, XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_ATTR_DEFAULT, loc_data)
      .as_float();
  flrpln_strtg.set_tile_default_utilization(default_util);
  for (pugi::xml_node xml_tile : xml_node.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_tile.name() != std::string(XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_NODE_TILE)) {
      bad_tag(xml_node, loc_data, xml_tile, {XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_NODE_TILE});
    }
    std::string tile_name =
      get_attribute(xml_tile, XML_FLOORPLANSTRATEGY_TILE_UTILIZATION_TILE_ATTR_NAME, loc_data)
        .as_string();
    float val = std::stof(std::string(xml_tile.child_value()));
    flrpln_strtg.set_tile_utilization(tile_name, val);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_tile_gap(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                             FloorplanStrategy& flrpln_strtg) {
  pugi::xml_node xml_hor =
    get_single_child(xml_node, XML_FLOORPLANSTRATEGY_TILE_GAP_NODE_HORIZENTAL, loc_data);
  std::string h_val = std::string(xml_hor.child_value());
  flrpln_strtg.set_tile_horizental_gap(h_val);

  pugi::xml_node xml_vet =
    get_single_child(xml_node, XML_FLOORPLANSTRATEGY_TILE_GAP_NODE_VERTICAL, loc_data);
  std::string v_val = std::string(xml_vet.child_value());
  flrpln_strtg.set_tile_vertical_gap(v_val);
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_side_gap(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                             FloorplanStrategy& flrpln_strtg) {
  std::vector<std::string> valid_side_str;
  for (e_side curr_side : flrpln_strtg.sides()) {
    SideManager s_mgr(curr_side);
    valid_side_str.push_back(s_mgr.to_string());
  }
  for (pugi::xml_node xml_side : xml_node.children()) {
    e_side gside = NUM_2D_SIDES;
    for (e_side curr_side : flrpln_strtg.sides()) {
      SideManager s_mgr(curr_side);
      if (xml_side.name() == s_mgr.to_string()) {
        gside = curr_side;
        break;
      }
    }
    if (gside == NUM_2D_SIDES) {
      bad_tag(xml_node, loc_data, xml_side, valid_side_str);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::string gap_val = std::string(xml_side.child_value());
    flrpln_strtg.set_side_gap(gside, gap_val);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_metal_stack(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                FloorplanStrategy& flrpln_strtg) {
  for (pugi::xml_node xml_layer : xml_node.children()) {
    if (xml_layer.name() != std::string(XML_FLOORPLANSTRATEGY_METAL_STACK_NODE_LAYER)) {
      bad_tag(xml_node, loc_data, xml_layer, {XML_FLOORPLANSTRATEGY_METAL_STACK_NODE_LAYER});
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::string layer_name =
      get_attribute(xml_layer, XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_NAME, loc_data)
        .as_string();

    std::string len_str =
      get_attribute(xml_layer, XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_LENGTH, loc_data)
        .as_string();

    std::string width_str =
      get_attribute(xml_layer, XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_WIDTH, loc_data)
        .as_string();

    std::string spacing_str =
      get_attribute(xml_layer, XML_FLOORPLANSTRATEGY_METAL_STACK_LAYER_ATTR_SPACING, loc_data)
        .as_string();

    flrpln_strtg.add_metal_layer(layer_name, std::stof(len_str), std::stof(width_str),
                                 std::stof(spacing_str));
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_signal_distribution_network(pugi::xml_node& xml_node,
                                                const pugiutil::loc_data& loc_data,
                                                FloorplanStrategy& flrpln_strtg) {
  for (pugi::xml_node xml_ntwk : xml_node.children()) {
    if (xml_ntwk.name() !=
        std::string(XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NODE_NETWORK)) {
      bad_tag(xml_node, loc_data, xml_ntwk,
              {XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NODE_NETWORK});
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::string port_str =
      get_attribute(xml_ntwk, XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_PORT,
                    loc_data)
        .as_string();

    std::string type_str =
      get_attribute(xml_ntwk, XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_TYPE,
                    loc_data)
        .as_string();

    std::string bufcell_str =
      get_attribute(
        xml_ntwk, XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_BUFCELL, loc_data)
        .as_string();

    int depth =
      get_attribute(xml_ntwk, XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ATTR_DEPTH,
                    loc_data, pugiutil::ReqOpt::OPTIONAL)
        .as_int(0);

    /* Define the entry point */
    pugi::xml_node xml_entry = get_single_child(
      xml_ntwk, XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_NODE_ENTRY, loc_data);

    int entry_x =
      get_attribute(
        xml_entry, XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ENTRY_ATTR_X, loc_data)
        .as_int();

    int entry_y =
      get_attribute(
        xml_entry, XML_FLOORPLANSTRATEGY_SIGNAL_DISTRIBUTION_NETWORK_NETWORK_ENTRY_ATTR_Y, loc_data)
        .as_int();

    bool status = flrpln_strtg.add_signal_distribution_network(
      port_str, type_str, bufcell_str, depth, vtr::Point<size_t>(entry_x, entry_y));
    if (!status) {
      VTR_LOG_ERROR("Failed to parse signal distribution network for port '%s'\n",
                    port_str.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_floorplan_strategy(const char* fname, FloorplanStrategy& flrpln_strtg) {
  vtr::ScopedStartFinishTimer timer("Read Floorplan Strategy File");
  size_t num_err = 0;

  flrpln_strtg.clear();

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_FLOORPLANSTRATEGY_ROOT_NODE_NAME, loc_data);

    /* Parse boundary cell width */
    pugi::xml_node xml_bcw =
      get_single_child(xml_root, XML_FLOORPLANSTRATEGY_NODE_BOUNDARY_CELL_WIDTH, loc_data);
    int status = read_xml_boundary_cell_width(xml_bcw, loc_data, flrpln_strtg);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
    /* Parse height width ratio  */
    pugi::xml_node xml_hwr =
      get_single_child(xml_root, XML_FLOORPLANSTRATEGY_NODE_HEIGHT_WIDTH_RATIO, loc_data);
    status = read_xml_height_width_ratio(xml_hwr, loc_data, flrpln_strtg);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
    /* utilization  */
    pugi::xml_node xml_tutil =
      get_single_child(xml_root, XML_FLOORPLANSTRATEGY_NODE_TILE_UTILIZATION, loc_data);
    status = read_xml_tile_utilization(xml_tutil, loc_data, flrpln_strtg);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
    /* tile gap  */
    pugi::xml_node xml_tgap =
      get_single_child(xml_root, XML_FLOORPLANSTRATEGY_NODE_TILE_GAP, loc_data);
    status = read_xml_tile_gap(xml_tgap, loc_data, flrpln_strtg);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
    /* side gap  */
    pugi::xml_node xml_sgap =
      get_single_child(xml_root, XML_FLOORPLANSTRATEGY_NODE_SIDE_GAP, loc_data);
    status = read_xml_side_gap(xml_sgap, loc_data, flrpln_strtg);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
    /* metal stack details */
    pugi::xml_node xml_mstk = get_single_child(xml_root, XML_FLOORPLANSTRATEGY_NODE_METAL_STACK,
                                               loc_data, pugiutil::ReqOpt::OPTIONAL);
    if (xml_mstk) {
      status = read_xml_metal_stack(xml_mstk, loc_data, flrpln_strtg);
      if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
        num_err++;
      }
    }
    /* signal distribution details */
    pugi::xml_node xml_sgn_distr =
      get_single_child(xml_root, XML_FLOORPLANSTRATEGY_NODE_SIGNAL_DISTRIBUTION_NETWORK, loc_data,
                       pugiutil::ReqOpt::OPTIONAL);
    if (xml_sgn_distr) {
      status = read_xml_signal_distribution_network(xml_sgn_distr, loc_data, flrpln_strtg);
      if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
        num_err++;
      }
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

}  // End of namespace arkangel
