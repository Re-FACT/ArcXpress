#include "vpr_arch_primitive.h"

#include <cmath>
#include <filesystem>

#include "arch_gen_circuit_model_utils.h"
#include "command_exit_codes.h"
#include "openfpga_arch_xml_constants.h"
#include "openfpga_tokenizer.h"
#include "vpr_arch_xml_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arcxpress {  // Begin namespace arcxpress

VprArchPrimitive::VprArchPrimitive(const std::string& name) {
  name_ = name;
  circuit_model_type_ = e_model_type::NUM_TYPES;
  is_physical_model_ = false;
  require_external_netlist_ = true;
}

VprArchPrimitive::e_model_type VprArchPrimitive::circuit_model_type() const {
  return circuit_model_type_;
}

int VprArchPrimitive::vpr_model_xml_node(pugi::xml_node& models_node) const {
  pugi::xml_node model_node = models_node.append_child(XML_VPR_ARCH_MODELS_MODEL_NODE_NAME);
  model_node.append_attribute(XML_VPR_ARCH_MODELS_MODEL_ATTR_NAME) = blif_model_.c_str();
  if (!input_ports_.empty()) {
    pugi::xml_node i_port_node =
      model_node.append_child(XML_VPR_ARCH_MODELS_MODEL_INPUT_PORTS_NODE_NAME);
    for (size_t iport = 0; iport < input_ports_.size(); iport++) {
      pugi::xml_node curr_port_node =
        i_port_node.append_child(XML_VPR_ARCH_MODELS_MODEL_PORT_NODE_NAME);
      curr_port_node.append_attribute(XML_VPR_ARCH_MODELS_MODEL_PORT_ATTR_NAME) =
        input_ports_[iport].c_str();
      if (!input_comb_sink_ports_[iport].empty()) {
        std::string sink_str;
        for (auto isink_port : input_comb_sink_ports_[iport]) {
          sink_str += output_ports_[isink_port] + std::string(" ");
        }
        sink_str.pop_back();
        curr_port_node.append_attribute(XML_VPR_ARCH_MODELS_MODEL_PORT_ATTR_COMB_SINK_PORT) =
          sink_str.c_str();
      }
      if (valid_input_id(input_seq_src_ports_[iport])) {
        curr_port_node.append_attribute(XML_VPR_ARCH_MODELS_MODEL_PORT_ATTR_CLOCK) =
          input_ports_[input_seq_src_ports_[iport]].c_str();
      }
      if (input_is_clock_[iport]) {
        curr_port_node.append_attribute(XML_VPR_ARCH_MODELS_MODEL_PORT_ATTR_IS_CLOCK) = "1";
      }
    }
  }
  if (!output_ports_.empty()) {
    pugi::xml_node o_port_node =
      model_node.append_child(XML_VPR_ARCH_MODELS_MODEL_OUTPUT_PORTS_NODE_NAME);
    for (size_t iport = 0; iport < output_ports_.size(); iport++) {
      pugi::xml_node curr_port_node =
        o_port_node.append_child(XML_VPR_ARCH_MODELS_MODEL_PORT_NODE_NAME);
      curr_port_node.append_attribute(XML_VPR_ARCH_MODELS_MODEL_PORT_ATTR_NAME) =
        output_ports_[iport].c_str();
      if (valid_input_id(output_seq_src_ports_[iport])) {
        curr_port_node.append_attribute(XML_VPR_ARCH_MODELS_MODEL_PORT_ATTR_CLOCK) =
          input_ports_[output_seq_src_ports_[iport]].c_str();
      }
    }
  }
  return 0;
}

bool VprArchPrimitive::set_blif_model(const std::string& val) {
  blif_model_ = val;
  return true;
}

bool VprArchPrimitive::set_mode_bits(const std::string& tag, const std::string& val) {
  mode_bits_[tag] = val;
  return true;
}

bool VprArchPrimitive::add_input_port(const std::string& name, const size_t& width,
                                      const bool& is_clock) {
  auto result_i = input_name2ids_.find(name);
  if (result_i != input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' already exists. Ports in the same name is not allowed!\n",
                  name.c_str());
    return false;
  }
  auto result_o = output_name2ids_.find(name);
  if (result_o != output_name2ids_.end()) {
    VTR_LOG_ERROR(
      "The output port in the same name '%s' already exists. Ports in the same name is not "
      "allowed!\n",
      name.c_str());
    return false;
  }
  size_t new_id = input_ports_.size();
  input_ports_.push_back(name);
  input_lib_ports_.push_back(name);
  input_port_width_.push_back(width);
  input_is_clock_.push_back(is_clock);
  input_comb_sink_ports_.emplace_back();
  input_seq_src_ports_.push_back(-1);
  input_only_in_circuit_model_.push_back(false);
  input_is_global_.push_back(false);
  input_is_data_io_.push_back(false);
  input_is_io_.push_back(false);
  input_default_val_.push_back(0);
  input_name2ids_[name] = new_id;
  return true;
}

bool VprArchPrimitive::add_input_circuit_port(const std::string& name, const size_t& width,
                                              const std::string& lib_name) {
  if (!add_input_port(name, width)) {
    return false;
  }
  auto result_i = input_name2ids_.find(name);
  if (result_i == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  input_lib_ports_[port_id] = lib_name;
  input_only_in_circuit_model_[port_id] = true;

  return true;
}

bool VprArchPrimitive::add_global_io_input_circuit_port(const std::string& name,
                                                        const size_t& width,
                                                        const std::string& lib_name,
                                                        const size_t& default_val) {
  if (!add_input_port(name, width)) {
    return false;
  }
  auto result_i = input_name2ids_.find(name);
  if (result_i == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  input_lib_ports_[port_id] = lib_name;
  input_only_in_circuit_model_[port_id] = true;
  input_is_global_[port_id] = true;
  input_is_io_[port_id] = true;
  input_default_val_[port_id] = default_val;

  return true;
}

bool VprArchPrimitive::add_global_input_circuit_port(const std::string& name, const size_t& width,
                                                     const std::string& lib_name,
                                                     const size_t& default_val) {
  if (!add_input_port(name, width)) {
    return false;
  }
  auto result_i = input_name2ids_.find(name);
  if (result_i == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  input_lib_ports_[port_id] = lib_name;
  input_only_in_circuit_model_[port_id] = true;
  input_is_global_[port_id] = true;
  input_default_val_[port_id] = default_val;

  return true;
}

bool VprArchPrimitive::add_output_port(const std::string& name, const size_t& width) {
  auto result_i = input_name2ids_.find(name);
  if (result_i != input_name2ids_.end()) {
    VTR_LOG_ERROR(
      "The input port in the same name '%s' already exists. Ports in the same name is not "
      "allowed!\n",
      name.c_str());
    return false;
  }
  auto result_o = output_name2ids_.find(name);
  if (result_o != output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' already exists. Ports in the same name is not allowed!\n",
                  name.c_str());
    return false;
  }
  size_t new_id = output_ports_.size();
  output_ports_.push_back(name);
  output_lib_ports_.push_back(name);
  output_port_width_.push_back(width);
  output_seq_src_ports_.emplace_back(-1);
  output_only_in_circuit_model_.push_back(false);
  output_port_frac_level_.push_back(0);
  output_is_global_.push_back(false);
  output_is_data_io_.push_back(false);
  output_is_io_.push_back(false);
  output_name2ids_[name] = new_id;
  return true;
}

bool VprArchPrimitive::add_output_circuit_port(const std::string& name, const size_t& width,
                                               const std::string& lib_name) {
  if (!add_output_port(name, width)) {
    return false;
  }
  auto result_i = output_name2ids_.find(name);
  if (result_i == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  output_lib_ports_[port_id] = lib_name;
  output_only_in_circuit_model_[port_id] = true;

  return true;
}

bool VprArchPrimitive::add_global_output_circuit_port(const std::string& name, const size_t& width,
                                                      const std::string& lib_name) {
  if (!add_output_port(name, width)) {
    return false;
  }
  auto result_i = output_name2ids_.find(name);
  if (result_i == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  output_lib_ports_[port_id] = lib_name;
  output_only_in_circuit_model_[port_id] = true;
  output_is_global_[port_id] = true;

  return true;
}

bool VprArchPrimitive::add_input_comb_sink_port(const std::string& name,
                                                const std::string& s_port) {
  auto result_i = input_name2ids_.find(name);
  if (result_i == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' does not exists!\n", name.c_str());
    return false;
  }
  size_t input_id = result_i->second;

  auto result_o = output_name2ids_.find(s_port);
  if (result_o == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' does not exists!\n", s_port.c_str());
    return false;
  }
  size_t output_id = result_o->second;
  /* Do not allow duplicated ports */
  for (size_t iport : input_comb_sink_ports_[input_id]) {
    if (iport == output_id) {
      VTR_LOG_ERROR(
        "The output port '%s' is already defined as one of the combinational sink for input port "
        "'%s'!\n",
        s_port.c_str(), name.c_str());
      return false;
    }
  }
  input_comb_sink_ports_[input_id].push_back(output_id);
  return true;
}

bool VprArchPrimitive::set_input_require_clock_port(const std::string& name,
                                                    const std::string& ck_port) {
  auto result_i = input_name2ids_.find(name);
  if (result_i == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' does not exists!\n", name.c_str());
    return false;
  }
  size_t input_id = result_i->second;

  auto result_ck = input_name2ids_.find(ck_port);
  if (result_ck == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' does not exists!\n", ck_port.c_str());
    return false;
  }
  size_t ck_id = result_ck->second;
  if (!input_is_clock_[ck_id]) {
    VTR_LOG_ERROR("The input port '%s' is not a clock!\n", ck_port.c_str());
    return false;
  }
  input_seq_src_ports_[input_id] = ck_id;
  return true;
}

bool VprArchPrimitive::set_output_require_clock_port(const std::string& name,
                                                     const std::string& ck_port) {
  auto result_o = output_name2ids_.find(name);
  if (result_o == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' does not exists!\n", name.c_str());
    return false;
  }
  size_t output_id = result_o->second;

  auto result_ck = input_name2ids_.find(ck_port);
  if (result_ck == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' does not exists!\n", ck_port.c_str());
    return false;
  }
  size_t ck_id = result_ck->second;
  if (!input_is_clock_[ck_id]) {
    VTR_LOG_ERROR("The input port '%s' is not a clock!\n", ck_port.c_str());
    return false;
  }
  output_seq_src_ports_[output_id] = ck_id;
  return true;
}

bool VprArchPrimitive::valid_input_id(const size_t& port_id) const {
  return port_id < input_ports_.size();
}

std::string VprArchPrimitive::name() const { return name_; }
std::string VprArchPrimitive::blif_model() const { return blif_model_; }
std::string VprArchPrimitive::circuit_model_name() const { return circuit_model_; }
std::string VprArchPrimitive::mode_bits(const std::string& tag) const {
  auto result = mode_bits_.find(tag);
  if (result == mode_bits_.end()) {
    VTR_LOG_ERROR("The tag '%s' for mode bits of primitive '%s' is not defined!\n", tag.c_str(),
                  name().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return result->second;
}

void VprArchPrimitive::check_valid_mode_bit_tag(const std::string& tag) const {
  auto result = mode_bits_.find(tag);
  if (result == mode_bits_.end()) {
    VTR_LOG_ERROR("The tag '%s' for mode bits of primitive '%s' is not defined!\n", tag.c_str(),
                  name().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

bool VprArchPrimitive::contain_mode_bits() const { return !mode_bits_.empty(); }
std::string VprArchPrimitive::pb_type_name() const {
  if (contain_mode_bits()) {
    VTR_LOG_ERROR(
      "The primitive '%s' require mode bits. Please provide a tag when access pb_type name!\n",
      name().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return name_ + std::string("_core");
}

std::vector<std::string> VprArchPrimitive::mode_bit_tags() const {
  std::vector<std::string> ret;
  for (auto pair : mode_bits_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::vector<std::string> VprArchPrimitive::pb_type_names() const {
  std::vector<std::string> ret;
  if (!contain_mode_bits()) {
    ret.push_back(pb_type_name());
  } else {
    for (auto pair : mode_bits_) {
      ret.push_back(pair.first + std::string("_") + name_);
    }
  }
  return ret;
}

std::string VprArchPrimitive::pb_type_name(const std::string& tag) const {
  check_valid_mode_bit_tag(tag);
  return tag + std::string("_") + name_;
}

std::vector<VprArchPrimitive::PortId> VprArchPrimitive::ports() const {
  std::vector<PortId> ret;
  for (size_t i = 0; i < input_ports_.size(); ++i) {
    if (input_only_in_circuit_model_[i]) {
      continue;
    }
    ret.push_back(PortId(i));
  }
  for (size_t i = 0; i < output_ports_.size(); ++i) {
    if (output_only_in_circuit_model_[i]) {
      continue;
    }
    ret.push_back(PortId(i + input_ports_.size()));
  }
  return ret;
}

std::vector<VprArchPrimitive::PortId> VprArchPrimitive::circuit_ports() const {
  std::vector<PortId> ret;
  for (size_t i = 0; i < input_ports_.size(); ++i) {
    ret.push_back(PortId(i));
  }
  for (size_t i = 0; i < output_ports_.size(); ++i) {
    ret.push_back(PortId(i + input_ports_.size()));
  }
  return ret;
}

std::vector<VprArchPrimitive::PortId> VprArchPrimitive::input_circuit_ports() const {
  std::vector<PortId> ret;
  for (size_t i = 0; i < input_ports_.size(); ++i) {
    ret.push_back(PortId(i));
  }
  return ret;
}

std::vector<VprArchPrimitive::PortId> VprArchPrimitive::output_circuit_ports() const {
  std::vector<PortId> ret;
  for (size_t i = 0; i < output_ports_.size(); ++i) {
    ret.push_back(PortId(i + input_ports_.size()));
  }
  return ret;
}

openfpga::BasicPort VprArchPrimitive::port(const VprArchPrimitive::PortId& port_id) const {
  openfpga::BasicPort ret;
  if (size_t(port_id) < input_ports_.size()) {
    ret.set_name(input_ports_[size_t(port_id)]);
    ret.set_width(input_port_width_[size_t(port_id)]);
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    ret.set_name(output_ports_[size_t(port_id) - input_ports_.size()]);
    ret.set_width(output_port_width_[size_t(port_id) - input_ports_.size()]);
  } else {
    VTR_LOG_ERROR("Invalid port id '%lu' for primitive '%s' (blif_model: '%s')!\n", size_t(port_id),
                  name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret;
}

openfpga::BasicPort VprArchPrimitive::circuit_port(const VprArchPrimitive::PortId& port_id) const {
  openfpga::BasicPort ret;
  if (size_t(port_id) < input_ports_.size()) {
    ret.set_name(input_lib_ports_[size_t(port_id)]);
    ret.set_width(input_port_width_[size_t(port_id)]);
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    ret.set_name(output_lib_ports_[size_t(port_id) - input_ports_.size()]);
    ret.set_width(output_port_width_[size_t(port_id) - input_ports_.size()]);
  } else {
    VTR_LOG_ERROR("Invalid port id '%lu' for primitive '%s' (blif_model: '%s')!\n", size_t(port_id),
                  name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret;
}

VprArchLayout::e_port_type VprArchPrimitive::port_type(
  const VprArchPrimitive::PortId& port_id) const {
  if (size_t(port_id) < input_ports_.size()) {
    if (input_is_clock_[size_t(port_id)]) {
      return VprArchLayout::e_port_type::CLOCK;
    } else {
      return VprArchLayout::e_port_type::INPUT;
    }
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    return VprArchLayout::e_port_type::OUTPUT;
  } else {
    VTR_LOG_ERROR("Invalid port id '%lu' for primitive '%s' (blif_model: '%s')!\n", size_t(port_id),
                  name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

bool VprArchPrimitive::require_mode_select_parameters() const {
  return !mode_select_parameters().empty();
}
std::vector<std::string> VprArchPrimitive::mode_select_parameters() const {
  std::vector<std::string> ret;
  for (auto pair : mode_select_offsets_) {
    ret.push_back(pair.first);
  }
  return ret;
}

std::string VprArchPrimitive::mode_select_parameter_key(
  const std::string& curr_pb_type_name) const {
  /* Find the mode bit which matches the pb_type name */
  if (mode_bit_tag2keys_.empty()) {
    return DEFAULT_MODE_SELECT_PARAMETER_KEY_;
  }
  if (mode_bits_.empty()) {
    return DEFAULT_MODE_SELECT_PARAMETER_KEY_;
  }
  std::string matched_tag;
  for (auto pair : mode_bits_) {
    if (curr_pb_type_name == pb_type_name(pair.first)) {
      matched_tag = pair.first;
      break;
    }
  }
  if (matched_tag.empty()) {
    VTR_LOG_ERROR("The mode bit tag that matches pb_type '%s' does not exist for primitive '%s'!\n",
                  curr_pb_type_name.c_str(), name().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return mode_bit_tag2keys_.at(matched_tag);
}

size_t VprArchPrimitive::mode_select_offset(const std::string& param,
                                            const std::string& key) const {
  auto result = mode_select_offsets_.find(param);
  if (result == mode_select_offsets_.end()) {
    VTR_LOG_ERROR("The mode-select parameter '%s' does not exist for primitive '%s'!\n",
                  param.c_str(), name().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  auto k_result = result->second.find(key);
  if (k_result == result->second.end()) {
    VTR_LOG_ERROR(
      "The key '%s' for mode-select parameter '%s' does not exist for primitive '%s'!\n",
      key.c_str(), param.c_str(), name().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return k_result->second;
}

bool VprArchPrimitive::add_mode_select_offset(const std::string& param, const size_t& offset,
                                              const std::string& key) {
  mode_select_offsets_[param][key] = offset;
  return true;
}

bool VprArchPrimitive::is_physical_model() const { return is_physical_model_; }

bool VprArchPrimitive::is_lut() const { return circuit_model_type_ == e_model_type::LUT; }

size_t VprArchPrimitive::num_mode_bits() const {
  size_t ret = 0;
  /* TODO: enhance checks on the mode bit length. All the pair.second should have the same length!!!
   */
  for (auto pair : mode_bits_) {
    /* Split with ' if possible */
    openfpga::StringToken tokenizer(pair.second);
    std::vector<std::string> tokens = tokenizer.split('\'');
    if (tokens.size() == 2) {
      ret = tokens[1].size();
    } else {
      ret = pair.second.size();
    }
  }
  return ret;
}

std::string VprArchPrimitive::circuit_model_type2str() const {
  switch (circuit_model_type_) {
    case e_model_type::LUT:
      return "lut";
    case e_model_type::FF:
      return "ff";
    case e_model_type::IO:
      return "iopad";
    case e_model_type::HARDLOGIC:
      return "hard_logic";
    default:
      VTR_LOG_ERROR("Invalid type of circuit model for pb_type '%s' (blif model '%s')!\n",
                    name_.c_str(), blif_model_.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return std::string("");
}

std::string VprArchPrimitive::circuit_port_type2str(const PortId& port_id) const {
  std::string ret;
  if (size_t(port_id) < input_ports_.size()) {
    if (input_is_clock_[size_t(port_id)]) {
      return std::string("clock");
    } else {
      return std::string("input");
    }
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    return std::string("output");
  } else {
    VTR_LOG_ERROR("Invalid port id '%lu' for primitive '%s' (blif_model: '%s')!\n", size_t(port_id),
                  name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret;
}

openfpga::BasicPort VprArchPrimitive::mode_port() const {
  openfpga::BasicPort ret("mode_i", num_mode_bits());
  return ret;
}

bool VprArchPrimitive::is_port_global_input(const PortId& port_id) const {
  if (size_t(port_id) < input_ports_.size()) {
    return input_is_global_[size_t(port_id)] && !input_is_io_[size_t(port_id)] &&
           !input_is_data_io_[size_t(port_id)];
  } else {
    VTR_LOG_ERROR("Invalid input port id '%lu' for primitive '%s' (blif_model: '%s')!\n",
                  size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return false;
}
bool VprArchPrimitive::is_port_global_output(const PortId& port_id) const {
  if (size_t(port_id) < input_ports_.size()) {
    VTR_LOG_ERROR(
      "Invalid output port id '%lu' for primitive '%s' (blif_model: '%s')! The port id belongs to "
      "an input port\n",
      size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    return output_is_global_[size_t(port_id) - input_ports_.size()] &&
           !output_is_io_[size_t(port_id) - input_ports_.size()] &&
           !output_is_data_io_[size_t(port_id) - input_ports_.size()];
  } else {
    VTR_LOG_ERROR("Invalid output port id '%lu' for primitive '%s' (blif_model: '%s')!\n",
                  size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return false;
}

bool VprArchPrimitive::is_port_data_io(const PortId& port_id) const {
  if (size_t(port_id) < input_ports_.size()) {
    return input_is_data_io_[size_t(port_id)];
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    return output_is_data_io_[size_t(port_id) - input_ports_.size()];
  } else {
    VTR_LOG_ERROR("Invalid output port id '%lu' for primitive '%s' (blif_model: '%s')!\n",
                  size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return false;
}

bool VprArchPrimitive::is_port_io(const PortId& port_id) const {
  if (size_t(port_id) < input_ports_.size()) {
    return input_is_io_[size_t(port_id)];
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    return output_is_io_[size_t(port_id) - input_ports_.size()];
  } else {
    VTR_LOG_ERROR("Invalid output port id '%lu' for primitive '%s' (blif_model: '%s')!\n",
                  size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return false;
}

size_t VprArchPrimitive::input_port_default_value(const PortId& port_id) const {
  if (size_t(port_id) < input_ports_.size()) {
    return input_default_val_[size_t(port_id)];
  } else {
    VTR_LOG_ERROR(
      "Default value is only accessible for input port but provided is an output port whose id is "
      "'%lu' for primitive '%s' (blif_model: '%s')!\n",
      size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return 0;
}

size_t VprArchPrimitive::lut_input_size() const {
  /* Only for LUTs */
  if (!is_lut()) {
    VTR_LOG_ERROR("The primitive is not a LUT!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  /* Expect only 1 input */
  if (input_ports_.size() != 1) {
    VTR_LOG_ERROR("Expect only 1 input port for the LUT model '%s' (blif_model: '%s')!\n",
                  name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return input_port_width_[0];
}

size_t VprArchPrimitive::num_srams() const { return std::pow(2, lut_input_size()); }

std::string VprArchPrimitive::inter_buf_loc_map() const {
  std::string ret(lut_input_size() - 1, '-');
  ret[lut_input_size() / 2] = '1';
  return ret;
}

std::string VprArchPrimitive::tri_state_map(const PortId& port_id) const {
  std::string ret(lut_input_size(), '-');
  ret[lut_input_size() - 1] = '1';
  return ret;
}

size_t VprArchPrimitive::output_port_frac_level(const PortId& port_id) const {
  if (size_t(port_id) < input_ports_.size()) {
    VTR_LOG_ERROR(
      "Invalid output port id '%lu' for primitive '%s' (blif_model: '%s')! It belongs to an input "
      "port\n",
      size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  } else if (size_t(port_id) < input_ports_.size() + output_ports_.size()) {
    return output_port_frac_level_[size_t(port_id) - input_ports_.size()];
  } else {
    VTR_LOG_ERROR("Invalid output port id '%lu' for primitive '%s' (blif_model: '%s')!\n",
                  size_t(port_id), name_.c_str(), blif_model_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return 0;
}

std::string VprArchPrimitive::port_output_mask(const PortId& port_id) const {
  size_t offset = lut_input_size() - output_port_frac_level(port_id);
  std::string ret;
  for (size_t i = 0; i < offset + 1; ++i) {
    ret += std::to_string(i) + std::string(",");
  }
  ret.pop_back();
  return ret;
}

int VprArchPrimitive::openfpga_circuit_model_xml_node_frac_lut(
  pugi::xml_node& clib_node, const bool& is_default, const std::string& custom_vlog_nlist) const {
  int num_err = 0;
  if (!is_lut()) {
    VTR_LOG_ERROR("The primitive is not a LUT!\n");
    return 1;
  }
  pugi::xml_node cmdl_node = clib_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) =
    circuit_model_type2str().c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = circuit_model_.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = circuit_model_.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  /* TODO: if the type is LUT, use auto-generation */
  if (require_external_netlist()) {
    cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST) =
      custom_vlog_nlist.c_str();
  }

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_FRAC_LUT) = "true";

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "true";
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME) =
      "BUFD2_HVT_WRAPPER";
  }
  { /* LUT input inverter */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LUT_INPUT_INVERTER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "true";
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME) =
      "INV_HVT_WRAPPER";
  }
  { /* LUT input buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LUT_INPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "true";
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME) =
      "BUFD2_HVT_WRAPPER";
  }
  { /* LUT intermediate buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LUT_INTER_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "true";
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME) =
      "BUFD2_HVT_WRAPPER";
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_LOCATION_MAP) =
      inter_buf_loc_map().c_str();
  }
  { /* Pass gate logic */
    pugi::xml_node pgl_node = cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PGL_NODE_NAME);
    pgl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PGL_ATTR_CIRCUIT_MODEL_NAME) =
      "LUT_MUX2_WRAPPER";
  }
  for (auto curr_port : input_circuit_ports()) { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) =
      circuit_port_type2str(curr_port).c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) =
      port(curr_port).get_name().c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) =
      circuit_port(curr_port).get_name().c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
      std::to_string(circuit_port(curr_port).get_width()).c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TRI_STATE_MAP) =
      tri_state_map(curr_port).c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_CIRCUIT_MODEL_NAME) =
      "OR2_HVT_WRAPPER";
  }
  for (auto curr_port : output_circuit_ports()) { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) =
      circuit_port_type2str(curr_port).c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) =
      port(curr_port).get_name().c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) =
      circuit_port(curr_port).get_name().c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
      std::to_string(circuit_port(curr_port).get_width()).c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LUT_FRAC_LEVEL) =
      std::to_string(output_port_frac_level(curr_port)).c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LUT_OUTPUT_MASK) =
      port_output_mask(curr_port).c_str();
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "sram";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "sram";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
      std::to_string(num_srams()).c_str();
  }
  if (num_mode_bits()) {
    num_err += add_xml_openfpga_arch_circuit_model_mode_port(cmdl_node, mode_port().get_name(),
                                                             num_mode_bits());
  }
  return num_err;
}

bool VprArchPrimitive::require_external_netlist() const { return require_external_netlist_; }

int VprArchPrimitive::openfpga_circuit_model_xml_node_generic(
  pugi::xml_node& clib_node, const bool& is_default, const std::string& custom_vlog_nlist) const {
  int num_err = 0;

  pugi::xml_node cmdl_node = clib_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) =
    circuit_model_type2str().c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = circuit_model_.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = circuit_model_.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  /* TODO: if the type is LUT, use auto-generation */
  if (require_external_netlist()) {
    cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST) =
      custom_vlog_nlist.c_str();
  }

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "true";
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME) =
      "INV_WRAPPER";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "true";
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME) =
      "INV_WRAPPER";
  }

  for (auto curr_port : input_circuit_ports()) { /* Port */
    if (is_port_global_input(curr_port)) {
      num_err += add_xml_openfpga_arch_circuit_model_global_input(
        cmdl_node, port(curr_port).get_name(), circuit_port(curr_port).get_width(),
        circuit_port(curr_port).get_name());
    } else {
      pugi::xml_node port_node =
        cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) =
        circuit_port_type2str(curr_port).c_str();
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) =
        port(curr_port).get_name().c_str();
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) =
        circuit_port(curr_port).get_name().c_str();
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
        std::to_string(circuit_port(curr_port).get_width()).c_str();
      if (is_port_data_io(curr_port) || is_port_io(curr_port)) {
        port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL) = "true";
        port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_IO) = "true";
        if (is_port_data_io(curr_port)) {
          port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_DATA_IO) = "true";
        }
      }
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL) =
        std::to_string(input_port_default_value(curr_port)).c_str();
    }
  }
  for (auto curr_port : output_circuit_ports()) { /* Port */
    if (is_port_global_output(curr_port)) {
      num_err += add_xml_openfpga_arch_circuit_model_global_output(
        cmdl_node, port(curr_port).get_name(), circuit_port(curr_port).get_width(),
        circuit_port(curr_port).get_name());
    } else {
      pugi::xml_node port_node =
        cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) =
        circuit_port_type2str(curr_port).c_str();
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) =
        port(curr_port).get_name().c_str();
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) =
        circuit_port(curr_port).get_name().c_str();
      port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) =
        std::to_string(circuit_port(curr_port).get_width()).c_str();
      if (is_port_data_io(curr_port) || is_port_io(curr_port)) {
        port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL) = "true";
        port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_IO) = "true";
        if (is_port_data_io(curr_port)) {
          port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_DATA_IO) = "true";
        }
      }
    }
  }

  if (num_mode_bits()) {
    num_err += add_xml_openfpga_arch_circuit_model_mode_port(cmdl_node, mode_port().get_name(),
                                                             num_mode_bits());
  }

  return num_err;
}

int VprArchPrimitive::openfpga_circuit_model_xml_node(pugi::xml_node& clib_node,
                                                      const bool& is_default,
                                                      const std::string& custom_vlog_nlist) const {
  if (!is_physical_model()) {
    VTR_LOG_ERROR("Circuit model is not available for logical models!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  int num_err = 0;
  if (is_lut()) {
    num_err += openfpga_circuit_model_xml_node_frac_lut(clib_node, is_default, custom_vlog_nlist);
  } else {
    num_err += openfpga_circuit_model_xml_node_generic(clib_node, is_default, custom_vlog_nlist);
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

bool VprArchPrimitive::set_physical_model(const bool& val) {
  is_physical_model_ = val;
  return true;
}

bool VprArchPrimitive::set_require_external_netlist(const bool& val) {
  require_external_netlist_ = val;
  return true;
}

bool VprArchPrimitive::set_circuit_model(const std::string& val) {
  circuit_model_ = val;
  return true;
}

bool VprArchPrimitive::set_circuit_model_type(const e_model_type& val) {
  circuit_model_type_ = val;
  return true;
}

bool VprArchPrimitive::set_input_port_lib_name(const std::string& name,
                                               const std::string& lib_name) {
  auto result_i = input_name2ids_.find(name);
  if (result_i == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  input_lib_ports_[port_id] = lib_name;

  return true;
}

bool VprArchPrimitive::set_input_port_data_io(const std::string& name, const bool& val) {
  auto result_i = input_name2ids_.find(name);
  if (result_i == input_name2ids_.end()) {
    VTR_LOG_ERROR("The input port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  input_is_data_io_[port_id] = val;

  return true;
}

bool VprArchPrimitive::set_output_port_lib_name(const std::string& name,
                                                const std::string& lib_name) {
  auto result_i = output_name2ids_.find(name);
  if (result_i == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  output_lib_ports_[port_id] = lib_name;

  return true;
}

bool VprArchPrimitive::set_output_port_data_io(const std::string& name, const bool& val) {
  auto result_i = output_name2ids_.find(name);
  if (result_i == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  output_is_data_io_[port_id] = val;

  return true;
}

bool VprArchPrimitive::set_output_port_io(const std::string& name, const bool& val) {
  auto result_i = output_name2ids_.find(name);
  if (result_i == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  output_is_io_[port_id] = val;

  return true;
}

bool VprArchPrimitive::set_output_port_frac_level(const std::string& name, const size_t& val) {
  auto result_i = output_name2ids_.find(name);
  if (result_i == output_name2ids_.end()) {
    VTR_LOG_ERROR("The output port '%s' is not yet defined!\n", name.c_str());
    return false;
  }
  size_t port_id = result_i->second;
  output_port_frac_level_[port_id] = val;

  return true;
}

void VprArchPrimitive::add_mode_bit_tag_key_pair(const std::string& tag, const std::string& key) {
  mode_bit_tag2keys_[tag] = key;
}

}  // End namespace arcxpress
