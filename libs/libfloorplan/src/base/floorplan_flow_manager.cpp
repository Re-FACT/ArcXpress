#include "floorplan_flow_manager.h"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "command_exit_codes.h"
#include "tile_qor.h"
namespace arkangel {

FloorplanFlow::FloorplanFlow(const FloorplanParams& params, TileQor& tile_qor)
  : floorplan_params_(params), tile_qor_(tile_qor) {
  // update tile_qor_utilization using the utilization values get from floorplan_task.yaml
  if (floorplan_params_.default_utilization != 0) {
    tile_qor_.set_default_utilization(floorplan_params_.default_utilization);
  } else {
    tile_qor_.set_default_utilization(0.8);
  }

  for (const auto& [key, value] : floorplan_params_.utilization_map) {
    if (std::find(tile_qor_.tiles().begin(), tile_qor_.tiles().end(), key) !=
        tile_qor_.tiles().end()) {
      tile_qor_.set_utilization(key, value);
    }
  }

  tile_qor_.set_top_side_gap(floorplan_params_.tile_params.top_side_gap);
  tile_qor_.set_bottom_side_gap(floorplan_params_.tile_params.bottom_side_gap);
  tile_qor_.set_right_side_gap(floorplan_params_.tile_params.right_side_gap);
  tile_qor_.set_left_side_gap(floorplan_params_.tile_params.left_side_gap);

  tile_qor_.set_horizontal_tile_gap(floorplan_params_.tile_params.horizontal_tile_gap);
  tile_qor_.set_vertical_tile_gap(floorplan_params_.tile_params.vertical_tile_gap);

  MAX_ITERATION_ = 50;
};

/*
The following functions provide three methods to calculate tile dimensions:
1. calculate_tile_info: assumes the tile is a square.
2. calculate_tile_info_fixed_width: fixes the tile width and computes the corresponding height based
on the given area.
3. calculate_tile_info_fixed_height: fixes the tile height and computes the corresponding width
based on the given area.

Notes:
- cpp: Contacted Poly Pitch.
- sc_height: Standard cell height.
- is_gap_odd:
    - If the tile gap along the y-axis is odd, the tile height must be an odd-numbered multiple.
    - Otherwise, the tile height must be an even-numbered multiple.
- The tile height must be an integer multiple of sc_height.
- The tile width must be an integer multiple of cpp. Additionally, the returned width includes the
boundary cell width (gap along the x-axis).
*/

TileInfo FloorplanFlow::calculate_tile_info(const float& area, const float& utilization,
                                            const float& tile_width_height_ratio,
                                            const bool& is_gap_odd, const float& sc_height,
                                            const float& cpp,
                                            const float& boundary_cell_width) const {
  int height_unit =
    std::ceil((std::sqrt((area / utilization) / tile_width_height_ratio)) / sc_height);
  if (height_unit % 2 != is_gap_odd) {
    height_unit++;
  }
  float height = height_unit * sc_height;
  float width = std::ceil(((area / utilization) / height + boundary_cell_width * 2) / cpp) * cpp;
  arkangel::TileInfo seed_tile(height, width);
  return seed_tile;
}

// given width, calculate the rest info
TileInfo FloorplanFlow::calculate_tile_info_fixed_width(const float& area, const float& width,
                                                        const float& utilization,
                                                        const bool& is_gap_odd,
                                                        const float& sc_height, const float& cpp,
                                                        const float& boundary_cell_width) const {
  int height_unit =
    std::ceil(((area / utilization) / (width - boundary_cell_width * 2)) / sc_height);
  if (height_unit % 2 != is_gap_odd) {
    height_unit++;
  }
  float height = height_unit * sc_height;
  arkangel::TileInfo tile_info(height, width);
  return tile_info;
}

// given height, calculate the rest info
TileInfo FloorplanFlow::calculate_tile_info_fixed_height(const float& area, const float& height,
                                                         const float& utilization,
                                                         const bool& is_gap_odd,
                                                         const float& sc_height, const float& cpp,
                                                         const float& boundary_cell_width) const {
  float width = std::ceil(((area / utilization) / height + boundary_cell_width * 2) / cpp) * cpp;
  arkangel::TileInfo tile_info(height, width);
  return tile_info;
}

std::vector<std::string> FloorplanFlow::validate_floorplan(const bool& verbose) const {
  // try to validate the result: actual area – boundary_cell_width*height*2 > qor area/utilization
  std::vector<std::string> violated_tile = {};
  for (auto tile : tile_qor_.tiles()) {
    if ((tile_qor_.get_tile_actual_area(tile) -
         floorplan_params_.boundary_cell_width * 2 * tile_qor_.get_tile_height(tile)) <
        (tile_qor_.cell_area(tile) / tile_qor_.utilization(tile))) {
      VTR_LOGV(verbose, "Tile %s violates floorplan requirements\n", tile.c_str());
      VTR_LOGV(verbose, "\tTile name: %s height: %g width: %g Grid height: %lu Grid width: %lu\n",
               tile.c_str(), tile_qor_.get_tile_height(tile), tile_qor_.get_tile_width(tile),
               tile_qor_.grid_height(tile), tile_qor_.grid_width(tile));
      violated_tile.push_back(tile);
    }
  }
  return violated_tile;
}

float FloorplanFlow::cal_total_actual_area() const { return tile_qor_.cal_total_actual_area(); }

float FloorplanFlow::cal_total_cell_area() const { return tile_qor_.cal_total_cell_area(); }

int FloorplanFlow::run(const std::string& dom_tile_name, const bool& verbose) {
  int num_err = 0;
  std::string tile_main = dom_tile_name;
  if (tile_main.empty()) {
    tile_main = tile_qor_.find_tile_main();
    VTR_LOGV(verbose, "Selected tile '%s' as the dominant tile\n", tile_main.c_str());
  } else {
    if (!tile_qor_.valid_tile_name(tile_main)) {
      VTR_LOG_ERROR(
        "User specify tile '%s' as the dominant tile but it does not exist in tile reference\n",
        tile_main.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    VTR_LOGV(verbose, "User specify tile '%s' as the dominant tile\n", tile_main.c_str());
  }
  float tile_width_height_ratio = floorplan_params_.tile_params.tile_width_height_ratio;
  bool is_gap_odd = (floorplan_params_.tile_params.tile_gap[1] % 2 == 0) ? 0 : 1;
  float sc_height = floorplan_params_.sc_height;
  float cpp = floorplan_params_.cpp;
  float boundary_cell_width = floorplan_params_.boundary_cell_width;
  /* For each grid (both in height and width), get the max area in the same row and columns of tile
   * main instances Each grid has a max area
   */
  float max_area = tile_qor_.cell_area(tile_main) / tile_qor_.utilization(tile_main);
  for (size_t x_ofs = 0; x_ofs < tile_qor_.grid_width(tile_main); ++x_ofs) {
    for (size_t y_ofs = 0; y_ofs < tile_qor_.grid_height(tile_main); ++y_ofs) {
      float grid_max_area =
        max_area / (tile_qor_.grid_width(tile_main) * tile_qor_.grid_height(tile_main));
      for (auto tile : tile_qor_.tiles()) {
        if (tile_qor_.tile_share_width(tile_main, x_ofs, tile) &&
            tile_qor_.tile_share_height(tile_main, y_ofs, tile)) {
          float curr_grid_area = tile_qor_.cell_grid_area(tile) / tile_qor_.utilization(tile);
          if (curr_grid_area > grid_max_area) {
            grid_max_area = grid_max_area / tile_qor_.utilization(tile);
          }
        }
      }
      grid_max_area = grid_max_area * tile_qor_.utilization(tile_main);
      // calculate the tile height and tile width using the max_area
      arkangel::TileInfo tile_main_info = calculate_tile_info(
        grid_max_area, tile_qor_.utilization(tile_main), tile_width_height_ratio, is_gap_odd,
        sc_height, cpp, boundary_cell_width);
      tile_qor_.set_tile_info(tile_main, tile_main_info, x_ofs, y_ofs);
    }
  }
  VTR_LOGV(
    verbose, "\tSized dominant tile: %s height: %g width: %g Grid height: %lu Grid width: %lu\n",
    tile_main.c_str(), tile_qor_.get_tile_height(tile_main), tile_qor_.get_tile_width(tile_main),
    tile_qor_.grid_height(tile_main), tile_qor_.grid_width(tile_main));
  tile_qor_.update_neighbours_width(tile_main);
  tile_qor_.update_neighbours_height(tile_main);

  // start to calculate other tiles' info
  VTR_LOGV(verbose, "Initialize height and width for rest of tiles\n");
  // Initial trial
  for (auto tile : tile_qor_.tiles()) {
    if (tile == tile_main) continue;
    if (tile_qor_.cell_area(tile) == 0) continue;

    /* For each offset in x, update the width for the current tile if it shares the height with
     * tile_main For each offset in y, update the height for the current tile if it shares the width
     * with tile_main If the current tile does not share height or width with tile_main,
     */
    // if the tile shares width with tile_main, calculate its height with fixed width
    for (size_t x_ofs = 0; x_ofs < tile_qor_.grid_width(tile); ++x_ofs) {
      for (size_t y_ofs = 0; y_ofs < tile_qor_.grid_height(tile); ++y_ofs) {
        if (tile_qor_.tile_share_width(tile, x_ofs, tile_main)) {
          VTR_LOGV(verbose,
                   "Update tile '%s' that share the width with tile main '%s' at x_offset(=%lu), "
                   "y_offset(=%lu)\n",
                   tile.c_str(), tile_main.c_str(), x_ofs, y_ofs);
          size_t tile_main_x_ofs = tile_qor_.tile_share_width_target_offset(tile, x_ofs, tile_main);
          arkangel::TileInfo tile_info = calculate_tile_info_fixed_width(
            tile_qor_.cell_grid_area(tile),
            tile_qor_.get_tile_width_by_offset(tile_main, tile_main_x_ofs),
            tile_qor_.utilization(tile), is_gap_odd, sc_height, cpp, boundary_cell_width);
          VTR_LOGV(verbose, "Sized tile '%s' with initial height (%g) x width (%g)\n", tile.c_str(),
                   tile_info.height, tile_info.width);
          tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
          tile_qor_.update_neighbours_height_by_offset(tile, y_ofs);
        } else if (tile_qor_.tile_share_height(tile, y_ofs, tile_main)) {
          VTR_LOGV(verbose,
                   "Update tile '%s' that share the height with tile main '%s' at x_offset(=%lu), "
                   "y_offset(=%lu)\n",
                   tile.c_str(), tile_main.c_str(), x_ofs, y_ofs);
          size_t tile_main_y_ofs =
            tile_qor_.tile_share_height_target_offset(tile, y_ofs, tile_main);
          VTR_LOGV(verbose,
                   "Given tile main '%s' y_offset(=%lu) is considered as height reference\n",
                   tile_main.c_str(), tile_main_y_ofs);
          arkangel::TileInfo tile_info = calculate_tile_info_fixed_height(
            tile_qor_.cell_grid_area(tile),
            tile_qor_.get_tile_height_by_offset(tile_main, tile_main_y_ofs),
            tile_qor_.utilization(tile), is_gap_odd, sc_height, cpp, boundary_cell_width);
          VTR_LOGV(verbose, "Sized tile '%s' with initial height (%g) x width (%g)\n", tile.c_str(),
                   tile_info.height, tile_info.width);
          tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
          tile_qor_.update_neighbours_width_by_offset(tile, x_ofs);
        } else {
          float current_tile_height = tile_qor_.get_tile_height_by_offset(tile, y_ofs);
          float current_tile_width = tile_qor_.get_tile_width_by_offset(tile, x_ofs);
          VTR_LOGV(verbose,
                   "Auto-sizing independent tile '%s' with initial height (%g) x width (%g)\n",
                   tile.c_str(), current_tile_height, current_tile_width);
          if (tile_qor_.is_undefined(current_tile_height) &&
              tile_qor_.is_undefined(current_tile_width)) {
            // if the tile is not tile_main's neighbor and its height and width are not set, assume
            // it is a square to calculate its height and width
            arkangel::TileInfo tile_info =
              calculate_tile_info(tile_qor_.cell_grid_area(tile), tile_qor_.utilization(tile), 1,
                                  is_gap_odd, sc_height, cpp, boundary_cell_width);
            tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
            // as its height and width all changed, update its neighbours height and width with the
            // newly calculated tile size
            tile_qor_.update_neighbours_width_by_offset(tile, x_ofs);
            tile_qor_.update_neighbours_height_by_offset(tile, y_ofs);
          } else if (tile_qor_.is_undefined(current_tile_width) &&
                     !tile_qor_.is_undefined(current_tile_height)) {
            // if the tile's height has been set before, calculate its width with height fixed
            arkangel::TileInfo tile_info = calculate_tile_info_fixed_height(
              tile_qor_.cell_grid_area(tile), current_tile_height, tile_qor_.utilization(tile),
              is_gap_odd, sc_height, cpp, boundary_cell_width);
            tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
            tile_qor_.update_neighbours_width_by_offset(tile, x_ofs);
          } else if (!tile_qor_.is_undefined(current_tile_width) &&
                     tile_qor_.is_undefined(current_tile_height)) {
            // if the tile's width has been set before, calculate its height with width fixed
            arkangel::TileInfo tile_info = calculate_tile_info_fixed_width(
              tile_qor_.cell_grid_area(tile), current_tile_width, tile_qor_.utilization(tile),
              is_gap_odd, sc_height, cpp, boundary_cell_width);
            tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
            tile_qor_.update_neighbours_height_by_offset(tile, y_ofs);
          } else {
            // if its height and width have all been set during update_neighbor_height/width step of
            // other tiles, then we continue
            continue;
          }
        }
      }
    }
  }

  // Iteratively resolve violation
  std::vector<std::string> violated_tile = validate_floorplan(verbose);
  VTR_LOGV(verbose, "Start iteratively improving tile dimension\n");
  int iteration_num = 0;
  while (!violated_tile.empty()) {
    VTR_LOGV(verbose, "\tIteration %ld:\n", iteration_num);
    auto tile = violated_tile[0];
    for (size_t x_ofs = 0; x_ofs < tile_qor_.grid_width(tile); ++x_ofs) {
      for (size_t y_ofs = 0; y_ofs < tile_qor_.grid_height(tile); ++y_ofs) {
        if (tile_qor_.tile_share_width(tile, x_ofs, tile_main)) {
          size_t tile_main_x_ofs = tile_qor_.tile_share_width_target_offset(tile, x_ofs, tile_main);
          arkangel::TileInfo tile_info = calculate_tile_info_fixed_width(
            tile_qor_.cell_grid_area(tile),
            tile_qor_.get_tile_width_by_offset(tile_main, tile_main_x_ofs),
            tile_qor_.utilization(tile), is_gap_odd, sc_height, cpp, boundary_cell_width);
          VTR_LOGV(verbose, "\t\tUpdate tile '%s' width (x_offset=%ld) upon tile_main width\n",
                   tile.c_str(), x_ofs);
          tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
          tile_qor_.update_neighbours_height_by_offset(tile, y_ofs);
        } else if (tile_qor_.tile_share_height(tile, y_ofs, tile_main)) {
          size_t tile_main_y_ofs =
            tile_qor_.tile_share_height_target_offset(tile, x_ofs, tile_main);
          arkangel::TileInfo tile_info = calculate_tile_info_fixed_height(
            tile_qor_.cell_grid_area(tile),
            tile_qor_.get_tile_height_by_offset(tile_main, tile_main_y_ofs),
            tile_qor_.utilization(tile), is_gap_odd, sc_height, cpp, boundary_cell_width);
          VTR_LOGV(verbose, "Update tile '%s' width (y_offset=%ld) upon tile_main height\n",
                   tile.c_str(), y_ofs);
          tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
          tile_qor_.update_neighbours_width_by_offset(tile, x_ofs);
        } else {
          float current_tile_height = tile_qor_.get_tile_height_by_offset(tile, y_ofs);
          float current_tile_width = tile_qor_.get_tile_width_by_offset(tile, x_ofs);
          if (current_tile_height > current_tile_width) {
            arkangel::TileInfo tile_info = calculate_tile_info_fixed_height(
              tile_qor_.cell_grid_area(tile), current_tile_height, tile_qor_.utilization(tile),
              is_gap_odd, sc_height, cpp, boundary_cell_width);
            tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
            tile_qor_.update_neighbours_width_by_offset(tile, x_ofs);
          } else {
            arkangel::TileInfo tile_info = calculate_tile_info_fixed_width(
              tile_qor_.cell_grid_area(tile), current_tile_width, tile_qor_.utilization(tile),
              is_gap_odd, sc_height, cpp, boundary_cell_width);
            tile_qor_.set_tile_info(tile, tile_info, x_ofs, y_ofs);
            tile_qor_.update_neighbours_height_by_offset(tile, y_ofs);
          }
        }
      }
    }
    // once we resolve a viloation, we validate the current floorplan to see whether the solution
    // convers all the other violations.
    violated_tile = validate_floorplan(verbose);
    iteration_num++;
    if (iteration_num > MAX_ITERATION_) {
      VTR_LOG(
        "Floorplan violations persist after reaching the maximum of %lu iterations. Please check "
        "the design manually!\n",
        MAX_ITERATION_);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  VTR_LOGV(verbose, "Complete after %lu iterations\n", iteration_num);

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arkangel
