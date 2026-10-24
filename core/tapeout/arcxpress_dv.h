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

int setup_dv(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
             const openfpga::CommandContext& cmd_context);

int run_preconfig_dv(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int run_benchmark_power_analysis(ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                                 const openfpga::CommandContext& cmd_ctx);

int run_benchmark_sta(ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_ctx);

} /* end namespace arcxpress */