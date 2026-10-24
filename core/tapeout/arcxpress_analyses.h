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

int analyze_pre_arch_qor(const ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                         const openfpga::CommandContext& cmd_context);

int analyze_arch_eval_qor(const ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                          const openfpga::CommandContext& cmd_context);

} /* end namespace arcxpress */
