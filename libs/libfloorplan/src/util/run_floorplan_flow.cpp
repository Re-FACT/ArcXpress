#include "run_floorplan_flow.h"

#include <cmath>

#include "command_exit_codes.h"
#include "floorplan_flow_manager.h"
#include "floorplan_params.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

/* Load the data from a floorplan strategy to the parameters that floorplanner can accept
 * TODO: Floorplanner should accept strategy file directly
 */
static int load_strategy_to_floorplan_parameters(const FloorplanStrategy& flrpln_strtg,
                                                 FloorplanParams* flrpln_params) {
  flrpln_params->sc_height = flrpln_strtg.standard_cell_height();
  flrpln_params->cpp = flrpln_strtg.cpp();
  flrpln_params->boundary_cell_width = flrpln_strtg.boundary_cell_width();
  for (std::string tile : flrpln_strtg.tiles()) {
    flrpln_params->utilization_map[tile] = flrpln_strtg.tile_utilization(tile);
  }
  flrpln_params->tile_params.tile_width_height_ratio = flrpln_strtg.height_width_ratio();
  /* Converting from float to int may lead to a loss of precision. For example, a value such as
   * 12.999—which is semantically intended to be 13—would be truncated to 12 during implicit
   * conversion, potentially causing downstream errors (e.g., incorrect odd/even height
   * handling). Therefore, the value must be rounded explicitly before performing a static
   * cast to int.
   */
  flrpln_params->tile_params.tile_gap.push_back(
    static_cast<int>(std::round(flrpln_strtg.tile_horizental_gap() / flrpln_strtg.cpp())));

  flrpln_params->tile_params.tile_gap.push_back(static_cast<int>(
    std::round(flrpln_strtg.tile_vertical_gap() / flrpln_strtg.standard_cell_height())));
  flrpln_params->default_utilization = flrpln_strtg.tile_default_utilization();
  flrpln_params->tile_params.top_side_gap = flrpln_strtg.side_gap(e_side::TOP);
  flrpln_params->tile_params.bottom_side_gap = flrpln_strtg.side_gap(e_side::BOTTOM);
  flrpln_params->tile_params.left_side_gap = flrpln_strtg.side_gap(e_side::LEFT);
  flrpln_params->tile_params.right_side_gap = flrpln_strtg.side_gap(e_side::RIGHT);
  flrpln_params->tile_params.horizontal_tile_gap = flrpln_strtg.tile_horizental_gap();
  flrpln_params->tile_params.vertical_tile_gap = flrpln_strtg.tile_vertical_gap();
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Main function to run a floorplanning
 * Calculate the width and height for each unique tile w.r.t. the constraints in strategy
 */
int run_floorplan_flow(const FloorplanStrategy& flrpln_strtg, TileQor& tile_qor,
                       const std::string& dom_tile_name, const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Initialization */
  FloorplanParams floorplan_params;
  status = load_strategy_to_floorplan_parameters(flrpln_strtg, &floorplan_params);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Floorplanning failed with errors!\n");
  }

  VTR_LOGV(verbose, "Floorplanning...\n");
  FloorplanFlow floorplan_flow(floorplan_params, tile_qor);
  status = floorplan_flow.run(dom_tile_name, verbose);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Floorplanning failed with errors!\n");
  }
  VTR_LOGV(verbose, "Done\n");

  return status;
}

}  // End of namespace arcxpress
