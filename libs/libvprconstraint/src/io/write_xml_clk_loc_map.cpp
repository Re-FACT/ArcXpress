#include "write_xml_clk_loc_map.h"

#include "command_exit_codes.h"
#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"
#include "vpr_constraint_file_constants.h"
#include "vtr_assert.h"
#include "vtr_time.h"

namespace arcxpress {
/* get the global signal ports info from clock_loc_map.xml */
int write_xml_clk_loc_map(const std::string& ck_loc_map_file,
                          const GlobalPortLoc& global_port_loc) {
  /* Parse the file */
  pugi::xml_document doc;

  pugi::xml_node xml_root = doc.append_child(XML_CLK_LOC_MAP_FILE_INSTANCE_ROOT);
  /* Walk thru all the ports */
  for (auto gport_id : global_port_loc.global_ports()) {
    openfpga::BasicPort gport = global_port_loc.get_global_port_name(gport_id);
    pugi::xml_node xml_node = xml_root.append_child(XML_CLK_LOC_MAP_FILE_INSTANCE_NODE);
    xml_node.append_attribute(XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_PIN) =
      gport.to_verilog_string().c_str();
    xml_node.append_attribute(XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_TYPE) =
      global_port_loc.global_port_type_string(gport_id).c_str();
    xml_node.append_attribute(XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_X) =
      std::to_string(global_port_loc.get_global_port_coord(gport_id).x()).c_str();
    xml_node.append_attribute(XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_Y) =
      std::to_string(global_port_loc.get_global_port_coord(gport_id).y()).c_str();
    xml_node.append_attribute(XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_Z) =
      std::to_string(global_port_loc.get_global_port_subtile(gport_id)).c_str();
  }

  doc.save_file(ck_loc_map_file.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}
}  // namespace arcxpress
