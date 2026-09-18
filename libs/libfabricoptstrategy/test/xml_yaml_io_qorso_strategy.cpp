/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "read_xml_qorso_strategy.h"
#include "read_yaml_tile_ref.h"
#include "read_yaml_tile_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc == 5);
  int status = 0;

  std::array<std::string, 4> corner_info = {"tt_v1p0_25c", "tt_v1p0", "tt", "tt"};
  arkangel::QorsoStrategy qs_strtg("default_name", "/home", "s40", corner_info);
  qs_strtg.set_netlist_input("../netlists/gl/");
  qs_strtg.add_netlist_exclude("fabric_netlists.v");
  qs_strtg.add_netlist_exclude("efpga_circuits.v");
  qs_strtg.set_netlist_type("gl");
  qs_strtg.set_task_report_dirpath("../report/gl/ultimate/timing");
  qs_strtg.set_summary_report_dirpath("../../report/gl/ultimate/timing/");

  /* Parse from an XML file */
  VTR_LOG("Load the Qorso strategy data from an XML file: %s.\n", argv[1]);
  status = arkangel::read_xml_qorso_strategy(argv[1], qs_strtg, false, true);
  if (0 != status) {
    return status;
  }
  arkangel::TileRef tref;
  status = arkangel::read_yaml_tile_ref(argv[2], tref, true);
  if (status != 0) {
    return status;
  }
  status = arkangel::read_yaml_tile_types(argv[3], tref, true);
  if (status != 0) {
    return status;
  }
  /* Validation */
  if (!qs_strtg.validate(tref, true)) {
    return status;
  }
  VTR_LOG("Write the Qorso task to an YAML file: %s.\n", argv[4]);
  status = qs_strtg.write_yaml_qorso_timing_task(std::string(argv[4]), true);

  return status;
}
