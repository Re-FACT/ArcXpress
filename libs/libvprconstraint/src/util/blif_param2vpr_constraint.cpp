#include "blif_param2vpr_constraint.h"

#include "atom_netlist.h"
#include "command_exit_codes.h"
#include "physical_types.h"
#include "read_blif.h"
#include "read_xml_arch_file.h"
#include "vpr_constraint_file_constants.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arkangel {

/** This function converts the location parameters defined in the blif file to the vpr constraints
 *  The input blif may contain the location parameters as follows
 *    .subckt pcnt ...
 *    .param LOC_X 1
 *    .param LOC_Y 2
 *    .param LOC_Z 3
 *  The resulting vpr constraint will contain
 *    <partition name="pcnt_x1y2z3">
 *      <add_atom name_patten="[atom_block_name_by_VPR_API]">
 *        <add_region x_low="1" x_high="1" y_low="2" y_high="2" subtile="3"/>
 *      </add_atom>
 *    </partition>
 */
int blif_param2vpr_constraint(const std::string& blif_fpath, const std::string& arch_fpath,
                              pugi::xml_node& vpr_constr_xml_root, const bool& verbose) {
  /* Read architecture file to get all the logical models */
  t_arch vpr_arch;
  std::vector<t_physical_tile_type> physical_tile_types;
  std::vector<t_logical_block_type> logical_block_types;
  xml_read_arch(arch_fpath.c_str(), true, &vpr_arch, physical_tile_types, logical_block_types,
                false);

  /* Read blif file thru VPR API */
  AtomNetlist nlist;
  nlist = read_blif(e_circuit_format::EBLIF, blif_fpath.c_str(), vpr_arch.models);

  /* Extract location parameters */
  std::string LOC_X_PARAM_NAME = "FPGA_LOC_X";
  std::string LOC_Y_PARAM_NAME = "FPGA_LOC_Y";
  std::string LOC_Z_PARAM_NAME = "FPGA_LOC_Z";
  std::map<std::string, std::array<int, 3>> atom_blk_loc_params;
  for (const AtomBlockId& atom_blk : nlist.blocks()) {
    /* Check if the block has all the parameters required. Must have all or have none. */
    short num_req_params = 0;
    for (const auto& param_search : nlist.block_params(atom_blk)) {
      /* Bypass unrelated parameters */
      if (param_search.first == LOC_X_PARAM_NAME) {
        num_req_params++;
      }
      if (param_search.first == LOC_Y_PARAM_NAME) {
        num_req_params++;
      }
      if (param_search.first == LOC_Z_PARAM_NAME) {
        num_req_params++;
      }
    }
    if (num_req_params == 0) {
      continue;
    }
    if (num_req_params != 3) {
      VTR_LOG_ERROR(
        "Atom block '%s' does not contain all the location parameters. Expect [ %s | %s | %s ]!\n",
        nlist.block_name(atom_blk).c_str(), LOC_X_PARAM_NAME.c_str(), LOC_Y_PARAM_NAME.c_str(),
        LOC_Z_PARAM_NAME.c_str());
    }
    std::array<int, 3> loc_info;
    for (const auto& param_search : nlist.block_params(atom_blk)) {
      /* FIXME: This is not generic. Should consider a better solution. For some reason, Yosys
       * convert parameter from source verilog in real number to string. For example, 3.0 -> "3.0"
       * in the outputted blif. So chop the 1st and the last character to make stoi happy */
      std::string l_param_str = param_search.second;
      l_param_str.pop_back();
      l_param_str.erase(0, 1);
      /* Bypass unrelated parameters */
      if (param_search.first == LOC_X_PARAM_NAME) {
        loc_info[0] = std::stoi(l_param_str);
      }
      if (param_search.first == LOC_Y_PARAM_NAME) {
        loc_info[1] = std::stoi(l_param_str);
      }
      if (param_search.first == LOC_Z_PARAM_NAME) {
        loc_info[2] = std::stoi(l_param_str);
      }
    }
    /* Now get all the parameters required */
    atom_blk_loc_params[nlist.block_name(atom_blk)] = loc_info;
  }
  /* Append to the existing vpr constraint XML node */
  for (auto pair : atom_blk_loc_params) {
    pugi::xml_node parti_node =
      vpr_constr_xml_root.append_child(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE);
    std::string parti_name = pair.first + std::string("x") + std::to_string(pair.second[0]) +
                             std::string("y") + std::to_string(pair.second[1]) + std::string("z") +
                             std::to_string(pair.second[2]);
    parti_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_NAME) = parti_name.c_str();

    pugi::xml_node atom_node = parti_node.append_child(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM);
    atom_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM_NAME_PATTERN) =
      pair.first.c_str();
    atom_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_ATOM_IS_REGEX) =
      XML_VPR_CONSTRAINT_FILE_BOOL_FALSE;

    pugi::xml_node region_node =
      parti_node.append_child(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION);
    region_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_X_LOW) =
      std::to_string(pair.second[0]).c_str();
    region_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_X_HIGH) =
      std::to_string(pair.second[0]).c_str();
    region_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_Y_LOW) =
      std::to_string(pair.second[1]).c_str();
    region_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_Y_HIGH) =
      std::to_string(pair.second[1]).c_str();
    region_node.append_attribute(XML_VPR_CONSTRAINT_FILE_INSTANCE_NODE_REGION_SUBTILE) =
      std::to_string(pair.second[2]).c_str();
  }
  return openfpga::CMD_EXEC_SUCCESS;
}
}  // namespace arkangel
