/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "vpr_log_parser.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc > 2);
  int status = 0;

  arcxpress::VprLogParser parser;

  /* Parse from an XML file */
  VTR_LOG("Load the keyword from an XML file: %s.\n", argv[1]);
  status = parser.load_keywords_from_xml(std::string(argv[1]), true);
  if (0 != status) {
    return status;
  }
  VTR_LOG("Parse VTR log from file: %s.\n", argv[2]);
  status = parser.parse(std::string(argv[2]), true);
  if (0 != status) {
    return status;
  }
  /* Validation */
  VTR_LOG("Detailed extracted data:\n");
  for (auto kw : parser.keywords()) {
    VTR_LOG("%s = %e\n", kw.c_str(), parser.value(kw));
  }

  return status;
}
