#include "check_floorplan.h"

#include <cmath>

#include "command_exit_codes.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

/* All the CPP of standard cell libraries selected in synthesis strategy should be the same */
int check_synth_sclib_cpp(const PdkManager& pdk_mgr, const FabricSynthStrategy& synth_strtg,
                          float& ref_cpp, const bool& verbose) {
  /* Collect all the standard cell lib names from the synth strategy
   * Check if CPP of the libs match */
  ref_cpp = 0.;
  std::string ref_cpp_sclib_name;
  for (std::string curr_sclib_name : synth_strtg.target_libs()) {
    float curr_cpp = pdk_mgr.find_sclib_cpp_by_name(curr_sclib_name);
    if (ref_cpp == 0.) {
      ref_cpp = curr_cpp;
      ref_cpp_sclib_name = curr_sclib_name;
    } else if (curr_cpp != ref_cpp) {
      /* CPP mismatches found, there is problem ! */
      VTR_LOG_ERROR(
        "The standard cell lib '%s' used in fabric synthesis has a different Contant Poly Pitch "
        "(CPP) '%f' than the CPP '%f' of another standard cell lib '%s'! Expect all the CPP should "
        "be the same!\n",
        curr_sclib_name.c_str(), curr_cpp, ref_cpp_sclib_name.c_str(), ref_cpp);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Reach here, check passed */
  VTR_LOGV(verbose, "Standard cell lib CPPs are all consistency.\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

/* All the heights of standard cell libraries selected in synthesis strategy should be the same */
int check_synth_sclib_height(const PdkManager& pdk_mgr, const FabricSynthStrategy& synth_strtg,
                             float& ref_sc_h, const bool& verbose) {
  /* Collect all the standard cell lib names from the synth strategy
   * Check if CPP of the libs match */
  ref_sc_h = 0.;
  std::string ref_sc_h_sclib_name;
  for (std::string curr_sclib_name : synth_strtg.target_libs()) {
    float curr_sc_h = pdk_mgr.find_sclib_height_by_name(curr_sclib_name);
    if (ref_sc_h == 0.) {
      ref_sc_h = curr_sc_h;
      ref_sc_h_sclib_name = curr_sclib_name;
    } else if (curr_sc_h != ref_sc_h) {
      /* Standar cell height mismatches found, there is problem ! */
      VTR_LOG_ERROR(
        "The standard cell lib '%s' used in fabric synthesis has a different height '%f' than the "
        "height '%f' of another standard cell lib '%s'! Expect all the CPP should be the same!\n",
        curr_sclib_name.c_str(), curr_sc_h, ref_sc_h_sclib_name.c_str(), ref_sc_h);
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Reach here, check passed */
  VTR_LOGV(verbose, "Standard cell lib heights are all consistency.\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Find feature sizes: CPP and Standard cell heights from PDK config based on selection in synthesis
 * strategy */
int init_floorplan_from_synth_sclib(const PdkManager& pdk_mgr,
                                    const FabricSynthStrategy& synth_strtg,
                                    FloorplanStrategy& floorplan_strtg, const bool& verbose) {
  float ref_cpp = 0.;
  int status = check_synth_sclib_cpp(pdk_mgr, synth_strtg, ref_cpp, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Standard cell lib CPP check failed!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  float ref_sc_h = 0.;
  status = check_synth_sclib_height(pdk_mgr, synth_strtg, ref_sc_h, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Standard cell lib height check failed!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Reach here, CPP and sc height are o.k. Initialize */
  floorplan_strtg.set_cpp(ref_cpp);
  floorplan_strtg.set_standard_cell_height(ref_sc_h);
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arcxpress
