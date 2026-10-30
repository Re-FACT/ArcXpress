#include "arch_gen_command_interface.h"
#include "command_parser.h"
#include "command_echo.h"
#include "command_exit_codes.h"
#include "format_arg.h"
#include "vtr_log.h"
#include "side_manager.h"
#include "openfpga_tokenizer.h"
#include "arch_io_constraints.h"
#include "write_xml_vpr_arch_template.h"
#include "write_xml_openfpga_arch_template.h"
#include "write_xml_openfpga_gntwk.h"
#include "write_xml_openfpga_bitstream_setting.h"
#include "write_xml_arch_ckbuf_loc_map.h"

namespace arcxpress { // begin namespace arcxpress

bool ArchGenCommandInterface::parse(const std::string& cmd_name, int argc, char** argv) {
  openfpga::Command exec_cmd(cmd_name.c_str());
  opt_vpr_arch = exec_cmd.add_option("vpr_arch", true, "Specify the path to VPR architecture file"); 
  exec_cmd.set_option_require_value(opt_vpr_arch, openfpga::OPT_STRING);
  
  opt_of_arch = exec_cmd.add_option("openfpga_arch", true, "Specify the path to OpenFPGA architecture file"); 
  exec_cmd.set_option_require_value(opt_of_arch, openfpga::OPT_STRING);
  
  opt_dp_gntwk = exec_cmd.add_option("openfpga_dp_global_network", true, "Specify the path to OpenFPGA global network file for design-planning fabric"); 
  exec_cmd.set_option_require_value(opt_dp_gntwk, openfpga::OPT_STRING);
  
  opt_ut_gntwk = exec_cmd.add_option("openfpga_ultimate_global_network", true, "Specify the path to OpenFPGA global network file for ultimate fabric"); 
  exec_cmd.set_option_require_value(opt_ut_gntwk, openfpga::OPT_STRING);
  
  opt_bitset = exec_cmd.add_option("openfpga_bitstream_setting", true, "Specify the path to OpenFPGA bitstream setting file"); 
  exec_cmd.set_option_require_value(opt_bitset, openfpga::OPT_STRING);
  
  opt_dp_ckbuf_loc_map = exec_cmd.add_option("dp_ckbuf_loc_map", true, "Specify the path to clock location buf map file for design-planning fabric"); 
  exec_cmd.set_option_require_value(opt_dp_ckbuf_loc_map, openfpga::OPT_STRING);
  
  opt_ut_ckbuf_loc_map = exec_cmd.add_option("ultimate_ckbuf_loc_map", true, "Specify the path to clock location buf map file for ultimate fabric"); 
  exec_cmd.set_option_require_value(opt_ut_ckbuf_loc_map_, openfpga::OPT_STRING);

  opt_height = exec_cmd.add_option("height", true, "Specify the height of the core fabric, excluding I/Os"); 
  exec_cmd.set_option_require_value(opt_height, openfpga::OPT_STRING);
  
  opt_width = exec_cmd.add_option("width", true, "Specify the width of the core fabric, excluding I/Os"); 
  exec_cmd.set_option_require_value(opt_width, openfpga::OPT_STRING);
  
  opt_num_inputs = exec_cmd.add_option("num_inputs", false, "Specify the number of inputs for each side. For example, top:2,right:3,bottom:4,left:5"); 
  exec_cmd.set_option_require_value(opt_num_inputs, openfpga::OPT_STRING);
  
  opt_num_outputs = exec_cmd.add_option("num_outputs", false, "Specify the number of outputs for each side. For example, top:2,right:3,bottom:4,left:5"); 
  exec_cmd.set_option_require_value(opt_num_outputs, openfpga::OPT_STRING);
  
  opt_num_clks = exec_cmd.add_option("num_clks", true, "Specify the number of clocks for the eFPGA fabric"); 
  exec_cmd.set_option_require_value(opt_num_clks, openfpga::OPT_STRING);
  
  opt_num_rsts = exec_cmd.add_option("num_rsts", true, "Specify the number of resets for the eFPGA fabric"); 
  exec_cmd.set_option_require_value(opt_num_rsts, openfpga::OPT_STRING);
  
  opt_num_pclks = exec_cmd.add_option("num_pclks", true, "Specify the number of programming clocks for the eFPGA fabric"); 
  exec_cmd.set_option_require_value(opt_num_pclks, openfpga::OPT_STRING);
  
  opt_num_config_regions = exec_cmd.add_option("num_config_regions", true, "Specify the number of configuration regions for the eFPGA fabric"); 
  exec_cmd.set_option_require_value(opt_num_config_regions, openfpga::OPT_STRING);
  
  opt_sync_dp_dimension = exec_cmd.add_option("sync_dp_dimension", false, "Force the dp fabric has the same dimension as the ultimate fabric"); 

  std::vector<std::string> cmd_opts = openfpga::format_argv(exec_cmd_.name(), argc, (const char**)(argv));
  openfpga::CommandContext exec_cmd_ctx(exec_cmd);
  if (!parse_command(cmd_opts, exec_cmd, exec_cmd_ctx)) {
    /* Parse fail: echo the command options */
    openfpga::print_command_options(exec_cmd);
    return false;
  }
  /* Load input parameters */
  params_.height = std::stoi(exec_cmd_ctx.option_value(exec_cmd, opt_height));
  params_.width = std::stoi(exec_cmd_ctx.option_value(exec_cmd, opt_width));
  params_.num_clks = std::stoi(exec_cmd_ctx.option_value(exec_cmd, opt_num_clks));
  params_.num_pclks = std::stoi(exec_cmd_ctx.option_value(exec_cmd, opt_num_pclks_));
  params_.num_config_regions = std::stoi(exec_cmd_ctx.option_value(exec_cmd, opt_num_config_regions));
  params_.num_rsts = std::stoi(exec_cmd_ctx.option_value(exec_cmd, opt_num_rsts));
  params_.sync_dp_dimension = exec_cmd_ctx.option_enable(exec_cmd, opt_sync_dp_dimension);

  std::map<e_side, int> num_inputs;
  if (exec_cmd_ctx_.option_enable(exec_cmd, opt_num_inputs)) {
    openfpga::StringToken num_input_tokenizer(exec_cmd_ctx.option_value(exec_cmd, opt_num_inputs));
    for (std::string str_per_side : num_input_tokenizer.split(',')) {
      openfpga::StringToken curr_tokenizer(str_per_side);
      std::vector<std::string> curr_str_side = curr_tokenizer.split(':');
      ArchIoConstraints arch_io_constr;
      e_side side_t = arch_io_constr.str2side(curr_str_side[0], true);
      int num_io = std::stoi(curr_str_side[1]);
      num_inputs[side_t] = num_io;
    }
  }
  params_.num_inputs = num_inputs;

  std::map<e_side, int> num_outputs;
  if (exec_cmd_ctx_.option_enable(exec_cmd, opt_num_outputs)) {
    openfpga::StringToken num_output_tokenizer(exec_cmd_ctx.option_value(exec_cmd, opt_num_outputs));
    for (std::string str_per_side : num_output_tokenizer.split(',')) {
      openfpga::StringToken curr_tokenizer(str_per_side);
      std::vector<std::string> curr_str_side = curr_tokenizer.split(':');
      ArchIoConstraints arch_io_constr;
      e_side side_t = arch_io_constr.str2side(curr_str_side[0], true);
      int num_io = std::stoi(curr_str_side[1]);
      num_outputs[side_t] = num_io;
    }
  }
  params_.num_outputs = num_outputs;

  /* Output file paths */
  vpr_arch_fpath_ = exec_cmd_ctx.option_value(exec_cmd, opt_vpr_arch_);
  of_arch_fpath_ = exec_cmd_ctx.option_value(exec_cmd, opt_of_arch_);
  of_dp_gntwk_fpath_ = exec_cmd_ctx.option_value(exec_cmd, opt_dp_gntwk_);
  of_ut_gntwk_fpath_ = exec_cmd_ctx.option_value(exec_cmd, opt_ut_gntwk_);
  of_bitset_fpath_ = exec_cmd_ctx.option_value(exec_cmd, opt_bitset_);
  dp_ckbuf_loc_map_fpath_ = exec_cmd_ctx.option_value(exec_cmd, opt_dp_ckbuf_loc_map_);
  ut_ckbuf_loc_map_fpath_ = exec_cmd_ctx.option_value(exec_cmd, opt_ut_ckbuf_loc_map_);

  /* Confirm options after parsing */
  VTR_LOG("eFPGA core dimension: %lu (H) x %lu (W)\n", ret.height, ret.width);
  VTR_LOG("eFPGA I/O arragements:\n");
  for (e_side curr_side : TOTAL_2D_SIDES) {
    VTR_LOG("\tSide: %s\n", SideManager(curr_side).c_str()); 
    auto in_result = num_inputs.find(curr_side);
    VTR_LOG("\t\tNum inputs:\t "); 
    if (in_result != num_inputs.end()) {
      VTR_LOG("%lu\n", in_result->second); 
    } else {
      VTR_LOG("DEFAULT\n"); 
    }  
    auto out_result = num_outputs.find(curr_side);
    VTR_LOG("\t\tNum outputs:\t "); 
    if (out_result != num_outputs.end()) {
      VTR_LOG("%lu\n", out_result->second); 
    } else {
      VTR_LOG("DEFAULT\n"); 
    }  
  }

  return true;
}

ArchGenInputParams ArchGenCommandInterface::arch_parameters() const {
  return params_;
}

int ArchGenCommandInterface::write_xml_templates(const ArchGen& arch_gen) const {
  int status = openfpga::CMD_EXEC_SUCCESS;

  /* Generate vpr arch template file */
  VTR_LOG("Write VPR arch template to an XML file: %s.\n", vpr_arch_fpath_.c_str());
  status = write_xml_vpr_arch_template(vpr_arch_fpath_.c_str(), arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate openfpga arch template file */
  VTR_LOG("Write OpenFPGA arch template to an XML file: %s.\n", of_arch_fpath_.c_str());
  status = write_xml_openfpga_arch_template(of_arch_fpath_.c_str(), arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate dp clock network */
  VTR_LOG("Write OpenFPGA global network for dp fabric to an XML file: %s.\n", of_dp_gntwk_fpath_.c_str());
  status = write_xml_openfpga_gntwk(of_dp_gntwk_fpath_.c_str(), arch_gen, VprArchLayout::e_fabric_type::DP);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate ultimate clock network */
  VTR_LOG("Write OpenFPGA global network for ultimate fabric to an XML file: %s.\n", of_ut_gntwk_fpath_.c_str());
  status = write_xml_openfpga_gntwk(of_ut_gntwk_fpath_.c_str(), arch_gen, VprArchLayout::e_fabric_type::ULTIMATE);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate bitstream settings */
  VTR_LOG("Write OpenFPGA bitstream settings to an XML file: %s.\n", of_bitset_fpath_.c_str());
  status = write_xml_openfpga_bitstream_setting(of_bitset_fpath_.c_str(), arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate dp ckbuf location map */
  VTR_LOG("Write ckbuf location map for dp fabric to an XML file: %s.\n", dp_ckbuf_loc_map_fpath_.c_str());
  status = write_xml_arch_ckbuf_loc_map(dp_ckbuf_loc_map_fpath_.c_str(), arch_gen, VprArchLayout::e_fabric_type::DP);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate ultimate ckbuf location map */
  VTR_LOG("Write ckbuf location map for ultimate fabric to an XML file: %s.\n", ut_ckbuf_loc_map_fpath_.c_str());
  status = write_xml_arch_ckbuf_loc_map(ut_ckbuf_loc_map_fpath_.c_str(), arch_gen, VprArchLayout::e_fabric_type::ULTIMATE);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return status;
}

} // end namespace arcxpress
