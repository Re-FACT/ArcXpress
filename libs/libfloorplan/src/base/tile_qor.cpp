

#include "tile_qor.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "arch_util.h"
#include "arcxpress_report_util.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "openfpga_naming.h"
#include "openfpga_port_parser.h"
#include "side_manager.h"
#include "sig_distr_ntwk_xml_constants.h"
#include "tableprinter.h"
#include "tile_ref.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

TileQor::TileQor(const TileRef& tile_ref) : tile_ref_(tile_ref) {
  cell_area_.resize(tile_ref_.num_tiles());
  utilization_.resize(tile_ref_.num_tiles());
  std::fill(utilization_.begin(), utilization_.end(), default_utilization());
  tile_height_.resize(tile_ref_.num_tiles());
  tile_width_.resize(tile_ref_.num_tiles());
  neighbors_share_width_.resize(tile_ref_.num_tiles());
  neighbors_share_height_.resize(tile_ref_.num_tiles());
  num_tiles_ = tile_ref_.num_tiles();
  init();
  construct_neighbour_info();
  compute_logical_bounding_box();
  build_location2tile_map();
  is_signal_distribution_network_frozen_ = false;
  is_io_coord_frozen_ = false;
  is_tile_pinloc_frozen_ = false;
}

void TileQor::init() {
  /* Initialization */
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    neighbors_share_width_[id].resize(tile_ref_.grid_width(tile));
    neighbors_share_height_[id].resize(tile_ref_.grid_height(tile));
    tile_width_[id].resize(tile_ref_.grid_width(tile), UNDEF_);
    tile_height_[id].resize(tile_ref_.grid_height(tile), UNDEF_);
  }
}

int TileQor::num_tiles() const { return num_tiles_; }

std::vector<std::string> TileQor::tiles() const { return tile_ref_.tiles(); }

bool TileQor::valid_tile_name(const std::string& name) const { return tile_ref_.tile_exist(name); }

size_t TileQor::find_internal_index(const std::string& tile) const {
  return tile_ref_.find_internal_index(tile);
}
// set the qor area of the tile using the tile's name
void TileQor::set_cell_area(const std::string& tile_name, const float& cell_area) {
  size_t id = find_internal_index(tile_name);
  cell_area_[id] = cell_area;
}
// set utilization of the tile using the tile's name
void TileQor::set_utilization(const std::string& tile_name, const float& utilization) {
  size_t id = find_internal_index(tile_name);
  utilization_[id] = utilization;
}

// set default utilization in the init step
void TileQor::set_default_utilization(const float& utilization) {
  default_utilization_ = utilization;
  for (size_t i = 0; i < num_tiles(); i++) {
    set_utilization(i, default_utilization_);
  }
}

void TileQor::set_utilization(size_t id, const float& utilization) {
  utilization_[id] = utilization;
}
// set default utilization in the init step
float TileQor::default_utilization() { return default_utilization_; }

// get the qor area of the tile using the tile name
float TileQor::cell_area(const std::string& tile_name) const {
  size_t id = find_internal_index(tile_name);
  return cell_area_[id];
}

float TileQor::cell_grid_area(const std::string& tile_name) const {
  return cell_area(tile_name) / (grid_height(tile_name) * grid_width(tile_name));
}

// get the utilization of the tile using the tile name
float TileQor::utilization(const std::string& tile_name) const {
  size_t id = find_internal_index(tile_name);
  return utilization_[id];
}

/** @brief utility function used for checking whether coord range has intersections or not
 * the principles behind is to check if the (x, y) of all the instances are overlapped or not
 * For example,
 * - tileA has instances at (x=1, y=1), (x=2, y=1)
 * - tileB has coorindates (x=1, y=2), (x=2, y=2)
 * The two tiles shares the width as they have overlaps on x coordinates
 */
bool has_intersection(const std::set<int>& set1, const std::set<int>& set2) {
  std::set<int> result;
  std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                        std::inserter(result, result.begin()));
  return !result.empty();
}

/* construct neighbour_map
  The index of x_range is the tile id. the element std::set<int> stores the x coords of the tile
  The index of y_range is the tile id. the element std::set<int> stores the y coords of the tile
*/
void TileQor::construct_neighbour_info() {
  std::vector<std::vector<std::set<int>>> x_range(num_tiles());
  std::vector<std::vector<std::set<int>>> y_range(num_tiles());

  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    x_range[id].resize(tile_ref_.grid_width(tile));
    y_range[id].resize(tile_ref_.grid_height(tile));
  }

  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      for (size_t ofs : tile_ref_.grid_width_offsets(tile)) {
        x_range[id][ofs].insert(coord.x() + ofs);
      }
      for (size_t ofs : tile_ref_.grid_height_offsets(tile)) {
        y_range[id][ofs].insert(coord.y() + ofs);
      }
    }
  }

  for (int src = 0; src < x_range.size(); src++) {
    for (size_t src_ofs = 0; src_ofs < x_range[src].size(); src_ofs++) {
      for (int des = src + 1; des < x_range.size(); des++) {
        for (size_t des_ofs = 0; des_ofs < x_range[des].size(); des_ofs++) {
          // if x_range[i] and x_range[j] has intersection, this means tile with id i and tile with
          // id j share the same width
          bool intersection_flag_x = has_intersection(x_range[src][src_ofs], x_range[des][des_ofs]);
          if (intersection_flag_x) {
            neighbors_share_width_[src][src_ofs][des] = des_ofs;
            neighbors_share_width_[des][des_ofs][src] = src_ofs;
          }
        }
      }
    }
  }
  for (int src = 0; src < y_range.size(); src++) {
    for (size_t src_ofs = 0; src_ofs < y_range[src].size(); src_ofs++) {
      for (int des = src + 1; des < y_range.size(); des++) {
        for (size_t des_ofs = 0; des_ofs < y_range[des].size(); des_ofs++) {
          // if y_range[i] and y_range[j] has intersection, this means tile with id i and tile with
          // id j share the same height
          bool intersection_flag_y = has_intersection(y_range[src][src_ofs], y_range[des][des_ofs]);
          if (intersection_flag_y) {
            neighbors_share_height_[src][src_ofs][des] = des_ofs;
            neighbors_share_height_[des][des_ofs][src] = src_ofs;
          }
        }
      }
    }
  }
}

/* tile_info includes the following information: height, width, actual_area*/
void TileQor::set_tile_info(const std::string& tile, const TileInfo& tile_info, const size_t& x_ofs,
                            const size_t& y_ofs) {
  size_t id = find_internal_index(tile);
  tile_height_[id][y_ofs] = tile_info.height;
  tile_width_[id][x_ofs] = tile_info.width;
}

// to check whether two tiles share the same width
bool TileQor::tile_share_width(const std::string& tile1, const size_t& x_ofs,
                               const std::string& tile2) const {
  size_t id_1 = find_internal_index(tile1);
  size_t id_2 = find_internal_index(tile2);
  auto result = neighbors_share_width_[id_1][x_ofs].find(id_2);
  return result != neighbors_share_width_[id_1][x_ofs].end();
}

size_t TileQor::tile_share_width_target_offset(const std::string& tile1, const size_t& x_ofs,
                                               const std::string& tile2) const {
  size_t id_1 = find_internal_index(tile1);
  size_t id_2 = find_internal_index(tile2);
  auto result = neighbors_share_width_[id_1][x_ofs].find(id_2);
  return result->second;
}

// to check whether two tiles share the same height
bool TileQor::tile_share_height(const std::string& tile1, const size_t& y_ofs,
                                const std::string& tile2) const {
  size_t id_1 = find_internal_index(tile1);
  size_t id_2 = find_internal_index(tile2);
  auto result = neighbors_share_height_[id_1][y_ofs].find(id_2);
  return result != neighbors_share_height_[id_1][y_ofs].end();
}

size_t TileQor::tile_share_height_target_offset(const std::string& tile1, const size_t& y_ofs,
                                                const std::string& tile2) const {
  size_t id_1 = find_internal_index(tile1);
  size_t id_2 = find_internal_index(tile2);
  auto result = neighbors_share_height_[id_1][y_ofs].find(id_2);
  return result->second;
}

// get tile width using tile name
float TileQor::get_tile_width_by_offset(const std::string& tile, const size_t& x_ofs) const {
  size_t id = find_internal_index(tile);
  return tile_width_[id][x_ofs];
}

// get tile width using tile id
float TileQor::get_tile_width_by_offset(size_t id, const size_t& x_ofs) const {
  return tile_width_[id][x_ofs];
}

// update tile with using tile name
void TileQor::update_tile_width(const std::string& tile, const size_t& x_ofs, const float& width) {
  size_t id = find_internal_index(tile);
  tile_width_[id][x_ofs] = width;
}

// update tile width using tile id
void TileQor::update_tile_width(size_t id, const size_t& x_ofs, const float& width) {
  tile_width_[id][x_ofs] = width;
}

// get tile height using tile name
float TileQor::get_tile_height_by_offset(const std::string& tile, const size_t& y_ofs) const {
  size_t id = find_internal_index(tile);
  return tile_height_[id][y_ofs];
}

float TileQor::get_tile_height(const std::string& tile) const {
  size_t id = find_internal_index(tile);
  return get_tile_height(id);
}

float TileQor::get_tile_height(const size_t& id) const {
  float ret = 0.;
  int cnt = 0;
  /* For tile which across multiple grids, the horizontal gaps between tiles should be included */
  for (float curr_h : tile_height_[id]) {
    ret += curr_h;
    if (cnt > 0) {
      ret += vertical_tile_gap_;
    }
    cnt++;
  }
  return ret;
}

float TileQor::get_tile_width(const std::string& tile) const {
  size_t id = find_internal_index(tile);
  return get_tile_width(id);
}

float TileQor::get_tile_width(const size_t& id) const {
  float ret = 0.;
  int cnt = 0;
  /* For tile which across multiple grids, the horizontal gaps between tiles should be included */
  for (float curr_w : tile_width_[id]) {
    ret += curr_w;
    if (cnt > 0) {
      ret += horizontal_tile_gap_;
    }
    cnt++;
  }
  return ret;
}

// get tile height using tile id
float TileQor::get_tile_height_by_offset(size_t id, const size_t& y_ofs) const {
  return tile_height_[id][y_ofs];
}

// update tile height using tile name
void TileQor::update_tile_height(const std::string& tile, const size_t& y_ofs,
                                 const float& height) {
  size_t id = find_internal_index(tile);
  tile_height_[id][y_ofs] = height;
}

// update tile height using tile id
void TileQor::update_tile_height(size_t id, const size_t& y_ofs, const float& height) {
  tile_height_[id][y_ofs] = height;
}

// get tile's actual area using tile name
float TileQor::get_tile_actual_area(const std::string& tile) const {
  size_t id = find_internal_index(tile);
  /* Compute actual area considering all the grids */
  return get_tile_width(tile) * get_tile_height(tile);
}

// get tile's actual utilization using tile name
float TileQor::get_tile_actual_utilization(const std::string& tile) const {
  size_t id = find_internal_index(tile);
  float actual_area = get_tile_actual_area(tile);
  if (actual_area != 0.) {
    return cell_area_[id] / actual_area;
  } else {
    return 0.;
  }
}

// get the ids of the neighbor tiles that share height with given tile
std::map<int, int> TileQor::get_neighbors_share_height(const std::string& tile,
                                                       const size_t& y_ofs) const {
  size_t id = find_internal_index(tile);
  return neighbors_share_height_[id][y_ofs];
}

// get the ids of the neighbor tiles that share width with given tile
std::map<int, int> TileQor::get_neighbors_share_width(const std::string& tile,
                                                      const size_t& x_ofs) const {
  size_t id = find_internal_index(tile);
  return neighbors_share_width_[id][x_ofs];
}

// update the height of neighbour tiles that share height with the given tile
void TileQor::update_neighbours_height(const std::string& tile) {
  for (size_t y_ofs = 0; y_ofs < tile_ref_.grid_height(tile); ++y_ofs) {
    update_neighbours_height_by_offset(tile, y_ofs);
  }
}

void TileQor::update_neighbours_height_by_offset(const std::string& tile, const size_t& y_ofs) {
  std::map<int, int> neighbors_share_height = get_neighbors_share_height(tile, y_ofs);
  float max_height = get_tile_height_by_offset(tile, y_ofs);
  for (auto pair : neighbors_share_height) {
    float neighbor_tile_height = get_tile_height_by_offset(pair.first, pair.second);
    if (neighbor_tile_height > max_height) {
      max_height = neighbor_tile_height;
    }
  }
  for (auto pair : neighbors_share_height) {
    update_tile_height(pair.first, pair.second, max_height);
  }
  update_tile_height(tile, y_ofs, max_height);
}

// update the width of neighbour tiles that share width with the given tile
void TileQor::update_neighbours_width(const std::string& tile) {
  for (size_t x_ofs = 0; x_ofs < tile_ref_.grid_width(tile); ++x_ofs) {
    update_neighbours_width_by_offset(tile, x_ofs);
  }
}
void TileQor::update_neighbours_width_by_offset(const std::string& tile, const size_t& x_ofs) {
  std::map<int, int> neighbors_share_width = get_neighbors_share_width(tile, x_ofs);
  float max_width = get_tile_width_by_offset(tile, x_ofs);
  for (auto pair : neighbors_share_width) {
    float neighbor_tile_width = get_tile_width_by_offset(pair.first, pair.second);
    if (neighbor_tile_width > max_width) {
      max_width = neighbor_tile_width;
    }
  }
  for (auto pair : neighbors_share_width) {
    update_tile_width(pair.first, pair.second, max_width);
  }
  update_tile_width(tile, x_ofs, max_width);
}

// get total actual area of all tiles
float TileQor::cal_total_actual_area() const {
  float total_area = 0;
  for (auto tile : tiles()) {
    total_area = total_area + get_tile_actual_area(tile);
  }
  return total_area;
}

// get total qor area of all tiles
float TileQor::cal_total_cell_area() const {
  float total_area = 0;
  for (auto tile : tiles()) {
    total_area = total_area + cell_area(tile);
  }
  return total_area;
}

float TileQor::efpga_height() const {
  /* Build a matrix of tile coord and tile */
  size_t max_x = 0;
  size_t max_y = 0;
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      max_x = std::max(coord.x(), max_x);
      max_y = std::max(coord.y(), max_y);
    }
  }
  vtr::NdMatrix<std::string, 2> efpga_tile_map({size_t(max_x) + 1, size_t(max_y) + 1});
  efpga_tile_map.fill(std::string());
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      efpga_tile_map[coord.x()][coord.y()] = tile;
    }
  }
  /* Compute the height by columns */
  float max_height = 0.;
  for (int ix = 0; ix < max_x + 1; ++ix) {
    float curr_height = 0.;
    for (int iy = 0; iy < max_y + 1; ++iy) {
      std::string curr_tile = efpga_tile_map[ix][iy];
      if (curr_tile.empty()) {
        continue;
      }
      size_t id = find_internal_index(curr_tile);
      curr_height += get_tile_height(id);
    }
    max_height = std::max(curr_height, max_height);
  }
  max_height = max_height + vertical_tile_gap_ * max_y + bottom_side_gap_ + top_side_gap_;
  return max_height;
}

float TileQor::efpga_width() const {
  /* Build a matrix of tile coord and tile */
  size_t max_x = 0;
  size_t max_y = 0;
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      max_x = std::max(coord.x(), max_x);
      max_y = std::max(coord.y(), max_y);
    }
  }
  vtr::NdMatrix<std::string, 2> efpga_tile_map({size_t(max_x) + 1, size_t(max_y) + 1});
  efpga_tile_map.fill(std::string());
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      efpga_tile_map[coord.x()][coord.y()] = tile;
    }
  }
  /* Compute the height by columns */
  float max_width = 0.;
  for (int iy = 0; iy < max_y + 1; ++iy) {
    float curr_width = 0.;
    for (int ix = 0; ix < max_x + 1; ++ix) {
      std::string curr_tile = efpga_tile_map[ix][iy];
      if (curr_tile.empty()) {
        continue;
      }
      size_t id = find_internal_index(curr_tile);
      curr_width += get_tile_width(id);
    }
    max_width = std::max(curr_width, max_width);
  }
  max_width = max_width + max_x * horizontal_tile_gap_ + right_side_gap_ + left_side_gap_;
  return max_width;
}

float TileQor::average_target_utilization() const {
  float avg = 0.;
  for (float curr_util : utilization_) {
    avg += curr_util;
  }
  return avg / utilization_.size();
}

float TileQor::average_actual_utilization() const {
  float avg = 0.;
  for (auto tile : tiles()) {
    avg += get_tile_actual_utilization(tile);
  }
  return avg / utilization_.size();
}

std::string TileQor::find_tile_main() {
  int max_instance_num = 0;
  std::string tile_name;
  for (auto tile : tiles()) {
    size_t cnt = 0;
    for (size_t x_ofs = 0; x_ofs < tile_ref_.grid_width(tile); ++x_ofs) {
      for (size_t y_ofs = 0; y_ofs < tile_ref_.grid_height(tile); ++y_ofs) {
        if (!get_neighbors_share_height(tile, y_ofs).empty()) {
          cnt++;
        }
        if (!get_neighbors_share_width(tile, x_ofs).empty()) {
          cnt++;
        }
      }
    }
    if (cnt == 0) {
      continue;
    }
    if (tile_ref_.num_instances(tile) > max_instance_num && cell_area(tile) != 0) {
      max_instance_num = tile_ref_.num_instances(tile);
      tile_name = tile;
    }
  }
  return tile_name;
}

int TileQor::tile_instance_and_coord_pairs(const std::string& tile,
                                           std::vector<std::string>& instance_names,
                                           std::vector<vtr::Point<size_t>>& coords,
                                           const bool& verbose) const {
  return tile_ref_.tile_instance_and_coord_pairs(tile, instance_names, coords, verbose);
}

int TileQor::generate_shaping_txt(const std::string& file_name, const size_t& precision) const {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  std::filesystem::path dirpath = file_name;
  openfpga::create_directory(dirpath.parent_path().string(), true, false);
  /* Build a matrix of tile coord and tile_instance and tile */
  TablePrinter tp({"INSTANCE", "MODULE", "LOC_X", "LOC_Y", "SHAPE", "BBOX_PT", "POINTS"});
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();

  for (auto tile : tiles()) {
    std::vector<std::string> inst;
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.tile_instance_and_coord_pairs(tile, inst, coords, false);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    VTR_ASSERT_SAFE(inst.size() == coords.size());
    for (size_t iinst = 0; iinst < inst.size(); ++iinst) {
      vtr::Point<size_t> lgk_coord = coords[iinst];
      vtr::Point<float> phy_loc(efpga_physical_start_x(lgk_coord),
                                efpga_physical_start_y(lgk_coord));
      vtr::Point lgk_end_coord(lgk_coord.x(), lgk_coord.y());
      if (grid_width(tile) > 1) {
        lgk_end_coord.set_x(lgk_coord.x() + grid_width(tile) - 1);
      }
      if (grid_height(tile) > 1) {
        lgk_end_coord.set_y(lgk_coord.y() + grid_height(tile) - 1);
      }
      float tile_width = efpga_physical_end_x(lgk_end_coord) - phy_loc.x();
      float tile_height = efpga_physical_end_y(lgk_end_coord) - phy_loc.y();

      tp.add_row({inst[iinst], tile, to_string_with_precision(phy_loc.x(), precision),
                  to_string_with_precision(phy_loc.y(), precision), "rect", "4",
                  shaping_coord_string(tile_height, tile_width, precision)});
    }
  }
  /* add fpga core info */
  std::string fpga_core_name = openfpga::generate_fpga_core_module_name();
  tp.add_row({fpga_core_name, fpga_core_name, "0.0", "0.0", "rect", "4",
              shaping_coord_string(efpga_height(), efpga_width(), precision)});
  tp.set_delim(" ");
  tp.print(file_name);
  return openfpga::CMD_EXEC_SUCCESS;
}

std::string TileQor::shaping_coord_string(const float& hgt, const float& wid,
                                          const size_t& precision) const {
  std::string ret;
  /* Rect bottom left */
  ret += to_string_with_precision(0., precision) + std::string(" ");
  ret += to_string_with_precision(0., precision) + std::string(" ");
  /* Rect top left */
  ret += to_string_with_precision(0., precision) + std::string(" ");
  ret += to_string_with_precision(hgt, precision) + std::string(" ");
  /* Rect top right */
  ret += to_string_with_precision(wid, precision) + std::string(" ");
  ret += to_string_with_precision(hgt, precision) + std::string(" ");
  /* Rect bottom right */
  ret += to_string_with_precision(wid, precision) + std::string(" ");
  ret += to_string_with_precision(0., precision) + std::string(" ");
  ret.pop_back();  // Remove the redundant space
  return ret;
}

void TileQor::set_top_side_gap(const float& num) { top_side_gap_ = num; }
void TileQor::set_left_side_gap(const float& num) { left_side_gap_ = num; }
void TileQor::set_right_side_gap(const float& num) { right_side_gap_ = num; }
void TileQor::set_bottom_side_gap(const float& num) { bottom_side_gap_ = num; }
void TileQor::set_horizontal_tile_gap(const float& num) { horizontal_tile_gap_ = num; }
void TileQor::set_vertical_tile_gap(const float& num) { vertical_tile_gap_ = num; }

float TileQor::horizontal_tile_gap() const { return horizontal_tile_gap_; }
float TileQor::vertical_tile_gap() const { return vertical_tile_gap_; }

size_t TileQor::grid_height(const std::string& tile) const { return tile_ref_.grid_height(tile); }
size_t TileQor::grid_width(const std::string& tile) const { return tile_ref_.grid_width(tile); }
bool TileQor::is_undefined(const float& val) const { return val == UNDEF_; }

void TileQor::build_location2tile_map() {
  vtr::Rect<size_t> bb = efpga_logical_bounding_box();
  loc2tile_map_.resize({bb.width() + 1, bb.height() + 1}, std::string());
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      loc2tile_map_[coord.x()][coord.y()] = tile;
    }
  }
}

void TileQor::compute_logical_bounding_box() {
  /* Build a matrix of tile coord and tile */
  size_t max_x = 0;
  size_t max_y = 0;
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      max_x = std::max(coord.x() + tile_ref_.grid_width(tile) - 1, max_x);
      max_y = std::max(coord.y() + tile_ref_.grid_height(tile) - 1, max_y);
    }
  }
  size_t min_x = max_x;
  size_t min_y = max_y;
  for (auto tile : tiles()) {
    size_t id = find_internal_index(tile);
    std::vector<vtr::Point<size_t>> coords;
    int status = tile_ref_.instance_coordinates(tile, coords);
    if (status != openfpga::CMD_EXEC_SUCCESS) continue;
    for (const auto& coord : coords) {
      min_x = std::min(coord.x(), min_x);
      min_y = std::min(coord.y(), min_y);
    }
  }
  logical_bounding_box_.set_xmin(min_x);
  logical_bounding_box_.set_ymin(min_y);
  logical_bounding_box_.set_xmax(max_x);
  logical_bounding_box_.set_ymax(max_y);
}

vtr::Rect<size_t> TileQor::efpga_logical_bounding_box() const { return logical_bounding_box_; }

bool TileQor::valid_efpga_location(const vtr::Point<size_t>& entry_point) const {
  vtr::Rect<size_t> bb = efpga_logical_bounding_box();
  if (entry_point.x() >= bb.xmin() && entry_point.x() <= bb.xmax() &&
      entry_point.y() >= bb.ymin() && entry_point.y() <= bb.ymax()) {
    return true;
  }
  return false;
}

int TileQor::add_buf2loc(const openfpga::BasicPort& port, const vtr::Point<size_t>& entry_point,
                         const std::string& buf_cell, const size_t& buf_depth) {
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(entry_point)) {
    VTR_LOG_ERROR(
      "Cannot add a buffer to the location (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, "
      "ymin=%lu, xmax=%lu, ymax=%lu)\n",
      entry_point.x(), entry_point.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  SdNtwkNode node(entry_point, buf_cell, buf_depth);
  signal_distribution_network_[port.to_verilog_string()].push_back(node);
  return openfpga::CMD_EXEC_SUCCESS;
}

e_side TileQor::find_side4entry(const vtr::Rect<size_t>& bb,
                                const vtr::Point<size_t>& entry_point) const {
  if (!(entry_point.x() >= bb.xmin() && entry_point.x() <= bb.xmax() &&
        entry_point.y() >= bb.ymin() && entry_point.y() <= bb.ymax())) {
    VTR_LOG_ERROR(
      "The entry point (x=%lu, y=%lu) is not contained by the bounding box (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      entry_point.x(), entry_point.y(), bb.xmin(), bb.ymin(), bb.xmax(), bb.ymax());
    return NUM_2D_SIDES;
  }

  if (entry_point.x() == bb.xmin()) {
    return LEFT;
  }
  if (entry_point.x() == bb.xmax()) {
    return RIGHT;
  }
  if (entry_point.y() == bb.ymin()) {
    return BOTTOM;
  }
  if (entry_point.y() == bb.ymax()) {
    return TOP;
  }

  return NUM_2D_SIDES;
}

std::vector<e_side> TileQor::find_sides4io(const vtr::Rect<size_t>& bb,
                                           const vtr::Point<size_t>& entry_point) const {
  std::vector<e_side> ret;
  if (!(entry_point.x() >= bb.xmin() && entry_point.x() <= bb.xmax() &&
        entry_point.y() >= bb.ymin() && entry_point.y() <= bb.ymax())) {
    VTR_LOG_ERROR(
      "The entry point (x=%lu, y=%lu) is not contained by the bounding box (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      entry_point.x(), entry_point.y(), bb.xmin(), bb.ymin(), bb.xmax(), bb.ymax());
    return ret;
  }

  if (entry_point.x() == bb.xmin()) {
    ret.push_back(LEFT);
  }
  if (entry_point.x() == bb.xmax()) {
    ret.push_back(RIGHT);
  }
  if (entry_point.y() == bb.ymin()) {
    ret.push_back(BOTTOM);
  }
  if (entry_point.y() == bb.ymax()) {
    ret.push_back(TOP);
  }
  return ret;
}

int TileQor::add_fishbone_connection2region(const openfpga::BasicPort& port,
                                            const vtr::Point<size_t>& entry_point,
                                            const vtr::Rect<size_t> lgk_bb,
                                            const vtr::Rect<size_t> bb, const std::string& buf_cell,
                                            const bool& verbose) {
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(entry_point)) {
    VTR_LOG_ERROR(
      "Cannot add a buffer to the location (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, "
      "ymin=%lu, xmax=%lu, ymax=%lu)\n",
      entry_point.x(), entry_point.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!efpga_bb.contains(bb)) {
    VTR_LOG_ERROR(
      "The bounding box (xmin=%lu, ymin=%lu, xmax=%lu, ymax=%lu) for fishbone connection is out of "
      "the eFPGA fabric (xmin=%lu, ymin=%lu, xmax=%lu, ymax=%lu)\n",
      bb.xmin(), bb.ymin(), bb.xmax(), bb.ymax(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Check the entry point must be on an edge of the bounding box */
  e_side entry_side = find_side4entry(lgk_bb, entry_point);
  if (entry_side == NUM_2D_SIDES) {
    VTR_LOG_ERROR(
      "The entry point (x=%lu, y=%lu) is not on any edge of the logical bounding box (xmin=%lu, "
      "ymin=%lu, xmax=%lu, ymax=%lu)\n",
      entry_point.x(), entry_point.y(), lgk_bb.xmin(), lgk_bb.ymin(), lgk_bb.xmax(), lgk_bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Add all the valid tiles to the entry point as leaves */
  auto result = signal_distribution_network_.find(port.to_verilog_string());
  if (result == signal_distribution_network_.end()) {
    VTR_LOG_ERROR("A signal distribution network has not been created for port '%s'\n",
                  port.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (auto& curr_node : result->second) {
    if (curr_node.location != entry_point) {
      continue;
    }
    /* Skip the intermediate node */
    if (!curr_node.child_nodes.empty()) {
      VTR_LOGV(verbose, "Skip the intermediate buffer node at (x=%lu, y=%lu) of depth '%lu'\n",
               curr_node.location.x(), curr_node.location.y(), curr_node.depth);
      continue;
    }
    for (size_t ix = bb.xmin(); ix <= bb.xmax(); ix++) {
      for (size_t iy = bb.ymin(); iy <= bb.ymax(); iy++) {
        if (loc2tile_map_[ix][iy].empty()) {
          continue;
        }
        VTR_LOGV(verbose,
                 "Add tile[%lu][%lu] as a sink to the entry point (x=%lu, y=%lu) for the bounding "
                 "box (xmin=%lu, ymin=%lu, xmax=%lu, ymax=%lu)\n",
                 ix, iy, entry_point.x(), entry_point.y(), bb.xmin(), bb.ymin(), bb.xmax(),
                 bb.ymax());
        curr_node.leaf_tiles.push_back(vtr::Point<size_t>(ix, iy));
      }
    }
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR(
    "The entry point (x=%lu, y=%lu) does not have a valid node to expand in the signal "
    "distribution network for port '%s'\n",
    entry_point.x(), entry_point.y(), port.to_verilog_string().c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int TileQor::connect_signal_distribution_network_bufs(
  const openfpga::BasicPort& port, const vtr::Point<size_t>& src_coord, const size_t& src_depth,
  const vtr::Point<size_t>& des_coord, const size_t& des_depth, const bool& verbose) {
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(src_coord)) {
    VTR_LOG_ERROR(
      "The source point (x=%lu, y=%lu) is out of the eFPGA fabric (xmin=%lu, ymin=%lu, xmax=%lu, "
      "ymax=%lu)\n",
      src_coord.x(), src_coord.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!valid_efpga_location(des_coord)) {
    VTR_LOG_ERROR(
      "The destination point (x=%lu, y=%lu) is out of the eFPGA fabric (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      des_coord.x(), des_coord.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  auto result = signal_distribution_network_.find(port.to_verilog_string());
  if (result == signal_distribution_network_.end()) {
    VTR_LOG_ERROR("A signal distribution network has not been created for port '%s'\n",
                  port.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t src_node_idx = result->second.size();
  size_t des_node_idx = result->second.size();
  for (size_t idx = 0; idx < result->second.size(); ++idx) {
    if (result->second[idx].location == src_coord && result->second[idx].depth == src_depth) {
      src_node_idx = idx;
    }
    if (result->second[idx].location == des_coord && result->second[idx].depth == des_depth) {
      des_node_idx = idx;
    }
  }
  if (src_node_idx == result->second.size()) {
    VTR_LOG_ERROR(
      "The source point (x=%lu, y=%lu) is has not been created in a signal distribution network "
      "for port '%s'\n",
      src_coord.x(), src_coord.y(), port.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (des_node_idx == result->second.size()) {
    VTR_LOG_ERROR(
      "The destination point (x=%lu, y=%lu) is has not been created in a signal distribution "
      "network for port '%s'\n",
      des_coord.x(), des_coord.y(), port.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  result->second[src_node_idx].child_nodes.push_back(des_node_idx);
  result->second[des_node_idx].parent_node = src_node_idx;
  VTR_LOGV(verbose,
           "Connected the two nodes between the source (x=%lu, y=%lu) and the destination (x=%lu, "
           "y=%lu)\n",
           src_coord.x(), src_coord.y(), des_coord.x(), des_coord.y());
  return openfpga::CMD_EXEC_SUCCESS;
}

int TileQor::find_center_location(const vtr::Rect<size_t>& bb,
                                  const vtr::Point<size_t>& entry_point,
                                  vtr::Point<size_t>& ctr_loc) const {
  e_side entry_side = find_side4entry(bb, entry_point);
  if (entry_side == NUM_2D_SIDES) {
    VTR_LOG_ERROR(
      "The entry point (x=%lu, y=%lu) is not on any edge of the bounding box (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      entry_point.x(), entry_point.y(), bb.xmin(), bb.ymin(), bb.xmax(), bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (entry_side == TOP || entry_side == BOTTOM) {
    ctr_loc.set_x(entry_point.x());
    ctr_loc.set_y(std::floor((bb.ymin() + bb.ymax()) / 2));
  }
  if (entry_side == RIGHT || entry_side == LEFT) {
    ctr_loc.set_y(entry_point.y());
    ctr_loc.set_x(std::floor((bb.xmin() + bb.xmax()) / 2));
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int TileQor::build_signal_distribution_network_physical_details(const bool& verbose) {
  /* Freeze the current network */
  is_signal_distribution_network_frozen_ = true;
  /* Compute details for each network */
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  for (auto& pair : signal_distribution_network_) {
    for (auto& node : pair.second) {
      if (!valid_efpga_location(node.location)) {
        VTR_LOG_ERROR(
          "The logical coordinate (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, "
          "ymin=%lu, xmax=%lu, ymax=%lu)\n",
          node.location.x(), node.location.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
          efpga_bb.ymax());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      node.phy_loc = logical_coord2physical_location(node.location);
      VTR_LOGV(verbose,
               "Convert logical coordinate (x=%lu, y=%lu) to physical location (x=%g, y=%g)\n",
               node.location.x(), node.location.y(), node.phy_loc.x(), node.phy_loc.y());
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

vtr::Point<float> TileQor::logical_coord2physical_location(
  const vtr::Point<size_t>& lgk_coord) const {
  /* Compute the width and height based on the bounding box */
  vtr::Point<float> ret(efpga_physical_end_x(lgk_coord), efpga_physical_end_y(lgk_coord));
  return ret;
}

float TileQor::efpga_physical_start_x(const vtr::Point<size_t>& lgk_coord) const {
  /* Compute the height by columns */
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(lgk_coord)) {
    VTR_LOG_ERROR(
      "The logical coordinate (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      lgk_coord.x(), lgk_coord.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  float phy_x = 0.;
  for (int iy = 0; iy <= efpga_bb.ymax(); ++iy) {
    float curr_width = 0.;
    for (int ix = 0; ix < lgk_coord.x(); ++ix) {
      std::string curr_tile = loc2tile_map_[ix][iy];
      if (curr_tile.empty()) {
        continue;
      }
      size_t id = find_internal_index(curr_tile);
      /* For tile that spans multiple column, only calculate the width within the range  */
      for (size_t ofs : tile_ref_.grid_width_offsets(curr_tile)) {
        if (ix + ofs < lgk_coord.x()) {
          curr_width += get_tile_width_by_offset(id, ofs) + horizontal_tile_gap_;
        }
      }
    }
    phy_x = std::max(curr_width, phy_x);
  }
  /* Consider the missing bottom side gap and deduct the extra vertical gap between tiles */
  phy_x = phy_x + left_side_gap_;
  return phy_x;
}

float TileQor::efpga_physical_center_x(const vtr::Point<size_t>& lgk_coord) const {
  return (efpga_physical_start_x(lgk_coord) + efpga_physical_end_x(lgk_coord)) / 2.;
}
float TileQor::efpga_physical_center_y(const vtr::Point<size_t>& lgk_coord) const {
  return (efpga_physical_start_y(lgk_coord) + efpga_physical_end_y(lgk_coord)) / 2.;
}

float TileQor::efpga_physical_start_y(const vtr::Point<size_t>& lgk_coord) const {
  /* Compute the height by columns */
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(lgk_coord)) {
    VTR_LOG_ERROR(
      "The logical coordinate (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      lgk_coord.x(), lgk_coord.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  float phy_y = 0.;
  for (int ix = 0; ix <= efpga_bb.xmax(); ++ix) {
    float curr_height = 0.;
    for (int iy = 0; iy < lgk_coord.y(); ++iy) {
      std::string curr_tile = loc2tile_map_[ix][iy];
      if (curr_tile.empty()) {
        continue;
      }
      size_t id = find_internal_index(curr_tile);
      for (size_t ofs : tile_ref_.grid_height_offsets(curr_tile)) {
        if (iy + ofs < lgk_coord.y()) {
          curr_height += get_tile_height_by_offset(id, ofs) + vertical_tile_gap_;
        }
      }
    }
    phy_y = std::max(curr_height, phy_y);
  }
  /* Consider the missing bottom side gap and deduct the extra vertical gap between tiles */
  phy_y = phy_y + bottom_side_gap_;
  return phy_y;
}

float TileQor::efpga_physical_end_x(const vtr::Point<size_t>& lgk_coord) const {
  /* Compute the height by columns */
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(lgk_coord)) {
    VTR_LOG_ERROR(
      "The logical coordinate (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      lgk_coord.x(), lgk_coord.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  float phy_x = 0.;
  for (int iy = 0; iy <= efpga_bb.ymax(); ++iy) {
    float curr_width = 0.;
    for (int ix = 0; ix <= lgk_coord.x(); ++ix) {
      std::string curr_tile = loc2tile_map_[ix][iy];
      if (curr_tile.empty()) {
        continue;
      }
      size_t id = find_internal_index(curr_tile);
      /* For tile that spans multiple column, only calculate the width within the range  */
      for (size_t ofs : tile_ref_.grid_width_offsets(curr_tile)) {
        if (ix + ofs <= lgk_coord.x()) {
          curr_width += get_tile_width_by_offset(id, ofs) + horizontal_tile_gap_;
        }
      }
    }
    phy_x = std::max(curr_width, phy_x);
  }
  /* Consider the missing bottom side gap and deduct the extra vertical gap between tiles */
  phy_x = phy_x + left_side_gap_ - horizontal_tile_gap_;
  return phy_x;
}

float TileQor::efpga_physical_end_y(const vtr::Point<size_t>& lgk_coord) const {
  /* Compute the height by columns */
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(lgk_coord)) {
    VTR_LOG_ERROR(
      "The logical coordinate (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      lgk_coord.x(), lgk_coord.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  float phy_y = 0.;
  for (int ix = 0; ix <= efpga_bb.xmax(); ++ix) {
    float curr_height = 0.;
    for (int iy = 0; iy <= lgk_coord.y(); ++iy) {
      std::string curr_tile = loc2tile_map_[ix][iy];
      if (curr_tile.empty()) {
        continue;
      }
      size_t id = find_internal_index(curr_tile);
      for (size_t ofs : tile_ref_.grid_height_offsets(curr_tile)) {
        if (iy + ofs <= lgk_coord.y()) {
          curr_height += get_tile_height_by_offset(id, ofs) + vertical_tile_gap_;
        }
      }
    }
    phy_y = std::max(curr_height, phy_y);
  }
  /* Consider the missing bottom side gap and deduct the extra vertical gap between tiles */
  phy_y = phy_y + bottom_side_gap_ - vertical_tile_gap_;
  return phy_y;
}

int TileQor::append_xml_signal_distribution_network(pugi::xml_node& root_node) const {
  if (!is_signal_distribution_network_frozen_) {
    VTR_LOG_ERROR("Cannot output signal distribution network data as it is not yet frozen\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  for (auto pair : signal_distribution_network_) {
    pugi::xml_node ntwk_node = root_node.append_child(XML_SIG_DISTR_NTWK_NODE_NTWK);
    ntwk_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_ATTR_PORT) = pair.first.c_str();
    for (auto node : pair.second) {
      openfpga::BasicPort prefix_port = str2global_port(pair.first);
      if (!valid_global_port(prefix_port, true)) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      std::string prefix = prefix_port.get_name() + std::to_string(prefix_port.get_lsb());
      pugi::xml_node buf_node = ntwk_node.append_child(XML_SIG_DISTR_NTWK_NTWK_NODE_BUF);
      buf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_NAME) =
        node.instance_name(prefix).c_str();
      buf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_CELL) = node.buf_cell.c_str();
      buf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_X) =
        std::to_string(node.phy_loc.x()).c_str();
      buf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_Y) =
        std::to_string(node.phy_loc.y()).c_str();
      if (node.is_parent_node()) {
        buf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_PARENT) = "";
      } else {
        buf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_PARENT) =
          pair.second[node.parent_node].instance_name(prefix).c_str();
      }
      /* Add list of child nodes */
      if (!node.is_leaf_node()) {
        for (auto child : node.child_nodes) {
          pugi::xml_node buf_child_node =
            buf_node.append_child(XML_SIG_DISTR_NTWK_NTWK_BUF_NODE_CHILD);
          buf_child_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_CHILD_ATTR_NAME) =
            pair.second[child].instance_name(prefix).c_str();
        }
      } else {
        for (auto leaf_coord : node.leaf_tiles) {
          pugi::xml_node buf_leaf_node =
            buf_node.append_child(XML_SIG_DISTR_NTWK_NTWK_BUF_NODE_LEAF_TILE);
          buf_leaf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_LEAF_TILE_ATTR_X) =
            std::to_string(leaf_coord.x()).c_str();
          buf_leaf_node.append_attribute(XML_SIG_DISTR_NTWK_NTWK_BUF_LEAF_TILE_ATTR_Y) =
            std::to_string(leaf_coord.y()).c_str();
        }
      }
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

std::string TileQor::coord2str(const vtr::Point<size_t>& coord) const {
  return std::string("x") + std::to_string(coord.x()) + std::string("y") +
         std::to_string(coord.y());
}

int TileQor::add_io_logical_coordinate(const openfpga::BasicPort& io_port,
                                       const vtr::Point<size_t>& lgk_coord,
                                       const size_t& subtile_idx) {
  /* Sanity check */
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  if (!valid_efpga_location(lgk_coord)) {
    VTR_LOG_ERROR(
      "The logical coordinate (x=%lu, y=%lu) which out of the eFPGA fabric (xmin=%lu, ymin=%lu, "
      "xmax=%lu, ymax=%lu)\n",
      lgk_coord.x(), lgk_coord.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
      efpga_bb.ymax());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  IoLoc io_loc(io_port, lgk_coord, subtile_idx);
  size_t new_id = io_location_.size();
  io_location_.push_back(io_loc);
  io_coord2loc_map_[coord2str(lgk_coord)].push_back(new_id);
  return openfpga::CMD_EXEC_SUCCESS;
}

int TileQor::build_data_io_physical_location(const std::string& mlayer, const float& pin_min_height,
                                             const float& pin_min_width,
                                             const float& pin_min_spacing, const bool& verbose) {
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  /* For each coordinate, get the side and calculate the offset for each subtile */
  for (auto pair : io_coord2loc_map_) {
    size_t num_io_curr_loc = pair.second.size();
    if (num_io_curr_loc == 0) {
      continue;
    }
    vtr::Point<size_t> curr_io_tile_loc = io_location_[pair.second[0]].coord;
    std::vector<e_side> io_sides = find_sides4io(efpga_bb, curr_io_tile_loc);
    if (io_sides.empty()) {
      VTR_LOG_ERROR(
        "The io coordinate (x=%lu, y=%lu) is not on any edge of the eFPGA bounding box (xmin=%lu, "
        "ymin=%lu, xmax=%lu, ymax=%lu)\n",
        curr_io_tile_loc.x(), curr_io_tile_loc.y(), efpga_bb.xmin(), efpga_bb.ymin(),
        efpga_bb.xmax(), efpga_bb.ymax());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Get dimension limits of the current tile location */
    float max_total_width =
      get_tile_width(loc2tile_map_[curr_io_tile_loc.x()][curr_io_tile_loc.y()]);
    float max_total_height =
      get_tile_height(loc2tile_map_[curr_io_tile_loc.x()][curr_io_tile_loc.y()]);
    /* Compute each location for each subtile */
    size_t max_num_pins_all_sides = 0;
    for (auto io_side : io_sides) {
      /* Compute the total number of pins for each side that can accomodate. If the total number
       * exceeds the current number of pins on the location, error out */
      if (io_side == TOP || io_side == BOTTOM) {
        max_num_pins_all_sides += std::floor(max_total_width / (pin_min_width + pin_min_spacing));
      } else {
        VTR_ASSERT(io_side == RIGHT || io_side == LEFT);
        max_num_pins_all_sides += std::floor(max_total_height / (pin_min_height + pin_min_spacing));
      }
    }
    if (max_num_pins_all_sides < num_io_curr_loc) {
      VTR_LOG_ERROR(
        "The number of I/O pins (%lu) exceeds the max. capacity (%lu) to fit the design rule at "
        "coordinate (x=%lu, y=%lu). Suggest to reduce the I/O number or increase the height/width "
        "of the tile\n",
        num_io_curr_loc, max_num_pins_all_sides, curr_io_tile_loc.x(), curr_io_tile_loc.y());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Find the correct sequence to walk through sides. Prefer the longest edge to assign all the
     * pins */
    std::vector<e_side> io_sides2iter;
    if (max_total_width > max_total_height) {
      for (auto io_side : io_sides) {
        if (io_side == TOP || io_side == BOTTOM) {
          io_sides2iter.push_back(io_side);
        }
      }
      for (auto io_side : io_sides) {
        if (io_side == RIGHT || io_side == LEFT) {
          io_sides2iter.push_back(io_side);
        }
      }
    } else {
      for (auto io_side : io_sides) {
        if (io_side == RIGHT || io_side == LEFT) {
          io_sides2iter.push_back(io_side);
        }
      }
      for (auto io_side : io_sides) {
        if (io_side == TOP || io_side == BOTTOM) {
          io_sides2iter.push_back(io_side);
        }
      }
    }
    /* Now assign pin one by one. Prefer the longest edge to assign all the pins */
    float io_x = efpga_physical_start_x(curr_io_tile_loc);
    float io_y = efpga_physical_start_y(curr_io_tile_loc);
    size_t io_loc_cnt = 0;
    for (auto io_side : io_sides2iter) {
      if (io_side == TOP || io_side == BOTTOM) {
        float base_offset = efpga_physical_start_x(curr_io_tile_loc);
        float offset = 0.;
        while (offset < max_total_width) {
          auto curr_io_loc_id = pair.second[io_loc_cnt];
          /* Evenly spread the pin */
          io_location_[curr_io_loc_id].metal_layer = mlayer;
          io_location_[curr_io_loc_id].side = io_side;
          io_location_[curr_io_loc_id].width = pin_min_width;
          io_location_[curr_io_loc_id].length = pin_min_height;
          io_location_[curr_io_loc_id].phy_loc.set(base_offset + offset, io_y);
          VTR_LOGV(verbose,
                   "Assign data I/O pin '%s' to (x=%g, y=%g) on side '%s' on metal layer '%s' with "
                   "pin width '%g' and pin length '%g'\n",
                   io_location_[curr_io_loc_id].name.to_simple_verilog_string().c_str(),
                   io_location_[curr_io_loc_id].phy_loc.x(),
                   io_location_[curr_io_loc_id].phy_loc.y(),
                   SideManager(io_location_[curr_io_loc_id].side).c_str(),
                   io_location_[curr_io_loc_id].metal_layer.c_str(),
                   io_location_[curr_io_loc_id].width, io_location_[curr_io_loc_id].length);
          offset += pin_min_width + pin_min_spacing;
          io_loc_cnt++;
          if (io_loc_cnt >= num_io_curr_loc) {
            break;
          }
        }
      } else {
        VTR_ASSERT(io_side == RIGHT || io_side == LEFT);
        /* Evenly spread the pin */
        float base_offset = efpga_physical_start_y(curr_io_tile_loc);
        float offset = 0.;
        while (offset < max_total_height) {
          auto curr_io_loc_id = pair.second[io_loc_cnt];
          io_location_[curr_io_loc_id].metal_layer = mlayer;
          io_location_[curr_io_loc_id].side = io_side;
          io_location_[curr_io_loc_id].width = pin_min_height;
          io_location_[curr_io_loc_id].length = pin_min_width;
          io_location_[curr_io_loc_id].phy_loc.set(io_x, base_offset + offset);
          VTR_LOGV(verbose,
                   "Assign data I/O pin '%s' to (x=%g, y=%g) on side '%s' on metal layer '%s' with "
                   "pin width '%g' and pin length '%g'\n",
                   io_location_[curr_io_loc_id].name.to_simple_verilog_string().c_str(),
                   io_location_[curr_io_loc_id].phy_loc.x(),
                   io_location_[curr_io_loc_id].phy_loc.y(),
                   SideManager(io_location_[curr_io_loc_id].side).c_str(),
                   io_location_[curr_io_loc_id].metal_layer.c_str(),
                   io_location_[curr_io_loc_id].width, io_location_[curr_io_loc_id].length);

          offset += pin_min_height + pin_min_spacing;
          io_loc_cnt++;
          if (io_loc_cnt >= num_io_curr_loc) {
            break;
          }
        }
      }
      if (io_loc_cnt >= num_io_curr_loc) {
        break;
      }
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

std::vector<TileQor::IoLoc> TileQor::io_locations() const { return io_location_; }

openfpga::BasicPort TileQor::str2global_port(const std::string& port_str) const {
  openfpga::PortParser port_parser(port_str);
  return port_parser.port();
}

bool TileQor::valid_global_port(const openfpga::BasicPort& port, const bool& verbose) const {
  if (port.get_width() != 1) {
    VTR_LOGV_ERROR(
      verbose,
      "The port for each signal distribution network requires to be single-bit but see '%s'\n",
      port.to_verilog_string().c_str());
    return false;
  }
  return true;
}

int TileQor::build_global_io_physical_location(const std::string& mlayer,
                                               const float& pin_min_height,
                                               const float& pin_min_width,
                                               const float& pin_min_spacing, const bool& verbose) {
  vtr::Rect<size_t> efpga_bb = efpga_logical_bounding_box();
  /* Build the map on entry_point -> ports. Ports share the sam entry point should be spread out */
  /* ! Caution: it is a bad idea to use the vtr::Point<> as the key which will cause look-up
   * failure. Here we always walk through keys. So it is fine. It saves the effort to convert
   * between string and vtr::Point<> in later steps */
  std::map<vtr::Point<size_t>, std::vector<std::string>> entry2port_map;
  for (auto pair : signal_distribution_network_) {
    /* Get the entry point */
    size_t cnt_entry_point = 0;
    for (auto node : pair.second) {
      if (!node.is_parent_node()) {
        continue;
      }
      entry2port_map[node.location].push_back(pair.first);
      cnt_entry_point++;
    }
    if (cnt_entry_point != 1) {
      VTR_LOG_ERROR("Invalid global I/O '%s' which has %lu entry point(s). Expect only 1\n",
                    pair.first.c_str(), cnt_entry_point);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  for (auto pair : entry2port_map) {
    vtr::Point<size_t> entry_point = pair.first;
    e_side entry_side = find_side4entry(efpga_bb, entry_point);
    if (entry_side == NUM_2D_SIDES) {
      VTR_LOG_ERROR(
        "The entry point (x=%lu, y=%lu) is not on any edge of the eFPGA bounding box (xmin=%lu, "
        "ymin=%lu, xmax=%lu, ymax=%lu)\n",
        entry_point.x(), entry_point.y(), efpga_bb.xmin(), efpga_bb.ymin(), efpga_bb.xmax(),
        efpga_bb.ymax());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    /* Walk though all the ports under the entry point */
    float offset = 0.;
    float base_offset = 0.;
    if (entry_side == TOP || entry_side == BOTTOM) {
      base_offset = efpga_physical_start_x(entry_point);
    } else {
      VTR_ASSERT(entry_side == RIGHT || entry_side == LEFT);
      base_offset = efpga_physical_start_y(entry_point);
    }
    float io_x = efpga_physical_end_x(entry_point);
    float io_y = efpga_physical_end_y(entry_point);
    for (auto gport : pair.second) {
      /* Sanity check */
      auto result = signal_distribution_network_.find(gport);
      openfpga::BasicPort port = str2global_port(gport);
      if (!valid_global_port(port, true)) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      /* Assign pin location */
      IoLoc io_loc(port, entry_point, 0);
      io_loc.metal_layer = mlayer;
      io_loc.side = entry_side;
      if (entry_side == TOP || entry_side == BOTTOM) {
        io_loc.width = pin_min_width;
        io_loc.length = pin_min_height;
        io_loc.phy_loc.set(base_offset + offset, io_y);
        offset += pin_min_width + pin_min_spacing;
      } else {
        VTR_ASSERT(entry_side == RIGHT || entry_side == LEFT);
        io_loc.width = pin_min_height;
        io_loc.length = pin_min_width;
        io_loc.phy_loc.set(io_x, base_offset + offset);
        offset += pin_min_height + pin_min_spacing;
      }
      size_t new_id = io_location_.size();
      io_location_.push_back(io_loc);
      VTR_LOGV(verbose,
               "Assign global I/O pin '%s' to (x=%g, y=%g) on side '%s' on metal layer '%s' with "
               "pin width '%g' and pin length '%g'\n",
               io_location_[new_id].name.to_simple_verilog_string().c_str(),
               io_location_[new_id].phy_loc.x(), io_location_[new_id].phy_loc.y(),
               SideManager(io_location_[new_id].side).c_str(),
               io_location_[new_id].metal_layer.c_str(), io_location_[new_id].width,
               io_location_[new_id].length);
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

std::string TileQor::side2str(const e_side& type, const bool& verbose) const {
  if (!valid_side(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid side! Expect %s\n", side_all2str().c_str());
    return std::string();
  }
  SideManager side_mgr(type);
  return side_mgr.to_string();
}

std::string TileQor::side_all2str() const {
  std::string all_side_str = "[";
  for (size_t iside = 0; iside < size_t(NUM_2D_SIDES); iside++) {
    SideManager side_mgr(iside);
    all_side_str += side_mgr.to_string() + std::string("|");
  }
  all_side_str.pop_back();
  all_side_str += "]";
  return all_side_str;
}

e_side TileQor::str2side(const std::string& side_str, const bool& verbose) const {
  e_side curr_side = NUM_2D_SIDES;
  for (size_t iside = 0; iside < size_t(NUM_2D_SIDES); iside++) {
    SideManager side_mgr(iside);
    if (side_str == side_mgr.to_string()) {
      curr_side = side_mgr.get_side();
      break;
    }
  }
  if (!valid_side(curr_side)) {
    VTR_LOGV_ERROR(verbose, "Invalid side '%s'. Expect %s\n", side_str.c_str(),
                   side_all2str().c_str());
  }
  return curr_side;
}

bool TileQor::valid_side(const e_side& f_type) const { return f_type != NUM_2D_SIDES; }

int TileQor::add_tile_pin_logical_location(const std::string& tile, const openfpga::BasicPort& port,
                                           const std::string& side_str) {
  /* Sanity checks */
  if (port.get_width() != 1) {
    VTR_LOG_ERROR("The pin for tile '%s' requires to be single-bit but see '%s'\n", tile.c_str(),
                  port.to_verilog_string().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  e_side side = str2side(side_str, true);
  if (!valid_side(side)) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Assign */
  size_t new_id = tile_pin_location_.size();
  TilePinLoc pinloc(tile, port, side);
  tile_pin_location_.push_back(pinloc);
  tile_pinloc_map_[tile][side].push_back(new_id);
  return openfpga::CMD_EXEC_SUCCESS;
}

int TileQor::build_tile_pin_physical_location(const std::string& mlayer,
                                              const float& pin_min_height,
                                              const float& pin_min_width,
                                              const float& pin_min_spacing, const bool& verbose) {
  /* Walk through each tile, get the height/width, compute the maximum number of pins each side can
   * accomodate. */
  for (auto tile2side_pair : tile_pinloc_map_) {
    float tile_width = get_tile_width(tile2side_pair.first);
    float tile_height = get_tile_height(tile2side_pair.first);
    for (auto side2pins_pair : tile2side_pair.second) {
      /* Walk though all the ports under the entry point */
      float offset = 0.;
      e_side pin_side = side2pins_pair.first;
      size_t num_pins_curr_side = side2pins_pair.second.size();
      size_t max_num_pins_curr_side = 0;
      /* Run a quick check on if the height/width is not sufficient to accomodate all the pins on
       * the side */
      if (pin_side == TOP || pin_side == BOTTOM) {
        max_num_pins_curr_side = std::floor(tile_width / (pin_min_width + pin_min_spacing));
      } else {
        VTR_ASSERT(pin_side == RIGHT || pin_side == LEFT);
        max_num_pins_curr_side = std::floor(tile_height / (pin_min_height + pin_min_spacing));
      }
      if (max_num_pins_curr_side < num_pins_curr_side) {
        VTR_LOG_ERROR(
          "The number of pins (%lu) exceeds the max. capacity (%lu) to fit the design rule for "
          "side '%s' of tile '%s'. Suggest to reduce the I/O number or increase the height/width "
          "of the tile\n",
          num_pins_curr_side, max_num_pins_curr_side, side2str(pin_side).c_str(),
          tile2side_pair.first.c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      /* Assign the pin one by one */
      for (auto pin_id : side2pins_pair.second) {
        /* Assign pin location */
        tile_pin_location_[pin_id].metal_layer = mlayer;
        tile_pin_location_[pin_id].side = pin_side;
        if (pin_side == TOP || pin_side == BOTTOM) {
          tile_pin_location_[pin_id].width = pin_min_width;
          tile_pin_location_[pin_id].length = pin_min_height;
          if (pin_side == TOP) {
            tile_pin_location_[pin_id].phy_loc.set(offset, tile_height);
          } else {
            tile_pin_location_[pin_id].phy_loc.set(offset, 0.);
          }
          offset += pin_min_width + pin_min_spacing;
        } else {
          VTR_ASSERT(pin_side == RIGHT || pin_side == LEFT);
          tile_pin_location_[pin_id].width = pin_min_height;
          tile_pin_location_[pin_id].length = pin_min_width;
          if (pin_side == RIGHT) {
            tile_pin_location_[pin_id].phy_loc.set(tile_width, offset);
          } else {
            tile_pin_location_[pin_id].phy_loc.set(0., offset);
          }
          offset += pin_min_height + pin_min_spacing;
        }
        VTR_LOGV(verbose,
                 "Assign tile '%s' pin '%s' to (x=%g, y=%g) on side '%s' on metal layer '%s' with "
                 "pin width '%g' and pin length '%g'\n",
                 tile_pin_location_[pin_id].tile.c_str(),
                 tile_pin_location_[pin_id].pin.to_simple_verilog_string().c_str(),
                 tile_pin_location_[pin_id].phy_loc.x(), tile_pin_location_[pin_id].phy_loc.y(),
                 SideManager(tile_pin_location_[pin_id].side).c_str(),
                 tile_pin_location_[pin_id].metal_layer.c_str(), tile_pin_location_[pin_id].width,
                 tile_pin_location_[pin_id].length);
      }
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

const std::vector<TileQor::TilePinLoc>& TileQor::tile_pin_loc() const { return tile_pin_location_; }

size_t TileQor::side2int(const openfpga::BasicPort& port, const std::string& tile,
                         const e_side& side) const {
  /* ICC2 follows a clockwise side indexing. The left-most vertical edge is tagged as side=1, then
   * increment by clockwise rotating. To be exact, LEFT=1, TOP=2, RIGHT=3, BOTTOM=4 */
  switch (side) {
    case BOTTOM:
      return 4;
    case RIGHT:
      return 3;
    case TOP:
      return 2;
    case LEFT:
      return 1;
    default:
      VTR_LOG_ERROR("Invalid side for I/O '%s' of '%s'. Expect %s\n",
                    port.to_verilog_string().c_str(), tile.c_str(), side_all2str().c_str());
      return 0;
  }
}

void TileQor::check_signal_distribution_network_frozen() const {
  if (!is_signal_distribution_network_frozen_) {
    VTR_LOG_ERROR("Cannot output signal distribution network data as it is not yet frozen\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::vector<std::string> TileQor::signal_distribution_networks() const {
  check_signal_distribution_network_frozen();
  std::vector<std::string> ret;
  for (auto pair : signal_distribution_network_) {
    ret.push_back(pair.first);
  }
  return ret;
}

void TileQor::check_valid_signal_distribution_network_name(const std::string& ntwk_name) const {
  auto result = signal_distribution_network_.find(ntwk_name);
  if (result == signal_distribution_network_.end()) {
    VTR_LOG_ERROR("Invalid name '%s' for signal distribution network which is not defined\n",
                  ntwk_name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TileQor::check_valid_signal_distribution_network_buffer(
  const std::string& ntwk_name, const SigDistrNtwkBufId& buf_id) const {
  auto result = signal_distribution_network_.find(ntwk_name);
  if (result == signal_distribution_network_.end()) {
    VTR_LOG_ERROR("Invalid name '%s' for signal distribution network which is not defined\n",
                  ntwk_name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (size_t(buf_id) >= result->second.size()) {
    VTR_LOG_ERROR(
      "Invalid buffer (id='%ld') for signal distribution network '%s'. Expect range [0, %lu)\n",
      size_t(buf_id), ntwk_name.c_str(), result->second.size());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

std::vector<SigDistrNtwkBufId> TileQor::signal_distribution_network_buffers(
  const std::string& ntwk_name) const {
  check_signal_distribution_network_frozen();
  check_valid_signal_distribution_network_name(ntwk_name);
  std::vector<SigDistrNtwkBufId> ret;
  ret.resize(signal_distribution_network_.at(ntwk_name).size());
  for (size_t idx = 0; idx < ret.size(); ++idx) {
    ret[idx] = SigDistrNtwkBufId(idx);
  }
  return ret;
}

vtr::Point<float> TileQor::signal_distribution_network_buffer_physical_location(
  const std::string& ntwk_name, const SigDistrNtwkBufId& buf_id) const {
  check_signal_distribution_network_frozen();
  check_valid_signal_distribution_network_buffer(ntwk_name, buf_id);
  return signal_distribution_network_.at(ntwk_name)[size_t(buf_id)].phy_loc;
}

bool TileQor::is_signal_distribution_network_leaf_buffer(const std::string& ntwk_name,
                                                         const SigDistrNtwkBufId& buf_id) const {
  check_signal_distribution_network_frozen();
  check_valid_signal_distribution_network_buffer(ntwk_name, buf_id);
  return signal_distribution_network_.at(ntwk_name)[size_t(buf_id)].is_leaf_node();
}

const std::vector<vtr::Point<size_t>>& TileQor::signal_distribution_network_buffer_leaf_tiles(
  const std::string& ntwk_name, const SigDistrNtwkBufId& buf_id) const {
  check_signal_distribution_network_frozen();
  check_valid_signal_distribution_network_buffer(ntwk_name, buf_id);
  return signal_distribution_network_.at(ntwk_name)[size_t(buf_id)].leaf_tiles;
}

std::vector<SigDistrNtwkBufId> TileQor::signal_distribution_network_buffer_fanouts(
  const std::string& ntwk_name, const SigDistrNtwkBufId& buf_id) const {
  check_signal_distribution_network_frozen();
  check_valid_signal_distribution_network_buffer(ntwk_name, buf_id);
  std::vector<SigDistrNtwkBufId> ret;
  for (auto idx : signal_distribution_network_.at(ntwk_name)[size_t(buf_id)].child_nodes) {
    ret.push_back(SigDistrNtwkBufId(idx));
  }
  return ret;
}

}  // namespace arcxpress
