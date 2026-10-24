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

int generate_netlist(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int netlist_opt(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_context);

int analyze_tile_area(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                      const openfpga::CommandContext& cmd_context);

int coarse_floorplan(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                     const openfpga::CommandContext& cmd_context);

int extract_netlist_qor(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                        const openfpga::CommandContext& cmd_context);

} /* end namespace arcxpress */