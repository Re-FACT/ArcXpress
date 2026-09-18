#ifndef ARKANGEL_BASIC_COMMAND_H
#define ARKANGEL_BASIC_COMMAND_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#include "shell.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace openfpga */
namespace arkangel {

void add_basic_commands(openfpga::Shell<ArkangelContext>& shell);

}  // namespace arkangel

#endif
