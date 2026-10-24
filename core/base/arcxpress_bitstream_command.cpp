/********************************************************************
 * Add commands to the OpenFPGA shell interface,
 * in purpose of generate Verilog netlists modeling the full FPGA fabric
 * This is one of the core engine of openfpga, including:
 * - repack : create physical pbs and redo packing
 *******************************************************************/
#include "arcxpress_bitstream_command.h"

#include "openfpga_bitstream_command_template.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Top-level function to add all the commands related to FPGA-Bitstream
 *******************************************************************/
void add_bitstream_commands(openfpga::Shell<ArcxpressContext>& shell) {
  /* Add bitstream commands from openfpga */
#ifdef ARCXPRESS_PRODUCTION_EDITION
  openfpga::add_bitstream_command_templates<ArcxpressContext>(shell, true);
#elif ARCXPRESS_ENGINEERING_EDITION
  openfpga::add_bitstream_command_templates<ArcxpressContext>(shell, false);
#endif
}

} /* end namespace arcxpress */
