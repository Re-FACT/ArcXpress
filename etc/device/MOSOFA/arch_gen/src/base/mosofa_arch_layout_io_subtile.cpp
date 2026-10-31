/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "mosofa_arch_layout_io_subtile.h"
#include "mosofa_naming.h"
#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA { // begin namespace MOSOFA

int add_pi_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout) {
  int num_err = 0;

  std::string pb_type_name = gen_pi_pb_type_name();
  num_err +=
    va_layout.create_pb_type(pb_type_name, arcxpress::VprArchLayout::e_subtile_type::PI) ? 0 : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "a2f_o", arcxpress::VprArchLayout::e_port_type::OUTPUT)
      ? 0
      : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int add_po_pb_type_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout) {
  int num_err = 0;

  std::string pb_type_name = gen_po_pb_type_name();
  num_err +=
    va_layout.create_pb_type(pb_type_name, arcxpress::VprArchLayout::e_subtile_type::PI) ? 0 : 1;
  num_err +=
    va_layout.add_pb_type_port(pb_type_name, "f2a_i", arcxpress::VprArchLayout::e_port_type::INPUT)
      ? 0
      : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace MOSOFA
