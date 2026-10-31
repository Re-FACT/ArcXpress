#include "mosofa_naming.h"
#include <string>
#include "side_manager.h"

namespace MOSOFA { // begin namespace MOSOFA

std::string gen_latch_pb_name(const std::string& prefix) {
  return prefix + std::string("_") + std::string(OPERATING_BUILTIN_LATCH_NAME);
}

std::string gen_io_tile_name(const e_side& io_side) {
  SideManager side_mgr(io_side);
  return std::string(TILE_IO_PREFIX) + std::string("_") + side_mgr.to_string();
}

std::string gen_pi_subtile_name(const e_side& io_side) {
  SideManager side_mgr(io_side);
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += side_mgr.to_string() + std::string("_");
  ret += std::string(SUBTILE_PI_PREFIX);
  return ret;
}

std::string gen_po_subtile_name(const e_side& io_side) {
  SideManager side_mgr(io_side);
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += side_mgr.to_string() + std::string("_");
  ret += std::string(SUBTILE_PO_PREFIX);
  return ret;
}

std::string gen_pi_pb_type_name() {
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += std::string(SUBTILE_PI_PREFIX);
  return ret;
}

std::string gen_po_pb_type_name() {
  std::string ret = std::string(TILE_IO_PREFIX) + std::string("_");
  ret += std::string(SUBTILE_PO_PREFIX);
  return ret;
}

}  // namespace MOSOFA
