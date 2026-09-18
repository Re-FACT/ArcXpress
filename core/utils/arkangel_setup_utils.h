#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "arch_dev_manager.h"
#include "arch_eval_manager.h"
#include "benchmark_manager.h"
#include "benchmark_packet.h"
#include "dv_manager.h"
#include "netlist_dev_manager.h"
#include "qor_manager.h"
#include "tapeout_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arkangel */
namespace arkangel {

int setup_qor_manager_post_synth(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                                 const PdkManager& pdk_mgr, QorManager& qor_mgr,
                                 const TapeoutManager::e_qorso_task_type& qor_type,
                                 const std::string& req_qtask_name, const std::string& qor_corner,
                                 const std::string& clock_period, const bool& verbose);

int setup_qor_manager_dynamic_power(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                                    const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id,
                                    const DvManager& dv_mgr, QorManager& qor_mgr,
                                    const std::string& qor_corner, const bool& verbose);

int setup_arch_dev_manager(const TapeoutManager& to_mgr, const BenchmarkManager& bm_mgr,
                           ArchDevManager& ad_mgr, BenchmarkPacket& bm_pkt);

int setup_arch_eval_manager(const TapeoutManager& to_mgr, ArchEvalManager& ae_mgr,
                            const std::string& arch_run);
int setup_dv_manager_for_qor_analysis(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                                      const ArchEvalManager& ae_mgr, DvManager& dv_mgr,
                                      const bool& verbose);
int setup_qor_manager_sta(const TapeoutManager& to_mgr, const NetlistDevManager& nd_mgr,
                          const BenchmarkManager& bm_mgr, const BenchmarkSuiteId& st_id,
                          const BenchmarkId& bm_id, const DvManager& dv_mgr, QorManager& qor_mgr,
                          const std::string& qor_corner, const std::string& clock_period,
                          const bool& verbose);

} /* end namespace arkangel */
