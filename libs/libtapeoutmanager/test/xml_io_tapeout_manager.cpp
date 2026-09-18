/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "read_xml_tapeout_manager.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "write_xml_tapeout_manager.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc == 2 || argc == 3);
  int status = 0;

  arkangel::TapeoutManager to_mgr;

  /* Parse from an XML file */
  VTR_LOG("Load the tapeout data to an XML file: %s.\n", argv[1]);
  status = arkangel::read_xml_tapeout_manager(argv[1], to_mgr);
  if (0 != status) {
    return status;
  }
  /* Validation */
  status = to_mgr.validate();
  if (!status) {
    return status;
  }
  to_mgr.add_disabled_corner("tt_25C_v0p9");
  VTR_LOG("Write the tapeout data to an XML file: %s.\n", argv[2]);
  status = arkangel::write_xml_tapeout_manager(argv[2], to_mgr, false);

  return status;
}
