/********************************************************************
 * Add commands to the OpenFPGA shell interface,
 * in purpose of generate Verilog netlists modeling the full FPGA fabric
 * This is one of the core engine of openfpga, including:
 * - generate_fabric_verilog : generate Verilog netlists about FPGA fabric
 * - generate_fabric_verilog_testbench : TODO: generate Verilog testbenches
 *******************************************************************/
#include "arkangel_openfpga_verilog_command.h"

#include "openfpga_verilog_command_template.h"

/* begin namespace arcxpress */
namespace arcxpress {

void add_openfpga_verilog_commands(openfpga::Shell<ArkangelContext>& shell) {
  openfpga::add_verilog_command_templates<ArkangelContext>(shell, true);
}

} /* end namespace arcxpress */
