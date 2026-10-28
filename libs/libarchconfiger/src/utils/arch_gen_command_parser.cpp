#include "arch_gen_command_parser.h"
#include "command_parser.h"
#include "command_exit_codes.h"
#include "vtr_log.h"

namespace arcxpress { // begin namespace arcxpress

ArchGenCommandParser::ArchGenCommandParser(const std::string& cmd_name) : exec_cmd_(cmd_name) {
  init_options();
}

bool ArchGenCommandParser::parse(int argc, char** argv, openfpga::CommandContext& exec_cmd_ctx) {
  std::vector<std::string> cmd_opts;
  cmd_opts.push_back(exec_cmd_.name());  
  for (int iarg = 1; iarg < argc; ++iarg) {
    cmd_opts.push_back(std::string(argv[iarg]));
  }

  exec_cmd_ctx = openfpga::CommandContext(exec_cmd_);
  if (!parse_command(cmd_opts, exec_cmd_, exec_cmd_ctx)) {
    /* Parse fail: echo the command options */
    openfpga::print_command_options(exec_cmd_);
    return false;
  }
  return true;
}

const openfpga::Command& ArchGenCommandParser::command() const {
  return exec_cmd_;
}

openfpga::CommandOptionId ArchGenCommandParser::vpr_arch() const {
  return opt_vpr_arch_;
}

openfpga::CommandOptionId ArchGenCommandParser::openfpga_arch() const {
  return opt_of_arch_;
}

openfpga::CommandOptionId ArchGenCommandParser::openfpga_dp_global_network() const {
  return opt_dp_gntwk_;
}

openfpga::CommandOptionId ArchGenCommandParser::openfpga_ultimate_global_network() const {
  return opt_ut_gntwk_;
}

openfpga::CommandOptionId ArchGenCommandParser::openfpga_bitstream_setting() const {
  return opt_bitset_;
}

openfpga::CommandOptionId ArchGenCommandParser::dp_ckbuf_loc_map() const {
  return opt_dp_ckbuf_loc_map_;
}

openfpga::CommandOptionId ArchGenCommandParser::ultimate_ckbuf_loc_map() const {
  return opt_ut_ckbuf_loc_map_;
}

openfpga::CommandOptionId ArchGenCommandParser::height() const {
  return opt_height_;
}

openfpga::CommandOptionId ArchGenCommandParser::width() const {
  return opt_width_;
}

openfpga::CommandOptionId ArchGenCommandParser::num_inputs() const {
  return opt_num_inputs_;
}

openfpga::CommandOptionId ArchGenCommandParser::num_outputs() const {
  return opt_num_outputs_;
}

openfpga::CommandOptionId ArchGenCommandParser::num_clks() const {
  return opt_num_clks_;
}

openfpga::CommandOptionId ArchGenCommandParser::num_rsts() const {
  return opt_num_rsts_;
}

openfpga::CommandOptionId ArchGenCommandParser::num_pclks() const {
  return opt_num_pclks_;
}

openfpga::CommandOptionId ArchGenCommandParser::num_config_regions() const {
  return opt_num_config_regions_;
}

openfpga::CommandOptionId ArchGenCommandParser::sync_dp_dimension() const {
  return opt_sync_dp_dimension_;
}

void ArchGenCommandParser::init_options() {
  opt_vpr_arch_ = exec_cmd_.add_option("vpr_arch", true, "Specify the path to VPR architecture file"); 
  exec_cmd_.set_option_require_value(opt_vpr_arch_, openfpga::OPT_STRING);
  
  opt_of_arch_ = exec_cmd_.add_option("openfpga_arch", true, "Specify the path to OpenFPGA architecture file"); 
  exec_cmd_.set_option_require_value(opt_of_arch_, openfpga::OPT_STRING);
  
  opt_dp_gntwk_ = exec_cmd_.add_option("openfpga_dp_global_network", true, "Specify the path to OpenFPGA global network file for design-planning fabric"); 
  exec_cmd_.set_option_require_value(opt_dp_gntwk_, openfpga::OPT_STRING);
  
  opt_ut_gntwk_ = exec_cmd_.add_option("openfpga_ultimate_global_network", true, "Specify the path to OpenFPGA global network file for ultimate fabric"); 
  exec_cmd_.set_option_require_value(opt_ut_gntwk_, openfpga::OPT_STRING);
  
  opt_bitset_ = exec_cmd_.add_option("openfpga_bitstream_setting", true, "Specify the path to OpenFPGA bitstream setting file"); 
  exec_cmd_.set_option_require_value(opt_bitset_, openfpga::OPT_STRING);
  
  opt_dp_ckbuf_loc_map_ = exec_cmd_.add_option("dp_ckbuf_loc_map", true, "Specify the path to clock location buf map file for design-planning fabric"); 
  exec_cmd_.set_option_require_value(opt_dp_ckbuf_loc_map_, openfpga::OPT_STRING);
  
  opt_ut_ckbuf_loc_map_ = exec_cmd_.add_option("ultimate_ckbuf_loc_map", true, "Specify the path to clock location buf map file for ultimate fabric"); 
  exec_cmd_.set_option_require_value(opt_ut_ckbuf_loc_map_, openfpga::OPT_STRING);

  opt_height_ = exec_cmd_.add_option("height", true, "Specify the height of the core fabric, excluding I/Os"); 
  exec_cmd_.set_option_require_value(opt_height_, openfpga::OPT_STRING);
  
  opt_width_ = exec_cmd_.add_option("width", true, "Specify the width of the core fabric, excluding I/Os"); 
  exec_cmd_.set_option_require_value(opt_width_, openfpga::OPT_STRING);
  
  opt_num_inputs_ = exec_cmd_.add_option("num_inputs", false, "Specify the number of inputs for each side. For example, top:2,right:3,bottom:4,left:5"); 
  exec_cmd_.set_option_require_value(opt_num_inputs_, openfpga::OPT_STRING);
  
  opt_num_outputs_ = exec_cmd_.add_option("num_outputs", false, "Specify the number of outputs for each side. For example, top:2,right:3,bottom:4,left:5"); 
  exec_cmd_.set_option_require_value(opt_num_outputs_, openfpga::OPT_STRING);
  
  opt_num_clks_ = exec_cmd_.add_option("num_clks", true, "Specify the number of clocks for the eFPGA fabric"); 
  exec_cmd_.set_option_require_value(opt_num_clks_, openfpga::OPT_STRING);
  
  opt_num_rsts_ = exec_cmd_.add_option("num_rsts", true, "Specify the number of resets for the eFPGA fabric"); 
  exec_cmd_.set_option_require_value(opt_num_rsts_, openfpga::OPT_STRING);
  
  opt_num_pclks_ = exec_cmd_.add_option("num_pclks", true, "Specify the number of programming clocks for the eFPGA fabric"); 
  exec_cmd_.set_option_require_value(opt_num_pclks_, openfpga::OPT_STRING);
  
  opt_num_config_regions_ = exec_cmd_.add_option("num_config_regions", true, "Specify the number of configuration regions for the eFPGA fabric"); 
  exec_cmd_.set_option_require_value(opt_num_config_regions_, openfpga::OPT_STRING);
  
  opt_sync_dp_dimension_ = exec_cmd_.add_option("sync_dp_dimension", false, "Force the dp fabric has the same dimension as the ultimate fabric"); 
}

} // end namespace arcxpress
