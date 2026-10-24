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

int create_arch(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_context);

int setup_arch(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
               const openfpga::CommandContext& cmd_context);

int show_tapeout_arch(const ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int set_tapeout_arch(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_io_num(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                    const openfpga::CommandContext& cmd_context);

int set_arch_sync_dp_dimension(ArcxpressContext& arcxpress_ctx, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_ctx);

int set_arch_core_dimension(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                            const openfpga::CommandContext& cmd_context);

int set_arch_num_clk(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_num_rst(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_num_pclk(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int set_arch_num_config_region(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_context);

int remove_arch_io(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_context);

int eval_tapeout_arch(ArcxpressContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

} /* end namespace arcxpress */
