
#include "global_signal_assigner.h"
#include "pugixml.hpp"
#include "vpr_constraint_file_constants.h"

int main(int argc, char** argv) {
  std::string cell_map_file = std::string(argv[1]);
  std::string pcf_file = std::string(argv[2]);
  std::string clock_loc_map_file = std::string(argv[3]);
  std::string vpr_constr_file = std::string(argv[4]);
  std::string final_pcf_file = std::string(argv[5]);

  pugi::xml_document out_xml;
  pugi::xml_node root_node = out_xml.append_child(arkangel::XML_VPR_CONSTRAINT_FILE_ROOT_NAME);
  arkangel::generate_global_signal_vpr_constraint_file(cell_map_file, pcf_file, clock_loc_map_file,
                                                       root_node, final_pcf_file);
  out_xml.save_file(vpr_constr_file.c_str());

  return 0;
}
