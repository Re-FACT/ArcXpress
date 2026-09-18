/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "fabric_synth_strategy.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc == 2);
  int status = 0;

  arkangel::FabricSynthStrategy syn_strtg(".", "s40", "tt_v1p0");
  syn_strtg.set_netlist_input("../netlists/rtl/");
  syn_strtg.set_netlist_output("../netlists/gl/subblock");
  syn_strtg.add_netlist_exclude("fabric_netlists.v");
  syn_strtg.add_netlist_exclude("efpga_circuits.v");

  syn_strtg.add_design_for_analyze("tile_0_");
  syn_strtg.add_design_for_analyze("tile_1_");
  syn_strtg.add_design_for_analyze("tile_2_");
  syn_strtg.add_design_for_analyze("tile_3_");
  /* Validation */
  if (!syn_strtg.validate_general_settings()) {
    return status;
  }

  VTR_LOG("Write the synthesis task to an YAML file: %s.\n", argv[1]);
  status = syn_strtg.write_yaml_analyze_task(std::string(argv[1]), true);

  return status;
}
