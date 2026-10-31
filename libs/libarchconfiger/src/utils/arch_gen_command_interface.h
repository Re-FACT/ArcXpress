#pragma once

#include <map>
#include <string>
#include <vector>

#include "arch_gen.h"
#include "arch_gen_input_params.h"
#include "command.h"
#include "command_context.h"
#include "physical_types.h"

namespace arcxpress {  // begin namespace arcxpress

class ArchGenCommandInterface {
 public:
  /**
   * Parse command-line arguments and populate the command context.
   * Returns true on success, or false if parsing fails.
   */
  bool parse(const std::string& cmd_name, int argc, char** argv);

  /**
   * Return a package of parameters for the architecture definition, to drive arch planner
   */
  ArchGenInputParams arch_parameters() const;

  /**
   * Encapsulates writing and generating all VPR and OpenFPGA template files,
   * global clock networks, bitstream settings, and location maps.
   */
  int write_xml_templates(const ArchGen& arch_gen) const;

 private: /* internal data */
  ArchGenInputParams params_;
  std::string vpr_arch_fpath_;
  std::string of_arch_fpath_;
  std::string of_dp_gntwk_fpath_;
  std::string of_ut_gntwk_fpath_;
  std::string of_bitset_fpath_;
  std::string dp_ckbuf_loc_map_fpath_;
  std::string ut_ckbuf_loc_map_fpath_;
};

}  // end namespace arcxpress
