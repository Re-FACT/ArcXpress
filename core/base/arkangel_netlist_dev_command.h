#pragma once

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

void add_arkangel_netlist_dev_commands(openfpga::Shell<ArkangelContext>& shell);

} /* end namespace arkangel */
