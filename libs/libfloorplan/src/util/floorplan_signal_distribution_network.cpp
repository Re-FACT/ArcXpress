#include "floorplan_signal_distribution_network.h"

#include <cmath>

#include "command_exit_codes.h"
#include "vtr_geometry.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

/* Depending on the center loc, split the bounding box into two halves
 * - Horizontally: upper half and lower half
 * - Vertically: left half and right half
 */
static int split_efpga_logical_bounding_box(std::array<vtr::Rect<size_t>, 2>& lgk_bbs,
                                            const vtr::Rect<size_t>& bb,
                                            const vtr::Point<size_t>& entry_point,
                                            const vtr::Point<size_t>& center_loc,
                                            const bool& verbose) {
  /* Find the direction to cut depending on the relationship between the entry point and the center
   * location */
  if (entry_point.x() == center_loc.x()) {
    /* Enter in a vertically direction, cut vertically */
    /* Left half */
    lgk_bbs[0] =
      vtr::Rect<size_t>(bb.bottom_left(), vtr::Point<size_t>(entry_point.x(), bb.ymax()));
    /* Right half */
    vtr::Point<size_t> rt_half_bottom_left(entry_point.x(), bb.ymin());
    lgk_bbs[1] = vtr::Rect<size_t>(rt_half_bottom_left, bb.top_right());
    VTR_LOGV(
      verbose,
      "Split vertically eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)' into two partitions "
      "given the entry point (x=%lu, y=%lu) and the center location (x=%lu, y=%lu):\n",
      bb.bottom_left().x(), bb.bottom_left().y(), bb.top_right().x(), bb.top_right().y(),
      entry_point.x(), entry_point.y(), center_loc.x(), center_loc.y());
    VTR_LOGV(verbose, "\tLeft half(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)'\n",
             lgk_bbs[0].bottom_left().x(), lgk_bbs[0].bottom_left().y(), lgk_bbs[0].top_right().x(),
             lgk_bbs[0].top_right().y());
    VTR_LOGV(verbose, "\tRight half(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)'\n",
             lgk_bbs[1].bottom_left().x(), lgk_bbs[1].bottom_left().y(), lgk_bbs[1].top_right().x(),
             lgk_bbs[1].top_right().y());
  } else if (entry_point.y() == center_loc.y()) {
    /* Enter in a horizontal direction, cut horizontally */
    /* upper half */
    lgk_bbs[0] = vtr::Rect<size_t>(vtr::Point<size_t>(bb.xmin(), entry_point.y()), bb.top_right());
    /* bottom half */
    vtr::Point<size_t> bt_half_top_right(bb.xmax(), entry_point.y());
    lgk_bbs[1] = vtr::Rect<size_t>(bb.bottom_left(), bt_half_top_right);
    VTR_LOGV(
      verbose,
      "Split horizontally eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)' into two partitions "
      "given the entry point (x=%lu, y=%lu) and the center location (x=%lu, y=%lu):\n",
      bb.bottom_left().x(), bb.bottom_left().y(), bb.top_right().x(), bb.top_right().y(),
      entry_point.x(), entry_point.y(), center_loc.x(), center_loc.y());
    VTR_LOGV(verbose, "\tUpper half(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)'\n",
             lgk_bbs[0].bottom_left().x(), lgk_bbs[0].bottom_left().y(), lgk_bbs[0].top_right().x(),
             lgk_bbs[0].top_right().y());
    VTR_LOGV(verbose, "\tLower half(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)'\n",
             lgk_bbs[1].bottom_left().x(), lgk_bbs[1].bottom_left().y(), lgk_bbs[1].top_right().x(),
             lgk_bbs[1].top_right().y());
  } else {
    VTR_LOG_ERROR(
      "Cannot split eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)' into two partitions as the "
      "entry point (x=%lu, y=%lu) and the center location (x=%lu, y=%lu) does not share x or y!\n",
      bb.bottom_left().x(), bb.bottom_left().y(), bb.top_right().x(), bb.top_right().y(),
      entry_point.x(), entry_point.y(), center_loc.x(), center_loc.y());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int rec_build_htree_network_for_signal(const FloorplanStrategy::SignalDistributionNetwork& ntwk,
                                       TileQor& tile_qor, const vtr::Rect<size_t>& bounding_box,
                                       const vtr::Point<size_t>& entry_point,
                                       const size_t& curr_depth, const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  if ((bounding_box.height() + 1) * (bounding_box.width() + 1) < 4 ||
      ntwk.reach_depth(curr_depth)) {
    /* Compute the actual bounding box where the tiles on left and bottom side should be excluded
     * for the connections When splitting the region by the center location, the left half and right
     * half has an overlapped column, while the top half and bottom half has an overlapped row. The
     * rule is that the overlapped column is considered to be connected in the left half and bottom
     * half, and should be excluded from the right and top half when building connections
     */
    vtr::Rect<size_t> actual_bb = bounding_box;
    if (actual_bb.xmin() != tile_qor.efpga_logical_bounding_box().xmin()) {
      actual_bb.set_xmin(bounding_box.xmin() + 1);
    }
    if (actual_bb.ymin() != tile_qor.efpga_logical_bounding_box().ymin()) {
      actual_bb.set_ymin(bounding_box.ymin() + 1);
    }
    /* If the actual bb is no longer valid, no need to build any connection */
    if (actual_bb.height() < 0 || actual_bb.width() < 0) {
      VTR_LOGV(verbose, "Skip building H-tree connections as the actual region is not valid\n");
      return openfpga::CMD_EXEC_SUCCESS;
    }
    /* build detailed network inside the bounding box in a fishbone style, each crosspoint on the
     * fishbone requires a buffer */
    status = tile_qor.add_fishbone_connection2region(ntwk.port, entry_point, bounding_box,
                                                     actual_bb, ntwk.buf_cell, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR(
        "Building H-tree connections for eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)' "
        "failed with errors!\n",
        bounding_box.bottom_left().x(), bounding_box.bottom_left().y(),
        bounding_box.top_right().x(), bounding_box.top_right().y());
    }
    return status;
  }
  /* Go recursively */
  vtr::Point<size_t> center_loc(0, 0);
  status = tile_qor.find_center_location(bounding_box, entry_point, center_loc);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::array<vtr::Rect<size_t>, 2> child_bbs;
  status =
    split_efpga_logical_bounding_box(child_bbs, bounding_box, entry_point, center_loc, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Insert buffer and build fanouts */
  status = tile_qor.add_buf2loc(ntwk.port, center_loc, ntwk.buf_cell, curr_depth + 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR(
      "Floorplanning H-tree for eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)' failed with "
      "errors!\n",
      bounding_box.bottom_left().x(), bounding_box.bottom_left().y(), bounding_box.top_right().x(),
      bounding_box.top_right().y());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = tile_qor.connect_signal_distribution_network_bufs(ntwk.port, entry_point, curr_depth,
                                                             center_loc, curr_depth + 1, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR(
      "Floorplanning H-tree for eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)' failed with "
      "errors!\n",
      bounding_box.bottom_left().x(), bounding_box.bottom_left().y(), bounding_box.top_right().x(),
      bounding_box.top_right().y());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Recursively build the downstream network */
  for (auto child_bb : child_bbs) {
    status = rec_build_htree_network_for_signal(ntwk, tile_qor, child_bb, center_loc,
                                                curr_depth + 1, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR(
        "Floorplanning H-tree for eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, y1=%lu)' failed with "
        "errors!\n",
        child_bb.bottom_left().x(), child_bb.bottom_left().y(), child_bb.top_right().x(),
        child_bb.top_right().y());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  return status;
}

/* Run a detailed floorplanning on a given global signal to build a H-tree distribution network
 * The H-tree will be built in a recursively way.
 * - With a specified starting point, enter the center location the region and add a buffer
 * - If the current region is less than 8 (height x width), build a detailed tree. Otherwise, go
 * recursively
 * - Bipartition the current region based on the center location and use the center location as the
 * next starting point for each
 */
static int floorplan_htree_network_for_signal(
  const FloorplanStrategy::SignalDistributionNetwork& ntwk, TileQor& tile_qor,
  const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  /* Get the starting point and compute the direction to go into the center location */
  vtr::Rect<size_t> lgk_bb = tile_qor.efpga_logical_bounding_box();
  VTR_LOGV(verbose,
           "Consider the complete eFPGA fabric as a bounding box of (x0=%lu, y0=%lu) -> (x1=%lu, "
           "y1=%lu)\n",
           lgk_bb.bottom_left().x(), lgk_bb.bottom_left().y(), lgk_bb.top_right().x(),
           lgk_bb.top_right().y());

  /* Insert buffer and build fanouts */
  size_t curr_depth = 0;
  status = tile_qor.add_buf2loc(ntwk.port, ntwk.entry_point, ntwk.buf_cell, curr_depth);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR(
      "Cannot add buffer to build tree inside eFPGA region '(x0=%lu, y0=%lu) -> (x1=%lu, "
      "y1=%lu)'!\n",
      lgk_bb.bottom_left().x(), lgk_bb.bottom_left().y(), lgk_bb.top_right().x(),
      lgk_bb.top_right().y());
    return status;
  }

  status = rec_build_htree_network_for_signal(ntwk, tile_qor, lgk_bb, ntwk.entry_point, curr_depth,
                                              verbose);

  return status;
}

/* Run a detailed floorplanning on a given global signal to build a H-tree distribution network */
static int floorplan_fishbone_network_for_signal(
  const FloorplanStrategy::SignalDistributionNetwork& ntwk, TileQor& tile_qor,
  const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  VTR_LOG_ERROR("Fishbone network is not yet supported\n");

  return status;
}

/* Main function to run a detailed floorplanning on global signal distribution network */
int floorplan_signal_distribution_network(const FloorplanStrategy& flrpln_strtg, TileQor& tile_qor,
                                          const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Sanity checks */
  if (!flrpln_strtg.valid_signal_distribution_network()) {
    VTR_LOG_ERROR("Floorplanning cannot be started due to inputs are not valid!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbose, "Floorplanning detailed signal distribution network...\n");
  for (const auto& ntwk : flrpln_strtg.signal_distribution_networks()) {
    status = openfpga::CMD_EXEC_FATAL_ERROR;
    if (ntwk.type == FloorplanStrategy::e_distribution_network_type::HTREE) {
      VTR_LOGV(verbose, "Floorplanning detailed H-tree for port '%s'...\n",
               ntwk.port.to_verilog_string().c_str());
      status = floorplan_htree_network_for_signal(ntwk, tile_qor, verbose);
    } else if (ntwk.type == FloorplanStrategy::e_distribution_network_type::FISHBONE) {
      status = floorplan_fishbone_network_for_signal(ntwk, tile_qor, verbose);
    } else {
      VTR_LOG_ERROR("Network type for port '%s' is not valid. Expect %s!\n",
                    ntwk.port.to_verilog_string().c_str(),
                    flrpln_strtg.distribution_network_type_all2str().c_str());
    }
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      VTR_LOG_ERROR("Floorplanning detailed signal distribution network failed with errors!\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  VTR_LOGV(verbose, "Done\n");

  VTR_LOGV(verbose, "Build physical details for signal distribution network...\n");
  status = tile_qor.build_signal_distribution_network_physical_details(verbose);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Build physical details for signal distribution network failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");

  return status;
}

}  // End of namespace arcxpress
