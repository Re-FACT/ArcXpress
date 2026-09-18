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

int generate_netlist(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int netlist_opt(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_context);

int analyze_tile_area(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int coarse_floorplan(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int extract_netlist_qor(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_context);

} /* end namespace arkangel */