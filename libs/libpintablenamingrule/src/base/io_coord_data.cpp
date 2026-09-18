#include "io_coord_data.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

IoCoordData::IoCoordData() { is_valid_ = false; }

void IoCoordData::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("I/O coordinate data is invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void IoCoordData::check_valid_side(const e_side& b_side) const {
  if (size_t(b_side) >= NUM_2D_SIDES) {
    VTR_LOG_ERROR("Borderline side '%d' is invalid! Expect [0, %d)\n", size_t(b_side),
                  size_t(NUM_2D_SIDES));
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}
void IoCoordData::check_valid_borderline(const e_side& b_side, const IoBorderlineId& b_id) const {
  check_valid_side(b_side);
  if (size_t(b_id) >= borders_[size_t(b_side)].size()) {
    VTR_LOG_ERROR("Borderline index '%lu is invalid!", size_t(b_id));
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void IoCoordData::clear() {
  coords_.clear();
  visited_coords_.clear();
  for (size_t i = 0; i < NUM_2D_SIDES; ++i) {
    borders_[i].clear();
  }
  is_valid_ = false;
}

std::array<e_side, NUM_2D_SIDES> IoCoordData::borderline_sides() const { return TOTAL_2D_SIDES; }

std::vector<IoBorderlineId> IoCoordData::borderlines(const e_side& b_side) const {
  check_valid_side(b_side);
  std::vector<IoBorderlineId> ret;
  ret.reserve(borders_[size_t(b_side)].size());
  for (size_t i_b = 0; i_b < borders_[size_t(b_side)].size(); ++i_b) {
    ret.push_back(IoBorderlineId(i_b));
  }
  return ret;
}

vtr::Point<size_t> IoCoordData::start_point(const e_side& b_side,
                                            const IoBorderlineId& b_id) const {
  check_valid_borderline(b_side, b_id);
  return borders_[size_t(b_side)][size_t(b_id)][0];
}

vtr::Point<size_t> IoCoordData::end_point(const e_side& b_side, const IoBorderlineId& b_id) const {
  check_valid_borderline(b_side, b_id);
  return borders_[size_t(b_side)][size_t(b_id)][1];
}

size_t IoCoordData::num_coordinates() const { return coords_.size(); }

std::string IoCoordData::side2str(const e_side& b_side) const {
  check_valid_side(b_side);
  return std::string(TOTAL_2D_SIDE_STRINGS[size_t(b_side)]);
}

void IoCoordData::add_coord(const vtr::Point<size_t>& coord) { coords_.push_back(coord); }

vtr::Rect<size_t> IoCoordData::find_bb(const bool& verbose) {
  vtr::Rect<size_t> bb(-1, -1, -1, -1);
  for (size_t icrd = 0; icrd < coords_.size(); ++icrd) {
    if (visited_coords_[icrd]) {
      continue;
    }
    if (bb.xmin() == -1 || bb.xmin() > coords_[icrd].x()) {
      bb.set_xmin(coords_[icrd].x());
    }
    if (bb.ymin() == -1 || bb.ymin() > coords_[icrd].y()) {
      bb.set_ymin(coords_[icrd].y());
    }
    if (bb.xmax() == -1 || bb.xmax() < coords_[icrd].x()) {
      bb.set_xmax(coords_[icrd].x());
    }
    if (bb.ymax() == -1 || bb.ymax() < coords_[icrd].y()) {
      bb.set_ymax(coords_[icrd].y());
    }
  }
  /* Updated visited */
  size_t cnt = 0;
  for (size_t icrd = 0; icrd < coords_.size(); ++icrd) {
    if (!bb.strictly_contains(coords_[icrd])) {
      visited_coords_[icrd] = true;
      cnt++;
    }
  }
  VTR_LOGV(verbose, "Marked %lu/%lu nodes as visited\n", cnt, visited_coords_.size());
  return bb;
}

int IoCoordData::bb2borderline(const vtr::Rect<size_t>& ref_bb, const vtr::Rect<size_t>& bb,
                               const bool& verbose) {
  bool curr_bb_is_ref = (ref_bb == bb);
  /* If this is the reference bounding box, side assignment is easy:
   *  ymax -> top; xmax -> right; ymin -> bottom; xmin -> left;
   * The condition above is first satisfied first assignment.
   * The first satisfied condition is found, the side will be assigned
   * A corner case to handle: if the bounding box is a line
   * In this case, only consider bottom and left sides
   */
  bool bb_is_line = false;
  if (bb.xmin() == bb.xmax() || bb.ymin() == bb.ymax()) {
    bb_is_line = true;
    VTR_LOGV(verbose,
             "Found current bounding-box is a line (xmin=%lu, ymin=%lu) -> (xmax=%lu, ymax=%lu)\n",
             bb.xmin(), bb.ymin(), bb.xmax(), bb.ymax());
  }
  /* If this is an inner bounding box, the side assignment depends on the middle lines
   *  - If a line is on the left of (or on) the midline on x-axis, xmin + (xmax - xmin) / 2, it
   * should be on LEFT side.
   *  - If a line goes across the midline on x-axis, xmin + (xmax - xmin) / 2, it should be on RIGHT
   * side.
   *  - If a line is on the top of the midline on y-axis, ymin + (ymax - ymin) / 2, it should be on
   * TOP side.
   *  - If a line is below (or on) the midline on y-axis, ymin + (ymax - ymin) / 2, it should be on
   * BOTTOM side.
   */
  size_t mid_x = 0;
  size_t mid_y = 0;
  if (!curr_bb_is_ref) {
    mid_x = (ref_bb.xmax() + ref_bb.xmin()) / 2;
    mid_y = (ref_bb.ymax() + ref_bb.ymin()) / 2;
  }
  std::array<std::vector<vtr::Point<size_t>>, NUM_2D_SIDES> bb_lines;
  for (auto coord : coords_) {
    e_side c_side = NUM_2D_SIDES;
    if (coord.y() == bb.ymax()) {
      if (curr_bb_is_ref) {
        c_side = bb_is_line ? BOTTOM : TOP;
      } else {
        c_side = coord.y() > mid_y ? TOP : BOTTOM;
      }
    } else if (coord.x() == bb.xmax()) {
      if (curr_bb_is_ref) {
        c_side = bb_is_line ? LEFT : RIGHT;
      } else {
        c_side = coord.x() > mid_x ? RIGHT : LEFT;
      }
    } else if (coord.y() == bb.ymin()) {
      if (curr_bb_is_ref) {
        c_side = BOTTOM;
      } else {
        c_side = coord.y() > mid_y ? TOP : BOTTOM;
      }
    } else if (coord.x() == bb.xmin()) {
      if (curr_bb_is_ref) {
        c_side = LEFT;
      } else {
        c_side = coord.x() > mid_x ? RIGHT : LEFT;
      }
    } else {
      continue; /* Not on the boundary, skip */
    }
    bb_lines[size_t(c_side)].push_back(coord);
  }
  /* For each line, find the start & end points */
  for (size_t iside = 0; iside < NUM_2D_SIDES; ++iside) {
    vtr::Line<size_t> bb_line(bb_lines[iside]);
    vtr::Rect<size_t> borderline = bb_line.bounding_box();
    if (borderline.xmin() == borderline.xmax() || borderline.ymin() == borderline.ymax()) {
      vtr::Point<size_t> line_endpoints_0(borderline.xmin(), borderline.ymin());
      vtr::Point<size_t> line_endpoints_1(borderline.xmax(), borderline.ymax());
      std::array<vtr::Point<size_t>, 2> line_endpoints = {line_endpoints_0, line_endpoints_1};
      borders_[iside].push_back(line_endpoints);
      VTR_LOGV(verbose, "Detect borderline (xmin=%lu, ymin=%lu) -> (xmax=%lu, ymax=%lu)\n",
               line_endpoints_0.x(), line_endpoints_0.y(), line_endpoints_1.x(),
               line_endpoints_1.y());
    } else {
      VTR_LOG_ERROR(
        "Invalid %s borderline (xmin=%lu, ymin=%lu) -> (xmax=%lu, ymax=%lu). Expect a line but "
        "found a rectanlge!\n",
        TOTAL_2D_SIDE_STRINGS[iside], borderline.xmin(), borderline.ymin(), borderline.xmax(),
        borderline.ymax());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int IoCoordData::compute_borders(const bool& verbose) {
  /* Initialized */
  visited_coords_.resize(coords_.size(), false);
  /* Find the outter borderline: top-right (xmax, ymax) and bottom-left (xmin, ymin) */
  vtr::Rect<size_t> outter_bb = find_bb(verbose);
  VTR_LOGV(verbose, "Found outter borderline (xmin=%lu, ymin=%lu) -> (xmax=%lu, ymax=%lu)\n",
           outter_bb.xmin(), outter_bb.ymin(), outter_bb.xmax(), outter_bb.ymax());
  /* Assign existing border lines */
  int status = bb2borderline(outter_bb, outter_bb, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Find the outter borderline: top-right (xmax, ymax) and bottom-left (xmin, ymin)
   * TODO: Find the inner borderline within  bounding box (xmin, ymin, xmax, ymax)
   * Iterate until there are no more coordinates left
   */
  bool all_visited = true;
  for (auto node_visited : visited_coords_) {
    if (!node_visited) {
      all_visited = false;
      break;
    }
  }
  while (!all_visited) {
    vtr::Rect<size_t> inner_bb = find_bb(verbose);
    VTR_LOGV(verbose, "Found inner borderline (xmin=%lu, ymin=%lu) -> (xmax=%lu, ymax=%lu)\n",
             inner_bb.xmin(), inner_bb.ymin(), inner_bb.xmax(), inner_bb.ymax());
    /* Assign existing border lines */
    int status = bb2borderline(outter_bb, inner_bb, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    all_visited = true;
    for (auto node_visited : visited_coords_) {
      if (!node_visited) {
        all_visited = false;
        break;
      }
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arkangel
