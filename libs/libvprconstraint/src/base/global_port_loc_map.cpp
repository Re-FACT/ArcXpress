

#include "global_port_loc_map.h"

#include "command_exit_codes.h"
#include "vtr_log.h"
#include "vtr_util.h"

namespace arkangel {

GlobalPortLoc::GlobalPortId_range GlobalPortLoc::global_ports() const {
  return vtr::make_range(global_port_ids_.begin(), global_port_ids_.end());
}

GlobalPortLoc::GlobalPortId GlobalPortLoc::create_port_info_instance(
  const openfpga::BasicPort& port, const vtr::Point<int>& coord, const int subtile,
  const std::string& pin_type) {
  GlobalPortId new_id = GlobalPortId(global_port_ids_.size());
  global_port_ids_.push_back(new_id);
  global_port_name_.push_back(port);
  global_port_coords_.push_back(coord);
  global_port_subtile_.push_back(subtile);
  global_port_is_clk_.emplace_back();
  global_port_is_reset_.emplace_back();
  global_port_is_assigned_.push_back(false);
  set_global_port_type(new_id, pin_type);
  return new_id;
}

std::vector<openfpga::BasicPort> GlobalPortLoc::clock_ports() const {
  std::vector<openfpga::BasicPort> ret;
  for (auto port_id : global_ports()) {
    if (global_port_is_clk_[port_id]) {
      ret.push_back(global_port_name_[port_id]);
    }
  }
  return ret;
}

std::vector<openfpga::BasicPort> GlobalPortLoc::reset_ports() const {
  std::vector<openfpga::BasicPort> ret;
  for (auto port_id : global_ports()) {
    if (global_port_is_reset_[port_id]) {
      ret.push_back(global_port_name_[port_id]);
    }
  }
  return ret;
}

int GlobalPortLoc::global_port_num() const {
  int global_port_num = 0;
  for (auto port_id : global_ports()) {
    if (global_port_is_clk_[port_id]) {
      global_port_num++;
    }
    if (global_port_is_reset_[port_id]) {
      global_port_num++;
    }
  }
  return global_port_num;
}

void GlobalPortLoc::set_global_port_name(GlobalPortId port_id, openfpga::BasicPort port) {
  VTR_ASSERT(valid_global_port_id(port_id));
  global_port_name_[port_id] = port;
}

void GlobalPortLoc::set_global_port_subtile(GlobalPortId port_id, int subtile) {
  VTR_ASSERT(valid_global_port_id(port_id));
  global_port_subtile_[port_id] = subtile;
}

void GlobalPortLoc::set_global_port_type(GlobalPortId port_id, std::string port_type) {
  VTR_ASSERT(valid_global_port_id(port_id));
  if (port_type == "clock") {
    global_port_is_clk_[port_id] = true;
    global_port_is_reset_[port_id] = false;
  } else if (port_type == "reset") {
    global_port_is_clk_[port_id] = false;
    global_port_is_reset_[port_id] = true;
  } else {
    VTR_LOG_ERROR("Invalid port type: ", port_type.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::string GlobalPortLoc::global_port_type_string(GlobalPortId port_id) const {
  VTR_ASSERT(valid_global_port_id(port_id));
  if (global_port_is_clk_[port_id]) {
    return "clock";
  }
  if (global_port_is_reset_[port_id]) {
    return "reset";
  }
  VTR_LOG_ERROR("Invalid port type which is neither clock nor reset!\n");
  exit(openfpga::CMD_EXEC_FATAL_ERROR);
}

bool GlobalPortLoc::is_global_port_assigned(GlobalPortId port_id) const {
  VTR_ASSERT(valid_global_port_id(port_id));
  return global_port_is_assigned_[port_id];
}

bool GlobalPortLoc::is_global_port_clk(GlobalPortId port_id) const {
  VTR_ASSERT(valid_global_port_id(port_id));
  return global_port_is_clk_[port_id];
}

bool GlobalPortLoc::is_global_port_clk(const openfpga::BasicPort& global_port_name) const {
  for (auto id : global_ports()) {
    if (get_global_port_name(id) == global_port_name) {
      return is_global_port_clk(id);
    }
  }
  return false;
}

bool GlobalPortLoc::is_global_port_reset(GlobalPortId port_id) const {
  VTR_ASSERT(valid_global_port_id(port_id));
  return global_port_is_reset_[port_id];
}
bool GlobalPortLoc::is_global_port_reset(const openfpga::BasicPort& global_port_name) const {
  for (auto id : global_ports()) {
    if (get_global_port_name(id) == global_port_name) {
      return is_global_port_reset(id);
    }
  }
  return false;
}

void GlobalPortLoc::set_global_port_coords(GlobalPortId port_id, vtr::Point<int> coords) {
  VTR_ASSERT(valid_global_port_id(port_id));
  global_port_coords_[port_id] = coords;
}

void GlobalPortLoc::set_global_port_assigned_status(GlobalPortId port_id, bool flag) {
  VTR_ASSERT(valid_global_port_id(port_id));
  global_port_is_assigned_[port_id] = flag;
}

openfpga::BasicPort GlobalPortLoc::get_global_port_name(GlobalPortId port_id) const {
  VTR_ASSERT(valid_global_port_id(port_id));
  return global_port_name_[port_id];
}

vtr::Point<int> GlobalPortLoc::get_global_port_coord(GlobalPortId port_id) const {
  VTR_ASSERT(valid_global_port_id(port_id));
  return global_port_coords_[port_id];
}

int GlobalPortLoc::get_global_port_subtile(GlobalPortId port_id) const {
  VTR_ASSERT(valid_global_port_id(port_id));
  return global_port_subtile_[port_id];
}

bool GlobalPortLoc::valid_global_port_id(GlobalPortId port_id) const {
  return (size_t(port_id) < global_port_ids_.size()) && (port_id == global_port_ids_[port_id]);
}

};  // namespace arkangel
