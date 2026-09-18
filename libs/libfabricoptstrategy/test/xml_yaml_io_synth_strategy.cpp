/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "read_xml_synth_strategy.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc == 3);
  int status = 0;

  arkangel::FabricSynthStrategy syn_strtg("/home", "s40", "tt_v1p0");
  syn_strtg.set_netlist_input("../netlists/rtl/");
  syn_strtg.set_netlist_output("../netlists/gl/subblock");
  syn_strtg.add_netlist_exclude("fabric_netlists.v");
  syn_strtg.add_netlist_exclude("efpga_circuits.v");

  /* Parse from an XML file */
  VTR_LOG("Load the synthesis strategy data from an XML file: %s.\n", argv[1]);
  status = arkangel::read_xml_synth_strategy(argv[1], syn_strtg, true);
  if (0 != status) {
    return status;
  }
  /* Validation */
  if (!syn_strtg.validate_strategies()) {
    return status;
  }

  syn_strtg.add_design_for_synth("cbx_0_", true);
  syn_strtg.add_design_for_synth("cbx_1_", true);
  syn_strtg.add_design_for_synth("cbx_2_", true);
  syn_strtg.add_design_for_synth("cby_0_", true);
  syn_strtg.add_design_for_synth("cby_1_", true);
  syn_strtg.add_design_for_synth("cby_2_", true);
  syn_strtg.add_design_for_synth("sb_0_", true);
  syn_strtg.add_design_for_synth("sb_1_", true);
  syn_strtg.add_design_for_synth("grid_clb", true);
  syn_strtg.add_design_for_synth("grid_io_topL_top", true);
  syn_strtg.add_design_for_synth("grid_io_rightL_right", true);
  syn_strtg.add_design_for_synth("grid_io_leftL_left", true);
  syn_strtg.add_design_for_synth("grid_io_bottomL_bottom", true);
  syn_strtg.add_design_for_synth("grid_io_leftL_ecb56_left", true);

  /* Validation */
  if (!syn_strtg.validate_general_settings()) {
    return status;
  }

  VTR_LOG("Write the synthesis task to an YAML file: %s.\n", argv[2]);
  status = syn_strtg.write_yaml_synth_task(std::string(argv[2]), true);

  return status;
}
