/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "command_exit_codes.h"
#include "mosofa_arch_layout_ckbuf_subtile.h"
#include "mosofa_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA {  // begin namespace MOSOFA

int add_ckbuf_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout) {
  int num_err = 0;

  std::string pb_type_name("ckbuf");
  num_err +=
    va_layout.create_pb_type(pb_type_name, arcxpress::VprArchLayout::e_subtile_type::CKBUF) ? 0 : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "in", arcxpress::VprArchLayout::e_port_type::INPUT) ? 0
                                                                                                : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "out", arcxpress::VprArchLayout::e_port_type::OUTPUT)
      ? 0
      : 1;
  num_err += va_layout.set_pb_type_port_fc(pb_type_name, "out", 0.) ? 0 : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_ckbuf_subtile_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                          const std::string& tile_name,
                                          const ArchPlanner& arch_plan) {
  int num_err = 0;

  /* Later should be managed by number of clocks and resets */
  size_t num_ckbuf = arch_plan.num_ckbuf_per_tile();

  std::string subtile_name("ckbuf");
  num_err += va_layout.add_subtile(tile_name, subtile_name, "ckbuf", {}, num_ckbuf) ? 0 : 1;
  /* Add custom pin location */
  for (size_t istile = 0; istile < num_ckbuf; ++istile) {
    num_err += va_layout.add_subtile_pin_location(tile_name, subtile_name, istile, "in", 0, RIGHT) ? 0 : 1;
    num_err += va_layout.add_subtile_pin_location(tile_name, subtile_name, istile, "out", 0, LEFT) ? 0 : 1;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}


}  // namespace MOSOFA
