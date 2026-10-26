/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML of project file to the associated
 * data structures
 *******************************************************************/
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_time.h"

/* Headers from libarchfpga */
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "report_arch_eval_qor_utils.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

int report_pre_arch_qor_to_csv_file(const std::string& fname, const ArchEvalManager& ae_mgr) {
  return ae_mgr.report_pre_arch_qor_to_csv_file(fname);
}

int report_arch_eval_qor_to_csv_file(const std::string& fname, const ArchEvalManager& ae_mgr,
                                     const std::vector<std::string>& corners, const bool& verbose) {
  return ae_mgr.gen_report_bitgen_qor(fname, corners, verbose);
}

}  // End of namespace arcxpress
