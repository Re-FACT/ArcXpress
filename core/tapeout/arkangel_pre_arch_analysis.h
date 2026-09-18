#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#include "command.h"
#include "command_context.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arkangel */
namespace arkangel {

int analyze_logic_resource(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                           const openfpga::CommandContext& cmd_context);

} /* end namespace arkangel */
