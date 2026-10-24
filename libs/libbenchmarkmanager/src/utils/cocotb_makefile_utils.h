#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "benchmark_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

std::vector<std::string> extract_benchmark_cocotb_makefile_cell_sim_files(
  const BenchmarkManager& bm_mgr, const BenchmarkId& bm_id, const bool& verbose);

} /* end namespace arcxpress */
