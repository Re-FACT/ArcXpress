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

/* begin namespace arcxpress */
namespace arcxpress {

int report_corners(const ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_context);

int report_module_names(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx);

int report_ref_module_names(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                            const openfpga::CommandContext& cmd_ctx);

int report_io(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
              const openfpga::CommandContext& cmd_ctx);

int report_static_power(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx);

int report_benchmark_power(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                           const openfpga::CommandContext& cmd_ctx);

int report_config_power(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx);

int report_config_stats(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_ctx);

int report_qor_scale(const ArkangelContext& arkangel_ctx, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_ctx);

} /* end namespace arcxpress */
