#ifndef ARKANGEL_BITSTREAM_COMMAND_H
#define ARKANGEL_BITSTREAM_COMMAND_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#include "shell.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arkangel */
namespace arkangel {

void add_bitstream_commands(openfpga::Shell<ArkangelContext>& shell);

} /* end namespace arkangel */

#endif
