#include "ArchGenCommandInterface.h"
#include "command_parser.h"
#include "command_exit_codes.h"
#include "vtr_log.h"
#include "side_manager.h"
#include "openfpga_tokenizer.h"
#include "alkaid_arch_primitive.h"
#include "alkaidl_arch_layout.h"
#include "arch_io_constraints.h"
#include "write_xml_vpr_arch_template.h"
#include "write_xml_openfpga_arch_template.h"
#include "write_xml_openfpga_gntwk.h"
#include "write_xml_openfpga_bitstream_setting.h"
#include "write_xml_arch_ckbuf_loc_map.h"

namespace arcxpress { // begin namespace arcxpress

ArchGenCommandInterface::ArchGenCommandInterface(const std::string& cmd_name) : exec_cmd_(cmd_name) {
  init_options();
}

bool ArchGenCommandInterface::parse(int argc, char** argv, openfpga::CommandContext& exec_cmd_ctx) {
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

int ArchGenCommandInterface::init_primitives(arkangel::ArchGen& arch_gen) const {
  int status = alkaid::init_vpr_arch_primitives(arch_gen, false, false);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in creating primitive details\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchGenCommandInterface::init_layout(const openfpga::CommandContext& exec_cmd_ctx, 
                                          arkangel::ArchGen& arch_gen) const {
  int height = std::stoi(exec_cmd_ctx.option_value(exec_cmd_, opt_height_));
  int width = std::stoi(exec_cmd_ctx.option_value(exec_cmd_, opt_width_));
  int num_clks = std::stoi(exec_cmd_ctx.option_value(exec_cmd_, opt_num_clks_));
  int num_pclks = std::stoi(exec_cmd_ctx.option_value(exec_cmd_, opt_num_pclks_));
  int num_regions = std::stoi(exec_cmd_ctx.option_value(exec_cmd_, opt_num_config_regions_));
  int num_rsts = std::stoi(exec_cmd_ctx.option_value(exec_cmd_, opt_num_rsts_));
  bool sync_dp_dimension = exec_cmd_ctx.option_enable(exec_cmd_, opt_sync_dp_dimension_);

  std::map<e_side, int> num_inputs;
  if (exec_cmd_ctx.option_enable(exec_cmd_, opt_num_inputs_)) {
    openfpga::StringToken num_input_tokenizer(exec_cmd_ctx.option_value(exec_cmd_, opt_num_inputs_));
    for (std::string str_per_side : num_input_tokenizer.split(',')) {
      openfpga::StringToken curr_tokenizer(str_per_side);
      std::vector<std::string> curr_str_side = curr_tokenizer.split(':');
      arkangel::ArchIoConstraints arch_io_constr;
      e_side side_t = arch_io_constr.str2side(curr_str_side[0], true);
      int num_io = std::stoi(curr_str_side[1]);
      num_inputs[side_t] = num_io;
    }
  }

  std::map<e_side, int> num_outputs;
  if (exec_cmd_ctx.option_enable(exec_cmd_, opt_num_outputs_)) {
    openfpga::StringToken num_output_tokenizer(exec_cmd_ctx.option_value(exec_cmd_, opt_num_outputs_));
    for (std::string str_per_side : num_output_tokenizer.split(',')) {
      openfpga::StringToken curr_tokenizer(str_per_side);
      std::vector<std::string> curr_str_side = curr_tokenizer.split(':');
      arkangel::ArchIoConstraints arch_io_constr;
      e_side side_t = arch_io_constr.str2side(curr_str_side[0], true);
      int num_io = std::stoi(curr_str_side[1]);
      num_outputs[side_t] = num_io;
    }
  }

  /* Confirm options after parsing */
  VTR_LOG("eFPGA core dimension: %lu (H) x %lu (W)\n", height, width);
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

  int status = alkaidL::init_vpr_arch_layout(arch_gen, height, width, num_clks, num_rsts, num_inputs, num_outputs, num_pclks, num_regions, sync_dp_dimension);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed in creating layout details\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchGenCommandInterface::write_xml_templates(const openfpga::CommandContext& exec_cmd_ctx, 
                                                 arkangel::ArchGen& arch_gen) const {
  int status = openfpga::CMD_EXEC_SUCCESS;

  /* Generate vpr arch template file */
  std::string vpr_arch_fpath = exec_cmd_ctx.option_value(exec_cmd_, opt_vpr_arch_);
  VTR_LOG("Write VPR arch template to an XML file: %s.\n", vpr_arch_fpath.c_str());
  status = arkangel::write_xml_vpr_arch_template(vpr_arch_fpath.c_str(), arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate openfpga arch template file */
  std::string of_arch_fpath = exec_cmd_ctx.option_value(exec_cmd_, opt_of_arch_);
  VTR_LOG("Write OpenFPGA arch template to an XML file: %s.\n", of_arch_fpath.c_str());
  status = arkangel::write_xml_openfpga_arch_template(of_arch_fpath.c_str(), arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate dp clock network */
  std::string of_dp_gntwk_fpath = exec_cmd_ctx.option_value(exec_cmd_, opt_dp_gntwk_);
  VTR_LOG("Write OpenFPGA global network for dp fabric to an XML file: %s.\n", of_dp_gntwk_fpath.c_str());
  status = arkangel::write_xml_openfpga_gntwk(of_dp_gntwk_fpath.c_str(), arch_gen, arkangel::VprArchLayout::e_fabric_type::DP);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate ultimate clock network */
  std::string of_ut_gntwk_fpath = exec_cmd_ctx.option_value(exec_cmd_, opt_ut_gntwk_);
  VTR_LOG("Write OpenFPGA global network for ultimate fabric to an XML file: %s.\n", of_ut_gntwk_fpath.c_str());
  status = arkangel::write_xml_openfpga_gntwk(of_ut_gntwk_fpath.c_str(), arch_gen, arkangel::VprArchLayout::e_fabric_type::ULTIMATE);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate bitstream settings */
  std::string of_bitset_fpath = exec_cmd_ctx.option_value(exec_cmd_, opt_bitset_);
  VTR_LOG("Write OpenFPGA bitstream settings to an XML file: %s.\n", of_bitset_fpath.c_str());
  status = arkangel::write_xml_openfpga_bitstream_setting(of_bitset_fpath.c_str(), arch_gen);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate dp ckbuf location map */
  std::string dp_ckbuf_loc_map_fpath = exec_cmd_ctx.option_value(exec_cmd_, opt_dp_ckbuf_loc_map_);
  VTR_LOG("Write ckbuf location map for dp fabric to an XML file: %s.\n", dp_ckbuf_loc_map_fpath.c_str());
  status = arkangel::write_xml_arch_ckbuf_loc_map(dp_ckbuf_loc_map_fpath.c_str(), arch_gen, arkangel::VprArchLayout::e_fabric_type::DP);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Generate ultimate ckbuf location map */
  std::string ut_ckbuf_loc_map_fpath = exec_cmd_ctx.option_value(exec_cmd_, opt_ut_ckbuf_loc_map_);
  VTR_LOG("Write ckbuf location map for ultimate fabric to an XML file: %s.\n", ut_ckbuf_loc_map_fpath.c_str());
  status = arkangel::write_xml_arch_ckbuf_loc_map(ut_ckbuf_loc_map_fpath.c_str(), arch_gen, arkangel::VprArchLayout::e_fabric_type::ULTIMATE);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return status;
}

const openfpga::Command& ArchGenCommandInterface::command() const {
  return exec_cmd_;
}

openfpga::CommandOptionId ArchGenCommandInterface::vpr_arch() const { return opt_vpr_arch_; }
openfpga::CommandOptionId ArchGenCommandInterface::openfpga_arch() const { return opt_of_arch_; }
openfpga::CommandOptionId ArchGenCommandInterface::openfpga_dp_global_network() const { return opt_dp_gntwk_; }
openfpga::CommandOptionId ArchGenCommandInterface::openfpga_ultimate_global_network() const { return opt_ut_gntwk_; }
openfpga::CommandOptionId ArchGenCommandInterface::openfpga_bitstream_setting() const { return opt_bitset_; }
openfpga::CommandOptionId ArchGenCommandInterface::dp_ckbuf_loc_map() const { return opt_dp_ckbuf_loc_map_; }
openfpga::CommandOptionId ArchGenCommandInterface::ultimate_ckbuf_loc_map() const { return opt_ut_ckbuf_loc_map_; }
openfpga::CommandOptionId ArchGenCommandInterface::height() const { return opt_height_; }
openfpga::CommandOptionId ArchGenCommandInterface::width() const { return opt_width_; }
openfpga::CommandOptionId ArchGenCommandInterface::num_inputs() const { return opt_num_inputs_; }
openfpga::CommandOptionId ArchGenCommandInterface::num_outputs() const { return opt_num_outputs_; }
openfpga::CommandOptionId ArchGenCommandInterface::num_clks() const { return opt_num_clks_; }
openfpga::CommandOptionId ArchGenCommandInterface::num_rsts() const { return opt_num_rsts_; }
openfpga::CommandOptionId ArchGenCommandInterface::num_pclks() const { return opt_num_pclks_; }
openfpga::CommandOptionId ArchGenCommandInterface::num_config_regions() const { return opt_num_config_regions_; }
openfpga::CommandOptionId ArchGenCommandInterface::sync_dp_dimension() const { return opt_sync_dp_dimension_; }

void ArchGenCommandInterface::init_options() {
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
