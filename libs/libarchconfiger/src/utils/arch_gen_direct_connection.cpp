/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "arch_gen_direct_connection.h"
#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arcxpress {

int add_full_capable_direct_connections(VprArchLayout& va_layout, const std::string& clb_tile,
                                        const std::string& clb_eq_input_port_prefix,
                                        const std::string& clb_non_eq_input_port_prefix,
                                        const std::string& clb_output_port, const size_t& lut_size,
                                        const size_t& num_fle, const size_t& num_outputs) {
  int num_err = 0;

  for (size_t i_out = 0; i_out < num_outputs; ++i_out) {
    for (size_t i_in_g = 0; i_in_g < num_fle; ++i_in_g) {
      for (size_t i_in = 0; i_in < lut_size; ++i_in) {
        size_t o_start = i_in;
        size_t o_end = num_outputs - lut_size + i_in;
        /* Build fully capable connection pattern */
        if (i_out < o_start || i_out > o_end) {
          continue;
        }
        std::string input_port_name = clb_non_eq_input_port_prefix;
        if (i_in < lut_size - 1) {
          input_port_name = clb_eq_input_port_prefix + std::to_string(i_in_g);
          num_err += va_layout.add_feedback_direct_connection(clb_tile, clb_output_port, i_out,
                                                              input_port_name, i_in)
                       ? 0
                       : 1;
        } else {
          num_err += va_layout.add_feedback_direct_connection(clb_tile, clb_output_port, i_out,
                                                              input_port_name, i_in_g)
                       ? 0
                       : 1;
        }
      }
    }
  }

  return num_err;
}

}  // namespace arcxpress
