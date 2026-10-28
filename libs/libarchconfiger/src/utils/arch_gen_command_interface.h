#pragma once

#include <vector>
#include <string>
#include <map>
#include "command.h"
#include "command_context.h"
#include "arch_gen.h"
#include "physical_types.h"

namespace arcxpress { // begin namespace arcxpress

class ArchGenCommandInterface {
public:
  explicit ArchGenCommandInterface(const std::string& cmd_name);

  /**
   * Parse command-line arguments and populate the command context.
   * Returns true on success, or false if parsing fails.
   */
  bool parse(int argc, char** argv, openfpga::CommandContext& exec_cmd_ctx);

  /**
   * Encapsulates architecture primitives initialization.
   */
  int init_primitives(arkangel::ArchGen& arch_gen) const;

  /**
   * Encapsulates layout extraction, input/output constraints parsing, 
   * option logging, and layout initialization.
   */
  int init_layout(const openfpga::CommandContext& exec_cmd_ctx, 
                  arkangel::ArchGen& arch_gen) const;

  /**
   * Encapsulates writing and generating all VPR and OpenFPGA template files,
   * global clock networks, bitstream settings, and location maps.
   */
  int write_xml_templates(const openfpga::CommandContext& exec_cmd_ctx, 
                          arkangel::ArchGen& arch_gen) const;

  // Accessor for the underlying command object (needed for option lookups)
  const openfpga::Command& command() const;

  // Option ID Getters
  openfpga::CommandOptionId vpr_arch() const;
  openfpga::CommandOptionId openfpga_arch() const;
  openfpga::CommandOptionId openfpga_dp_global_network() const;
  openfpga::CommandOptionId openfpga_ultimate_global_network() const;
  openfpga::CommandOptionId openfpga_bitstream_setting() const;
  openfpga::CommandOptionId dp_ckbuf_loc_map() const;
  openfpga::CommandOptionId ultimate_ckbuf_loc_map() const;
  openfpga::CommandOptionId height() const;
  openfpga::CommandOptionId width() const;
  openfpga::CommandOptionId num_inputs() const;
  openfpga::CommandOptionId num_outputs() const;
  openfpga::CommandOptionId num_clks() const;
  openfpga::CommandOptionId num_rsts() const;
  openfpga::CommandOptionId num_pclks() const;
  openfpga::CommandOptionId num_config_regions() const;
  openfpga::CommandOptionId sync_dp_dimension() const;

private:
  void init_options();

  openfpga::Command exec_cmd_;
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
