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

int export_netlist(const ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                   const openfpga::CommandContext& cmd_context);

int export_arch(const ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                const openfpga::CommandContext& cmd_context);

// If Graphviz is found, enable the following functionality
// This is a temporay fix due to the graphviz package issue on CentOS 7.9
#ifdef USE_GRAPHVIZ
int export_benchmark_schematic(ArkangelContext& arkangel_context, const openfpga::Command& cmd,
                               const openfpga::CommandContext& cmd_context);
#endif

} /* end namespace arkangel */
