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

int export_netlist(const ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_context);

int export_arch(const ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_context);

// If Graphviz is found, enable the following functionality
// This is a temporay fix due to the graphviz package issue on CentOS 7.9
#ifdef USE_GRAPHVIZ
int export_benchmark_schematic(ArkangelContext& arcxpress_context, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_context);
#endif

} /* end namespace arcxpress */
