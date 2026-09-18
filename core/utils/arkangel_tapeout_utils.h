#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "benchmark_manager.h"
#include "benchmark_packet.h"
#include "tapeout_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arkangel */
namespace arkangel {

int get_tapeout_default_benchmark_packet(const TapeoutManager& to_mgr,
                                         const BenchmarkManager& bm_mgr, BenchmarkPacket& bm_pkt);

int get_tapeout_default_benchmark_packet_for_clock_sweep(const TapeoutManager& to_mgr,
                                                         const BenchmarkManager& bm_mgr,
                                                         BenchmarkPacket& bm_pkt);

int get_tapeout_default_benchmark_packet_for_reset_sweep(const TapeoutManager& to_mgr,
                                                         const BenchmarkManager& bm_mgr,
                                                         BenchmarkPacket& bm_pkt);

int autopick_tapeout_step_run(TapeoutManager& to_mgr, const TapeoutManager::e_step_name& step_name,
                              const std::string& user_defined_run, std::string& selected_run);

int try_pick_mutable_tapeout_step_run(const TapeoutManager& to_mgr,
                                      const TapeoutManager::e_step_name& step_name,
                                      const std::string& user_defined_run,
                                      std::string& selected_run);

int try_pick_existing_tapeout_step_run(const TapeoutManager& to_mgr,
                                       const TapeoutManager::e_step_name& step_name,
                                       const std::string& user_defined_run,
                                       std::string& selected_run);

int pick_existing_tapeout_step_dep_run(const TapeoutManager& to_mgr,
                                       const TapeoutManager::e_step_name& step_name,
                                       const std::string& user_defined_run,
                                       std::string& selected_run);

std::vector<std::string> find_corners_to_sweep(const TapeoutManager& to_mgr,
                                               const std::string& eval_corner);

} /* end namespace arkangel */
