#ifndef ARCXPRESS_BITSTREAM_COMMAND_H
#define ARCXPRESS_BITSTREAM_COMMAND_H

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

void add_bitstream_commands(openfpga::Shell<ArkangelContext>& shell);

} /* end namespace arcxpress */

#endif
