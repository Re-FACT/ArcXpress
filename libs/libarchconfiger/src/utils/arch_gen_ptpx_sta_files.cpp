#include "arch_gen_ptpx_sta_files.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "arch_gen.h"
#include "command_exit_codes.h"
#include "global_port_loc_map.h"
#include "openfpga_naming.h"
#include "pin_constraints.h"
#include "read_xml_clk_loc_map.h"
#include "read_xml_pin_constraints.h"

namespace arkangel {

template <typename T>
std::string to_string_with_precision(const T& value, const int precision = 6) {
  std::ostringstream out;
  out.precision(precision);
  out << std::fixed << value;
  return out.str();
}
int generate_disable_config(const std::string& disable_config_file_path) {
  std::vector<std::string> disable_timing_ports;
  arkangel::ArchGen arch_gen;
  std::string circuit_mode_port_attr_prefix = arch_gen.circuit_mode_port_attr_prefix();
  disable_timing_ports.push_back(
    openfpga::generate_sram_port_name(e_config_protocol_type::CONFIG_MEM_SCAN_CHAIN,
                                      e_circuit_model_port_type::CIRCUIT_MODEL_PORT_OUTPUT));

  disable_timing_ports.push_back(
    openfpga::generate_sram_port_name(e_config_protocol_type::CONFIG_MEM_SCAN_CHAIN,
                                      e_circuit_model_port_type::CIRCUIT_MODEL_PORT_INPUT));

  std::ofstream ofs(disable_config_file_path);
  if (!ofs.is_open()) {
    VTR_LOG_ERROR("Failed to open disable_config.tcl file: ", disable_config_file_path.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  for (auto port : disable_timing_ports) {
    ofs << "set_disable_timing [get_ports " << port << "*] \n";
  }

  ofs << "set_case_analysis 1 [get_ports " << circuit_mode_port_attr_prefix << "] \n";
  return openfpga::CMD_EXEC_SUCCESS;
}

int generate_constrain_io_file(const std::string& constran_io_file_path,
                               const std::string& final_gclk_pcf,
                               const std::string& clock_loc_map_file,
                               const std::string& clock_period, const bool& verbose) {
  std::string clock_name = DEFAULT_VIRTUAL_CLOCK4STA;
  GlobalPortLoc global_port_loc;
  std::string pin_name = "";
  std::string half_period = to_string_with_precision(std::stod(clock_period) / 2.0, 6);
  if (set_up_global_ports(clock_loc_map_file, global_port_loc) != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to get global ports from clock location map file \n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Reading PCF file from: %s\n", final_gclk_pcf.c_str());
  if (!std::filesystem::exists(final_gclk_pcf)) {
    VTR_LOG_ERROR("PCF file does not exist: %s\n", final_gclk_pcf.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  PinConstraints final_gclk_constraint = read_xml_pin_constraints(final_gclk_pcf.c_str());

  std::ofstream ofs(constran_io_file_path);
  if (!ofs.is_open()) {
    VTR_LOG_ERROR("Failed to open constrain_io.sdc file: %s\n", constran_io_file_path.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  ofs << "set_max_delay -from \"*pinput*\" 0\n";
  ofs << "set_max_delay -to \"*poutput*\" 0\n";

  ofs << "create_clock -name " << DEFAULT_VIRTUAL_CLOCK4STA << " -period " << clock_period << "\n";

  size_t num_mapped_pins = 0;
  for (auto pin_id : final_gclk_constraint.pin_constraints()) {
    std::string net = final_gclk_constraint.net(pin_id);
    openfpga::BasicPort pin = final_gclk_constraint.pin(pin_id);
    if (!final_gclk_constraint.unmapped_net(net) && global_port_loc.is_global_port_clk(pin)) {
      clock_name = net;
      /* Ensure the pin is single-bit. As PT only supports 'clk[0]' than 'clk[0:0]'. Assemble data
       * locally. TODO: This should be an API of OpenFPGA */
      if (pin.get_width() != 1) {
        VTR_LOG_ERROR(
          "The global clock/reset pin to be constrained must be a single-bit one. But see a '%s'\n",
          pin.to_verilog_string().c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      pin_name =
        pin.get_name() + std::string("[") + std::to_string(pin.get_lsb()) + std::string("]");
      num_mapped_pins++;

      if (clock_name == DEFAULT_VIRTUAL_CLOCK4STA) {
        VTR_LOG_ERROR(
          "The clock signal '%s' is a reserved one. Please name your clock signal in a different "
          "name\n",
          DEFAULT_VIRTUAL_CLOCK4STA.c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      ofs << "create_clock -name " << clock_name << " -period " << clock_period << " -waveform {0 "
          << half_period << "} "
          << "[get_ports \"" << pin_name << "\"]" << std::endl;
    }
  }
  ofs.close();
  VTR_LOGV(verbose, "Generated sdc constraints for %lu global signals\n", num_mapped_pins);
  return openfpga::CMD_EXEC_SUCCESS;
}

// Modified function signature to not depend on NetlistDevManager for this specific function
int generate_timing_gclk_strategy_file(const std::string& template_file,
                                       const std::string& output_file, int num_clocks,
                                       int num_resets) {
  VTR_LOG("DEBUG generate_timing_gclk_strategy_file: num_clocks = %d\n", num_clocks);
  VTR_LOG("DEBUG generate_timing_gclk_strategy_file: num_resets = %d\n", num_resets);

  // Open and read the template file
  std::ifstream ifs(template_file);
  if (!ifs.is_open()) {
    VTR_LOG_ERROR("Failed to open timing_gclk template file: ", template_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  // Read the entire template content
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();

  std::string result = content;

  // Replace all placeholder variables directly
  // Since we assume at least 2 clocks/resets, sweep variables will always be created
  // The end value for sweep is (num_ports - 1) to create range 0..N-1

  // Replace clock count placeholder
  std::string num_clocks_str = std::to_string(num_clocks - 1);  // end = num_clocks - 1
  size_t pos;
  while ((pos = result.find("[NUM_CLOCK]")) != std::string::npos) {
    result.replace(pos, std::string("[NUM_CLOCK]").length(), num_clocks_str);
  }

  // Replace reset count placeholder
  std::string num_resets_str = std::to_string(num_resets - 1);  // end = num_resets - 1
  while ((pos = result.find("[NUM_RESET]")) != std::string::npos) {
    result.replace(pos, std::string("[NUM_RESET]").length(), num_resets_str);
  }

  // Write the modified content to output file
  std::ofstream ofs(output_file);
  if (!ofs.is_open()) {
    VTR_LOG_ERROR("Failed to open output timing_gclk strategy file: ", output_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  ofs << result;
  ofs.close();

  // Log generation summary
  VTR_LOG("Generated strategy file: %s\n", output_file.c_str());
  VTR_LOG("  - Clock sweep: 0 to %d\n", num_clocks - 1);
  VTR_LOG("  - Reset sweep: 0 to %d\n", num_resets - 1);

  return openfpga::CMD_EXEC_SUCCESS;
}

// New function to update timing_gclk strategy file in-place (overwrites original)
int update_timing_gclk_strategy_file(const std::string& template_file, int num_clocks,
                                     int num_resets) {
  VTR_LOG("DEBUG update_timing_gclk_strategy_file: num_clocks = %d\n", num_clocks);
  VTR_LOG("DEBUG update_timing_gclk_strategy_file: num_resets = %d\n", num_resets);

  // Open and read the template file
  std::ifstream ifs(template_file);
  if (!ifs.is_open()) {
    VTR_LOG_ERROR("Failed to open timing_gclk template file: %s\n", template_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  // Read the entire template content
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();

  std::string result = content;

  // Replace all placeholder variables directly
  // Replace clock count placeholder
  std::string num_clocks_str = std::to_string(num_clocks - 1);  // end = num_clocks - 1
  size_t pos;
  while ((pos = result.find("[NUM_CLOCK]")) != std::string::npos) {
    result.replace(pos, std::string("[NUM_CLOCK]").length(), num_clocks_str);
  }

  // Replace reset count placeholder
  std::string num_resets_str = std::to_string(num_resets - 1);  // end = num_resets - 1
  while ((pos = result.find("[NUM_RESET]")) != std::string::npos) {
    result.replace(pos, std::string("[NUM_RESET]").length(), num_resets_str);
  }

  // Write the modified content back to the original file (overwrite)
  std::ofstream ofs(template_file);
  if (!ofs.is_open()) {
    VTR_LOG_ERROR("Failed to write to timing_gclk strategy file: %s\n", template_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  ofs << result;
  ofs.close();

  VTR_LOG("Updated strategy file in-place: %s\n", template_file.c_str());
  VTR_LOG("  - Clock sweep: 0 to %d\n", num_clocks - 1);
  VTR_LOG("  - Reset sweep: 0 to %d\n", num_resets - 1);

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arkangel