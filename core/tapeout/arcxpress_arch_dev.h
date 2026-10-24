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

int create_arch(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_context);

int setup_arch(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
               const openfpga::CommandContext& cmd_context);

int show_tapeout_arch(const ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int set_tapeout_arch(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_io_num(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                    const openfpga::CommandContext& cmd_context);

int set_arch_sync_dp_dimension(ArkangelContext& arcxpress_ctx, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_ctx);

int set_arch_core_dimension(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                            const openfpga::CommandContext& cmd_context);

int set_arch_num_clk(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_num_rst(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_num_pclk(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int set_arch_num_config_region(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_context);

int remove_arch_io(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_context);

int eval_tapeout_arch(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

} /* end namespace arcxpress */
