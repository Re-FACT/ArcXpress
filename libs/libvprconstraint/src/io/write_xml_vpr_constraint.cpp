#include "write_xml_vpr_constraint.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "openfpga_digest.h"
#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"
#include "vpr_constraint_file_constants.h"
namespace arkangel {
/*write ckbuf instance info*/

void write_xml_vpr_constraints_file(pugi::xml_node& root_node, const CkbufInfo& ckbuf_info,
                                    const GlobalPortLoc& global_port_loc,
                                    const PinConstraints& pcf_info) {
  /*get global reset and clock signals*/
  std::vector<std::string> reset_names;
  std::vector<std::string> clock_names;
  for (auto id : global_port_loc.global_ports()) {
    std::string constrainted_net_in_pcf =
      pcf_info.pin_net(global_port_loc.get_global_port_name(id));
    if (global_port_loc.is_global_port_reset(id) &&
        !pcf_info.unconstrained_net(constrainted_net_in_pcf) &&
        !pcf_info.unmapped_net(constrainted_net_in_pcf)) {
      reset_names.push_back(constrainted_net_in_pcf);
    } else if (global_port_loc.is_global_port_clk(id) &&
               !pcf_info.unconstrained_net(constrainted_net_in_pcf) &&
               !pcf_info.unmapped_net(constrainted_net_in_pcf)) {
      clock_names.push_back(constrainted_net_in_pcf);
    }
  }

  if (!reset_names.empty() || !clock_names.empty()) {
    pugi::xml_node sub_root_node_global_signal =
      root_node.append_child(XML_VPR_CONSTRAINT_FILE_SUBROOT_GLOBAL_ROUTE_CONSTRAINTS_NAME);
    /* Set route model for global signals to be idea.
    When the route model is set to ideal for global clock and global reset signals, these nets
    should not require routing.*/
    if (!reset_names.empty()) {
      for (auto reset_name : reset_names) {
        pugi::xml_node global_signal =
          sub_root_node_global_signal.append_child(XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL);
        global_signal.append_attribute(XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_NAME) =
          reset_name.c_str();
        global_signal.append_attribute(XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_ROUTE_MODEL) =
          XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_ROUTE_MODEL_IDEAL;
      }
    }

    if (!clock_names.empty()) {
      for (auto clock_name : clock_names) {
        pugi::xml_node global_signal =
          sub_root_node_global_signal.append_child(XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL);
        global_signal.append_attribute(XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_NAME) =
          clock_name.c_str();
        global_signal.append_attribute(XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_ROUTE_MODEL) =
          XML_VPR_CONSTRAINT_FILE_SET_GLOBAL_SIGNAL_ROUTE_MODEL_IDEAL;
      }
    }
  }

  if (!ckbuf_info.ckbufs().empty()) {
    pugi::xml_node sub_root_node = root_node.append_child(XML_VPR_CONSTRAINT_FILE_SUBROOT_NAME);
    for (auto ckbuf_id : ckbuf_info.ckbufs()) {
      pugi::xml_node ckbuf_node = sub_root_node.append_child(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE);
      ckbuf_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_NAME) =
        ckbuf_info.get_ckbuf_name(ckbuf_id).c_str();
      // add_region
      auto coord = ckbuf_info.get_ckbuf_assigned_location(ckbuf_id);
      pugi::xml_node ckbuf_node_region =
        ckbuf_node.append_child(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION);
      ckbuf_node_region.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_X_LOW) =
        std::to_string(coord.x()).c_str();
      ckbuf_node_region.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_Y_LOW) =
        std::to_string(coord.y()).c_str();
      ckbuf_node_region.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_X_HIGH) =
        std::to_string(coord.x()).c_str();
      ckbuf_node_region.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_Y_HIGH) =
        std::to_string(coord.y()).c_str();
      ckbuf_node_region.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_SUBTILE) =
        std::to_string(ckbuf_info.get_ckbuf_assigned_subtile(ckbuf_id)).c_str();

      // add_atom
      pugi::xml_node ckbuf_node_atom =
        ckbuf_node.append_child(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM);
      ckbuf_node_atom.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM_NAME_PATTERN) =
        ckbuf_info.get_ckbuf_name(ckbuf_id).c_str();
      ckbuf_node_atom.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM_IS_REGEX) =
        XML_VPR_CONSTRAINT_FILE_BOOL_FALSE;
    }
  }
}
}  // namespace arkangel
