#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#include "shell.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

void add_arkangel_pre_arch_commands(openfpga::Shell<ArkangelContext>& shell);

} /* end namespace arcxpress */
