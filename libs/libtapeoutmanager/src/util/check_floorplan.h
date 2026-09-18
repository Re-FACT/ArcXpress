#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "fabric_synth_strategy.h"
#include "floorplan_strategy.h"
#include "pdk_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arkangel {  // Begin namespace arkangel

int check_synth_sclib_cpp(const PdkManager& pdk_mgr, const FabricSynthStrategy& synth_strtg,
                          float& ref_cpp, const bool& verbose);

int check_synth_sclib_height(const PdkManager& pdk_mgr, const FabricSynthStrategy& synth_strtg,
                             float& ref_sc_h, const bool& verbose);

int init_floorplan_from_synth_sclib(const PdkManager& pdk_mgr,
                                    const FabricSynthStrategy& synth_strtg,
                                    FloorplanStrategy& floorplan_strtg, const bool& verbose);

}  // End of namespace arkangel
