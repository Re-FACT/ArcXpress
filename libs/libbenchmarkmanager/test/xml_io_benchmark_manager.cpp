/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "read_xml_benchmark_manager.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "write_xml_benchmark_manager.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc > 2);
  int status = 0;

  arkangel::BenchmarkManager bm_mgr;

  /* Last arg is to output file. All the others except the first are the input files */

  /* Parse from an XML file */
  for (size_t iarg = 1; iarg < argc - 1; ++iarg) {
    VTR_LOG("Load the benchmark data to an XML file: %s.\n", argv[iarg]);
    status = arkangel::read_xml_benchmark_manager(argv[iarg], bm_mgr, false);
    if (0 != status) {
      return status;
    }
    /* Validation */
    status = bm_mgr.validate();
    if (!status) {
      return status;
    }
  }

  VTR_LOG("Write the benchmark data to an XML file: %s.\n", argv[argc - 1]);
  status = arkangel::write_xml_benchmark_manager(argv[argc - 1], bm_mgr);

  return status;
}
