/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "generate_pin_table_naming_rules.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc == 3);
  int status = 0;

  return arkangel::generate_pin_table_naming_rules(std::string(argv[1]), std::string(argv[2]),
                                                   true);
}
