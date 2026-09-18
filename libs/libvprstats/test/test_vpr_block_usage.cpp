/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "vpr_block_usage_parser.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc > 1);
  int status = 0;

  arkangel::VprBlockUsageParser parser;

  /* Parse from an XML file */
  VTR_LOG("Load the VPR block usage from file: %s.\n", argv[1]);
  status = parser.parse(std::string(argv[1]), true);
  if (0 != status) {
    return status;
  }
  /* Validation */
  VTR_LOG("Num inputs: %ld\n", parser.num_inputs());
  VTR_LOG("Num outputs: %ld\n", parser.num_outputs());
  VTR_LOG("Num nets: %ld\n", parser.num_nets());
  VTR_LOG("Num blocks: %ld\n", parser.num_blocks());
  VTR_LOG("Detailed block usage:\n");
  for (auto type : parser.block_types()) {
    VTR_LOG("type: %s, Count: %ld\n", type.c_str(), parser.block_usage(type));
  }

  return status;
}
