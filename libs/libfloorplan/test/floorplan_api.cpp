#include <cmath>

// #include "exprtk.hpp"  // opensource tools use for parsing mathematical expressions
#include "floorplan_argparse.h"
#include "floorplan_flow_manager.h"
#include "read_floorplan_params.h"

void floorplan_init(const int argc, const char** argv, floorplan_args* options,
                    arcxpress::FloorplanParams& floorplan_params, arcxpress::TileQor& tile_qor) {
  /* Read in user options */
  *options = read_args(argc, argv);
  /* Set conditional defaults */
  set_conditional_defaults(*options);
  /* Verify args */
  if (!verify_args(*options)) {
    std::cerr << "Error: Invalid arguments." << std::endl;
    exit(EXIT_FAILURE);
  }
  int status = read_qor_file(options->tile_ref_file.value().c_str(),
                             options->tile_qor_csv.value().c_str(), tile_qor);
  init_floorplan_params_with_yaml_file(options->config_file.value().c_str(), &floorplan_params);
  if (status != 0) {
    std::cerr << "Error: Failed to define tile size." << std::endl;
    exit(EXIT_FAILURE);
  }
}

int main(int argc, const char** argv) {
  floorplan_args Options;
  arcxpress::FloorplanParams floorplan_params;
  arcxpress::TileQor tile_qor;
  // read floorplan_task.yaml & tile_reference.yaml files
  floorplan_init(argc, argv, &Options, floorplan_params, tile_qor);
  std::cout << "floorplan init finished! " << std::endl;

  arcxpress::FloorplanFlow floorplan_flow(floorplan_params, tile_qor);

  int status = floorplan_flow.run("", true);
  if (status == 1) {
    std::cout << "error here" << std::endl;
  }
  /* Signal success to scripts */
  std::cout << "total actual area is: " << floorplan_flow.cal_total_actual_area() << std::endl;
  std::cout << "total qor area is: " << floorplan_flow.cal_total_cell_area() << std::endl;
  return 0;
}
