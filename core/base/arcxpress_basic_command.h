#ifndef ARCXPRESS_BASIC_COMMAND_H
#define ARCXPRESS_BASIC_COMMAND_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#include "shell.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace openfpga */
namespace arcxpress {

void add_basic_commands(openfpga::Shell<ArkangelContext>& shell);

}  // namespace arcxpress

#endif
