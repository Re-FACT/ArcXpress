#pragma once

#include <vector>
#include <string>
#include <map>
#include "command.h"
#include "command_context.h"
#include "arch_gen.h"
#include "physical_types.h"
#include "arch_gen_input_params.h"

namespace arcxpress { // begin namespace arcxpress

class ArchGenCommandInterface {
public:
  explicit ArchGenCommandInterface(const std::string& cmd_name);

  /**
   * Parse command-line arguments and populate the command context.
   * Returns true on success, or false if parsing fails.
   */
  bool parse(int argc, char** argv);

  /**
   * Return a package of parameters for the architecture definition, to drive arch planner
   */
  ArchGenInputParams arch_parameters() const;

  /**
   * Encapsulates writing and generating all VPR and OpenFPGA template files,
   * global clock networks, bitstream settings, and location maps.
   */
  int write_xml_templates(const openfpga::CommandContext& exec_cmd_ctx, 
                          const ArchGen& arch_gen) const;

private: /* Utilities */
  void init_options();

private: /* internal data */
  openfpga::Command exec_cmd_;
  openfpga::CommandContext_ exec_cmd_ctx_;
  openfpga::CommandOptionId opt_vpr_arch_;
  openfpga::CommandOptionId opt_of_arch_;
  openfpga::CommandOptionId opt_dp_gntwk_;
  openfpga::CommandOptionId opt_ut_gntwk_;
  openfpga::CommandOptionId opt_bitset_;
  openfpga::CommandOptionId opt_dp_ckbuf_loc_map_;
  openfpga::CommandOptionId opt_ut_ckbuf_loc_map_;
  openfpga::CommandOptionId opt_height_;
  openfpga::CommandOptionId opt_width_;
  openfpga::CommandOptionId opt_num_inputs_;
  openfpga::CommandOptionId opt_num_outputs_;
  openfpga::CommandOptionId opt_num_clks_;
  openfpga::CommandOptionId opt_num_rsts_;
  openfpga::CommandOptionId opt_num_pclks_;
  openfpga::CommandOptionId opt_num_config_regions_;
  openfpga::CommandOptionId opt_sync_dp_dimension_;
};

} // end namespace arcxpress
