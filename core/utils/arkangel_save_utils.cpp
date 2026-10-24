/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
#include "arkangel_save_utils.h"

#include "arch_dev_manager.h"
#include "arch_eval_manager.h"
#include "command_exit_codes.h"
#include "dv_manager.h"
#include "netlist_dev_manager.h"
#include "vtr_log.h"

/* begin namespace arcxpress */
namespace arcxpress {

int save_results_for_step_run(const TapeoutManager& to_mgr, const BenchmarkManager& bm_mgr,
                              const PdkManager& pdk_mgr,
                              const TapeoutManager::e_step_name& curr_step,
                              const std::string& curr_run, const int& verbosity) {
  /* Sanity checks */
  to_mgr.check_valid_run(curr_step, curr_run);
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  switch (curr_step) {
    case TapeoutManager::e_step_name::ARCH: {
      ArchDevManager ad_mgr(to_mgr, curr_run);
      /* TODO: Create a dedicated function */
      for (std::string corner : to_mgr.corner_tags()) {
        std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
        ad_mgr.add_arch_timing_file(corner, arch_timing_file);
      }
      if (!ad_mgr.set_default_corner(to_mgr.default_corner())) {
        VTR_LOG_ERROR("Invalid default corner '%s' which is not a valid one!\n",
                      to_mgr.default_corner().c_str());
        to_mgr.show_corners();
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      /* For each substeps, use save results to proper files */
      std::vector<TapeoutManager::e_substep_name> s_step2save;
      for (auto s_step : to_mgr.substeps(curr_step, curr_run)) {
        if (TapeoutManager::e_run_status::SUCCESS ==
            to_mgr.substep_status(curr_step, curr_run, s_step)) {
          s_step2save.push_back(s_step);
        }
      }
      status = ad_mgr.save_results(s_step2save, verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::BITSTREAM: {
      ArchEvalManager ae_mgr(to_mgr, curr_run, bm_mgr);
      /* TODO: Create a dedicated function */
      for (std::string corner : to_mgr.corner_tags()) {
        std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
        ae_mgr.add_arch_timing_file(corner, arch_timing_file);
      }
      status = ae_mgr.save_bitgen_results(verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::NETLIST: {
      NetlistDevManager nd_mgr(to_mgr, curr_run, pdk_mgr);
      nd_mgr.set_default_corner(to_mgr.default_corner());
      std::vector<TapeoutManager::e_substep_name> s_step2save;
      for (auto s_step : to_mgr.substeps(curr_step, curr_run)) {
        if (TapeoutManager::e_run_status::SUCCESS ==
            to_mgr.substep_status(curr_step, curr_run, s_step)) {
          s_step2save.push_back(s_step);
        }
      }
      status = nd_mgr.save_results(s_step2save, verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::DV: {
      ArchEvalManager ae_mgr(to_mgr, curr_run, bm_mgr);
      /* TODO: Create a dedicated function */
      for (std::string corner : to_mgr.corner_tags()) {
        std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
        ae_mgr.add_arch_timing_file(corner, arch_timing_file);
      }
      NetlistDevManager nd_mgr(to_mgr, curr_run, pdk_mgr);
      DvManager dv_mgr(to_mgr, curr_run, ae_mgr, nd_mgr);
      status = dv_mgr.save_results(verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::PD: {
      break;
    }
    case TapeoutManager::e_step_name::SO: {
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid tapeout step!\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return status;
}

int check_results_for_step_run(const TapeoutManager& to_mgr, const BenchmarkManager& bm_mgr,
                               const PdkManager& pdk_mgr,
                               const TapeoutManager::e_step_name& curr_step,
                               const std::string& curr_run, const int& verbosity) {
  /* Sanity checks */
  to_mgr.check_valid_run(curr_step, curr_run);
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  switch (curr_step) {
    case TapeoutManager::e_step_name::ARCH: {
      ArchDevManager ad_mgr(to_mgr, curr_run);
      /* TODO: Create a dedicated function */
      for (std::string corner : to_mgr.corner_tags()) {
        std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
        ad_mgr.add_arch_timing_file(corner, arch_timing_file);
      }
      if (!ad_mgr.set_default_corner(to_mgr.default_corner())) {
        VTR_LOG_ERROR("Invalid default corner '%s' which is not a valid one!\n",
                      to_mgr.default_corner().c_str());
        to_mgr.show_corners();
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      status = ad_mgr.check_results(verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::BITSTREAM: {
      ArchEvalManager ae_mgr(to_mgr, curr_run, bm_mgr);
      /* TODO: Create a dedicated function */
      for (std::string corner : to_mgr.corner_tags()) {
        std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
        ae_mgr.add_arch_timing_file(corner, arch_timing_file);
      }
      status = ae_mgr.check_bitgen_results(verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::NETLIST: {
      NetlistDevManager nd_mgr(to_mgr, curr_run, pdk_mgr);
      nd_mgr.set_default_corner(to_mgr.default_corner());
      status = nd_mgr.check_results(verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::DV: {
      ArchEvalManager ae_mgr(to_mgr, curr_run, bm_mgr);
      /* TODO: Create a dedicated function */
      for (std::string corner : to_mgr.corner_tags()) {
        std::string arch_timing_file = to_mgr.arch_timing_file(corner, false);
        ae_mgr.add_arch_timing_file(corner, arch_timing_file);
      }
      NetlistDevManager nd_mgr(to_mgr, curr_run, pdk_mgr);
      DvManager dv_mgr(to_mgr, curr_run, ae_mgr, nd_mgr);
      status = dv_mgr.check_results(verbosity == 1);
      break;
    }
    case TapeoutManager::e_step_name::PD: {
      break;
    }
    case TapeoutManager::e_step_name::SO: {
      break;
    }
    default:
      VTR_LOG_ERROR("Invalid tapeout step!\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return status;
}

}  // namespace arcxpress
