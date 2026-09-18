#include "vpr_arch_layout.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "openfpga_tokenizer.h"
#include "side_manager.h"
#include "vpr_arch_xml_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "vtr_util.h"

namespace arkangel {  // Begin namespace arkangel

VprArchLayout::VprArchLayout() {
  default_fc_in_ = 0.15;
  default_fc_out_ = 0.10;
  BASE_PRIORITY_ = 100;
  CB_MUX_NAME_ = "ipin_cblock";
  CK_MUX_NAME_ = "CK_mux";
  SEGMENT_TYPE_STRING_ = {"x", "y"};
  SEGMENT_TYPE_STRING_UPPERCASE_ = {"X", "Y"};
}

size_t VprArchLayout::fabric_height(const VprArchLayout::e_fabric_type& type) const {
  check_valid_fabric_type(type);
  switch (type) {
    case e_fabric_type::DP:
      /* TODO: If some side does not have I/Os, should apply it here */
      return dp_layout_.y();
    case e_fabric_type::ULTIMATE:
      /* TODO: If some side does not have I/Os, should apply it here */
      return ultimate_layout_.y();
    default:
      VTR_LOG_ERROR("Invalid fabric type!\n");
  }
  return -1; /* never happen */
}

size_t VprArchLayout::fabric_width(const VprArchLayout::e_fabric_type& type) const {
  check_valid_fabric_type(type);
  switch (type) {
    case e_fabric_type::DP:
      /* TODO: If some side does not have I/Os, should apply it here */
      return dp_layout_.x();
    case e_fabric_type::ULTIMATE:
      /* TODO: If some side does not have I/Os, should apply it here */
      return ultimate_layout_.x();
    default:
      VTR_LOG_ERROR("Invalid fabric type!\n");
  }
  return -1; /* never happen */
}

bool VprArchLayout::valid_fabric_type(const VprArchLayout::e_fabric_type& type) const {
  return type != VprArchLayout::e_fabric_type::NUM_TYPES;
}
void VprArchLayout::check_valid_fabric_type(const VprArchLayout::e_fabric_type& type) const {
  if (!valid_fabric_type(type)) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::string VprArchLayout::cb_mux_name() const { return CB_MUX_NAME_; }

std::string VprArchLayout::clk_mux_name() const { return CK_MUX_NAME_; }

std::vector<size_t> VprArchLayout::segment_lengths(const e_segment_type& seg_type) const {
  check_valid_segment_type(seg_type);
  std::vector<size_t> ret;
  for (auto pair : seg_length2count_[size_t(seg_type)]) {
    ret.push_back(pair.first);
  }
  return ret;
}

void VprArchLayout::set_default_fc_in(const float& val) { default_fc_in_ = val; }

void VprArchLayout::set_default_fc_out(const float& val) { default_fc_out_ = val; }

std::string VprArchLayout::find_port_xml_node_name(const SubTilePort& port) const {
  switch (port.type) {
    case e_port_type::INPUT:
    case e_port_type::RESET:
    case e_port_type::SCANI:
    case e_port_type::CI:
      return std::string(XML_VPR_ARCH_TILES_TILE_SUBTILE_INPUT_NODE_NAME);
    case e_port_type::OUTPUT:
    case e_port_type::SCANO:
    case e_port_type::CO:
      return std::string(XML_VPR_ARCH_TILES_TILE_SUBTILE_OUTPUT_NODE_NAME);
    case e_port_type::CLOCK:
      return std::string(XML_VPR_ARCH_TILES_TILE_SUBTILE_CLOCK_NODE_NAME);
    default:
      VTR_LOG_ERROR("Invalid port type!");
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return std::string();
}

int VprArchLayout::vpr_one_tile_xml_node(pugi::xml_node& tiles_node,
                                         const std::string& tile_name) const {
  /* Validate the tile name */
  auto result = tile_name2ids_.find(tile_name);
  if (result == tile_name2ids_.end()) {
    VTR_LOG("Unable to find the tile '%s' which is not defined.\n", tile_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  pugi::xml_node tile_node = tiles_node.append_child(XML_VPR_ARCH_TILES_TILE_NODE_NAME);
  tile_node.append_attribute(XML_VPR_ARCH_TILES_TILE_ATTR_NAME) = tile_name.c_str();
  tile_node.append_attribute(XML_VPR_ARCH_TILES_TILE_ATTR_AREA) = "0";
  /* Add sub tiles */
  for (size_t isubtile = 0; isubtile < tile_pb_types_[result->second].size(); ++isubtile) {
    vpr_one_subtile_xml_node(tile_node, tile_name, isubtile);
  }
  return 0;
}

int VprArchLayout::vpr_one_subtile_xml_node(pugi::xml_node& tile_node, const std::string& tile_name,
                                            const size_t& subtile_id_in_parent) const {
  /* Validate the tile name */
  auto result = tile_name2ids_.find(tile_name);
  if (result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("Unable to find the tile '%s' which is not defined.\n", tile_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  size_t tile_id = result->second;
  std::string subtile_name = tile_pb_types_[tile_id][subtile_id_in_parent];
  size_t subtile_capacity = subtile_capacity_[tile_id][subtile_id_in_parent];

  pugi::xml_node subtile_node = tile_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_NODE_NAME);
  subtile_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_ATTR_NAME) = subtile_name.c_str();
  subtile_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_ATTR_CAPACITY) =
    std::to_string(subtile_capacity).c_str();
  /* equivalent site */
  pugi::xml_node eqsite_node =
    subtile_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_EQSITE_NODE_NAME);
  auto pbtype_result = pb_type_name2ids_.find(subtile_name);
  if (pbtype_result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Unable to find the subtile '%s' which is not defined under tile '%s'.\n",
                  subtile_name.c_str(), tile_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* add physical equivalent site */
  {
    pugi::xml_node site_node =
      eqsite_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_EQSITE_SITE_NODE_NAME);
    site_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_EQSITE_SITE_ATTR_PB_TYPE) =
      subtile_name.c_str();
  }
  /* add logical equivalent site */
  for (auto equ_pb_type_name : tile_equ_pb_types_[tile_id][subtile_id_in_parent]) {
    auto lgk_pbtype_result = pb_type_name2ids_.find(equ_pb_type_name);
    if (lgk_pbtype_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("Unable to find the subtile '%s' which is not defined under tile '%s'.\n",
                    subtile_name.c_str(), tile_name.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    pugi::xml_node site_node =
      eqsite_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_EQSITE_SITE_NODE_NAME);
    site_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_EQSITE_SITE_ATTR_PB_TYPE) =
      equ_pb_type_name.c_str();
  }

  /* ports */
  for (auto port : subtile_ports_[pbtype_result->second]) {
    std::string port_xml_node_name = find_port_xml_node_name(port);
    pugi::xml_node port_node = subtile_node.append_child(port_xml_node_name.c_str());
    port_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_PORT_ATTR_NAME) = port.name.c_str();
    port_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_PORT_ATTR_NUM_PINS) =
      std::to_string(port.width).c_str();
    if (port.logic_equi) {
      port_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_PORT_ATTR_EQUIVALENT) = "full";
    }
    if (port.type == e_port_type::RESET || port.type == e_port_type::SCANI) {
      port_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_PORT_ATTR_IS_NON_CLOCK_GLOBAL) =
        "true";
    }
  }
  /* Fc */
  pugi::xml_node fc_node = subtile_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_NODE_NAME);
  fc_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_ATTR_IN_TYPE) = "frac";
  fc_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_ATTR_IN_VAL) =
    std::to_string(default_fc_in_).c_str();
  fc_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_ATTR_OUT_TYPE) = "frac";
  fc_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_ATTR_OUT_VAL) =
    std::to_string(default_fc_out_).c_str();
  for (auto port : subtile_ports_[pbtype_result->second]) {
    bool req_spec_fc = false;
    if (port.type == e_port_type::RESET || port.type == e_port_type::SCANI ||
        port.type == e_port_type::CLOCK || port.type == e_port_type::SCANO ||
        port.type == e_port_type::CI || port.type == e_port_type::CO) {
      req_spec_fc = true;
    }
    if (port.fc == 0.) {
      req_spec_fc = true;
    }
    if (!req_spec_fc) {
      continue;
    }
    /* Global signals should not be connected to any ports */
    pugi::xml_node fc_ovrd_node =
      fc_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_OVERRIDE_NODE_NAME);
    fc_ovrd_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_OVERRIDE_ATTR_PORT_NAME) =
      port.name.c_str();
    fc_ovrd_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_OVERRIDE_ATTR_FC_TYPE) =
      "frac";
    fc_ovrd_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_FC_OVERRIDE_ATTR_FC_VAL) = "0";
  }

  /* Pin loc */
  pugi::xml_node pinloc_node =
    subtile_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_PINLOC_NODE_NAME);
  pinloc_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_PINLOC_ATTR_PATTERN) = "custom";
  for (e_side pin_side : TOTAL_2D_SIDES) {
    pugi::xml_node loc_node =
      pinloc_node.append_child(XML_VPR_ARCH_TILES_TILE_SUBTILE_PINLOC_LOC_NODE_NAME);
    SideManager side_mgr(pin_side);
    loc_node.append_attribute(XML_VPR_ARCH_TILES_TILE_SUBTILE_PINLOC_LOC_ATTR_SIDE) =
      side_mgr.c_str();
    std::vector<std::string> pin_names;
    for (auto pinloc_info : subtile_pinloc_[tile_id][subtile_id_in_parent][size_t(pin_side)]) {
      std::string pin_name = subtile_name;
      pin_name += std::string("[") + std::to_string(pinloc_info.subtile_id) + std::string(":") +
                  std::to_string(pinloc_info.subtile_id) + std::string("]");
      pin_name += std::string(".");
      size_t subtile_port_id = pinloc_info.pb_type_port_id;
      pin_name += subtile_ports_[pbtype_result->second][subtile_port_id].name;
      pin_name += std::string("[") + std::to_string(pinloc_info.pin_id) + std::string(":") +
                  std::to_string(pinloc_info.pin_id) + std::string("]");
      pin_names.push_back(pin_name);
    }
    if (!pin_names.empty()) {
      loc_node.text().set(vtr::join(pin_names.begin(), pin_names.end(), " ").c_str());
    }
  }

  return 0;
}

int VprArchLayout::vpr_tile_xml_node(pugi::xml_node& tiles_node) const {
  int num_err = 0;
  for (auto pair : tile_name2ids_) {
    num_err += vpr_one_tile_xml_node(tiles_node, pair.first);
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

bool VprArchLayout::create_pb_type(const std::string& name, const e_subtile_type& type) {
  auto result = pb_type_name2ids_.find(name);
  if (result != pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Duplicated definition of pb_type '%s'!\n", name.c_str());
    return false;
  }
  size_t new_id = subtile_types_.size();
  subtile_types_.push_back(type);
  subtile_ports_.emplace_back();
  pb_type_name2ids_[name] = new_id;
  return true;
}

bool VprArchLayout::add_pb_type_port(const std::string& pb_type_name, const std::string& port_name,
                                     const e_port_type& port_type, const size_t& port_width) {
  auto result = pb_type_name2ids_.find(pb_type_name);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Cannot add port to the pb_type '%s' is not defined yet!\n",
                  pb_type_name.c_str());
    return false;
  }
  SubTilePort port;
  port.name = port_name;
  port.width = port_width;
  port.type = port_type;
  port.logic_equi = false;
  port.driven_by_greset = true;
  if (port_type == e_port_type::INPUT) {
    port.fc = default_fc_in_;
    port.driven_by_greset = true;
  } else if (port_type == e_port_type::OUTPUT) {
    port.fc = default_fc_out_;
    port.driven_by_greset = false;
  } else {
    port.fc = 0.;
    port.driven_by_greset = false;
  }
  subtile_ports_[result->second].push_back(port);
  return true;
}

bool VprArchLayout::set_pb_type_port_driven_by_greset(const std::string& pb_type_name,
                                                      const std::string& port_name,
                                                      const bool& val) {
  auto result = pb_type_name2ids_.find(pb_type_name);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Cannot add port to the pb_type '%s' is not defined yet!\n",
                  pb_type_name.c_str());
    return false;
  }
  bool found_port = false;
  for (size_t iport = 0; iport < subtile_ports_[result->second].size(); ++iport) {
    if (port_name == subtile_ports_[result->second][iport].name) {
      subtile_ports_[result->second][iport].driven_by_greset = val;
      found_port = true;
      break;
    }
  }
  if (!found_port) {
    VTR_LOG_ERROR(
      "Cannot update the driven_by_reset attribute for port '%s' of the pb_type '%s' which is not "
      "defined yet!\n",
      port_name.c_str(), pb_type_name.c_str());
    return false;
  }
  return true;
}

bool VprArchLayout::set_pb_type_port_fc(const std::string& pb_type_name,
                                        const std::string& port_name, const float& val) {
  auto result = pb_type_name2ids_.find(pb_type_name);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Cannot add port to the pb_type '%s' is not defined yet!\n",
                  pb_type_name.c_str());
    return false;
  }
  bool found_port = false;
  for (size_t iport = 0; iport < subtile_ports_[result->second].size(); ++iport) {
    if (port_name == subtile_ports_[result->second][iport].name) {
      subtile_ports_[result->second][iport].fc = val;
      found_port = true;
      break;
    }
  }
  if (!found_port) {
    VTR_LOG_ERROR(
      "Cannot update the Fc attribute for port '%s' of the pb_type '%s' which is not "
      "defined yet!\n",
      port_name.c_str(), pb_type_name.c_str());
    return false;
  }
  return true;
}

bool VprArchLayout::set_pb_type_port_logic_equivalent(const std::string& pb_type_name,
                                                      const std::string& port_name,
                                                      const bool& val) {
  auto result = pb_type_name2ids_.find(pb_type_name);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Cannot add port to the pb_type '%s' is not defined yet!\n",
                  pb_type_name.c_str());
    return false;
  }
  bool found_port = false;
  for (size_t iport = 0; iport < subtile_ports_[result->second].size(); ++iport) {
    if (port_name == subtile_ports_[result->second][iport].name) {
      subtile_ports_[result->second][iport].logic_equi = val;
      found_port = true;
      break;
    }
  }
  if (!found_port) {
    VTR_LOG_ERROR(
      "Cannot update the logic equivalent attribute for port '%s' of the pb_type '%s' which is not "
      "defined yet!\n",
      port_name.c_str(), pb_type_name.c_str());
    return false;
  }
  return true;
}

bool VprArchLayout::create_tile(const std::string& name, const e_tile_type& type,
                                const e_side& side) {
  auto result = tile_name2ids_.find(name);
  if (result != tile_name2ids_.end()) {
    VTR_LOG_ERROR("Duplicated definition of tile '%s'!\n", name.c_str());
    return false;
  }
  bool is_major = false;
  /* If the tile is the first one added in the same type (non-I/O) make it major */
  if (type == e_tile_type::IO) {
    is_major = true;
    for (size_t itile = 0; itile < tile_types_.size(); ++itile) {
      if (tile_types_[itile] == e_tile_type::IO && side == tile_sides_[itile]) {
        is_major = false;
        break;
      }
    }
  } else {
    auto it_type = std::find(tile_types_.begin(), tile_types_.end(), type);
    if (it_type == tile_types_.end()) {
      is_major = true;
    }
  }

  size_t new_id = tile_types_.size();
  tile_sides_.push_back(side);
  tile_types_.push_back(type);
  tile_major_.push_back(is_major);
  subtile_names_.emplace_back();
  tile_pb_types_.emplace_back();
  tile_equ_pb_types_.emplace_back();
  subtile_capacity_.emplace_back();
  subtile_pinloc_.emplace_back();
  tile_name2ids_[name] = new_id;
  /* If the tile is the first one added in the same side (I/O) make it major */
  return true;
}

bool VprArchLayout::add_subtile(const std::string& tile_name, const std::string& subtile_name,
                                const std::string& pb_type_name,
                                const std::vector<std::string>& equ_pb_type_names,
                                const size_t& capacity) {
  auto result = tile_name2ids_.find(tile_name);
  if (result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("Cannot add subtile to the tile '%s' is not yet defined!\n", tile_name.c_str());
    return false;
  }
  size_t tile_id = result->second;
  /* Avoid duplicated subtiles! */
  for (auto st_name : subtile_names_[tile_id]) {
    if (st_name == subtile_name) {
      VTR_LOG_ERROR("Duplicated subtile '%s' under the tile '%s'!\n", subtile_name.c_str(),
                    tile_name.c_str());
      return false;
    }
  }
  subtile_names_[tile_id].push_back(subtile_name);
  tile_pb_types_[tile_id].push_back(pb_type_name);
  tile_equ_pb_types_[tile_id].push_back(equ_pb_type_names);
  subtile_capacity_[tile_id].push_back(capacity);
  subtile_pinloc_[tile_id].emplace_back();
  return true;
}

bool VprArchLayout::add_subtile_pin_location(const std::string& tile_name,
                                             const std::string& subtile_name,
                                             const size_t& subtile_idx, const std::string& pin_name,
                                             const size_t& pin_index, const e_side& pin_side) {
  auto result = tile_name2ids_.find(tile_name);
  if (result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("Cannot add subtile to the tile '%s' is not yet defined!\n", tile_name.c_str());
    return false;
  }
  size_t tile_id = result->second;
  /* Avoid duplicated subtiles! */
  size_t subtile_id = subtile_names_[tile_id].size();
  for (size_t istile = 0; istile < subtile_names_[tile_id].size(); ++istile) {
    if (subtile_name == subtile_names_[tile_id][istile]) {
      subtile_id = istile;
      break;
    }
  }
  if (subtile_id == subtile_names_[tile_id].size()) {
    VTR_LOG_ERROR("Cannot add subtile '%s' to the tile '%s' is not yet defined!\n",
                  subtile_name.c_str(), tile_name.c_str());
    return false;
  }
  /* Check subtile capacity */
  if (subtile_idx >= subtile_capacity_[tile_id][subtile_id]) {
    VTR_LOG_ERROR("Subtile %s[%lu] under tile '%s' is out of the range of its capacity [0:%lu]!\n",
                  subtile_name.c_str(), subtile_idx, tile_name.c_str(),
                  subtile_capacity_[tile_id][subtile_id] - 1);
    return false;
  }
  /* Check if the pb_type is already defined */
  std::string pb_type_name = tile_pb_types_[tile_id][subtile_id];
  auto pb_result = pb_type_name2ids_.find(pb_type_name);
  if (pb_result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR(
      "Cannot find the pb_type '%s' for subtile %s[%lu] under tile '%s' which is not defined "
      "yet!\n",
      pb_type_name.c_str(), subtile_name.c_str(), subtile_idx, tile_name.c_str());
    return false;
  }
  size_t pb_type_id = pb_result->second;
  /* Validate subtile port name */
  size_t pb_type_port_id = subtile_ports_[pb_type_id].size();
  for (size_t iport = 0; iport < subtile_ports_[pb_type_id].size(); iport++) {
    if (pin_name == subtile_ports_[pb_type_id][iport].name) {
      pb_type_port_id = iport;
      break;
    }
  }
  if (pb_type_port_id == subtile_ports_[pb_type_id].size()) {
    VTR_LOG_ERROR(
      "Cannot find the port '%s' of pb_type '%s' for subtile %s[%lu] under tile '%s' which is not "
      "defined yet!\n",
      pin_name.c_str(), pb_type_name.c_str(), subtile_name.c_str(), subtile_idx, tile_name.c_str());
    return false;
  }
  /* Validate subtile pin index */
  if (pin_index >= subtile_ports_[pb_type_id][pb_type_port_id].width) {
    VTR_LOG_ERROR(
      "The pin index '%lu' is out of the range for port '%s[0:%lu]' of pb_type '%s' for subtile "
      "%s[%lu] under tile '%s' which is not defined yet!\n",
      pin_index, pin_name.c_str(), subtile_ports_[pb_type_id][pb_type_port_id].width - 1,
      pb_type_name.c_str(), subtile_name.c_str(), subtile_idx, tile_name.c_str());
    return false;
  }
  /* Validate pin side */
  if (pin_side >= NUM_2D_SIDES) {
    SideManager side_mgr(pin_side);
    VTR_LOG_ERROR(
      "The pin side '%s' is not valid for port '%s[0:%lu]' of pb_type '%s' for subtile %s[%lu] "
      "under tile '%s' which is not defined yet!\n",
      side_mgr.c_str(), pin_name.c_str(), subtile_ports_[pb_type_id][pb_type_port_id].width - 1,
      pb_type_name.c_str(), subtile_name.c_str(), subtile_idx, tile_name.c_str());
    return false;
  }
  /* Passed all the checks, add the pin loc */
  SubTilePinloc pinloc;
  pinloc.pb_type_port_id = pb_type_port_id;
  pinloc.subtile_id = subtile_idx;
  pinloc.pin_id = pin_index;
  /* TODO: Check if the pin loc has already been defined or not */
  subtile_pinloc_[tile_id][subtile_id][pin_side].push_back(pinloc);
  return true;
}

bool VprArchLayout::valid_tile_id(const size_t& id) const { return id < tile_sides_.size(); }
bool VprArchLayout::valid_pb_type_id(const size_t& id) const { return id < subtile_types_.size(); }

bool VprArchLayout::set_dp_layout(const size_t& height, const size_t& width) {
  if (height == 0) {
    VTR_LOG_ERROR("Require minimum height for core fabric to be >1!\n");
    return false;
  }
  if (width == 0) {
    VTR_LOG_ERROR("Require minimum width for core fabric to be >1!\n");
    return false;
  }
  dp_layout_.set_x(width);
  dp_layout_.set_y(height);
  return true;
}

bool VprArchLayout::valid_tile_name(const std::string& name) const {
  auto result = tile_name2ids_.find(name);
  return result != tile_name2ids_.end();
}

std::string VprArchLayout::coordinate2str(const vtr::Point<size_t>& coord) const {
  return std::to_string(coord.x()) + std::string("_") + std::to_string(coord.y());
}

vtr::Point<size_t> VprArchLayout::str2coordinate(const std::string& coord_str) const {
  openfpga::StringToken tokenizer(coord_str);
  std::vector<std::string> tokens = tokenizer.split('_');
  if (tokens.size() != 2) {
    VTR_LOG_ERROR("This is an internal error! Invalid coordinate '%s', expect (x, y)!\n",
                  coord_str.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return vtr::Point<size_t>(std::stoi(tokens[0]), std::stoi(tokens[1]));
}

bool VprArchLayout::set_dp_layout_column(const std::string& tile_name, const size_t& col_idx) {
  if (col_idx == 0 || col_idx > dp_layout_.x() - 1) {
    VTR_LOG_ERROR("The column index '%lu' is out of range [1, %lu)!\n", col_idx, dp_layout_.x());
    return false;
  }
  if (!valid_tile_name(tile_name)) {
    VTR_LOG_ERROR("The tile name '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  dp_layout_column_tiles_[col_idx] = tile_name;
  return true;
}

bool VprArchLayout::set_dp_layout_single(const std::string& tile_name,
                                         const vtr::Point<size_t>& coord) {
  if (coord.x() > dp_layout_.x() - 1) {
    VTR_LOG_ERROR("The single tile '%s' coordinate (x=%lu, y=%lu) is out of x range [0, %lu)!\n",
                  tile_name.c_str(), coord.x(), coord.y(), dp_layout_.x());
    return false;
  }
  if (coord.y() > dp_layout_.y() - 1) {
    VTR_LOG_ERROR("The single tile '%s' coordinate (x=%lu, y=%lu) is out of y range [0, %lu)!\n",
                  tile_name.c_str(), coord.x(), coord.y(), dp_layout_.y());
    return false;
  }
  if (!valid_tile_name(tile_name)) {
    VTR_LOG_ERROR("The tile name '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  dp_layout_single_tiles_[coordinate2str(coord)] = tile_name;
  return true;
}

bool VprArchLayout::set_ultimate_layout_column(const std::string& tile_name,
                                               const size_t& col_idx) {
  if (col_idx == 0 || col_idx > ultimate_layout_.x() - 1) {
    VTR_LOG_ERROR("The column index '%lu' is out of range [1, %lu)!\n", col_idx,
                  ultimate_layout_.x());
    return false;
  }
  if (!valid_tile_name(tile_name)) {
    VTR_LOG_ERROR("The tile name '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  ultimate_layout_column_tiles_[col_idx] = tile_name;
  return true;
}

bool VprArchLayout::set_ultimate_layout_single(const std::string& tile_name,
                                               const vtr::Point<size_t>& coord) {
  if (coord.x() > ultimate_layout_.x() - 1) {
    VTR_LOG_ERROR("The single tile '%s' coordinate (x=%lu, y=%lu) is out of x range [0, %lu)!\n",
                  tile_name.c_str(), coord.x(), coord.y(), ultimate_layout_.x());
    return false;
  }
  if (coord.y() > ultimate_layout_.y() - 1) {
    VTR_LOG_ERROR("The single tile '%s' coordinate (x=%lu, y=%lu) is out of y range [0, %lu)!\n",
                  tile_name.c_str(), coord.x(), coord.y(), ultimate_layout_.y());
    return false;
  }
  if (!valid_tile_name(tile_name)) {
    VTR_LOG_ERROR("The tile name '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  ultimate_layout_single_tiles_[coordinate2str(coord)] = tile_name;
  return true;
}

bool VprArchLayout::set_ultimate_layout(const size_t& height, const size_t& width) {
  if (height == 0) {
    VTR_LOG_ERROR("Require minimum height for core fabric to be >1!\n");
    return false;
  }
  if (width == 0) {
    VTR_LOG_ERROR("Require minimum width for core fabric to be >1!\n");
    return false;
  }
  ultimate_layout_.set_x(width);
  ultimate_layout_.set_y(height);
  return true;
}

std::string VprArchLayout::find_tile_name(const e_tile_type& type, const e_side& side) const {
  for (auto pair : tile_name2ids_) {
    if (type == e_tile_type::IO) {
      if (tile_types_[pair.second] == type && side == tile_sides_[pair.second] &&
          tile_major_[pair.second]) {
        return pair.first;
      }
    } else {
      if (tile_types_[pair.second] == type) {
        return pair.first;
      }
    }
  }
  return std::string();
}

int VprArchLayout::vpr_layout_xml_node(pugi::xml_node& layout_node) const {
  int num_err = 0;
  /* tileable rr graph settings */
  layout_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TILEABLE) = "true";
  layout_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_THRU_CHANNEL) = "true";
  layout_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_SHRINK_BOUNDARY) = "true";
  layout_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_OPIN2ALL_SIDES) = "true";
  layout_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_CONCAT_WIRE) = "true";
  layout_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_CONCAT_PASS_WIRE) = "false";
  layout_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PERIMETER_CB) = "true";

  num_err += vpr_auto_layout_xml_node(layout_node);
  num_err += vpr_dp_layout_xml_node(layout_node);
  num_err += vpr_ultimate_layout_xml_node(layout_node);
  return num_err;
}

int VprArchLayout::vpr_auto_layout_xml_node(pugi::xml_node& layout_node) const {
  int num_err = 0;
  /* Add auto layout */
  pugi::xml_node auto_node = layout_node.append_child(XML_VPR_ARCH_LAYOUT_AUTO_NODE_NAME);
  auto_node.append_attribute(XML_VPR_ARCH_LAYOUT_AUTO_ATTR_ASPECT_RATIO) = "1.0";
  size_t type_priority = BASE_PRIORITY_;

  pugi::xml_node top_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_ROW_NODE_NAME);
  std::string top_tile_name = find_tile_name(e_tile_type::IO, TOP);
  if (top_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the top side!\n");
    num_err++;
  }
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = top_tile_name.c_str();
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTY) = "H-1";
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node bottom_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_ROW_NODE_NAME);
  std::string bottom_tile_name = find_tile_name(e_tile_type::IO, BOTTOM);
  if (bottom_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the bottom side!\n");
    num_err++;
  }
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = bottom_tile_name.c_str();
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTY) = 0;
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node right_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
  std::string right_tile_name = find_tile_name(e_tile_type::IO, RIGHT);
  if (right_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the right side!\n");
    num_err++;
  }
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = right_tile_name.c_str();
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = "W-1";
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node left_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
  std::string left_tile_name = find_tile_name(e_tile_type::IO, LEFT);
  if (left_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the left side!\n");
    num_err++;
  }
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = left_tile_name.c_str();
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = "0";
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node corners_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_CORNERS_NODE_NAME);
  corners_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) =
    "EMPTY"; /* TODO: Use VPR constants */
  corners_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();

  pugi::xml_node fill_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_FILL_NODE_NAME);
  std::string fill_tile_name = find_tile_name(e_tile_type::CLB);
  if (fill_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an CLB tile which should locate in the center fabric!\n");
    num_err++;
  }
  fill_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = fill_tile_name.c_str();
  fill_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(BASE_PRIORITY_ / 10).c_str();

  return num_err;
}

int VprArchLayout::vpr_dp_layout_xml_node(pugi::xml_node& layout_node) const {
  int num_err = 0;
  /* Add auto layout */
  pugi::xml_node auto_node = layout_node.append_child(XML_VPR_ARCH_LAYOUT_FIXED_NODE_NAME);
  auto_node.append_attribute(XML_VPR_ARCH_LAYOUT_FIXED_ATTR_NAME) = "dp";
  auto_node.append_attribute(XML_VPR_ARCH_LAYOUT_FIXED_ATTR_WIDTH) =
    std::to_string(dp_layout_.x()).c_str();
  auto_node.append_attribute(XML_VPR_ARCH_LAYOUT_FIXED_ATTR_HEIGHT) =
    std::to_string(dp_layout_.y()).c_str();
  size_t type_priority = BASE_PRIORITY_;

  pugi::xml_node top_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_ROW_NODE_NAME);
  std::string top_tile_name = find_tile_name(e_tile_type::IO, TOP);
  if (top_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the top side!\n");
    num_err++;
  }
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = top_tile_name.c_str();
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTY) = "H-1";
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node bottom_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_ROW_NODE_NAME);
  std::string bottom_tile_name = find_tile_name(e_tile_type::IO, BOTTOM);
  if (bottom_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the bottom side!\n");
    num_err++;
  }
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = bottom_tile_name.c_str();
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTY) = 0;
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node right_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
  std::string right_tile_name = find_tile_name(e_tile_type::IO, RIGHT);
  if (right_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the right side!\n");
    num_err++;
  }
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = right_tile_name.c_str();
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = "W-1";
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node left_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
  std::string left_tile_name = find_tile_name(e_tile_type::IO, LEFT);
  if (left_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the left side!\n");
    num_err++;
  }
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = left_tile_name.c_str();
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = "0";
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node corners_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_CORNERS_NODE_NAME);
  corners_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) =
    "EMPTY"; /* TODO: Use VPR constants */
  corners_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();

  pugi::xml_node fill_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_FILL_NODE_NAME);
  std::string fill_tile_name = find_tile_name(e_tile_type::CLB);
  if (fill_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an CLB tile which should locate in the center fabric!\n");
    num_err++;
  }
  fill_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = fill_tile_name.c_str();
  fill_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(BASE_PRIORITY_ / 10).c_str();

  /* Add columns to the core part */
  size_t col_priority = BASE_PRIORITY_ / 10 + 1;
  for (auto pair : dp_layout_column_tiles_) {
    pugi::xml_node col_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
    col_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = pair.second.c_str();
    col_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = std::to_string(pair.first).c_str();
    col_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
      std::to_string(col_priority++).c_str();
  }

  /* Add singles to the core part */
  for (auto pair : dp_layout_single_tiles_) {
    pugi::xml_node single_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_SINGLE_NODE_NAME);
    vtr::Point<size_t> coord = str2coordinate(pair.first);
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = pair.second.c_str();
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_X) = std::to_string(coord.x()).c_str();
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_Y) = std::to_string(coord.y()).c_str();
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
      std::to_string(type_priority++).c_str();
  }

  return num_err;
}

int VprArchLayout::vpr_ultimate_layout_xml_node(pugi::xml_node& layout_node) const {
  int num_err = 0;
  /* Add auto layout */
  pugi::xml_node auto_node = layout_node.append_child(XML_VPR_ARCH_LAYOUT_FIXED_NODE_NAME);
  auto_node.append_attribute(XML_VPR_ARCH_LAYOUT_FIXED_ATTR_NAME) = "ultimate";
  auto_node.append_attribute(XML_VPR_ARCH_LAYOUT_FIXED_ATTR_WIDTH) =
    std::to_string(ultimate_layout_.x()).c_str();
  auto_node.append_attribute(XML_VPR_ARCH_LAYOUT_FIXED_ATTR_HEIGHT) =
    std::to_string(ultimate_layout_.y()).c_str();
  size_t type_priority = BASE_PRIORITY_;

  pugi::xml_node top_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_ROW_NODE_NAME);
  std::string top_tile_name = find_tile_name(e_tile_type::IO, TOP);
  if (top_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the top side!\n");
    num_err++;
  }
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = top_tile_name.c_str();
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTY) = "H-1";
  top_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node bottom_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_ROW_NODE_NAME);
  std::string bottom_tile_name = find_tile_name(e_tile_type::IO, BOTTOM);
  if (bottom_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the bottom side!\n");
    num_err++;
  }
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = bottom_tile_name.c_str();
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTY) = 0;
  bottom_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node right_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
  std::string right_tile_name = find_tile_name(e_tile_type::IO, RIGHT);
  if (right_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the right side!\n");
    num_err++;
  }
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = right_tile_name.c_str();
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = "W-1";
  right_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node left_io_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
  std::string left_tile_name = find_tile_name(e_tile_type::IO, LEFT);
  if (left_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an I/O tile which should locate on the left side!\n");
    num_err++;
  }
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = left_tile_name.c_str();
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = "0";
  left_io_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();
  type_priority++;

  pugi::xml_node corners_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_CORNERS_NODE_NAME);
  corners_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) =
    "EMPTY"; /* TODO: Use VPR constants */
  corners_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(type_priority).c_str();

  pugi::xml_node fill_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_FILL_NODE_NAME);
  std::string fill_tile_name = find_tile_name(e_tile_type::CLB);
  if (fill_tile_name.empty()) {
    VTR_LOG_ERROR("Cannot find an CLB tile which should locate in the center fabric!\n");
    num_err++;
  }
  fill_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = fill_tile_name.c_str();
  fill_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
    std::to_string(BASE_PRIORITY_ / 10).c_str();
  /* Add columns to the core part */
  size_t col_priority = BASE_PRIORITY_ / 10 + 1;
  for (auto pair : ultimate_layout_column_tiles_) {
    pugi::xml_node col_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_COL_NODE_NAME);
    col_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = pair.second.c_str();
    col_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_STARTX) = std::to_string(pair.first).c_str();
    col_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
      std::to_string(col_priority++).c_str();
  }
  /* Add singles to the core part */
  for (auto pair : ultimate_layout_single_tiles_) {
    pugi::xml_node single_node = auto_node.append_child(XML_VPR_ARCH_LAYOUT_SINGLE_NODE_NAME);
    vtr::Point<size_t> coord = str2coordinate(pair.first);
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_TYPE) = pair.second.c_str();
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_X) = std::to_string(coord.x()).c_str();
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_Y) = std::to_string(coord.y()).c_str();
    single_node.append_attribute(XML_VPR_ARCH_LAYOUT_ATTR_PRIORITY) =
      std::to_string(type_priority++).c_str();
  }

  return num_err;
}

int VprArchLayout::vpr_device_xml_node(pugi::xml_node& device_node) const {
  int num_err = 0;

  /* sizing */
  pugi::xml_node sizing_node = device_node.append_child(XML_VPR_ARCH_DEVICE_SIZING_NODE_NAME);
  sizing_node.append_attribute(XML_VPR_ARCH_DEVICE_SIZING_ATTR_R_MINW_NMOS) = "8926";
  sizing_node.append_attribute(XML_VPR_ARCH_DEVICE_SIZING_ATTR_R_MINW_PMOS) = "16067";

  /* area */
  pugi::xml_node area_node = device_node.append_child(XML_VPR_ARCH_DEVICE_AREA_NODE_NAME);
  area_node.append_attribute(XML_VPR_ARCH_DEVICE_AREA_ATTR_GRID_LOGIC_TILE_AREA) = "0";

  /* chan width distribution */
  pugi::xml_node chandist_node = device_node.append_child(XML_VPR_ARCH_DEVICE_CHANDIST_NODE_NAME);

  size_t chanx_w = route_chan_width(e_segment_type::X);
  size_t chany_w = route_chan_width(e_segment_type::Y);
  float chanx_peak = chanx_w > chany_w ? 1.0 : chanx_w / chany_w;
  float chany_peak = chanx_w < chany_w ? 1.0 : chany_w / chanx_w;

  pugi::xml_node chanx_dist_node =
    chandist_node.append_child(XML_VPR_ARCH_DEVICE_CHANDIST_X_NODE_NAME);
  chanx_dist_node.append_attribute(XML_VPR_ARCH_DEVICE_CHANDIST_ATTR_DISTR) = "uniform";
  chanx_dist_node.append_attribute(XML_VPR_ARCH_DEVICE_CHANDIST_ATTR_PEAK) =
    std::to_string(chanx_peak).c_str();

  pugi::xml_node chany_dist_node =
    chandist_node.append_child(XML_VPR_ARCH_DEVICE_CHANDIST_Y_NODE_NAME);
  chany_dist_node.append_attribute(XML_VPR_ARCH_DEVICE_CHANDIST_ATTR_DISTR) = "uniform";
  chany_dist_node.append_attribute(XML_VPR_ARCH_DEVICE_CHANDIST_ATTR_PEAK) =
    std::to_string(chany_peak).c_str();

  /* switch block */
  pugi::xml_node sb_node = device_node.append_child(XML_VPR_ARCH_DEVICE_SWITCHBLOCK_NODE_NAME);
  sb_node.append_attribute(XML_VPR_ARCH_DEVICE_SWITCHBLOCK_ATTR_TYPE) = "wilton";
  sb_node.append_attribute(XML_VPR_ARCH_DEVICE_SWITCHBLOCK_ATTR_FS) = "3";
  sb_node.append_attribute(XML_VPR_ARCH_DEVICE_SWITCHBLOCK_ATTR_SUB_TYPE) = "subset";
  sb_node.append_attribute(XML_VPR_ARCH_DEVICE_SWITCHBLOCK_ATTR_SUB_FS) = "3";

  /* connection block */
  pugi::xml_node cb_node = device_node.append_child(XML_VPR_ARCH_DEVICE_CONNECTIONBLOCK_NODE_NAME);
  cb_node.append_attribute(XML_VPR_ARCH_DEVICE_CONNECTIONBLOCK_ATTR_INPUT_SWITCH_NAME) =
    CB_MUX_NAME_.c_str();

  return num_err;
}

bool VprArchLayout::add_segment(const e_segment_type& type, const short& length,
                                const short& count) {
  std::string seg_type_str = segment_type2str(type);
  if (length == 0) {
    VTR_LOG_ERROR("Invalid %s-direction routing segment length '%d' which must be > 1!\n",
                  seg_type_str.c_str(), length);
    return false;
  }
  if (count == 0) {
    VTR_LOG_ERROR("Invalid %s-direction routing segment count '%d' which must be > 1!\n",
                  seg_type_str.c_str(), count);
    return false;
  }
  /* Length should not exceed the width */
  if (type == e_segment_type::X) {
    if (ultimate_layout_.x() + 2 < length) {
      VTR_LOG_ERROR(
        "Invalid X-direction routing segment length '%d' which must be less than the logic array "
        "width '%lu'!\n",
        length, ultimate_layout_.x());
      return false;
    }
  } else {
    VTR_ASSERT_SAFE(type == e_segment_type::Y);
    if (ultimate_layout_.y() + 2 < length) {
      VTR_LOG_ERROR(
        "Invalid Y-direction routing segment length '%d' which must be less than the logic array "
        "height '%lu'!\n",
        length, ultimate_layout_.y());
      return false;
    }
  }
  seg_length2count_[size_t(type)][length] = count;
  return true;
}

int VprArchLayout::vpr_switchlist_xml_node(pugi::xml_node& swlist_node) const {
  int num_err = 0;

  /* CK MUX has a fixed name  */
  {
    pugi::xml_node sw_node = swlist_node.append_child(XML_VPR_ARCH_SWITCHLIST_SWITCH_NODE_NAME);
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_TYPE) = "mux";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_NAME) = CK_MUX_NAME_.c_str();
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_R) = "0";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_CIN) = "0";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_COUT) = "0";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_TDEL) = "${CK_MUX_DELAY}";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_MUX_TRANS_SIZE) = "2.630740";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_BUF_SIZE) = "27.645901";
  }

  /* SB MUX */
  for (auto seg_type : segment_types()) {
    for (auto pair : seg_length2count_[size_t(seg_type)]) {
      pugi::xml_node sw_node = swlist_node.append_child(XML_VPR_ARCH_SWITCHLIST_SWITCH_NODE_NAME);
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_TYPE) = "mux";
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_NAME) =
        sb_mux_name(seg_type, pair.first).c_str();
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_R) = "0";
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_CIN) = "0";
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_COUT) = "0";
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_TDEL) =
        sb_mux_tdel_name(seg_type, pair.first).c_str();
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_MUX_TRANS_SIZE) = "2.630740";
      sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_BUF_SIZE) = "27.645901";
    }
  }
  /* TODO: There is a bug in VPR when the CB MUX is the first in the list, it cannot be identified
   * as part of the directlist !!!*/
  /* CB MUX has a fixed name  */
  {
    pugi::xml_node sw_node = swlist_node.append_child(XML_VPR_ARCH_SWITCHLIST_SWITCH_NODE_NAME);
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_TYPE) = "mux";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_NAME) = CB_MUX_NAME_.c_str();
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_R) = "0";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_CIN) = "0";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_COUT) = "0";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_TDEL) = "${CB_MUX_DELAY}";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_MUX_TRANS_SIZE) = "2.630740";
    sw_node.append_attribute(XML_VPR_ARCH_SWITCHLIST_SWITCH_ATTR_BUF_SIZE) = "27.645901";
  }

  return num_err;
}

std::string VprArchLayout::sb_mux_name(const e_segment_type& type, const uint16_t& len) const {
  std::string seg_type_str = segment_type2str(type);
  std::string ret = std::string("L") + std::to_string(len) + seg_type_str + std::string("_mux");
  return ret;
}

std::string VprArchLayout::sb_mux_tdel_name(const e_segment_type& type, const uint16_t& len) const {
  std::string seg_type_str = segment_type2str_uppercase(type);
  std::string ret =
    std::string("${L") + std::to_string(len) + seg_type_str + std::string("_MUX_DELAY}");
  return ret;
}

size_t VprArchLayout::route_chan_width(const e_segment_type& type) const {
  size_t w = 0;
  for (auto pair : seg_length2count_[size_t(type)]) {
    w += pair.first * pair.second;
  }
  return w;
}

std::string VprArchLayout::clk_segment_name() const { return segment_name(e_segment_type::X, 1); }

std::string VprArchLayout::segment_name(const e_segment_type& type, const size_t& len) const {
  std::string seg_type_str = segment_type2str(type);
  std::string ret = std::string("L") + std::to_string(len) + seg_type_str;
  return ret;
}

int VprArchLayout::vpr_segmentlist_xml_node(pugi::xml_node& seglist_node) const {
  int num_err = 0;

  for (auto seg_type : segment_types()) {
    size_t chan_w = route_chan_width(seg_type);
    std::string seg_type_str = segment_type2str(seg_type);
    for (auto pair : seg_length2count_[size_t(seg_type)]) {
      float freq = (float)(pair.first * pair.second) / (float)(chan_w);
      pugi::xml_node seg_node =
        seglist_node.append_child(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_NODE_NAME);
      seg_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_ATTR_AXIS) = seg_type_str.c_str();
      seg_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_ATTR_NAME) =
        segment_name(seg_type, pair.first).c_str();
      seg_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_ATTR_FREQ) =
        std::to_string(freq).c_str();
      seg_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_ATTR_LENGTH) =
        std::to_string(pair.first).c_str();
      seg_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_ATTR_TYPE) = "unidir";
      seg_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_ATTR_RMETAL) = "0";
      seg_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SEGMENT_ATTR_CMETAL) = "0";

      pugi::xml_node mux_node = seg_node.append_child(XML_VPR_ARCH_SEGMENTLIST_MUX_NODE_NAME);
      mux_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_MUX_ATTR_NAME) =
        sb_mux_name(seg_type, pair.first).c_str();

      pugi::xml_node sb_node = seg_node.append_child(XML_VPR_ARCH_SEGMENTLIST_SB_NODE_NAME);
      sb_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_SB_ATTR_TYPE) = "pattern";
      std::vector<std::string> sb_pattern(pair.first + 1, "1");
      sb_node.text().set(vtr::join(sb_pattern.begin(), sb_pattern.end(), " ").c_str());

      pugi::xml_node cb_node = seg_node.append_child(XML_VPR_ARCH_SEGMENTLIST_CB_NODE_NAME);
      cb_node.append_attribute(XML_VPR_ARCH_SEGMENTLIST_CB_ATTR_TYPE) = "pattern";
      std::vector<std::string> cb_pattern(pair.first, "1");
      cb_node.text().set(vtr::join(cb_pattern.begin(), cb_pattern.end(), " ").c_str());
    }
  }

  return num_err;
}

void VprArchLayout::check_valid_segment_type(const VprArchLayout::e_segment_type& type) const {
  if (!valid_segment_type(type)) {
    VTR_LOG_ERROR("Segment type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

VprArchLayout::e_segment_type VprArchLayout::str2segment_type(const std::string& str,
                                                              const bool& verbose) const {
  for (int itype = size_t(VprArchLayout::e_segment_type::X);
       itype < size_t(VprArchLayout::e_segment_type::NUM_TYPES); ++itype) {
    if (str == std::string(SEGMENT_TYPE_STRING_[itype])) {
      return static_cast<VprArchLayout::e_segment_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for routing segment! Expect %s\n",
                 segment_type_all2str().c_str());
  return VprArchLayout::e_segment_type::NUM_TYPES;
}

std::string VprArchLayout::segment_type2str(const VprArchLayout::e_segment_type& type,
                                            const bool& verbose) const {
  if (!valid_segment_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for routing segment! Expect %s\n",
                   segment_type_all2str().c_str());
    return std::string();
  }
  return std::string(SEGMENT_TYPE_STRING_[size_t(type)]);
}

std::string VprArchLayout::segment_type2str_uppercase(const VprArchLayout::e_segment_type& type,
                                                      const bool& verbose) const {
  if (!valid_segment_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for routing segment! Expect %s\n",
                   segment_type_all2str().c_str());
    return std::string();
  }
  return std::string(SEGMENT_TYPE_STRING_UPPERCASE_[size_t(type)]);
}

std::string VprArchLayout::segment_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(VprArchLayout::e_segment_type::X);
       itype < size_t(VprArchLayout::e_segment_type::NUM_TYPES); ++itype) {
    full_types += std::string(SEGMENT_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::vector<VprArchLayout::e_segment_type> VprArchLayout::segment_types() const {
  std::vector<e_segment_type> segment_types;
  for (int itype = size_t(VprArchLayout::e_segment_type::X);
       itype < size_t(VprArchLayout::e_segment_type::NUM_TYPES); ++itype) {
    segment_types.push_back(static_cast<VprArchLayout::e_segment_type>(itype));
  }
  return segment_types;
}

bool VprArchLayout::valid_segment_type(const e_segment_type& type) const {
  return type != VprArchLayout::e_segment_type::NUM_TYPES;
}

int VprArchLayout::vpr_directlist_xml_node(pugi::xml_node& directlist_node) const {
  int num_err = 0;

  for (auto dirkt : direct_connects_) {
    pugi::xml_node dirkt_node =
      directlist_node.append_child(XML_VPR_ARCH_DIRECTLIST_DIRECT_NODE_NAME);
    dirkt_node.append_attribute(XML_VPR_ARCH_DIRECTLIST_DIRECT_ATTR_NAME) = dirkt.name().c_str();
    dirkt_node.append_attribute(XML_VPR_ARCH_DIRECTLIST_DIRECT_ATTR_FROM_PIN) =
      dirkt.from_pin_name().c_str();
    dirkt_node.append_attribute(XML_VPR_ARCH_DIRECTLIST_DIRECT_ATTR_TO_PIN) =
      dirkt.to_pin_name().c_str();
    dirkt_node.append_attribute(XML_VPR_ARCH_DIRECTLIST_DIRECT_ATTR_X_OFFSET) =
      std::to_string(dirkt.x_offset).c_str();
    dirkt_node.append_attribute(XML_VPR_ARCH_DIRECTLIST_DIRECT_ATTR_Y_OFFSET) =
      std::to_string(dirkt.y_offset).c_str();
    dirkt_node.append_attribute(XML_VPR_ARCH_DIRECTLIST_DIRECT_ATTR_Z_OFFSET) =
      std::to_string(dirkt.z_offset).c_str();
    if (!dirkt.switch_name.empty()) {
      dirkt_node.append_attribute(XML_VPR_ARCH_DIRECTLIST_DIRECT_ATTR_SWITCH_NAME) =
        dirkt.switch_name.c_str();
    }
  }

  return num_err;
}

bool VprArchLayout::add_bus_inter_subtile_direct_connection(const std::string& tile_name,
                                                            const openfpga::BasicPort& from_port,
                                                            const openfpga::BasicPort& to_port,
                                                            const int& z_offset,
                                                            const bool& keep_bus) {
  /* Sanity check */
  auto tile_result = tile_name2ids_.find(tile_name);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("The tile '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  bool valid_from_port = false;
  for (std::string pb_type_name : tile_pb_types_[tile_result->second]) {
    auto pb_type_result = pb_type_name2ids_.find(pb_type_name);
    if (pb_type_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("The subtile '%s' is not defined!\n", pb_type_name.c_str());
      return false;
    }
    for (auto port : subtile_ports_[pb_type_result->second]) {
      if (from_port.get_name() == port.name && from_port.get_width() <= port.width) {
        valid_from_port = true;
      }
    }
  }
  if (!valid_from_port) {
    VTR_LOG_ERROR("The from port '%s' is not defined in the tile '%s'!\n",
                  from_port.to_verilog_string().c_str(), tile_name.c_str());
    return false;
  }
  bool valid_to_port = false;
  std::string to_subtile;
  for (std::string pb_type_name : tile_pb_types_[tile_result->second]) {
    auto pb_type_result = pb_type_name2ids_.find(pb_type_name);
    if (pb_type_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("The subtile '%s' is not defined!\n", pb_type_name.c_str());
      return false;
    }
    for (auto port : subtile_ports_[pb_type_result->second]) {
      if (to_port.get_name() == port.name && to_port.get_width() <= port.width) {
        valid_to_port = true;
      }
    }
  }
  if (!valid_to_port) {
    VTR_LOG_ERROR("The to port '%s' is not defined in the tile '%s'!\n",
                  to_port.to_verilog_string().c_str(), tile_name.c_str());
    return false;
  }

  /* From port and to_port should  match in sizes */
  if (from_port.get_width() != to_port.get_width()) {
    VTR_LOG_ERROR(
      "The from port '%s' does not match the port size '%lu' of to port '%s' in the tile '%s'!\n",
      from_port.to_verilog_string().c_str(), from_port.get_width(),
      to_port.to_verilog_string().c_str(), tile_name.c_str());
    return false;
  }
  if (keep_bus) {
    DirectConnect dirkt;

    dirkt.from_tile = tile_name;
    dirkt.from_port = from_port;

    dirkt.to_tile = tile_name;
    dirkt.to_port = to_port;

    dirkt.x_offset = 0;
    dirkt.y_offset = 0;
    dirkt.z_offset = z_offset;

    direct_connects_.push_back(dirkt);
  } else {
    for (size_t ipin = 0; ipin < from_port.get_width(); ++ipin) {
      DirectConnect dirkt;

      dirkt.from_tile = tile_name;
      dirkt.from_port =
        openfpga::BasicPort(from_port.get_name(), from_port.pins()[ipin], from_port.pins()[ipin]);

      dirkt.to_tile = tile_name;
      dirkt.to_port =
        openfpga::BasicPort(to_port.get_name(), to_port.pins()[ipin], to_port.pins()[ipin]);

      dirkt.x_offset = 0;
      dirkt.y_offset = 0;
      dirkt.z_offset = z_offset;

      direct_connects_.push_back(dirkt);
    }
  }
  return true;
}

bool VprArchLayout::add_feedback_direct_connection(const std::string& tile_name,
                                                   const std::string& from_port,
                                                   const size_t& from_pin,
                                                   const std::string& to_port,
                                                   const size_t& to_pin) {
  /* Sanity check */
  auto tile_result = tile_name2ids_.find(tile_name);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("The tile '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  bool valid_from_port = false;
  for (std::string pb_type_name : tile_pb_types_[tile_result->second]) {
    auto pb_type_result = pb_type_name2ids_.find(pb_type_name);
    if (pb_type_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("The subtile '%s' is not defined!\n", pb_type_name.c_str());
      return false;
    }
    for (auto port : subtile_ports_[pb_type_result->second]) {
      if (from_port == port.name && from_pin < port.width) {
        valid_from_port = true;
      }
    }
  }
  if (!valid_from_port) {
    VTR_LOG_ERROR("The from port '%s[%lu]' is not defined in the tile '%s'!\n", from_port.c_str(),
                  from_pin, tile_name.c_str());
    return false;
  }
  bool valid_to_port = false;
  for (std::string pb_type_name : tile_pb_types_[tile_result->second]) {
    auto pb_type_result = pb_type_name2ids_.find(pb_type_name);
    if (pb_type_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("The subtile '%s' is not defined!\n", pb_type_name.c_str());
      return false;
    }
    for (auto port : subtile_ports_[pb_type_result->second]) {
      if (to_port == port.name && to_pin < port.width) {
        valid_to_port = true;
      }
    }
  }
  if (!valid_to_port) {
    VTR_LOG_ERROR("The to port '%s[%lu]' is not defined in the tile '%s'!\n", to_port.c_str(),
                  to_pin, tile_name.c_str());
    return false;
  }

  DirectConnect dirkt;

  dirkt.from_tile = tile_name;
  dirkt.from_port = openfpga::BasicPort(from_port, from_pin, from_pin);

  dirkt.to_tile = tile_name;
  dirkt.to_port = openfpga::BasicPort(to_port, to_pin, to_pin);

  dirkt.x_offset = 0;
  dirkt.y_offset = 0;
  dirkt.z_offset = 0;

  dirkt.switch_name = CB_MUX_NAME_.c_str();

  direct_connects_.push_back(dirkt);
  return true;
}

bool VprArchLayout::add_carrychain_direct_connection(const std::string& tile_name,
                                                     const std::string& from_port,
                                                     const std::string& to_port,
                                                     const e_segment_type& seg_type,
                                                     const e_dir_type& dir, const size_t& width) {
  /* Sanity check */
  auto tile_result = tile_name2ids_.find(tile_name);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("The tile '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  /* Find the CIN pin */
  bool valid_from_port = false;
  for (std::string pb_type_name : tile_pb_types_[tile_result->second]) {
    auto pb_type_result = pb_type_name2ids_.find(pb_type_name);
    if (pb_type_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("The subtile '%s' is not defined!\n", pb_type_name.c_str());
      return false;
    }
    for (auto port : subtile_ports_[pb_type_result->second]) {
      if (from_port == port.name && port.type == e_port_type::CO) {
        valid_from_port = true;
      }
    }
  }
  if (!valid_from_port) {
    VTR_LOG_ERROR("The from port '%s' is not defined in the tile '%s'!\n", from_port.c_str(),
                  tile_name.c_str());
    return false;
  }
  bool valid_to_port = false;
  for (std::string pb_type_name : tile_pb_types_[tile_result->second]) {
    auto pb_type_result = pb_type_name2ids_.find(pb_type_name);
    if (pb_type_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("The subtile '%s' is not defined!\n", pb_type_name.c_str());
      return false;
    }
    for (auto port : subtile_ports_[pb_type_result->second]) {
      if (to_port == port.name && port.type == e_port_type::CI) {
        valid_to_port = true;
      }
    }
  }
  if (!valid_to_port) {
    VTR_LOG_ERROR("The to port '%s' is not defined in the tile '%s'!\n", to_port.c_str(),
                  tile_name.c_str());
    return false;
  }

  DirectConnect dirkt;

  dirkt.from_tile = tile_name;
  dirkt.from_port = openfpga::BasicPort(from_port, 0, width - 1);

  dirkt.to_tile = tile_name;
  dirkt.to_port = openfpga::BasicPort(to_port, 0, width - 1);

  if (seg_type == e_segment_type::X) {
    if (dir == e_dir_type::INC) {
      dirkt.x_offset = 1;
      dirkt.y_offset = 0;
      dirkt.z_offset = 0;
    } else {
      VTR_ASSERT(dir == e_dir_type::DEC);
      dirkt.x_offset = -1;
      dirkt.y_offset = 0;
      dirkt.z_offset = 0;
    }
  } else {
    VTR_ASSERT(seg_type == e_segment_type::Y);
    if (dir == e_dir_type::INC) {
      dirkt.x_offset = 0;
      dirkt.y_offset = 1;
      dirkt.z_offset = 0;
    } else {
      VTR_ASSERT(dir == e_dir_type::DEC);
      dirkt.x_offset = 0;
      dirkt.y_offset = -1;
      dirkt.z_offset = 0;
    }
  }

  direct_connects_.push_back(dirkt);
  return true;
}

std::vector<std::string> VprArchLayout::tiles() const {
  std::vector<std::string> ret;
  for (auto pair : tile_name2ids_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<openfpga::BasicPort> VprArchLayout::find_tile_ports_by_type(
  const std::string& name, const e_port_type& port_type) const {
  std::vector<openfpga::BasicPort> ret;
  auto tile_result = tile_name2ids_.find(name);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("Invalid tile '%s' which is not defined yet!\n", name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  size_t tile_id = tile_result->second;
  for (std::string pb_type_name : tile_pb_types_[tile_id]) {
    auto pb_result = pb_type_name2ids_.find(pb_type_name);
    if (pb_result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("Invalid subtile/pb_type '%s' under tile '%s' which is not defined yet!\n",
                    pb_type_name.c_str(), name.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
    for (auto subtile_port : subtile_ports_[pb_result->second]) {
      if (port_type == subtile_port.type) {
        ret.push_back(openfpga::BasicPort(subtile_port.name, subtile_port.width));
      }
    }
  }
  return ret;
}

std::vector<std::string> VprArchLayout::subtiles() const {
  std::vector<std::string> ret;
  for (auto pair : pb_type_name2ids_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<VprArchLayout::PortId> VprArchLayout::subtile_ports(const std::string& name) const {
  auto pbtype_result = pb_type_name2ids_.find(name);
  if (pbtype_result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Unable to find the subtile '%s' which is not defined.\n", name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::vector<PortId> ret;
  for (size_t i = 0; i < subtile_ports_[pbtype_result->second].size(); ++i) {
    ret.push_back(PortId(i));
  }
  return ret;
}

openfpga::BasicPort VprArchLayout::subtile_port(const std::string& name,
                                                const PortId& port_id) const {
  auto pbtype_result = pb_type_name2ids_.find(name);
  if (pbtype_result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Unable to find the subtile '%s' which is not defined.\n", name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (size_t(port_id) >= subtile_ports_[pbtype_result->second].size()) {
    VTR_LOG_ERROR("Invalid port id '%lu' for the subtile '%s' which is not defined.\n",
                  size_t(port_id), name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  openfpga::BasicPort ret;
  ret.set_name(subtile_ports_[pbtype_result->second][size_t(port_id)].name);
  ret.set_width(subtile_ports_[pbtype_result->second][size_t(port_id)].width);
  return ret;
}

VprArchLayout::e_port_type VprArchLayout::subtile_port_type(const std::string& name,
                                                            const PortId& port_id) const {
  auto pbtype_result = pb_type_name2ids_.find(name);
  if (pbtype_result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Unable to find the subtile '%s' which is not defined.\n", name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (size_t(port_id) >= subtile_ports_[pbtype_result->second].size()) {
    VTR_LOG_ERROR("Invalid port id '%lu' for the subtile '%s' which is not defined.\n",
                  size_t(port_id), name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return subtile_ports_[pbtype_result->second][size_t(port_id)].type;
}

bool VprArchLayout::subtile_port_logic_equivalent(const std::string& name,
                                                  const PortId& port_id) const {
  auto pbtype_result = pb_type_name2ids_.find(name);
  if (pbtype_result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Unable to find the subtile '%s' which is not defined.\n", name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (size_t(port_id) >= subtile_ports_[pbtype_result->second].size()) {
    VTR_LOG_ERROR("Invalid port id '%lu' for the subtile '%s' which is not defined.\n",
                  size_t(port_id), name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return subtile_ports_[pbtype_result->second][size_t(port_id)].logic_equi;
}

std::vector<VprArchLayout::DirectConnectId> VprArchLayout::direct_connects() const {
  std::vector<VprArchLayout::DirectConnectId> ret;
  ret.reserve(direct_connects_.size());
  for (size_t i = 0; i < direct_connects_.size(); ++i) {
    ret.push_back(VprArchLayout::DirectConnectId(i));
  }
  return ret;
}

bool VprArchLayout::valid_direct_connect_id(const VprArchLayout::DirectConnectId& id) const {
  return size_t(id) < direct_connects_.size();
}

void VprArchLayout::check_valid_direct_connect_id(const VprArchLayout::DirectConnectId& id) const {
  if (!valid_direct_connect_id(id)) {
    VTR_LOG_ERROR("Invalid direct connect id!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::string VprArchLayout::direct_connect_name(const VprArchLayout::DirectConnectId& id) const {
  check_valid_direct_connect_id(id);
  return direct_connects_[size_t(id)].name();
}

bool VprArchLayout::direct_connect_is_feedback_connection(
  const VprArchLayout::DirectConnectId& id) const {
  check_valid_direct_connect_id(id);
  return direct_connects_[size_t(id)].is_feedback_connection();
}

std::string VprArchLayout::find_ckbuf_pb_type() const {
  std::string ret;
  size_t num_ckbuf_subtile = 0;
  for (auto pair : pb_type_name2ids_) {
    if (subtile_types_[pair.second] == e_subtile_type::CKBUF) {
      ret = pair.first;
      num_ckbuf_subtile++;
    }
  }
  if (num_ckbuf_subtile == 0 || num_ckbuf_subtile > 1) {
    VTR_LOG_ERROR("Expect only 1 CKBUF tile but see %lu!\n", num_ckbuf_subtile);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret;
}

openfpga::BasicPort VprArchLayout::ckbuf_output_port() const {
  std::string ckbuf_subtile = find_ckbuf_pb_type();
  auto result = pb_type_name2ids_.find(ckbuf_subtile);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("CKBUF tile '%s' is not defined!\n", ckbuf_subtile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  /* Expect only 1 output */
  openfpga::BasicPort ret;
  size_t num_outputs = 0;
  for (auto st_port : subtile_ports_[result->second]) {
    if (st_port.type == e_port_type::OUTPUT) {
      num_outputs++;
      if (st_port.width != 1) {
        VTR_LOG_ERROR("Expect only 1 pin for ckbuf output '%s' for subtile '%s'. But see '%lu'\n!",
                      st_port.name.c_str(), ckbuf_subtile.c_str(), st_port.width);
        exit(openfpga::CMD_EXEC_FATAL_ERROR);
      }
      ret.set_name(st_port.name);
      ret.set_width(1);
    }
  }
  return ret;
}

bool VprArchLayout::is_io_tile(const std::string& tile_name) const {
  if (!valid_tile_name(tile_name)) {
    VTR_LOG_ERROR("The tile name '%s' is not defined!\n", tile_name.c_str());
    return false;
  }
  auto result = tile_name2ids_.find(tile_name);
  if (tile_types_[result->second] == e_tile_type::IO) {
    return true;
  }
  return false;
}

std::string VprArchLayout::find_clb_tile() const {
  std::string ret;
  size_t num_clb_tile = 0;
  for (auto pair : tile_name2ids_) {
    if (tile_types_[pair.second] == e_tile_type::CLB) {
      ret = pair.first;
      num_clb_tile++;
    }
  }
  if (num_clb_tile == 0 || num_clb_tile > 1) {
    VTR_LOG_ERROR("Expect only 1 CLB tile but see %lu!\n", num_clb_tile);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret;
}

size_t VprArchLayout::ckbuf_subtile_starting_index() const {
  std::string ckbuf_subtile = find_ckbuf_pb_type();
  auto ckbuf_result = pb_type_name2ids_.find(ckbuf_subtile);
  if (ckbuf_result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("CKBUF tile '%s' is not defined!\n", ckbuf_subtile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  /* Only focus on clb tiles */
  std::string clb_tile = find_clb_tile();
  auto tile_result = tile_name2ids_.find(clb_tile);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("CLB tile '%s' is not defined!\n", clb_tile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  size_t clb_tile_idx = tile_result->second;
  size_t start_idx = 0;
  for (size_t ist = 0; ist < tile_pb_types_[clb_tile_idx].size(); ++ist) {
    if (tile_pb_types_[clb_tile_idx][ist] == ckbuf_subtile) {
      break;
    }
    start_idx += subtile_capacity_[clb_tile_idx][ist];
  }
  return start_idx;
}

openfpga::BasicPort VprArchLayout::ckbuf_subtile(const size_t& index) const {
  std::string clb_tile = find_clb_tile();
  openfpga::BasicPort ret(clb_tile, index, index);
  return ret;
}

std::vector<std::string> VprArchLayout::tiles_contain_ports(const e_port_type& port_type) const {
  std::vector<std::string> ret;
  for (auto pair : tile_name2ids_) {
    bool found_port = false;
    for (std::string curr_pb_type : tile_pb_types_[pair.second]) {
      auto result = pb_type_name2ids_.find(curr_pb_type);
      if (result == pb_type_name2ids_.end()) {
        VTR_LOG_ERROR("Top-level pb_type '%s' is not defined!\n", curr_pb_type.c_str());
        exit(openfpga::CMD_EXEC_FATAL_ERROR);
      }
      bool subtile_found_port = false;
      /* Bypass CKBUF */
      if (subtile_types_[result->second] == e_subtile_type::CKBUF) {
        continue;
      }
      for (auto subtile_port : subtile_ports_[result->second]) {
        if (subtile_port.type == port_type) {
          subtile_found_port = true;
          break;
        }
      }
      if (subtile_found_port) {
        found_port = true;
        break;
      }
    }
    if (found_port) {
      ret.push_back(pair.first);
    }
  }
  return ret;
}
std::vector<size_t> VprArchLayout::subtile_indices_contain_ports(
  const std::string& tile, const e_port_type& port_type) const {
  std::vector<size_t> ret;
  auto tile_result = tile_name2ids_.find(tile);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("Top-level pb_type '%s' is not defined!\n", tile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  size_t tile_idx = tile_result->second;
  size_t curr_subtile_idx = 0;
  for (size_t subtile_idx = 0; subtile_idx < tile_pb_types_[tile_idx].size(); ++subtile_idx) {
    std::string curr_pb_type = tile_pb_types_[tile_idx][subtile_idx];
    auto result = pb_type_name2ids_.find(curr_pb_type);
    if (result == pb_type_name2ids_.end()) {
      VTR_LOG_ERROR("Top-level pb_type '%s' is not defined!\n", curr_pb_type.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
    /* Bypass CKBUF */
    if (subtile_types_[result->second] == e_subtile_type::CKBUF) {
      continue;
    }
    bool subtile_found_port = false;
    for (auto subtile_port : subtile_ports_[result->second]) {
      if (subtile_port.type == port_type) {
        subtile_found_port = true;
        break;
      }
    }
    if (subtile_found_port) {
      for (size_t i_idx = 0; i_idx < subtile_capacity_[tile_idx][subtile_idx]; ++i_idx) {
        ret.push_back(curr_subtile_idx + i_idx);
      }
    }
    curr_subtile_idx += subtile_capacity_[tile_idx][subtile_idx];
  }
  return ret;
}

std::vector<openfpga::BasicPort> VprArchLayout::subtile_pins(const std::string& tile,
                                                             const size_t& subtile_idx,
                                                             const e_port_type& port_type) const {
  std::vector<openfpga::BasicPort> ret;
  auto tile_result = tile_name2ids_.find(tile);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("Top-level pb_type '%s' is not defined!\n", tile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  size_t tile_idx = tile_result->second;
  size_t curr_subtile_idx = 0;
  size_t target_st_idx = tile_pb_types_[tile_idx].size();
  for (size_t st_idx = 0; st_idx < tile_pb_types_[tile_idx].size(); ++st_idx) {
    curr_subtile_idx += subtile_capacity_[tile_idx][st_idx];
    if (subtile_idx < curr_subtile_idx) {
      target_st_idx = st_idx;
      break;
    }
  }
  if (target_st_idx >= tile_pb_types_[tile_idx].size()) {
    VTR_LOG_ERROR("Subtile index '%lu' is out of range [0, %lu) for tile '%s'\n", target_st_idx,
                  tile_pb_types_[tile_idx].size(), tile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::string curr_pb_type = tile_pb_types_[tile_idx][target_st_idx];
  auto result = pb_type_name2ids_.find(curr_pb_type);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Top-level pb_type '%s' is not defined!\n", curr_pb_type.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  for (auto subtile_port : subtile_ports_[result->second]) {
    if (subtile_port.type != port_type) {
      continue;
    }
    for (size_t ipin = 0; ipin < subtile_port.width; ++ipin) {
      ret.push_back(openfpga::BasicPort(subtile_port.name, ipin, ipin));
    }
  }
  return ret;
}

bool VprArchLayout::subtile_pin_driven_by_greset(const std::string& tile, const size_t& subtile_idx,
                                                 const openfpga::BasicPort& pin) const {
  auto tile_result = tile_name2ids_.find(tile);
  if (tile_result == tile_name2ids_.end()) {
    VTR_LOG_ERROR("Top-level pb_type '%s' is not defined!\n", tile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  size_t tile_idx = tile_result->second;
  size_t curr_subtile_idx = 0;
  size_t target_st_idx = tile_pb_types_[tile_idx].size();
  for (size_t st_idx = 0; st_idx < tile_pb_types_[tile_idx].size(); ++st_idx) {
    curr_subtile_idx += subtile_capacity_[tile_idx][st_idx];
    if (subtile_idx < curr_subtile_idx) {
      target_st_idx = st_idx;
      break;
    }
  }
  if (target_st_idx >= tile_pb_types_[tile_idx].size()) {
    VTR_LOG_ERROR("Subtile index '%lu' is out of range [0, %lu) for tile '%s'\n", target_st_idx,
                  tile_pb_types_[tile_idx].size(), tile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  std::string curr_pb_type = tile_pb_types_[tile_idx][target_st_idx];
  auto result = pb_type_name2ids_.find(curr_pb_type);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Top-level pb_type '%s' is not defined!\n", curr_pb_type.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  for (auto subtile_port : subtile_ports_[result->second]) {
    if (subtile_port.name != pin.get_name()) {
      continue;
    }
    openfpga::BasicPort curr_port(subtile_port.name, subtile_port.width);
    if (!curr_port.contained(pin)) {
      VTR_LOG_ERROR(
        "The given pin '%s' is not a valid in the range of port '%s' of pb_type '%s'!\n",
        pin.to_verilog_string().c_str(), curr_port.to_verilog_string().c_str(),
        curr_pb_type.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
    return subtile_port.driven_by_greset;
  }
  return false;
}

std::vector<std::string> VprArchLayout::subtiles(const std::string& tile) const {
  auto t_result = tile_name2ids_.find(tile);
  if (t_result == tile_name2ids_.end()) {
    return std::vector<std::string>();
  }
  return subtile_names_[t_result->second];
}

std::string VprArchLayout::subtile_physical_equivalent_site(const std::string& tile,
                                                            const std::string& subtile) const {
  auto t_result = tile_name2ids_.find(tile);
  if (t_result == tile_name2ids_.end()) {
    return std::string();
  }
  for (size_t ist = 0; ist < subtile_names_[t_result->second].size(); ist++) {
    if (subtile_names_[t_result->second][ist] == subtile) {
      return tile_pb_types_[t_result->second][ist];
    }
  }
  return std::string();
}

}  // End namespace arkangel
