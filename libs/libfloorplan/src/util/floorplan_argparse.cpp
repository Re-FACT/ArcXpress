#include "floorplan_argparse.h"

#include "argparse.hpp"
#include "argparse_value.hpp"
#include "vtr_util.h"

argparse::ArgumentParser create_floorplan_arg_parser(const std::string& prog_name,
                                                     floorplan_args& args) {
  std::string description = "Generate Tile Shapes for floorPlan";
  auto parser = argparse::ArgumentParser(prog_name, description);

  std::string epilog = vtr::replace_all(
    "Usage Examples\n"
    "--------------\n"
    "   #Perform floorplan\n"
    "   {prog} --config floorplan_task.yaml --tile_qor_csv tile_qor.csv --tile_ref_file "
    "tile_reference.yaml --layout ultimate\n"
    "\n",
    "{prog}", parser.prog());

  parser.epilog(epilog);

  auto& pos_grp = parser.add_argument_group("required input");
  pos_grp.add_argument(args.tile_qor_csv, "--tile_qor_csv")
    .help(
      "The QOR file contains information including the tile name, tile area, and the number of "
      "cells. \n"
      "   - CSV: this is the default file format\n");

  pos_grp.add_argument(args.tile_ref_file, "--tile_ref_file")
    .help("tile reference file")
    .help("The tile reference file lists tile names and their associated instances. \n");

  pos_grp.add_argument(args.config_file, "--config")
    .help(
      "The config file contains parameters used for floorplanning, such as the utilization values "
      "for different tiles.\n ");

  pos_grp.add_argument(args.layout_type, "--layout")
    .help("Specifies the layout type, which can be either \"dp\" or \"ultimate\". \n");

  auto& gen_grp = parser.add_argument_group("general options");

  gen_grp.add_argument(args.show_help, "--help", "-h")
    .help("Show this help message then exit")
    .action(argparse::Action::HELP);

  gen_grp.add_argument<bool, ParseOnOff>(args.show_version, "--version")
    .help("Show version information then exit")
    .action(argparse::Action::VERSION);

  return parser;
}

floorplan_args read_args(int argc, const char** argv) {
  floorplan_args args = floorplan_args();  // Explicitly initialize for zero initialization

  auto parser = create_floorplan_arg_parser(argv[0], args);

  parser.parse_args(argc, argv);

  set_conditional_defaults(args);

  verify_args(args);

  return args;
}

/*if some args are not specified, we will set them to default values*/
void set_conditional_defaults(floorplan_args& args) {
  /* no default config should be set in current version
     In the future, if we need to set some config to default, we can write it as the following:
     if (args.clk_tree_loc_file.provenance() != argparse::Provenance::SPECIFIED) {
       std::string clk_tree_loc_file = "clk_tree_location.txt";
       args.clk_tree_loc_file.set(clk_tree_loc_file, argparse::Provenance::INFERRED);
     }
  */
}

/*check whether required args are provided*/
bool verify_args(const floorplan_args& args) {
  /*
   * Check for conflicting paramaters or dependencies where one parameter set requires another
   * parameter to be included
   */

  if (args.config_file.provenance() != argparse::Provenance::SPECIFIED) {
    std::cout << "No config file specified. Error out." << std::endl;
    return false;
  }

  if (args.tile_qor_csv.provenance() != argparse::Provenance::SPECIFIED) {
    std::cout << "No tile_qor_csv file specified. Error out." << std::endl;
    return false;
  }

  if (args.tile_ref_file.provenance() != argparse::Provenance::SPECIFIED) {
    std::cout << "No tile_ref_file file specified. Error out." << std::endl;
    return false;
  }

  if (args.layout_type.provenance() != argparse::Provenance::SPECIFIED) {
    std::cout << "No layout type specified. Error out." << std::endl;
    return false;
  }
  return true;
}