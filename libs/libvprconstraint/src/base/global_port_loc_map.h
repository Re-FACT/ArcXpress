#pragma once

#include <map>

#include "openfpga_port.h"
#include "vtr_geometry.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

namespace arcxpress {
class GlobalPortLoc {
 public:
  GlobalPortLoc() { return; }

 public:
  /*GlobalPortId*/
  struct global_port_id_tag;
  typedef vtr::StrongId<global_port_id_tag> GlobalPortId;
  typedef vtr::vector<GlobalPortId, GlobalPortId>::const_iterator Global_PortId_iterator;
  typedef vtr::Range<Global_PortId_iterator> GlobalPortId_range;

  GlobalPortId_range global_ports() const;

  GlobalPortId create_port_info_instance(const openfpga::BasicPort& port,
                                         const vtr::Point<int>& coord, const int subtile,
                                         const std::string& pin_type);

  bool is_global_port_clk(GlobalPortId port_id) const;
  bool is_global_port_clk(const openfpga::BasicPort& global_port_name) const;

  bool is_global_port_reset(GlobalPortId port_id) const;
  bool is_global_port_reset(const openfpga::BasicPort& global_port_name) const;
  std::string global_port_type_string(GlobalPortId port_id) const;

  openfpga::BasicPort get_global_port_name(GlobalPortId port_id) const;

  vtr::Point<int> get_global_port_coord(GlobalPortId port_id) const;

  int get_global_port_subtile(GlobalPortId port_id) const;

  bool is_global_port_assigned(GlobalPortId port_id) const;

  void set_global_port_coords(GlobalPortId port_id, vtr::Point<int> coords);

  void set_global_port_subtile(GlobalPortId port_id, int subtile);

  void set_global_port_assigned_status(GlobalPortId port_id, bool flag);

  void set_global_port_name(GlobalPortId port_id, openfpga::BasicPort port);

  void set_global_port_type(GlobalPortId port_id, std::string port_type);
  int global_port_num() const;
  std::vector<openfpga::BasicPort> clock_ports() const;
  std::vector<openfpga::BasicPort> reset_ports() const;

 private:
  bool valid_global_port_id(GlobalPortId port_id) const;
  /*Information from clk_loc_map*/
  vtr::vector<GlobalPortId, GlobalPortId> global_port_ids_;
  vtr::vector<GlobalPortId, openfpga::BasicPort> global_port_name_;
  vtr::vector<GlobalPortId, vtr::Point<int>> global_port_coords_;
  vtr::vector<GlobalPortId, int> global_port_subtile_;
  vtr::vector<GlobalPortId, bool> global_port_is_clk_;
  vtr::vector<GlobalPortId, bool> global_port_is_reset_;
  vtr::vector<GlobalPortId, bool> global_port_is_assigned_;
};

}  // namespace arcxpress
