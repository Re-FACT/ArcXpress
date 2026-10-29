#pragma once

#include <map>

#include "openfpga_port.h"
#include "vtr_geometry.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

namespace arcxpress {
class CkbufInfo {
 public:
  CkbufInfo() { return; }

 public:
  /* CkbufId */
  struct ckbuf_info_id_tag;
  typedef vtr::StrongId<ckbuf_info_id_tag> CkbufId;
  typedef vtr::vector<CkbufId, CkbufId>::const_iterator CkbufId_iterator;
  typedef vtr::Range<CkbufId_iterator> CkbufId_range;
  CkbufId_range ckbufs() const;
  CkbufId create_ckbuf_info_instance(const std::string& block_name,
                                     const std::string& input_net_name,
                                     const std::string& port_type);

  std::string get_ckbuf_name(CkbufId ckbuf_id) const;

  std::string get_ckbuf_input_net(CkbufId ckbuf_id) const;

  vtr::Point<int> get_ckbuf_assigned_location(CkbufId ckbuf_id) const;

  int get_ckbuf_assigned_subtile(CkbufId ckbuf_id) const;

  bool is_ckbuf_clk(CkbufId ckbuf_id) const;

  bool is_ckbuf_reset(CkbufId ckbuf_id) const;

  void set_ckbuf_name(CkbufId ckbuf_id, const std::string& name);

  void set_ckbuf_input_net(CkbufId ckbuf_id, const std::string& input);

  void set_ckbuf_input_net_type(CkbufId ckbuf_id, const std::string& port_type);

  void set_ckbuf_assigned_port(CkbufId ckbuf_id, const openfpga::BasicPort& assigned_ckbuf_port);

  openfpga::BasicPort get_ckbuf_assigned_port(CkbufId ckbuf_id) const;

  void set_ckbuf_assigned_location(CkbufId ckbuf_id, const vtr::Point<int>& assigned_location);

  void set_ckbuf_assigned_subtile(CkbufId ckbuf_id, int assigned_subtile);

 private:
  bool valid_ckbuf_id(CkbufId ckbuf_id) const;
  /* information from cell_map */
  vtr::vector<CkbufId, std::string> ckbuf_name_;
  vtr::vector<CkbufId, std::string> ckbuf_input_net_;
  vtr::vector<CkbufId, CkbufId> ckbuf_ids_;
  vtr::vector<CkbufId, bool> ckbuf_is_clk_;
  vtr::vector<CkbufId, bool> ckbuf_is_reset_;
  vtr::vector<CkbufId, openfpga::BasicPort> ckbuf_assigned_port_;
  vtr::vector<CkbufId, vtr::Point<int>> ckbuf_assigned_location_;
  vtr::vector<CkbufId, int> ckbuf_assigned_subtile_;
};
}  // namespace arcxpress