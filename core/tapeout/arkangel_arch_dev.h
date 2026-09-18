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

int create_arch(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_context);

int setup_arch(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
               const openfpga::CommandContext& cmd_context);

int show_tapeout_arch(const ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int set_tapeout_arch(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_io_num(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                    const openfpga::CommandContext& cmd_context);

int set_arch_sync_dp_dimension(ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_ctx);

int set_arch_core_dimension(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                            const openfpga::CommandContext& cmd_context);

int set_arch_num_clk(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_num_rst(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int set_arch_num_pclk(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int set_arch_num_config_region(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_context);

int remove_arch_io(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_context);

int eval_tapeout_arch(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

} /* end namespace arkangel */
