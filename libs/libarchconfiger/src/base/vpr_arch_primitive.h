#pragma once

#include <map>
#include <string>
#include <vector>

#include "openfpga_port.h"
#include "pugixml.hpp"
#include "vpr_arch_layout.h"
#include "vtr_strong_id.h"

/* namespace alkaidC begins */
namespace arkangel {

/* Constants */
constexpr const char* DEFAULT_MODE_SELECT_PARAMETER_KEY_ = "default";

/**
 * arch package generator
 */
class VprArchPrimitive {
 public: /* -- Constructor -- */
  VprArchPrimitive(const std::string& name);

 public: /* -- TYPES -- */
  struct port_id_tag;
  typedef vtr::StrongId<port_id_tag> PortId;
  enum class e_model_type { LUT = 0, FF, IO, HARDLOGIC, NUM_TYPES };

 public: /* -- Accessors -- */
  int vpr_model_xml_node(pugi::xml_node& models_node) const;
  int openfpga_circuit_model_xml_node(pugi::xml_node& models_node, const bool& is_default,
                                      const std::string& custom_vlog_nlist) const;
  std::string name() const;
  std::string blif_model() const;
  std::string circuit_model_name() const;
  std::vector<std::string> pb_type_names() const;
  std::string pb_type_name(const std::string& tag) const;
  std::string mode_bits(const std::string& tag) const;
  std::vector<std::string> mode_bit_tags() const;
  std::vector<PortId> ports() const;
  openfpga::BasicPort port(const PortId& port_id) const;
  VprArchLayout::e_port_type port_type(const PortId& port_id) const;
  bool contain_mode_bits() const;
  std::string pb_type_name() const;
  bool require_mode_select_parameters() const;
  std::vector<std::string> mode_select_parameters() const;
  std::string mode_select_parameter_key(const std::string& curr_pb_type_name) const;
  size_t mode_select_offset(const std::string& param,
                            const std::string& key = DEFAULT_MODE_SELECT_PARAMETER_KEY_) const;

  bool is_physical_model() const;
  e_model_type circuit_model_type() const;
  std::string circuit_model_type2str() const;

 private: /* -- Internal utility -- */
  int openfpga_circuit_model_xml_node_frac_lut(pugi::xml_node& models_node, const bool& is_default,
                                               const std::string& custom_vlog_nlist) const;
  int openfpga_circuit_model_xml_node_generic(pugi::xml_node& models_node, const bool& is_default,
                                              const std::string& custom_vlog_nlist) const;
  bool is_lut() const;
  size_t num_mode_bits() const;
  std::vector<PortId> circuit_ports() const;
  std::vector<PortId> input_circuit_ports() const;
  std::vector<PortId> output_circuit_ports() const;
  std::string circuit_port_type2str(const PortId& port_id) const;
  openfpga::BasicPort circuit_port(const PortId& port_id) const;
  openfpga::BasicPort mode_port() const;
  bool is_port_global_input(const PortId& port_id) const;
  bool is_port_global_output(const PortId& port_id) const;
  bool is_port_data_io(const PortId& port_id) const;
  bool is_port_io(const PortId& port_id) const;
  size_t input_port_default_value(const PortId& port_id) const;
  size_t num_srams() const;
  size_t lut_input_size() const;
  size_t output_port_frac_level(const PortId& port_id) const;
  std::string inter_buf_loc_map() const;
  std::string tri_state_map(const PortId& port_id) const;
  std::string port_output_mask(const PortId& port_id) const;
  bool require_external_netlist() const;

 public: /* -- Mutators -- */
  bool set_blif_model(const std::string& val);
  bool set_mode_bits(const std::string& tag, const std::string& val);
  bool add_input_port(const std::string& name, const size_t& width, const bool& is_clock = false);
  bool add_output_port(const std::string& name, const size_t& width);
  bool add_input_comb_sink_port(const std::string& name, const std::string& s_port);
  bool set_input_require_clock_port(const std::string& name, const std::string& ck_port);
  bool set_output_require_clock_port(const std::string& name, const std::string& ck_port);
  bool add_mode_select_offset(const std::string& param, const size_t& offset,
                              const std::string& key = "default");

  bool set_circuit_model(const std::string& val);
  bool set_circuit_model_type(const e_model_type& val);
  bool set_input_port_lib_name(const std::string& name, const std::string& lib_name);
  bool set_output_port_lib_name(const std::string& name, const std::string& lib_name);
  bool add_input_circuit_port(const std::string& name, const size_t& width,
                              const std::string& lib_name);
  bool add_global_input_circuit_port(const std::string& name, const size_t& width,
                                     const std::string& lib_name, const size_t& default_val = 0);
  bool add_global_io_input_circuit_port(const std::string& name, const size_t& width,
                                        const std::string& lib_name, const size_t& default_val = 0);
  bool set_input_port_data_io(const std::string& name, const bool& val);
  bool add_output_circuit_port(const std::string& name, const size_t& width,
                               const std::string& lib_name);
  bool add_global_output_circuit_port(const std::string& name, const size_t& width,
                                      const std::string& lib_name);
  bool set_output_port_io(const std::string& name, const bool& val);
  bool set_output_port_data_io(const std::string& name, const bool& val);
  bool set_output_port_frac_level(const std::string& name, const size_t& val);
  bool set_physical_model(const bool& val = true);
  bool set_require_external_netlist(const bool& val = true);
  void add_mode_bit_tag_key_pair(const std::string& tag, const std::string& key);

 public: /* -- Validators -- */
  bool valid_input_id(const size_t& port_id) const;
  void check_valid_mode_bit_tag(const std::string& tag) const;

 private: /* -- Internal data -- */
  /* Basic Information */
  std::string name_; /* Used to name unique pb_type */
  std::string blif_model_;
  /* A flag to show the primitive requires a circuit model (a physical one) */
  std::string circuit_model_;
  /* tag -> bits */
  std::map<std::string, std::string> mode_bits_;
  /* tag -> key */
  std::map<std::string, std::string> mode_bit_tag2keys_;
  /* parameters -> key -> bits */
  std::map<std::string, std::map<std::string, size_t>> mode_select_offsets_;
  e_model_type circuit_model_type_;
  bool is_physical_model_;
  bool require_external_netlist_;

  std::map<std::string, size_t> input_name2ids_;
  /* TODO: Add the lib name of of the port */
  std::vector<std::string> input_ports_;
  std::vector<std::string> input_lib_ports_;
  std::vector<size_t> input_port_width_;
  std::vector<std::vector<size_t>> input_comb_sink_ports_; /* ids of output ports */
  std::vector<bool> input_is_clock_;
  std::vector<size_t> input_seq_src_ports_; /* id of input port which is a clock */
  std::vector<bool> input_only_in_circuit_model_;
  std::vector<bool> input_is_global_;
  std::vector<bool> input_is_data_io_;
  std::vector<bool> input_is_io_;
  std::vector<size_t> input_default_val_;

  std::map<std::string, size_t> output_name2ids_;
  /* TODO: Add the lib name of of the port */
  std::vector<std::string> output_ports_;
  std::vector<std::string> output_lib_ports_;
  std::vector<size_t> output_port_width_;
  std::vector<size_t> output_seq_src_ports_; /* id of input port which is a clock */
  std::vector<size_t> output_port_frac_level_;
  std::vector<bool> output_only_in_circuit_model_;
  std::vector<bool> output_is_global_;
  std::vector<bool> output_is_io_;
  std::vector<bool> output_is_data_io_;
};

}  // namespace arkangel
