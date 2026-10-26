#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "benchmark_manager.h"
#include "benchmark_packet.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

BenchmarkPacket create_benchmark_packet_by_name(const BenchmarkManager& bm_mgr,
                                                const std::string& bm_name);

} /* end namespace arcxpress */
