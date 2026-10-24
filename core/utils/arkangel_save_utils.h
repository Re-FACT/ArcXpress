#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "benchmark_manager.h"
#include "pdk_manager.h"
#include "tapeout_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

int save_results_for_step_run(const TapeoutManager& to_mgr, const BenchmarkManager& bm_mgr,
                              const PdkManager& pdk_mgr,
                              const TapeoutManager::e_step_name& curr_step,
                              const std::string& curr_run, const int& verbosity);

int check_results_for_step_run(const TapeoutManager& to_mgr, const BenchmarkManager& bm_mgr,
                               const PdkManager& pdk_mgr,
                               const TapeoutManager::e_step_name& curr_step,
                               const std::string& curr_run, const int& verbosity);

} /* end namespace arcxpress */
