#pragma once

#include <array>
#include <string>
#include <vector>

#include "io_coord_data_fwd.h"
#include "physical_types.h"
#include "vtr_geometry.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class IoCoordData {
 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  IoCoordData();

 public: /* -- Aggregators -- */
  std::array<e_side, NUM_2D_SIDES> borderline_sides() const;
  std::vector<IoBorderlineId> borderlines(const e_side& b_side) const;

 public: /* -- Accessors -- */
  vtr::Point<size_t> start_point(const e_side& b_side, const IoBorderlineId& b_id) const;
  vtr::Point<size_t> end_point(const e_side& b_side, const IoBorderlineId& b_id) const;
  size_t num_coordinates() const;
  std::string side2str(const e_side& b_side) const;

 public: /* -- Mutators -- */
  void add_coord(const vtr::Point<size_t>& coord);
  /* Compute the border lines based on the coordinates */
  int compute_borders(const bool& verbose);
  void clear();

 public: /* -- Validator -- */
  void check_valid() const;
  void check_valid_side(const e_side& b_side) const;
  void check_valid_borderline(const e_side& b_side, const IoBorderlineId& b_id) const;

 private: /* -- Utility -- */
  /* find the bounding box */
  vtr::Rect<size_t> find_bb(const bool& verbose);
  /* Convert bounding box to borderlines. The reference bounding box is the boundaries of all the
   * coordinates */
  int bb2borderline(const vtr::Rect<size_t>& ref_bb, const vtr::Rect<size_t>& bb,
                    const bool& verbose);

 private: /* -- Internal data -- */
  /* Raw data: Only keep the (x, y) for I/Os */
  std::vector<vtr::Point<size_t>> coords_;
  std::vector<bool> visited_coords_;
  /* Borderlines: [side][border_idx][starting_point, ending_point] */
  std::array<std::vector<std::array<vtr::Point<size_t>, 2>>, NUM_2D_SIDES> borders_;
  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress
