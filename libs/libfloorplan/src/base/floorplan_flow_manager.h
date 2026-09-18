#pragma once
#include <yaml-cpp/yaml.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "floorplan_params.h"
#include "read_floorplan_params.h"
#include "tile_qor.h"
namespace arkangel {
class FloorplanFlow {
 public: /* -- Constructors -- */
  /* Not allow to copy and copy assignment operator. Keep the copy unique */
  FloorplanFlow(const FloorplanParams& params, TileQor& tile_qor);
  FloorplanFlow(const FloorplanFlow&) = delete;
  FloorplanFlow& operator=(const FloorplanFlow&) = delete;
  FloorplanFlow() = default;

 public: /* -- Accessors -- */
  float cal_total_actual_area() const;
  float cal_total_cell_area() const;
  TileInfo calculate_tile_info(const float& area, const float& utilization,
                               const float& tile_width_height_ratio, const bool& is_gap_odd,
                               const float& sc_height, const float& cpp,
                               const float& boundary_cell_width) const;
  TileInfo calculate_tile_info_fixed_width(const float& area, const float& width,
                                           const float& utilization, const bool& is_gap_odd,
                                           const float& sc_height, const float& cpp,
                                           const float& boundary_cell_width) const;
  TileInfo calculate_tile_info_fixed_height(const float& area, const float& height,
                                            const float& utilization, const bool& is_gap_odd,
                                            const float& sc_height, const float& cpp,
                                            const float& boundary_cell_width) const;
  int run(const std::string& dom_tile_name, const bool& verbose);
  std::vector<std::string> validate_floorplan(const bool& verbose) const;

 private: /* -- Internal data -- */
  /* Bin paths */
  FloorplanParams floorplan_params_;
  TileQor& tile_qor_;
  size_t MAX_ITERATION_;
};

}  // namespace arkangel
