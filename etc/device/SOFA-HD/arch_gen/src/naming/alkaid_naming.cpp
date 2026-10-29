/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
#include "alkaid_naming.h"

#include <string>

#include "pcnt_gen.h"
#include "side_manager.h"

namespace alkaid {

std::string gen_latch_pb_name(const std::string& prefix) {
  return prefix + std::string("_") + std::string(OPERATING_BUILTIN_LATCH_NAME);
}

std::string gen_pcnt_pb_type_name(const std::string& prefix, const size_t& ipcnt) {
  return prefix + std::to_string(ipcnt);
}

std::string gen_pcnt_phy_core_pb_name() {
  PcntGen pcnt_gen;
  return pcnt_gen.vpr_arch_physical_primitive().pb_type_name(pcnt_gen.phy_mode_bit_tag());
}

std::string gen_io_tile_name(const e_side& io_side) {
  SideManager side_mgr(io_side);
  /* FIXME: The L postfix will be removed once downstream steps (netlist makeup, synthesis sdc etc.)
   * no longer depends on it */
  return std::string(TILE_IO_PREFIX) + std::string("_") + side_mgr.to_string() + std::string("L");
}

std::string gen_pi_subtile_name(const e_side& io_side, const size_t& num_emode_bits,
                                const bool& with_prog_delay_line, const bool& with_clock_output) {
  SideManager side_mgr(io_side);
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += side_mgr.to_string() + std::string("_");
  ret += std::string(SUBTILE_PI_PREFIX);
  if (with_prog_delay_line) {
    ret += std::string("_") + std::string(SUBTILE_PDC_PREFIX);
  }
  if (num_emode_bits != 0) {
    ret += std::string("_") + std::string(SUBTILE_ECB_PREFIX) + std::to_string(num_emode_bits);
  }
  if (with_clock_output) {
    ret += std::string("_") + std::string(SUBTILE_CKO_PREFIX);
  }
  return ret;
}

std::string gen_po_subtile_name(const e_side& io_side, const size_t& num_emode_bits,
                                const bool& with_prog_delay_line, const bool& with_clock_output) {
  SideManager side_mgr(io_side);
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += side_mgr.to_string() + std::string("_");
  ret += std::string(SUBTILE_PO_PREFIX);
  if (with_prog_delay_line) {
    ret += std::string("_") + std::string(SUBTILE_PDC_PREFIX);
  }
  if (num_emode_bits != 0) {
    ret += std::string("_") + std::string(SUBTILE_ECB_PREFIX) + std::to_string(num_emode_bits);
  }
  if (with_clock_output) {
    ret += std::string("_") + std::string(SUBTILE_CKO_PREFIX);
  }
  return ret;
}

std::string gen_pi_pb_type_name(const size_t& num_emode_bits, const bool& with_prog_delay_line,
                                const bool& with_clock_output) {
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += std::string(SUBTILE_PI_PREFIX);
  if (with_prog_delay_line) {
    ret += std::string("_") + std::string(SUBTILE_PDC_PREFIX);
  }
  if (num_emode_bits != 0) {
    ret += std::string("_") + std::string(SUBTILE_ECB_PREFIX) + std::to_string(num_emode_bits);
  }
  if (with_clock_output) {
    ret += std::string("_") + std::string(SUBTILE_CKO_PREFIX);
  }
  return ret;
}

std::string gen_po_pb_type_name(const size_t& num_emode_bits, const bool& with_prog_delay_line,
                                const bool& with_clock_output) {
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += std::string(SUBTILE_PO_PREFIX);
  if (with_prog_delay_line) {
    ret += std::string("_") + std::string(SUBTILE_PDC_PREFIX);
  }
  if (num_emode_bits != 0) {
    ret += std::string("_") + std::string(SUBTILE_ECB_PREFIX) + std::to_string(num_emode_bits);
  }
  if (with_clock_output) {
    ret += std::string("_") + std::string(SUBTILE_CKO_PREFIX);
  }
  return ret;
}

}  // namespace alkaid
