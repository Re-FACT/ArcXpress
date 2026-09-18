#include "blif_param2vpr_constraint.h"
#include "command_exit_codes.h"
#include "pugixml.hpp"
#include "vpr_constraint_file_constants.h"

int main(int argc, char** argv) {
  std::string blif_file = std::string(argv[1]);
  std::string vpr_arch_file = std::string(argv[2]);
  std::string vpr_constr_file = std::string(argv[3]);

  pugi::xml_document out_xml;
  pugi::xml_node root_node = out_xml.append_child(arkangel::XML_VPR_CONSTRAINT_FILE_ROOT_NAME);
  int status = arkangel::blif_param2vpr_constraint(blif_file, vpr_arch_file, root_node, true);
  out_xml.save_file(vpr_constr_file.c_str());

  return status;
}
