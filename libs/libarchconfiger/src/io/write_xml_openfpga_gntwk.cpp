/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "command_exit_codes.h"
#include "pugixml.hpp"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "write_xml_clock_network.h"
#include "write_xml_openfpga_gntwk.h"

namespace arcxpress {

/********************************************************************
 * A writer to output a VPR arch object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_openfpga_gntwk(const char* fname, const ArchGen& arch_gen,
                             const VprArchLayout::e_fabric_type& fb_type) {
  std::string timer_msg =
    std::string("Write openfpga global network to file: ") + std::string(fname);
  vtr::ScopedStartFinishTimer timer(timer_msg.c_str());

  int num_err = 0;

  openfpga::ClockNetwork gntwk;
  num_err += arch_gen.build_global_network(fb_type, gntwk);

  num_err += openfpga::write_xml_clock_network(fname, gntwk);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arcxpress
