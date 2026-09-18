#ifndef ARKANGEL_OPENFPGA_SETUP_COMMAND_H
#define ARKANGEL_OPENFPGA_SETUP_COMMAND_H

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

void add_openfpga_setup_commands(openfpga::Shell<ArkangelContext>& shell);

} /* end namespace arkangel */

#endif
