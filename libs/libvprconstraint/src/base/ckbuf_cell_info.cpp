
#include "ckbuf_cell_info.h"

#include "command_exit_codes.h"
#include "vtr_log.h"
#include "vtr_util.h"

namespace arcxpress {

/*This files stores information read from cell_map*/
CkbufInfo::CkbufId_range CkbufInfo::ckbufs() const {
  return vtr::make_range(ckbuf_ids_.begin(), ckbuf_ids_.end());
}

CkbufInfo::CkbufId CkbufInfo::create_ckbuf_info_instance(const std::string& block_name,
                                                         const std::string& input_net_name,
                                                         const std::string& port_type) {
  CkbufId new_id = CkbufId(ckbuf_ids_.size());
  ckbuf_ids_.push_back(new_id);
  ckbuf_name_.push_back(block_name);
  ckbuf_input_net_.push_back(input_net_name);
  ckbuf_is_clk_.emplace_back();
  ckbuf_is_reset_.emplace_back();
  ckbuf_assigned_port_.emplace_back();
  ckbuf_assigned_location_.emplace_back(vtr::Point<int>(-1, -1));
  ckbuf_assigned_subtile_.emplace_back();
  set_ckbuf_input_net_type(new_id, port_type);
  return new_id;
}

std::string CkbufInfo::get_ckbuf_name(CkbufId ckbuf_id) const {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  return ckbuf_name_[ckbuf_id];
}

std::string CkbufInfo::get_ckbuf_input_net(CkbufId ckbuf_id) const {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  return ckbuf_input_net_[ckbuf_id];
}

bool CkbufInfo::is_ckbuf_clk(CkbufId ckbuf_id) const {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  return ckbuf_is_clk_[ckbuf_id];
}

bool CkbufInfo::is_ckbuf_reset(CkbufId ckbuf_id) const {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  return ckbuf_is_reset_[ckbuf_id];
}

void CkbufInfo::set_ckbuf_name(CkbufId ckbuf_id, const std::string& name) {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  ckbuf_name_[ckbuf_id] = name;
}

void CkbufInfo::set_ckbuf_input_net(CkbufId ckbuf_id, const std::string& input) {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  ckbuf_input_net_[ckbuf_id] = input;
}

void CkbufInfo::set_ckbuf_input_net_type(CkbufId ckbuf_id, const std::string& port_type) {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  if (port_type == "clock") {
    ckbuf_is_clk_[ckbuf_id] = true;
    ckbuf_is_reset_[ckbuf_id] = false;
  } else if (port_type == "reset") {
    ckbuf_is_clk_[ckbuf_id] = false;
    ckbuf_is_reset_[ckbuf_id] = true;
  } else {
    VTR_LOG_ERROR("Invalid type: %s", port_type.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void CkbufInfo::set_ckbuf_assigned_port(CkbufId ckbuf_id,
                                        const openfpga::BasicPort& assigned_ckbuf_port) {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  ckbuf_assigned_port_[ckbuf_id] = assigned_ckbuf_port;
}

openfpga::BasicPort CkbufInfo::get_ckbuf_assigned_port(CkbufId ckbuf_id) const {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  return ckbuf_assigned_port_[ckbuf_id];
}

void CkbufInfo::set_ckbuf_assigned_location(CkbufId ckbuf_id,
                                            const vtr::Point<int>& assigned_location) {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  ckbuf_assigned_location_[ckbuf_id] = assigned_location;
}

vtr::Point<int> CkbufInfo::get_ckbuf_assigned_location(CkbufId ckbuf_id) const {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  return ckbuf_assigned_location_[ckbuf_id];
}

void CkbufInfo::set_ckbuf_assigned_subtile(CkbufId ckbuf_id, int assigned_subtile) {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  ckbuf_assigned_subtile_[ckbuf_id] = assigned_subtile;
}

int CkbufInfo::get_ckbuf_assigned_subtile(CkbufId ckbuf_id) const {
  VTR_ASSERT(valid_ckbuf_id(ckbuf_id));
  return ckbuf_assigned_subtile_[ckbuf_id];
}

bool CkbufInfo::valid_ckbuf_id(CkbufId ckbuf_id) const {
  return (size_t(ckbuf_id) < ckbuf_ids_.size()) && (ckbuf_id == ckbuf_ids_[ckbuf_id]);
}

};  // namespace arcxpress
