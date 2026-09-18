#pragma once

#include <cstdint>  // Required for uint16_t
#include <map>
#include <string>
#include <vector>

#include "openfpga_port.h"
#include "physical_types.h"
#include "pugixml.hpp"
#include "vtr_geometry.h"
#include "vtr_strong_id.h"

/* namespace alkaidC begins */
namespace arkangel {

/**
 * arch package generator
 */
class VprArchLayout {
 public: /* -- Constructor -- */
  VprArchLayout();

 public: /* -- TYPES -- */
  struct port_id_tag;
  typedef vtr::StrongId<port_id_tag> PortId;
  struct direct_connect_id_tag;
  typedef vtr::StrongId<direct_connect_id_tag> DirectConnectId;
  enum class e_tile_type { IO = 0, CLB, CFB, NUM_TYPES };
  enum class e_subtile_type {
    PI = 0,
    PO,
    CLB,
    CKBUF,
    CFB, /* Custom functional block */
    NUM_TYPES
  };
  enum class e_port_type { INPUT = 0, OUTPUT, CLOCK, RESET, SCANI, SCANO, CI, CO, NUM_TYPES };
  enum class e_segment_type { X = 0, Y, NUM_TYPES };
  enum class e_dir_type { INC = 0, DEC, NUM_TYPES };
  enum class e_fabric_type { DP = 0, ULTIMATE, NUM_TYPES };

  typedef struct t_sub_tile_port {
    std::string name;
    size_t width;
    e_port_type type;
    bool logic_equi;
    float fc;
    bool driven_by_greset; /* Allow the pin to be driven by global reset network. By default, it is
                              true for all the inputs */
  } SubTilePort;

  typedef struct t_sub_tile_pinloc {
    size_t pb_type_port_id;
    size_t subtile_id; /* The id in the range of capacity of the subtile */
    size_t pin_id;     /* The id in the range of pb type port width */
  } SubTilePinloc;

  typedef struct t_direct_connect {
    std::string from_tile;
    openfpga::BasicPort from_port;
    std::string to_tile;
    openfpga::BasicPort to_port;
    short x_offset;
    short y_offset;
    short z_offset;
    std::string switch_name = "";
    /* TODO: Move the methods to parent class to save memory */
    std::string name() const {
      return std::string(from_tile) + std::string("_") + from_port.to_verilog_string() +
             std::string("_to_") + std::string(to_tile) + std::string("_") +
             to_port.to_verilog_string();
    }
    std::string from_pin_name() const {
      return std::string(from_tile) + std::string(".") + from_port.get_name() + std::string("[") +
             std::to_string(from_port.get_msb()) + std::string(":") +
             std::to_string(from_port.get_lsb()) + std::string("]");
    }
    std::string to_pin_name() const {
      return std::string(to_tile) + std::string(".") + to_port.get_name() + std::string("[") +
             std::to_string(to_port.get_msb()) + std::string(":") +
             std::to_string(to_port.get_lsb()) + std::string("]");
    }
    bool is_feedback_connection() const { return x_offset == 0 && y_offset == 0 && z_offset == 0; }
  } DirectConnect;

 public: /* -- Accessors -- */
  int vpr_tile_xml_node(pugi::xml_node& tiles_node) const;
  int vpr_layout_xml_node(pugi::xml_node& layout_node) const;
  int vpr_device_xml_node(pugi::xml_node& device_node) const;
  int vpr_switchlist_xml_node(pugi::xml_node& switchlist_node) const;
  int vpr_segmentlist_xml_node(pugi::xml_node& switchlist_node) const;
  int vpr_directlist_xml_node(pugi::xml_node& directlist_node) const;
  std::vector<std::string> tiles() const;
  std::vector<std::string> subtiles(const std::string& tile) const;
  /* There must be a physical equivalent site. If not found, return an empty string */
  std::string subtile_physical_equivalent_site(const std::string& tile,
                                               const std::string& subtile) const;
  std::vector<openfpga::BasicPort> find_tile_ports_by_type(const std::string& name,
                                                           const e_port_type& port_type) const;
  std::vector<std::string> subtiles() const;
  std::vector<PortId> subtile_ports(const std::string& name) const;
  openfpga::BasicPort subtile_port(const std::string& name, const PortId& port_id) const;
  e_port_type subtile_port_type(const std::string& name, const PortId& port_id) const;
  bool subtile_port_logic_equivalent(const std::string& name, const PortId& port_id) const;
  std::string cb_mux_name() const;
  std::string clk_mux_name() const;
  std::string sb_mux_name(const e_segment_type& type, const uint16_t& len) const;
  std::vector<e_segment_type> segment_types() const;
  std::vector<size_t> segment_lengths(const e_segment_type& type) const;
  std::string segment_name(const e_segment_type& type, const size_t& len) const;
  std::string clk_segment_name() const;
  std::vector<DirectConnectId> direct_connects() const;
  std::string direct_connect_name(const DirectConnectId& id) const;
  bool direct_connect_is_feedback_connection(const DirectConnectId& id) const;
  size_t fabric_height(const e_fabric_type& fb_type) const;
  size_t fabric_width(const e_fabric_type& fb_type) const;
  openfpga::BasicPort ckbuf_output_port() const;
  size_t ckbuf_subtile_starting_index() const;
  openfpga::BasicPort ckbuf_subtile(const size_t& index) const;
  std::string find_ckbuf_pb_type() const;
  std::string find_clb_tile() const;
  bool is_io_tile(const std::string& tile_name) const;
  std::vector<std::string> tiles_contain_ports(const e_port_type& port_type) const;
  std::vector<size_t> subtile_indices_contain_ports(const std::string& tile,
                                                    const e_port_type& port_type) const;
  std::vector<openfpga::BasicPort> subtile_pins(const std::string& tile, const size_t& subtile_idx,
                                                const e_port_type& port_type) const;

  bool subtile_pin_driven_by_greset(const std::string& tile, const size_t& subtile_idx,
                                    const openfpga::BasicPort& pin) const;

 private: /* -- Private utilities -- */
  std::string find_tile_name(const e_tile_type& type, const e_side& side = NUM_2D_SIDES) const;
  int vpr_one_tile_xml_node(pugi::xml_node& tiles_node, const std::string& tile_name) const;
  int vpr_one_subtile_xml_node(pugi::xml_node& tile_node, const std::string& tile_name,
                               const size_t& subtile_id_in_parent) const;
  std::string find_port_xml_node_name(const SubTilePort& port) const;
  int vpr_auto_layout_xml_node(pugi::xml_node& layout_node) const;
  int vpr_dp_layout_xml_node(pugi::xml_node& layout_node) const;
  int vpr_ultimate_layout_xml_node(pugi::xml_node& layout_node) const;
  std::string sb_mux_tdel_name(const e_segment_type& type, const uint16_t& len) const;
  size_t route_chan_width(const e_segment_type& type) const;

  e_segment_type str2segment_type(const std::string& str, const bool& verbose = false) const;
  std::string segment_type2str(const e_segment_type& type, const bool& verbose = false) const;
  std::string segment_type2str_uppercase(const e_segment_type& type,
                                         const bool& verbose = false) const;
  std::string segment_type_all2str() const;
  bool valid_segment_type(const e_segment_type& type) const;
  void check_valid_segment_type(const e_segment_type& type) const;

  bool valid_tile_name(const std::string& name) const;

  std::string coordinate2str(const vtr::Point<size_t>& coord) const;
  vtr::Point<size_t> str2coordinate(const std::string& coord_str) const;

 public: /* -- Mutators -- */
  void set_default_fc_in(const float& val);
  void set_default_fc_out(const float& val);
  bool create_pb_type(const std::string& name, const e_subtile_type& type);
  bool add_pb_type_port(const std::string& pb_type_name, const std::string& port_name,
                        const e_port_type& port_type, const size_t& port_width = 1);
  bool set_pb_type_port_driven_by_greset(const std::string& pb_type_name,
                                         const std::string& port_name, const bool& val);
  bool set_pb_type_port_logic_equivalent(const std::string& pb_type_name,
                                         const std::string& port_name, const bool& val);
  bool set_pb_type_port_fc(const std::string& pb_type_name, const std::string& port_name,
                           const float& val);

  bool create_tile(const std::string& name, const e_tile_type& type,
                   const e_side& side = NUM_2D_SIDES);
  bool add_subtile(const std::string& tile_name, const std::string& subtile_name,
                   const std::string& pb_type_name,
                   const std::vector<std::string>& equ_pb_type_names, const size_t& capacity = 1);
  bool add_subtile_pin_location(const std::string& tile_name, const std::string& subtile_name,
                                const size_t& subtile_idx, const std::string& pin_name,
                                const size_t& pin_index, const e_side& pin_side);
  bool set_dp_layout(const size_t& height, const size_t& width);
  bool set_ultimate_layout(const size_t& height, const size_t& width);
  bool set_dp_layout_column(const std::string& tile_name, const size_t& col_idx);
  bool set_dp_layout_single(const std::string& tile_name, const vtr::Point<size_t>& coord);
  bool set_ultimate_layout_column(const std::string& tile_name, const size_t& col_idx);
  bool set_ultimate_layout_single(const std::string& tile_name, const vtr::Point<size_t>& coord);
  bool add_segment(const e_segment_type& type, const short& length, const short& count);
  bool add_feedback_direct_connection(const std::string& tile_name, const std::string& from_port,
                                      const size_t& from_pin, const std::string& to_port,
                                      const size_t& to_pin);
  bool add_carrychain_direct_connection(const std::string& tile_name, const std::string& from_port,
                                        const std::string& to_port, const e_segment_type& seg_type,
                                        const e_dir_type& dir, const size_t& width = 1);
  bool add_bus_inter_subtile_direct_connection(const std::string& tile_name,
                                               const openfpga::BasicPort& from_port,
                                               const openfpga::BasicPort& to_port,
                                               const int& z_offset, const bool& keep_bus = false);

 public: /* -- Validators -- */
  bool valid_tile_id(const size_t& id) const;
  bool valid_fabric_type(const VprArchLayout::e_fabric_type& type) const;
  void check_valid_fabric_type(const VprArchLayout::e_fabric_type& type) const;
  bool valid_pb_type_id(const size_t& id) const;
  bool valid_direct_connect_id(const VprArchLayout::DirectConnectId& id) const;
  void check_valid_direct_connect_id(const VprArchLayout::DirectConnectId& id) const;

 private: /* -- Internal data -- */
  /* Settings */
  float default_fc_in_;
  float default_fc_out_;

  /* Basic Information */
  std::map<std::string, size_t> tile_name2ids_;
  std::vector<e_side> tile_sides_;
  std::vector<bool> tile_major_;
  std::vector<e_tile_type> tile_types_;
  std::vector<std::vector<std::string>> subtile_names_;
  std::vector<std::vector<std::vector<std::string>>> tile_equ_pb_types_;
  std::vector<std::vector<std::string>> tile_pb_types_;
  std::vector<std::vector<size_t>> subtile_capacity_;
  /* [tile_id][subtile_id][side][pins] */
  std::vector<std::vector<std::array<std::vector<SubTilePinloc>, 4>>> subtile_pinloc_;

  std::map<std::string, size_t> pb_type_name2ids_;
  std::vector<e_subtile_type> subtile_types_;
  std::vector<std::vector<SubTilePort>> subtile_ports_;

  /* Layout */
  vtr::Point<size_t> dp_layout_ = vtr::Point<size_t>(-1, -1);
  std::map<size_t, std::string> dp_layout_column_tiles_;
  std::map<std::string, std::string> dp_layout_single_tiles_;
  vtr::Point<size_t> ultimate_layout_ = vtr::Point<size_t>(-1, -1);
  std::map<size_t, std::string> ultimate_layout_column_tiles_;
  std::map<std::string, std::string> ultimate_layout_single_tiles_;
  size_t BASE_PRIORITY_;

  /* Switchlist, length reprents L1, L2 etc.
   * Allow different segment distribution on x- and y- channels
   * count denotes the number wires with starting point per channel
   * For example, the actual number of L2 in a channel is length x count
   */
  std::array<std::map<uint16_t, uint16_t>, size_t(e_segment_type::NUM_TYPES)> seg_length2count_;
  std::string CB_MUX_NAME_;
  std::string CK_MUX_NAME_;
  std::array<const char*, size_t(e_segment_type::NUM_TYPES)> SEGMENT_TYPE_STRING_;
  std::array<const char*, size_t(e_segment_type::NUM_TYPES)> SEGMENT_TYPE_STRING_UPPERCASE_;

  /* Direct connections */
  std::vector<DirectConnect> direct_connects_;
};

}  // namespace arkangel
