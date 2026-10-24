#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arcxpress_context.h"
#include "command.h"
#include "command_context.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

int backannotate(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                 const openfpga::CommandContext& cmd_context);

} /* end namespace arcxpress */
