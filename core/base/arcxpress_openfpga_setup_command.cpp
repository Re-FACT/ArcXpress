/********************************************************************
 * Add commands to the OpenFPGA shell interface,
 * in purpose of setting up OpenFPGA core engine, including:
 * - read_openfpga_arch : read OpenFPGA architecture file
 *******************************************************************/
#include "arkangel_openfpga_setup_command.h"

#include "openfpga_setup_command_template.h"

/* begin namespace arcxpress */
namespace arcxpress {

void add_openfpga_setup_commands(openfpga::Shell<ArkangelContext>& shell) {
  openfpga::add_setup_command_templates<ArkangelContext>(shell, true);
}

} /* end namespace arcxpress */
