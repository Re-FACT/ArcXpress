#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <map>
#include <string>
#include <tuple>

#include "alkaid_constants.h"
#include "arch_gen_naming.h"
#include "physical_types.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace alkaid {  // Begin namespace alkaid

/* pb_type name, circuit model name */
typedef std::tuple<std::string, std::string> PhyPbInfo;

/* Map from physical pb type to operating pb types */
inline const std::map<std::string, std::string> OP2PHY_PB_TYPE_MAP = {
  {OPERATING_BLE4_FF_NAME, PHYSICAL_REGULAR_SCFF_NAME},
  {OPERATING_BLE5_FF_NAME, PHYSICAL_REGULAR_SCFF_NAME},
  {OPERATING_ALE_FF_NAME, PHYSICAL_REGULAR_SCFF_NAME},
  {OPERATING_PI_SCFF_NAME, PHYSICAL_PI_SCFF_NAME},
  {OPERATING_PI_ECB56_SCFF_NAME, PHYSICAL_PI_ECB56_SCFF_NAME},
  {OPERATING_PI_PDC_ECB1_SCFF_NAME, PHYSICAL_PI_PDC_ECB1_SCFF_NAME},
  {OPERATING_PO_SCFF_NAME, PHYSICAL_PO_SCFF_NAME},
  {OPERATING_PO_CKO_SCFF_NAME, PHYSICAL_PO_CKO_SCFF_NAME}};

std::string gen_latch_pb_name(const std::string& prefix);

std::string gen_pcnt_pb_type_name(const std::string& prefix, const size_t& ipcnt);

std::string gen_pcnt_phy_core_pb_name();

std::string gen_io_tile_name(const e_side& io_side);

std::string gen_pi_subtile_name(const e_side& io_side, const size_t& num_emode_bits,
                                const bool& with_prog_delay_line, const bool& with_clock_output);

std::string gen_po_subtile_name(const e_side& io_side, const size_t& num_emode_bits,
                                const bool& with_prog_delay_line, const bool& with_clock_output);

std::string gen_pi_pb_type_name(const size_t& num_emode_bits, const bool& with_prog_delay_line,
                                const bool& with_clock_output);

std::string gen_po_pb_type_name(const size_t& num_emode_bits, const bool& with_prog_delay_line,
                                const bool& with_clock_output);

}  // End of namespace alkaid
