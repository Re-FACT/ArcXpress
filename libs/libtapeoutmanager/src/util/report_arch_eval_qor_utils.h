#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arch_eval_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int report_pre_arch_qor_to_csv_file(const std::string& fname, const ArchEvalManager& ae_mgr);

int report_arch_eval_qor_to_csv_file(const std::string& fname, const ArchEvalManager& ae_mgr,
                                     const std::vector<std::string>& corners, const bool& verbose);

}  // End of namespace arcxpress
