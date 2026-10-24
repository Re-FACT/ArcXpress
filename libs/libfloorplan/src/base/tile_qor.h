#pragma once

#include <array>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include "bitstream_manager.h"
#include "floorplan_params.h"
#include "openfpga_port.h"
#include "physical_types.h"
#include "pugixml.hpp"
#include "tile_qor_fwd.h"
#include "tile_ref.h"
#include "vtr_geometry.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"

/* namespace arcxpress begins */
namespace arcxpress {
/**
 * The data structure inherits from tile_refs. It includes infomation and util functions for
 * floorplan
 */
class TileQor {
 public: /* -- Types -- */
  /* Each node represents a location where a buffer should be added or net partition is required
   * Each node drive a list of other nodes, which are not leaf nodes
   * Leaf node can only drive a number of tiles
   */
  struct SdNtwkNode {
    int parent_node = -1;
    vtr::Point<size_t> location; /* The logical location in the eFPGA fabric in terms of number of
                                    rows and columns */
    vtr::Point<float> phy_loc = vtr::Point<float>(
      -1., -1.); /* The physical location in the physical sizes of the eFPGA fabric */
    std::vector<int> child_nodes;
    std::vector<vtr::Point<size_t>> leaf_tiles;
    std::string buf_cell = "";
    size_t depth;
    SdNtwkNode(const vtr::Point<size_t>& loc, const std::string& b_cell, const size_t& dps)
      : location(loc), buf_cell(b_cell), depth(dps) {}
    bool is_leaf_node() const { return child_nodes.empty(); }
    bool is_parent_node() const { return parent_node == -1; }
    std::string instance_name(const std::string& prefix) const {
      return prefix + std::string("_buf_lvl") + std::to_string(depth) + std::string("_x") +
             std::to_string(location.x()) + std::string("_y") + std::to_string(location.y());
    }
  };
  /* I/O logical and physical coordinate */
  struct IoLoc {
    openfpga::BasicPort name;
    vtr::Point<size_t> coord;
    size_t subtile;
    std::string metal_layer = "";
    float width = -1.;
    float length = -1.;
    e_side side = NUM_2D_SIDES;
    vtr::Point<float> phy_loc = vtr::Point<float>(-1., -1.);
    IoLoc(const openfpga::BasicPort& port, const vtr::Point<size_t>& coord, const size_t& subtile)
      : name(port), coord(coord), subtile(subtile) {}
  };
  /* Pin logical and physical coordinate */
  struct TilePinLoc {
    std::string tile;
    openfpga::BasicPort pin;
    std::string metal_layer = "";
    float width = -1.;
    float length = -1.;
    e_side side = NUM_2D_SIDES;
    vtr::Point<float> phy_loc = vtr::Point<float>(-1., -1.);
    TilePinLoc(const std::string& tile, const openfpga::BasicPort& port, const e_side& side)
      : tile(tile), pin(port), side(side) {}
    std::string pin2str() const { return pin.to_simple_verilog_string(); }
  };

 public: /* -- Constructors -- */
  TileQor() = default;
  TileQor(const TileRef& tile_ref);

 public: /* -- Accessors -- */
  /* Check two tiles share the width at the given offset related to the tile1 */
  bool tile_share_width(const std::string& tile1, const size_t& x_ofs,
                        const std::string& tile2) const;
  size_t tile_share_width_target_offset(const std::string& tile1, const size_t& x_ofs,
                                        const std::string& tile2) const;
  /* Check two tiles share the height at the given offset related to the tile1 */
  bool tile_share_height(const std::string& tile1, const size_t& y_ofs,
                         const std::string& tile2) const;
  size_t tile_share_height_target_offset(const std::string& tile1, const size_t& y_ofs,
                                         const std::string& tile2) const;
  /* Return the pair of (idx of neighour, offset of the neighbour) */
  std::map<int, int> get_neighbors_share_height(const std::string& tile, const size_t& y_ofs) const;
  std::map<int, int> get_neighbors_share_width(const std::string& tile, const size_t& x_ofs) const;
  float cal_total_actual_area() const;
  float cal_total_cell_area() const;
  float cell_area(const std::string& tile_name) const;
  float cell_grid_area(const std::string& tile_name) const;
  float utilization(const std::string& tile_name) const;
  int generate_shaping_txt(const std::string& file_name, const size_t& precision) const;
  float get_tile_width_by_offset(const std::string& tile, const size_t& x_ofs) const;
  float get_tile_width(const std::string& tile) const;
  float get_tile_height_by_offset(const std::string& tile, const size_t& y_ofs) const;
  float get_tile_height(const std::string& tile) const;
  float get_tile_actual_area(const std::string& tile) const;
  float get_tile_actual_utilization(const std::string& tile) const;
  int num_tiles() const;
  std::vector<std::string> tiles() const;
  /* Get the height of efpga in [um] */
  float efpga_height() const;
  /* Get the width of efpga in [um] */
  float efpga_width() const;
  float average_target_utilization() const;
  float average_actual_utilization() const;
  float horizontal_tile_gap() const;
  float vertical_tile_gap() const;
  size_t grid_height(const std::string& tile) const;
  size_t grid_width(const std::string& tile) const;
  bool is_undefined(const float& val) const;
  vtr::Rect<size_t> efpga_logical_bounding_box() const;
  bool valid_efpga_location(const vtr::Point<size_t>& entry_point) const;
  e_side find_side4entry(const vtr::Rect<size_t>& bb, const vtr::Point<size_t>& entry_point) const;
  std::vector<e_side> find_sides4io(const vtr::Rect<size_t>& bb,
                                    const vtr::Point<size_t>& entry_point) const;
  int find_center_location(const vtr::Rect<size_t>& bb, const vtr::Point<size_t>& entry_point,
                           vtr::Point<size_t>& ctr_loc) const;
  vtr::Point<float> logical_coord2physical_location(const vtr::Point<size_t>& lgk_coord) const;
  int append_xml_signal_distribution_network(pugi::xml_node& root_node) const;
  std::vector<IoLoc> io_locations() const;
  const std::vector<TilePinLoc>& tile_pin_loc() const;
  float efpga_physical_start_x(const vtr::Point<size_t>& lgk_coord) const;
  float efpga_physical_start_y(const vtr::Point<size_t>& lgk_coord) const;
  float efpga_physical_end_x(const vtr::Point<size_t>& lgk_coord) const;
  float efpga_physical_end_y(const vtr::Point<size_t>& lgk_coord) const;
  float efpga_physical_center_x(const vtr::Point<size_t>& lgk_coord) const;
  float efpga_physical_center_y(const vtr::Point<size_t>& lgk_coord) const;
  int tile_instance_and_coord_pairs(const std::string& tile,
                                    std::vector<std::string>& instance_names,
                                    std::vector<vtr::Point<size_t>>& coords,
                                    const bool& verbose) const;
  std::vector<std::string> signal_distribution_networks() const;
  std::vector<SigDistrNtwkBufId> signal_distribution_network_buffers(
    const std::string& ntwk_name) const;
  vtr::Point<float> signal_distribution_network_buffer_physical_location(
    const std::string& ntwk_name, const SigDistrNtwkBufId& buf_id) const;
  bool is_signal_distribution_network_leaf_buffer(const std::string& ntwk_name,
                                                  const SigDistrNtwkBufId& buf_id) const;
  const std::vector<vtr::Point<size_t>>& signal_distribution_network_buffer_leaf_tiles(
    const std::string& ntwk_name, const SigDistrNtwkBufId& buf_id) const;
  std::vector<SigDistrNtwkBufId> signal_distribution_network_buffer_fanouts(
    const std::string& ntwk_name, const SigDistrNtwkBufId& buf_id) const;
  bool valid_tile_name(const std::string& name) const;

 public: /* -- Muatators -- */
  void set_cell_area(const std::string& tile_name, const float& cell_area);
  void set_utilization(const std::string& tile_name, const float& utilization);
  void construct_neighbour_info();
  void set_tile_info(const std::string& tile, const TileInfo& tile_info, const size_t& x_ofs,
                     const size_t& y_ofs);
  void update_neighbours_height_by_offset(const std::string& tile, const size_t& y_ofs);
  void update_neighbours_height(const std::string& tile);
  void update_neighbours_width_by_offset(const std::string& tile, const size_t& x_ofs);
  void update_neighbours_width(const std::string& tile);
  std::string find_tile_main();
  void set_default_utilization(const float& utilization);
  float default_utilization();

  /* TODO: Support tile gap and side gap part of the internal data !!! */
  void set_top_side_gap(const float& num);
  void set_left_side_gap(const float& num);
  void set_right_side_gap(const float& num);
  void set_bottom_side_gap(const float& num);
  void set_horizontal_tile_gap(const float& num);
  void set_vertical_tile_gap(const float& num);

  /* For signal distribution network */
  int add_buf2loc(const openfpga::BasicPort& port, const vtr::Point<size_t>& entry_point,
                  const std::string& buf_cell, const size_t& depth);
  int add_fishbone_connection2region(const openfpga::BasicPort& port,
                                     const vtr::Point<size_t>& entry_point,
                                     const vtr::Rect<size_t> lgk_bb, const vtr::Rect<size_t> bb,
                                     const std::string& buf_cell, const bool& verbose);
  int connect_signal_distribution_network_bufs(const openfpga::BasicPort& port,
                                               const vtr::Point<size_t>& src_coord,
                                               const size_t& src_depth,
                                               const vtr::Point<size_t>& des_coord,
                                               const size_t& des_depth, const bool& verbose);
  /* Build physical location for each buffers inserted by the signal distribution network.
   * Prequestics:  This requires the physical dimension of each tile to be computed before this
   * function is called Impact: this function will freeze all the signal distribution network.
   * Allowing no modication may use the unlock_signal_distribution_network() after calling this
   * function.
   */
  int build_signal_distribution_network_physical_details(const bool& verbose);
  int add_io_logical_coordinate(const openfpga::BasicPort& io_port, const vtr::Point<size_t>& coord,
                                const size_t& subtile_idx);
  int build_data_io_physical_location(const std::string& mlayer, const float& pin_min_height,
                                      const float& pin_min_width, const float& pin_min_spacing,
                                      const bool& verbose);
  int build_global_io_physical_location(const std::string& mlayer, const float& pin_min_height,
                                        const float& pin_min_width, const float& pin_min_spacing,
                                        const bool& verbose);
  int add_tile_pin_logical_location(const std::string& tile, const openfpga::BasicPort& port,
                                    const std::string& side_str);
  int build_tile_pin_physical_location(const std::string& mlayer, const float& pin_min_height,
                                       const float& pin_min_width, const float& pin_min_spacing,
                                       const bool& verbose);

 public: /* Public utility */
  e_side str2side(const std::string& str, const bool& verbose) const;
  std::string side2str(const e_side& type, const bool& verbose = false) const;
  std::string side_all2str() const;
  bool valid_side(const e_side& val) const;
  /* Caution: only use for ICC2 pin location assignment. The side truth table is special! This
   * function is design for structs IoLoc and TilePinLoc */
  size_t side2int(const openfpga::BasicPort& port, const std::string& tile,
                  const e_side& side) const;
  void check_signal_distribution_network_frozen() const;
  void check_valid_signal_distribution_network_name(const std::string& ntwk_name) const;
  void check_valid_signal_distribution_network_buffer(const std::string& ntwk_name,
                                                      const SigDistrNtwkBufId& buf_id) const;

 private: /* -- Private Accessors -- */
  size_t find_internal_index(const std::string& tile) const;
  float get_tile_width_by_offset(size_t id, const size_t& x_ofs) const;
  float get_tile_width(const size_t& id) const;
  float get_tile_height_by_offset(size_t id, const size_t& y_ofs) const;
  float get_tile_height(const size_t& id) const;
  std::string coord2str(const vtr::Point<size_t>& coord) const;
  openfpga::BasicPort str2global_port(const std::string& port_str) const;
  bool valid_global_port(const openfpga::BasicPort& port, const bool& verbose) const;
  std::string shaping_coord_string(const float& hgt, const float& wid,
                                   const size_t& precision) const;

 private: /* -- Private Mutators -- */
  void init();
  void compute_logical_bounding_box();
  /* Require the bounding box to be built first */
  void build_location2tile_map();
  void set_utilization(size_t id, const float& utilization);
  void update_tile_height(size_t id, const size_t& y_ofs, const float& height);
  void update_tile_height(const std::string& tile, const size_t& y_ofs, const float& height);
  void update_tile_width(size_t id, const size_t& x_ofs, const float& width);
  void update_tile_width(const std::string& tile, const size_t& x_ofs, const float& width);

 private: /* -- Internal data -- */
          /* qor area and utilization are info obtained from tile_qor.csv file
           tile_infos_ includes the tile information set through floorplan algo
          tile_infos_ includes each tile's height, width and actual area */
  std::vector<float> cell_area_;
  std::vector<float> utilization_;
  /* Index of tiles that shared width with the current tile by grid width offset
   * [tile_idx][width_offset][neighbour_idx] = x_offset
   * The x_offset is in the range of width of neighbour_idx */
  std::vector<std::vector<std::map<int, int>>>
    neighbors_share_width_;  // [[3,1,2]] means tiles with id 3,1,2
                             // share width with the tile with id 0;
  /* Index of tiles that shared height with the current tile by grid height offset
   * [tile_idx][height_offset][neighbour_idx] = y_offset
   * The y_offset is in the range of height of neighbour_idx */
  std::vector<std::vector<std::map<int, int>>> neighbors_share_height_;

  /* Height by grid w.r.t. the grid_height() in tile_ref. Each grid has a separated height. Tile
   * height is the sum of grid heights. */
  std::vector<std::vector<float>> tile_height_;
  /* Width by grid w.r.t. the grid_width() in tile_ref. Each grid has a separated width. Tile height
   * is the sum of grid width. */
  std::vector<std::vector<float>> tile_width_;
  /* eFPGA coordinate bounding box */
  vtr::Rect<size_t> logical_bounding_box_;
  vtr::NdMatrix<std::string, 2> loc2tile_map_;

  int num_tiles_;
  float default_utilization_;
  float top_side_gap_;
  float bottom_side_gap_;
  float right_side_gap_;
  float left_side_gap_;
  float horizontal_tile_gap_;
  float vertical_tile_gap_;
  TileRef tile_ref_;

  /* Signal distribution network : [port_name_str] -> [list of nodes] */
  std::map<std::string, std::vector<SdNtwkNode>> signal_distribution_network_;
  bool is_signal_distribution_network_frozen_;

  /* I/O logical coordinate and physical location */
  std::map<std::string, std::vector<size_t>>
    io_coord2loc_map_;  // Loc string x<int>y<int> maps to the index in the array of io_location_
  std::vector<IoLoc> io_location_;
  bool is_io_coord_frozen_;

  /* Tile pin logical location to physical location */
  std::map<std::string, std::map<e_side, std::vector<size_t>>>
    tile_pinloc_map_;  // [tile_name][side] = index in the array of tile_pin_location_
  std::vector<TilePinLoc> tile_pin_location_;
  bool is_tile_pinloc_frozen_;

  /* Constants */
  float UNDEF_ = -1.;
};
}  // namespace arcxpress
