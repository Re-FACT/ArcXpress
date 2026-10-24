#ifndef ARCXPRESS_OPENFPGA_VERILOG_COMMAND_H
#define ARCXPRESS_OPENFPGA_VERILOG_COMMAND_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arcxpress_context.h"
#include "shell.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

void add_openfpga_verilog_commands(openfpga::Shell<ArcxpressContext>& shell);

} /* end namespace arcxpress */

#endif
