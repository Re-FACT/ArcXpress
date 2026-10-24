#pragma once

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

void add_arcxpress_arch_dev_commands(openfpga::Shell<ArkangelContext>& shell);

} /* end namespace arcxpress */
