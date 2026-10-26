/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "read_xml_boot_config.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "write_xml_boot_config.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc > 2);
  int status = 0;

  arcxpress::BootConfig boot_cfg;

  /* Last arg is to output file. All the others except the first are the input files */

  /* Parse from an XML file */
  for (size_t iarg = 1; iarg < argc - 1; ++iarg) {
    VTR_LOG("Load the boot configuration data to an XML file: %s.\n", argv[iarg]);
    status = arcxpress::read_xml_boot_config(argv[iarg], boot_cfg);
    if (0 != status) {
      return status;
    }
    /* Validation */
    status = boot_cfg.validate();
    if (!status) {
      return status;
    }
  }

  VTR_LOG("Write the boot configuration data to an XML file: %s.\n", argv[argc - 1]);
  status = arcxpress::write_xml_boot_config(argv[argc - 1], boot_cfg);

  return status;
}
