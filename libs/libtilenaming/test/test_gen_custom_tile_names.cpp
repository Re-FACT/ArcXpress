/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "generate_tile_module_names.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc == 5);
  int status = 0;

  return arcxpress::generate_tile_module_names(std::string(argv[1]), std::string(argv[2]),
                                               std::string(argv[3]), std::string(argv[4]), true);
}
