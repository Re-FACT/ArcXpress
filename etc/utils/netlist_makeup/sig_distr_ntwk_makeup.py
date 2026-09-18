#####################################################################
# A script to generate pin table .csv file
#####################################################################
import os
from os.path import dirname, abspath
import argparse
import logging
import csv
import sig_distr_ntwk

#####################################################################
# Error codes
#####################################################################
error_codes = {"SUCCESS": 0, "ERROR": 1, "FILE_ERROR": 3}

#####################################################################
# Initialize logger
#####################################################################
logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.INFO)

#####################################################################
# Main function
#####################################################################
if __name__ == "__main__":
    # Execute when the module is not initialized from an import statement

    # Parse the options and apply sanity checks
    parser = argparse.ArgumentParser(
        description="Add signal distribution buffer to eFPGA top module and output physical location constraints for physical design flow. For instance, /mypath/sig_distr_ntwk.xml"
    )
    parser.add_argument(
        "--sig_distr_ntwk_file",
        required=True,
        help="Path to the .xml file that the description of signal distribution networks",
    )
    parser.add_argument(
        "--input_netlist",
        default="",
        help="The path of the verilog file fpga_core.v where all the tiles and routing are instanciated. For instance, /mypath/fpga_core.v",
    )
    parser.add_argument(
        "--output_netlist",
        default="",
        help="Path to the output verilog file which now contains signal distribution buffers. For instance, /mypath/fpga_core_new.v",
    )
    parser.add_argument(
        "--pd_constraint_file",
        default="",
        help="Path to the physical constraints for the signal distribution network in tcl file that will be outputted. For instance, /mypath/pd_constr.tcl",
    )
    parser.add_argument(
        "--fpga_core_instance_name",
        default="fpga_core_inst",
        help="The instance name of fpga_core under the fpga_top.v",
    )
    args = parser.parse_args()

    num_error = 0
    # read signal distribution network structure
    sig_distr_ntwk = sig_distr_ntwk.SigDistrNtwk()
    num_error += sig_distr_ntwk.read_signal_distribution_network_from_xml(args.sig_distr_ntwk_file)
    # read fabric netlists
    num_error += sig_distr_ntwk.read_fabric_netlist(args.input_netlist)
    # add signal distribution buffers to the netlists
    num_error += sig_distr_ntwk.add_sig_distr_buf_to_netlist()
    # write out modified netlists
    num_error += sig_distr_ntwk.write_fabric_netlist(args.output_netlist)
    # write pd constraint file
    num_error += sig_distr_ntwk.write_pd_constraint_tcl(
        args.pd_constraint_file, args.fpga_core_instance_name
    )
    if num_error == 0:
        logging.info("Generation succeed")
        exit(error_codes["SUCCESS"])
    else:
        logging.error("Generation failed in " + str(num_error) + " errors!")
        exit(error_codes["ERROR"])
