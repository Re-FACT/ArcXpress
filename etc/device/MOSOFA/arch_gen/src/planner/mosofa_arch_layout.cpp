/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>
/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
/* Headers from openfpga library */
#include "command_exit_codes.h"
/* Headers from arch gen library */
#include "vpr_arch_layout.h"
#include "arch_gen_direct_connection.h"
#include "add_hidden_primitive_pb_types.h"
/* Headers from sofa common arch gen library */
#include "mosofa_arch_layout_io_tile.h"
#include "mosofa_arch_layout_clb_tile.h"
#include "mosofa_prim_pb_type_timing.h"
#include "mosofa_complex_block_po.h"
#include "mosofa_complex_block_pi.h"
#include "mosofa_complex_block_ff.h"
#include "mosofa_complex_block_clb.h"
#include "mosofa_constants.h"
#include "mosofa_arch_layout.h"
#include "mosofa_arch_planner.h"

namespace MOSOFA { // begin namespace MOSOFA

static int add_dimension_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                            const MOSOFA::ArchPlanner& arch_plan) {
  int num_err = 0;

  num_err += va_layout.set_dp_layout(arch_plan.dp_height(), arch_plan.dp_width()) ? 0: 1;
  num_err += va_layout.set_ultimate_layout(arch_plan.height(), arch_plan.width()) ? 0 : 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_routing_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout) {
  int num_err = 0;

  num_err += va_layout.add_segment(arcxpress::VprArchLayout::e_segment_type::X, 1, 12) ? 0: 1;
  num_err += va_layout.add_segment(arcxpress::VprArchLayout::e_segment_type::X, 2, 6) ? 0: 1;
  num_err += va_layout.add_segment(arcxpress::VprArchLayout::e_segment_type::X, 4, 10) ? 0: 1;
  num_err += va_layout.add_segment(arcxpress::VprArchLayout::e_segment_type::Y, 1, 12) ? 0 : 1;
  num_err += va_layout.add_segment(arcxpress::VprArchLayout::e_segment_type::Y, 2, 6) ? 0 : 1;
  num_err += va_layout.add_segment(arcxpress::VprArchLayout::e_segment_type::Y, 4, 10) ? 0: 1;

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_direct_connect_to_vpr_arch_layout(arcxpress::VprArchLayout& va_layout,
                                                 const MOSOFA::ArchPlanner& arch_plan) {
  int num_err = 0;

  size_t num_fle = arch_plan.num_fle_per_clb();
  size_t lut_size = arch_plan.max_lut_size();
  size_t num_outputs = num_fle * 2;

  std::string clb_tile = "clb";
  std::string clb_eq_input_port_prefix = "I";
  std::string clb_noneq_input_port_prefix = "Ix";
  std::string clb_output_port = "O";

  /* Add carry chain connection */
  num_err += va_layout.add_carrychain_direct_connection(clb_tile, "cout", "cin", arcxpress::VprArchLayout::e_segment_type::Y, arcxpress::VprArchLayout::e_dir_type::DEC) ? 0 : 1;

  /* Feedback connection for CLB */
  num_err += arcxpress::add_full_capable_direct_connections(va_layout, clb_tile,
                                                clb_eq_input_port_prefix,
                                                clb_noneq_input_port_prefix,
                                                clb_output_port,
                                                lut_size, num_fle, num_outputs
                                               ); 

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int add_complex_blocks(arcxpress::ArchGen& arch_gen) {
  int num_err = 0;

  /* Initialize top-level pb_type from tile lists */
  num_err += arch_gen.init_top_pb_type(true);
  /* Initialize primitive pb_type from primitive lists */
  num_err += arch_gen.init_primitive_pb_type(true);
  /* Initialize hidden primitive pb_type from primitive lists */
  num_err += sofa::add_sofa_builtin_latch_primitive_pb_type(arch_gen, true);
  num_err += arcxpress::add_builtin_lut_primitive_pb_type(arch_gen, 3, "1", true);
  num_err += arcxpress::add_builtin_lut_primitive_pb_type(arch_gen, 4, "0", true);
  num_err += arcxpress::add_builtin_pi_primitive_pb_type(arch_gen, std::string(sofa::PI_NAME), std::string(), true);
  num_err += arcxpress::add_builtin_po_primitive_pb_type(arch_gen, std::string(sofa::PO_NAME), true);
  /* Initialize the primitive delay variables/numbers */
  num_err += add_primitive_pb_type_timing(arch_gen, true);

  /* Assemble top-level pb type */
  num_err += assemble_po_complex_block(arch_gen, true);
  num_err += assemble_pi_complex_block(arch_gen, true);
  num_err += assemble_clb_complex_block(arch_gen, true);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * A writer to output a VPR arch object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int init_vpr_arch_layout(arcxpress::ArchGen& arch_gen,
                         const ArchGenInputParams& arch_params) {
  int status = MOSOFA::create_vpr_arch_primitives(arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in creating primitive details\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  int num_err = 0;

  ArchPlanner arch_plan(arch_params);
height, width, num_clks, num_rsts, num_inputs, num_outputs, num_pclks, num_regions, sync_dp_dimension);

  arcxpress::VprArchLayout va_layout;
  va_layout.set_default_fc_in(0.15);
  va_layout.set_default_fc_out(0.10);
  /* Add I/O tiles */
  num_err += add_io_tiles_to_vpr_arch_layout(va_layout, arch_plan);
  /* Add CLB tiles */
  num_err += add_clb_tiles_to_vpr_arch_layout(va_layout, arch_plan);
  /* Add layout dimension: Only the size of core fabric, do NOT count in I/O columns and rows */
  num_err += add_dimension_to_vpr_arch_layout(va_layout, arch_plan);
  num_err += add_routing_to_vpr_arch_layout(va_layout);
  /* Add directlist */
  num_err += add_direct_connect_to_vpr_arch_layout(va_layout);

  /* TODO: Run a validation on the arch layout */

  arch_gen.set_arch_layout(va_layout);
  arch_gen.set_num_clocks(arch_plan.num_clocks());
  arch_gen.set_num_pclocks(arch_plan.num_pclocks());
  arch_gen.set_num_config_regions(arch_plan.num_config_regions());
  arch_gen.set_num_resets(arch_plan.num_resets());
  
  /* Add complex block graph to arch generator */
  num_err += add_complex_blocks(arch_gen);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace MOSOFA
