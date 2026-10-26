#include "read_xml_clk_loc_map.h"

#include <fstream>
#include <sstream>
#include <string>

#include "command_exit_codes.h"
#include "openfpga_port_parser.h"
#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"
#include "vpr_constraint_file_constants.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_vector.h"
#include "write_xml_utils.h"
namespace arcxpress {
/* get the global signal ports info from clock_loc_map.xml */
int set_up_global_ports(std::string clock_loc_map_file, GlobalPortLoc& global_port_loc) {
  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;
  try {
    loc_data = pugiutil::load_xml(doc, clock_loc_map_file);

    pugi::xml_node xml_root = get_single_child(doc, XML_CLK_LOC_MAP_FILE_INSTANCE_ROOT, loc_data);
    for (pugi::xml_node xml_block_info : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_block_info.name() == std::string(XML_CLK_LOC_MAP_FILE_INSTANCE_NODE)) {
        openfpga::PortParser port_parser(
          get_attribute(xml_block_info, XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_PIN, loc_data)
            .as_string());
        std::string pin_type =
          get_attribute(xml_block_info, XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_TYPE, loc_data)
            .as_string();
        int x_coord =
          get_attribute(xml_block_info, XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_X, loc_data).as_int();
        int y_coord =
          get_attribute(xml_block_info, XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_Y, loc_data).as_int();
        int subtile =
          get_attribute(xml_block_info, XML_CLK_LOC_MAP_FILE_INSTANCE_NODE_Z, loc_data).as_int();
        GlobalPortLoc::GlobalPortId unique_port_id = global_port_loc.create_port_info_instance(
          port_parser.port(), vtr::Point<int>(x_coord, y_coord), subtile, pin_type);
      } else {
        VTR_LOG_ERROR("Unexpected xml root: %s \n", xml_block_info.name());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  } catch (...) {
    VTR_LOG_ERROR(" Parse file error: %s\n", clock_loc_map_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}
}  // namespace arcxpress