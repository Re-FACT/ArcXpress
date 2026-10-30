#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <map>
#include <string>
#include <tuple>

#include "sofa_constants.h"
#include "arch_gen_naming.h"
#include "physical_types.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace Sofa {  // Begin namespace Sofa

/* pb_type name, circuit model name */
typedef std::tuple<std::string, std::string> PhyPbInfo;

/* Map from physical pb type to operating pb types */
inline const std::map<std::string, std::string> OP2PHY_PB_TYPE_MAP = {
  {OPERATING_BLE3_FF_NAME, PHYSICAL_REGULAR_SCFF_NAME},
  {OPERATING_BLE4_FF_NAME, PHYSICAL_REGULAR_SCFF_NAME},
  {OPERATING_SREG_FF_NAME, PHYSICAL_REGULAR_SCFF_NAME}};

std::string gen_latch_pb_name(const std::string& prefix);

std::string gen_io_tile_name(const e_side& io_side);

std::string gen_pi_subtile_name(const e_side& io_side);

std::string gen_po_subtile_name(const e_side& io_side);

}  // End of namespace Sofa
