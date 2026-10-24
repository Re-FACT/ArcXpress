#ifndef ARKANGEL_SIMULATION_H
#define ARKANGEL_SIMULATION_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#include "command.h"
#include "command_context.h"
#include "shell.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

int run_app_hdl_simulation(openfpga::Shell<ArkangelContext>* shell,
                           ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                           const openfpga::CommandContext& cmd_context);
} /* end namespace arcxpress */

#endif
