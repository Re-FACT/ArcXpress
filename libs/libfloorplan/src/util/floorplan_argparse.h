#pragma once
#include <algorithm>
#include <cctype>

#include "argparse.hpp"
#include "argparse_value.hpp"
#include "read_floorplan_params.h"
/*
This file defined apis for parsing command line arguments
and generating the floorplan tcl file
*/

// args of the whole floorplan tools
struct floorplan_args {
  argparse::ArgValue<std::string> config_file;
  argparse::ArgValue<std::string> tile_qor_csv;
  argparse::ArgValue<std::string> tile_ref_file;
  argparse::ArgValue<std::string> layout_type;
  argparse::ArgValue<bool> show_version;
  argparse::ArgValue<bool> show_help;
};

// utility function for parsing args
struct ParseOnOff {
  argparse::ConvertedValue<bool> from_str(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    argparse::ConvertedValue<bool> conv_value;
    if (str == "on")
      conv_value.set_value(true);
    else if (str == "off")
      conv_value.set_value(false);
    else {
      std::stringstream msg;
      msg << "Invalid conversion from '" << str
          << "' to boolean (expected one of: " << argparse::join(default_choices(), ", ") << ")";
      conv_value.set_error(msg.str());
    }
    return conv_value;
  }

  argparse::ConvertedValue<std::string> to_str(bool val) {
    argparse::ConvertedValue<std::string> conv_value;

    if (val)
      conv_value.set_value("on");
    else
      conv_value.set_value("off");

    return conv_value;
  }

  std::vector<std::string> default_choices() { return {"on", "off"}; }
};

argparse::ArgumentParser create_floorplan_arg_parser(const std::string& prog_name,
                                                     floorplan_args& args);
floorplan_args read_args(int argc, const char** argv);
void set_conditional_defaults(
  floorplan_args& args); /*if optional args are not given, set them to default values*/
bool verify_args(const floorplan_args& args); /*check whether required args are provided*/
