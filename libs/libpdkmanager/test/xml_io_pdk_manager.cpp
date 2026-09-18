/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "read_xml_pdk_manager.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "write_xml_pdk_manager.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc == 2 || argc == 3);
  int status = 0;

  arkangel::PdkManager pdk_mgr;

  /* Parse from an XML file */
  VTR_LOG("Load the PDK data to an XML file: %s.\n", argv[1]);
  status = arkangel::read_xml_pdk_manager(argv[1], pdk_mgr);
  if (0 != status) {
    return status;
  }
  /* Validation */
  status = pdk_mgr.validate();
  if (!status) {
    return status;
  }

  VTR_LOG("Write the PDK data to an XML file: %s.\n", argv[2]);
  status = arkangel::write_xml_pdk_manager(argv[2], pdk_mgr);

  return status;
}
