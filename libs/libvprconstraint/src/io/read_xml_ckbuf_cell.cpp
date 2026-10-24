
#include "read_xml_ckbuf_cell.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "command_exit_codes.h"
#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"
#include "vpr_constraint_file_constants.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_vector.h"

namespace arcxpress {
/*get the info of ckbuf atom*/
int set_up_ckbuf_info(std::string cell_map_file, CkbufInfo& ckbuf_info) {
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;
  try {
    loc_data = pugiutil::load_xml(doc, cell_map_file);

    pugi::xml_node xml_root = get_single_child(doc, XML_CELL_MAP_FILE_INSTANCE_ROOT, loc_data);
    for (pugi::xml_node xml_block_info : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (strcmp(xml_block_info.name(), XML_CELL_MAP_FILE_INSTANCE_NODE) == 0) {
        std::string input_net_name =
          get_attribute(xml_block_info, XML_CELL_MAP_FILE_INSTANCE_NODE_INPUT_NET, loc_data)
            .as_string();
        std::string block_name =
          get_attribute(xml_block_info, XML_CELL_MAP_FILE_INSTANCE_NODE_CELL, loc_data).as_string();
        std::string port_type =
          get_attribute(xml_block_info, XML_CELL_MAP_FILE_INSTANCE_NODE_TYPE, loc_data).as_string();
        CkbufInfo::CkbufId unique_id =
          ckbuf_info.create_ckbuf_info_instance(block_name, input_net_name, port_type);
      } else {
        VTR_LOG_ERROR("Unexpected xml root: %s \n", xml_block_info.name());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  } catch (...) {
    VTR_LOG_ERROR(" Parse file error: %s\n", cell_map_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}
}  // namespace arcxpress