#include "fracff_gen.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace SofaHD {  // Begin namespace SofaHD

FracffGen::FracffGen(const std::string& name) {
  vpr_physical_model_name_ = name;
  /* Constants */
  CIRCUIT_MODEL_NAME_ = "fracff_wrapper";
}

std::string FracffGen::vpr_model_name(const bool& scan_capable) const {
  return scan_capable ? std::string("scff") : std::string("latch")
}

arcxpress::VprArchPrimitive FracffGen::vpr_arch_primitive(const bool& scan_capable) const {
  arcxpress::VprArchPrimitive pmtv(vpr_model_name(scan_capable));
  pmtv.set_blif_model(vpr_model_name(scan_capable));
  /* input ports */
  pmtv.add_input_port("D", 1);
  pmtv.add_input_port("clk", 1, true);
  pmtv.set_input_require_clock_port("D", "clk");
  if (scan_capable) {
    pmtv.add_input_port("DI", 1);
  }
  /* output ports */
  pmtv.add_output_port("Q", 1);
  pmtv.set_output_require_clock_port("Q", "C");
  return pmtv;
}

std::vector<arcxpress::VprArchPrimitive> FracffGen::vpr_arch_primitives(
  const bool& include_physical) const {
  std::vector<arcxpress::VprArchPrimitive> ret;

  ret.push_back(vpr_arch_primitive(true));
  ret.push_back(vpr_arch_primitive(false));
  if (include_physical) {
    ret.push_back(vpr_arch_physical_primitive());
  }

  return ret;
}

std::vector<std::string> FracffGen::vpr_model_names(const bool& include_physical) const {
  std::vector<std::string> ret;
  ret.push_back(vpr_model_name(true));
  ret.push_back(vpr_model_name(false));
  if (include_physical) {
    ret.push_back(vpr_physical_model_name());
  }
  return ret;
}

std::string FracffGen::vpr_physical_model_name() const {
  return std::string(vpr_physical_model_name_);
}

std::string FracffGen::circuit_model_name() const { return std::string(CIRCUIT_MODEL_NAME_); }

arcxpress::VprArchPrimitive FracffGen::vpr_arch_physical_primitive() const {
  arcxpress::VprArchPrimitive pmtv(vpr_physical_model_name());
  int num_err = 0;
  num_err += pmtv.set_blif_model(vpr_physical_model_name()) ? 0 : 1;
  num_err += pmtv.set_circuit_model(circuit_model_name()) ? 0 : 1;
  num_err += pmtv.set_circuit_model_type(arcxpress::VprArchPrimitive::e_model_type::FF) ? 0 : 1;
  num_err += pmtv.set_physical_model() ? 0 : 1;
  num_err += pmtv.add_input_port("D", 1) ? 0 : 1;
  num_err += pmtv.set_input_port_lib_name("D", "d_i") ? 0 : 1;
  num_err += pmtv.add_input_port("DI", 1) ? 0 : 1;
  num_err += pmtv.set_input_port_lib_name("DI", "di_i") ? 0 : 1;
  num_err += pmtv.add_input_port("clk", 1, true) ? 0 : 1;
  num_err += pmtv.set_input_port_lib_name("clk", "clk_i") ? 0 : 1;
  num_err += pmtv.add_output_port("Q", 1) ? 0 : 1;
  num_err += pmtv.set_output_port_lib_name("Q", "q_o") ? 0 : 1;
  num_err += pmtv.set_input_require_clock_port("D", "C") ? 0 : 1;
  num_err += pmtv.set_input_require_clock_port("DI", "C") ? 0 : 1;
  num_err += pmtv.set_output_require_clock_port("Q", "C") ? 0 : 1;
  if (num_err) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return pmtv;
}

}  // namespace SofaHD
