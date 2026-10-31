#include "fracff_gen.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace MOSOFA {  // Begin namespace MOSOFA

FracffGen::FracffGen(const std::string& name) {
  vpr_physical_model_name_ = name;
  CIRCUIT_MODEL_NAME_ = "fracff_wrapper";
  /* Constants */
  CLK_TYPE_STRING_ = {"", "n"};
  CLR_FUNC_STRING_ = {"", "r", "s"};
  CLR_TYPE_STRING_ = {"s", ""};
  CLR_POLARITY_STRING_ = {"n", ""};
  VPR_MODEL_PREFIX_ = "dff";

  CLK_TYPE_MODE_BITS_ = {"1", "0"};
  CLR_FUNC_MODE_BITS_ = {"0", "0", "1"};
  CLR_TYPE_MODE_BITS_ = {"0", "1"};
  CLR_POLARITY_MODE_BITS_ = {"0", "1"};
}

FracffGen::e_clk_type FracffGen::str2clk_type(const std::string& str, const bool& verbose) const {
  for (int itype = size_t(FracffGen::e_clk_type::RISING);
       itype < size_t(FracffGen::e_clk_type::NUM_TYPES); ++itype) {
    if (str == std::string(CLK_TYPE_STRING_[itype])) {
      return static_cast<FracffGen::e_clk_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for ff clock type! Expect %s\n",
                 clk_type_all2str().c_str());
  return FracffGen::e_clk_type::NUM_TYPES;
}

std::string FracffGen::clk_type2str(const FracffGen::e_clk_type& type,
                                     const bool& verbose) const {
  if (!valid_clk_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clock type! Expect %s\n",
                   clk_type_all2str().c_str());
    return std::string();
  }
  return std::string(CLK_TYPE_STRING_[size_t(type)]);
}

std::string FracffGen::clk_type2mode_bit(const FracffGen::e_clk_type& type,
                                          const bool& verbose) const {
  if (!valid_clk_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clock type! Expect %s\n",
                   clk_type_all2str().c_str());
    return std::string();
  }
  return std::string(CLK_TYPE_MODE_BITS_[size_t(type)]);
}

std::string FracffGen::clk_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(FracffGen::e_clk_type::RISING);
       itype < size_t(FracffGen::e_clk_type::NUM_TYPES); ++itype) {
    full_types += std::string(CLK_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool FracffGen::valid_clk_type(const e_clk_type& type) const {
  return size_t(type) < size_t(FracffGen::e_clk_type::NUM_TYPES);
}

FracffGen::e_clr_func FracffGen::str2clr_func(const std::string& str, const bool& verbose) const {
  for (int itype = size_t(FracffGen::e_clr_func::NONE);
       itype < size_t(FracffGen::e_clr_func::NUM_TYPES); ++itype) {
    if (str == std::string(CLR_FUNC_STRING_[itype])) {
      return static_cast<FracffGen::e_clr_func>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for ff clear functionality! Expect %s\n",
                 clr_func_all2str().c_str());
  return FracffGen::e_clr_func::NUM_TYPES;
}

std::string FracffGen::clr_func2str(const FracffGen::e_clr_func& type,
                                     const bool& verbose) const {
  if (!valid_clr_func(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clear functionality! Expect %s\n",
                   clr_func_all2str().c_str());
    return std::string();
  }
  return std::string(CLR_FUNC_STRING_[size_t(type)]);
}

std::string FracffGen::clr_func2mode_bit(const FracffGen::e_clr_func& type,
                                          const bool& verbose) const {
  if (!valid_clr_func(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clear functionality! Expect %s\n",
                   clr_func_all2str().c_str());
    return std::string();
  }
  return std::string(CLR_FUNC_MODE_BITS_[size_t(type)]);
}

std::string FracffGen::clr_func_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(FracffGen::e_clr_func::NONE);
       itype < size_t(FracffGen::e_clr_func::NUM_TYPES); ++itype) {
    full_types += std::string(CLR_FUNC_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool FracffGen::valid_clr_func(const e_clr_func& type) const {
  return size_t(type) < size_t(FracffGen::e_clr_func::NUM_TYPES);
}

FracffGen::e_clr_type FracffGen::str2clr_type(const std::string& str, const bool& verbose) const {
  for (int itype = size_t(FracffGen::e_clr_type::SYNC);
       itype < size_t(FracffGen::e_clr_type::NUM_TYPES); ++itype) {
    if (str == std::string(CLR_TYPE_STRING_[itype])) {
      return static_cast<FracffGen::e_clr_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for ff clear type! Expect %s\n",
                 clr_type_all2str().c_str());
  return FracffGen::e_clr_type::NUM_TYPES;
}

std::string FracffGen::clr_type2str(const FracffGen::e_clr_type& type,
                                     const bool& verbose) const {
  if (!valid_clr_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clear type! Expect %s\n",
                   clr_type_all2str().c_str());
    return std::string();
  }
  return std::string(CLR_TYPE_STRING_[size_t(type)]);
}

std::string FracffGen::clr_type2mode_bit(const FracffGen::e_clr_type& type,
                                          const bool& verbose) const {
  if (!valid_clr_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clear type! Expect %s\n",
                   clr_type_all2str().c_str());
    return std::string();
  }
  return std::string(CLR_TYPE_MODE_BITS_[size_t(type)]);
}

std::string FracffGen::clr_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(FracffGen::e_clr_type::SYNC);
       itype < size_t(FracffGen::e_clr_type::NUM_TYPES); ++itype) {
    full_types += std::string(CLR_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool FracffGen::valid_clr_type(const e_clr_type& type) const {
  return size_t(type) < size_t(FracffGen::e_clr_type::NUM_TYPES);
}

FracffGen::e_clr_polarity FracffGen::str2clr_polarity(const std::string& str,
                                                        const bool& verbose) const {
  for (int itype = size_t(FracffGen::e_clr_polarity::ACTIVE_LOW);
       itype < size_t(FracffGen::e_clr_polarity::NUM_TYPES); ++itype) {
    if (str == std::string(CLR_POLARITY_STRING_[itype])) {
      return static_cast<FracffGen::e_clr_polarity>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for ff clear polarity! Expect %s\n",
                 clr_polarity_all2str().c_str());
  return FracffGen::e_clr_polarity::NUM_TYPES;
}

std::string FracffGen::clr_polarity2str(const FracffGen::e_clr_polarity& type,
                                         const bool& verbose) const {
  if (!valid_clr_polarity(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clear polarity! Expect %s\n",
                   clr_polarity_all2str().c_str());
    return std::string();
  }
  return std::string(CLR_POLARITY_STRING_[size_t(type)]);
}

std::string FracffGen::clr_polarity2mode_bit(const FracffGen::e_clr_polarity& type,
                                              const bool& verbose) const {
  if (!valid_clr_polarity(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for ff clear polarity! Expect %s\n",
                   clr_polarity_all2str().c_str());
    return std::string();
  }
  return std::string(CLR_POLARITY_MODE_BITS_[size_t(type)]);
}

std::string FracffGen::clr_polarity_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(FracffGen::e_clr_polarity::ACTIVE_LOW);
       itype < size_t(FracffGen::e_clr_polarity::NUM_TYPES); ++itype) {
    full_types += std::string(CLR_POLARITY_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool FracffGen::valid_clr_polarity(const e_clr_polarity& type) const {
  return size_t(type) < size_t(FracffGen::e_clr_polarity::NUM_TYPES);
}

std::string FracffGen::vpr_model_name(const e_clk_type& ck_type, const e_clr_func& clr_func,
                                    const e_clr_type& clr_type,
                                    const e_clr_polarity& clr_pol) const {
  std::string ret = VPR_MODEL_PREFIX_;
  if (clr_func != e_clr_func::NONE) {
    ret = clr_type2str(clr_type) + ret;
  }
  ret += clk_type2str(ck_type);
  if (clr_func != e_clr_func::NONE) {
    ret += clr_func2str(clr_func);
    ret += clr_polarity2str(clr_pol);
  }
  return ret;
}

arcxpress::VprArchPrimitive FracffGen::vpr_arch_primitive(const e_clk_type& ck_type,
                                                       const e_clr_func& clr_func,
                                                       const e_clr_type& clr_type,
                                                       const e_clr_polarity& clr_pol) const {
  arcxpress::VprArchPrimitive pmtv(vpr_model_name(ck_type, clr_func, clr_type, clr_pol));
  pmtv.set_blif_model(vpr_model_name(ck_type, clr_func, clr_type, clr_pol));
  pmtv.set_mode_bits(vpr_physical_model_name(), mode_bits(ck_type, clr_func, clr_type, clr_pol));
  /* input ports */
  pmtv.add_input_port("D", 1);
  pmtv.add_input_port("C", 1, true);
  pmtv.set_input_require_clock_port("D", "C");
  if (clr_func == e_clr_func::RESET) {
    std::string clr_port = std::string("R");
    if (clr_pol == e_clr_polarity::ACTIVE_LOW) {
      clr_port += std::string("N");
    }
    pmtv.add_input_port(clr_port, 1);
    pmtv.set_input_require_clock_port(clr_port, "C");
  } else if (clr_func == e_clr_func::SET) {
    std::string clr_port = std::string("S");
    if (clr_pol == e_clr_polarity::ACTIVE_LOW) {
      clr_port += std::string("N");
    }
    pmtv.add_input_port(clr_port, 1);
    pmtv.set_input_require_clock_port(clr_port, "C");
  }
  /* output ports */
  pmtv.add_output_port("Q", 1);
  pmtv.set_output_require_clock_port("Q", "C");
  return pmtv;
}

std::vector<FracffGen::e_clk_type> FracffGen::clk_types() const {
  std::vector<FracffGen::e_clk_type> ret;
  for (int ick_type = size_t(FracffGen::e_clk_type::RISING);
       ick_type < size_t(FracffGen::e_clk_type::NUM_TYPES); ++ick_type) {
    ret.push_back(static_cast<FracffGen::e_clk_type>(ick_type));
  }
  return ret;
}

std::vector<FracffGen::e_clr_type> FracffGen::clr_types() const {
  std::vector<FracffGen::e_clr_type> ret;
  for (int iclr_type = size_t(FracffGen::e_clr_type::SYNC);
       iclr_type < size_t(FracffGen::e_clr_type::NUM_TYPES); ++iclr_type) {
    ret.push_back(static_cast<FracffGen::e_clr_type>(iclr_type));
  }
  return ret;
}

std::vector<FracffGen::e_clr_func> FracffGen::clr_funcs() const {
  std::vector<FracffGen::e_clr_func> ret;
  for (int iclr_func = size_t(FracffGen::e_clr_func::NONE);
       iclr_func < size_t(FracffGen::e_clr_func::NUM_TYPES); ++iclr_func) {
    ret.push_back(static_cast<FracffGen::e_clr_func>(iclr_func));
  }
  return ret;
}

std::vector<FracffGen::e_clr_polarity> FracffGen::clr_polarities() const {
  std::vector<FracffGen::e_clr_polarity> ret;
  for (int iclr_polarity = size_t(FracffGen::e_clr_polarity::ACTIVE_LOW);
       iclr_polarity < size_t(FracffGen::e_clr_polarity::NUM_TYPES); ++iclr_polarity) {
    ret.push_back(static_cast<FracffGen::e_clr_polarity>(iclr_polarity));
  }
  return ret;
}

std::vector<arcxpress::VprArchPrimitive> FracffGen::vpr_arch_primitives(
  const bool& include_physical) const {
  std::vector<arcxpress::VprArchPrimitive> ret;

  for (e_clk_type ck_type : clk_types()) {
    /* Walk through clear func */
    for (e_clr_func clr_func : clr_funcs()) {
      if (clr_func == FracffGen::e_clr_func::NONE) {
        ret.push_back(vpr_arch_primitive(ck_type, clr_func, e_clr_type::ASYNC,
                                         e_clr_polarity::ACTIVE_HIGH));
        continue; /* no need to iterate over reset and set details */
      }
      /* Walk through clear type and polarity */
      for (e_clr_type clr_type : clr_types()) {
        for (e_clr_polarity clr_polarity : clr_polarities()) {
          ret.push_back(vpr_arch_primitive(ck_type, clr_func, clr_type, clr_polarity));
        }
      }
    }
  }
  if (include_physical) {
    ret.push_back(vpr_arch_physical_primitive());
  }

  return ret;
}

std::vector<std::string> FracffGen::vpr_model_names(const bool& include_physical) const {
  std::vector<std::string> ret;
  for (e_clk_type ck_type : clk_types()) {
    /* Walk through clear func */
    for (e_clr_func clr_func : clr_funcs()) {
      if (clr_func == FracffGen::e_clr_func::NONE) {
        ret.push_back(
          vpr_model_name(ck_type, clr_func, e_clr_type::ASYNC, e_clr_polarity::ACTIVE_HIGH));
        continue; /* no need to iterate over reset and set details */
      }
      /* Walk through clear type and polarity */
      for (e_clr_type clr_type : clr_types()) {
        for (e_clr_polarity clr_polarity : clr_polarities()) {
          ret.push_back(vpr_model_name(ck_type, clr_func, clr_type, clr_polarity));
        }
      }
    }
  }
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
  num_err += pmtv.set_mode_bits(vpr_physical_model_name(), default_mode_bits()) ? 0 : 1;
  num_err += pmtv.add_input_port("D", 1) ? 0 : 1;
  num_err += pmtv.set_input_port_lib_name("D", "d_i") ? 0 : 1;
  num_err += pmtv.add_input_port("R", 1) ? 0 : 1;
  num_err += pmtv.set_input_port_lib_name("R", "clr_i") ? 0 : 1;
  num_err += pmtv.add_input_port("C", 1, true) ? 0 : 1;
  num_err += pmtv.set_input_port_lib_name("C", "clk_i") ? 0 : 1;
  num_err += pmtv.add_output_port("Q", 1) ? 0 : 1;
  num_err += pmtv.set_output_port_lib_name("Q", "q_o") ? 0 : 1;
  num_err += pmtv.set_input_require_clock_port("D", "C") ? 0 : 1;
  num_err += pmtv.set_input_require_clock_port("R", "C") ? 0 : 1;
  num_err += pmtv.set_output_require_clock_port("Q", "C") ? 0 : 1;
  if (num_err) {
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return pmtv;
}

std::string FracffGen::default_mode_bits() const {
  return mode_bits(e_clk_type::RISING, e_clr_func::RESET, e_clr_type::ASYNC,
                   e_clr_polarity::ACTIVE_HIGH);
}

std::string FracffGen::mode_bits(const e_clk_type& ck_type, const e_clr_func& clr_func,
                               const e_clr_type& clr_type,
                               const e_clr_polarity& clr_pol) const {
  std::string mode_bits;
  mode_bits += clr_polarity2mode_bit(clr_pol);
  mode_bits += clr_func2mode_bit(clr_func);
  mode_bits += clr_type2mode_bit(clr_type);
  mode_bits += clk_type2mode_bit(ck_type);
  return mode_bits;
}

}  // namespace MOSOFA
