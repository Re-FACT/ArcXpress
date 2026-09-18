/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "benchmark_packet_utils.h"
#include "command_exit_codes.h"

/* begin namespace arkangel */
namespace arkangel {

/********************************************************************
 * Create a packet of data for a benchmark from the database
 *******************************************************************/
BenchmarkPacket create_benchmark_packet_by_name(const BenchmarkManager& bm_mgr,
                                                const std::string& bm_name) {
  BenchmarkPacket bm_pkt;
  BenchmarkId bm_id = bm_mgr.find_benchmark_by_name(bm_name);
  if (!bm_mgr.valid_benchmark_id(bm_id)) {
    VTR_LOG_ERROR("Invalid benchmark '%s' which does not exist in any suite!\n", bm_name.c_str());
    return bm_pkt;
  }
  /* Get all the design sources to the packet */
  for (auto bm_f_id : bm_mgr.design_sources(bm_id)) {
    bm_pkt.add_file(bm_mgr.design_source_type(bm_id, bm_f_id),
                    bm_mgr.design_source_file(bm_id, bm_f_id));
  }
  for (auto bm_f_id : bm_mgr.constraints(bm_id)) {
    bm_pkt.add_file(bm_mgr.constraint_type(bm_id, bm_f_id), bm_mgr.constraint_file(bm_id, bm_f_id));
  }
  return bm_pkt;
}

}  // namespace arkangel
