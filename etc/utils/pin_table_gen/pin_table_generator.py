#####################################################################
# A script to generate pin table .csv file
#####################################################################
import os
from os.path import dirname, abspath
import argparse
import logging
import csv
import io_coord_manager
import mini_pin_table
import pin_table_naming_manager

#####################################################################
# Error codes
#####################################################################
error_codes = {"SUCCESS": 0, "ERROR": 1, "FILE_ERROR": 3}

#####################################################################
# Initialize logger
#####################################################################
logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.INFO)

# Constants
CLK_ASSOCIATED = "clk"
CLK_EDGE = "rising"


#####################################################################
# Generate pin table to an output file
#####################################################################
def generate_pin_table_to_csv(io_coord_xml_file, naming_rule_file, csv_filename):
    num_errors = 0

    # Read the I/O coordinate xml into database
    io_coord_mgr = io_coord_manager.IoCoordManager()
    io_coord_mgr.load_from_xml(io_coord_xml_file)

    # Read the configuration XML file that defines side attributes for I/O
    naming_rule_mgr = pin_table_naming_manager.PinTableNamingManager()
    naming_rule_mgr.load_from_xml(naming_rule_file)

    # Pin table database
    ptable = mini_pin_table.MiniPinTable()

    # Walk through all the coordinates
    for rule_id in naming_rule_mgr.rules():
        io_bank_id = 0
        for curr_xy in naming_rule_mgr.coords(rule_id):
            curr_x = curr_xy[0]
            curr_y = curr_xy[1]
            input_count = 0
            output_count = 0
            for curr_z in io_coord_mgr.z_range(curr_x, curr_y):
                pad_name = io_coord_mgr.pad_name(curr_x, curr_y, curr_z)
                side_tag = naming_rule_mgr.side(rule_id)
                # Set a mapped pin name prefix
                mapped_pin_name = side_tag + "_" + str(io_bank_id) + "_"
                # FIXME: This is too restricted! Identify GPIO type by the postfix
                gpio_type = "N/A"
                # Complete the pin name
                if (
                    pad_name.startswith("gfpga_pad_pinput_A2F")
                    or pad_name.startswith("gfpga_pad_pinput_extmode_A2F")
                    or pad_name.startswith("gfpga_pad_pinput_extmode56_A2F")
                    or pad_name.startswith("gfpga_pad_pinput_dchain_A2F")
                    or pad_name.startswith("gfpga_pad_pinput_dchain_extmode_A2F")
                ):
                    gpio_type = "in"
                    mapped_pin_name += gpio_type.upper() + "[" + str(input_count) + "]"
                    input_count += 1
                elif pad_name.startswith("gfpga_pad_poutput_F2A"):
                    gpio_type = "out"
                    mapped_pin_name += gpio_type.upper() + "[" + str(output_count) + "]"
                    output_count += 1
                else:
                    raise Exception(
                        f"Invalid type of I/O: {pad_name}! Cannot identify if this is an input or output!"
                    )
                # Add content to pin table
                pin_id = ptable.create_pin()
                ptable.set_orientation(pin_id, side_tag)
                ptable.set_column(pin_id, curr_x)
                ptable.set_row(pin_id, curr_y)
                ptable.set_offset(pin_id, curr_z)
                ptable.set_pad_name(pin_id, pad_name)
                ptable.set_pin_name(pin_id, mapped_pin_name)
                ptable.set_gpio_type(pin_id, gpio_type)
                ptable.set_associated_clk(pin_id, CLK_ASSOCIATED)
                ptable.set_clock_edge(pin_id, CLK_EDGE)
            # Increment bank id
            io_bank_id += 1

    # Write pin table to csv
    ptable.write_csv(csv_filename)

    return num_errors


#####################################################################
# Main function
#####################################################################
if __name__ == "__main__":
    # Execute when the module is not initialized from an import statement

    # Parse the options and apply sanity checks
    parser = argparse.ArgumentParser(description="Generate pin table to a .csv file")
    parser.add_argument(
        "--io_coord_file",
        required=True,
        default="ultimate_io_coordinates.xml",
        help="Path to the .xml file that contains I/O coordinates",
    )
    parser.add_argument(
        "--naming_rules",
        required=True,
        default="pin_table_naming_rules.xml",
        help="Path to the .xml file that contains pin naming rules",
    )
    parser.add_argument(
        "--csv_file",
        required=True,
        default="efpga_pin_table.csv",
        help="Path to the .csv file that will be outputted",
    )
    args = parser.parse_args()

    num_error = generate_pin_table_to_csv(args.io_coord_file, args.naming_rules, args.csv_file)
    if num_error == 0:
        logging.info("Generation succeed")
        exit(error_codes["SUCCESS"])
    else:
        logging.error("Generation failed in " + str(num_error) + " errors!")
        exit(error_codes["ERROR"])
