/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "read_xml_floorplan_strategy.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "write_xml_floorplan_strategy.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc > 2);
  int status = 0;

  arcxpress::FloorplanStrategy flrpln_strtg(1., 0.5);

  /* Last arg is to output file. All the others except the first are the input files */

  /* Parse from an XML file */
  for (size_t iarg = 1; iarg < argc - 1; ++iarg) {
    VTR_LOG("Load the floorplan strategy data to an XML file: %s.\n", argv[iarg]);
    status = arcxpress::read_xml_floorplan_strategy(argv[iarg], flrpln_strtg);
    if (0 != status) {
      return status;
    }
    /* Validation */
    status = flrpln_strtg.validate();
    if (!status) {
      return status;
    }
  }

  VTR_LOG("Write the floorplan strategy data to an XML file: %s.\n", argv[argc - 1]);
  status = arcxpress::write_xml_floorplan_strategy(argv[argc - 1], flrpln_strtg);

  return status;
}
