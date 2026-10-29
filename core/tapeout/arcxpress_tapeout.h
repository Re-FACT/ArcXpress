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

int create_tapeout(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_context);

int load_tapeout(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                 const openfpga::CommandContext& cmd_context);

int save_tapeout(const ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                 const openfpga::CommandContext& cmd_context);

int show_tapeout_status(const ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_context);

int show_tapeout_dashboard(const ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                           const openfpga::CommandContext& cmd_context);

int disable_benchmark(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int disable_corner(ArcxpressContext& arcxpress_ctx, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_ctx);

int add_benchmark_vpr_constraints(ArcxpressContext& arcxpress_ctx, const openfpga::Command& cmd,
                                  const openfpga::CommandContext& cmd_ctx);

int set_benchmark_custom_pin_utilization(ArcxpressContext& arcxpress_ctx,
                                         const openfpga::Command& cmd,
                                         const openfpga::CommandContext& cmd_ctx);
} /* end namespace arcxpress */
