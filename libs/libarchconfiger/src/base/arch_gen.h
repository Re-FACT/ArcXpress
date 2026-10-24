#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "clock_network.h"
#include "global_port_loc_map.h"
#include "pugixml.hpp"
#include "vpr_arch_layout.h"
#include "vpr_arch_primitive.h"

/* namespace alkaidC begins */
namespace arcxpress {

/**
 * arch package generator
 * - TODO: vpr arch XML
 * - TODO: openfpg arch XML
 * - TODO: arch variable YAML
 * - TODO: arch timing YAML
 * - TODO: global network XML
 * - TODO: bitstream setting XML
 * - TODO: pin table naming rules
 * - TODO: PCF dummy
 */
class ArchGen {
 public: /* -- Constructor -- */
  ArchGen();

 public: /* -- TYPES -- */
  /* Modeling the interconnects between pb_types */
  enum class e_pb_type_interconnect_type { DIRECT = 0, MUX, COMPLETE, NUM_TYPES };
  enum class e_pb_type_timing_type { SETUP = 0, HOLD, CLK2Q, COMB, NUM_TYPES };
  enum class e_pb_type_class { LUT = 0, FF, NUM_TYPES };
  enum class e_port_class { LUT_IN = 0, LUT_OUT, FF_D, FF_Q, FF_CLOCK, NUM_TYPES };
  typedef struct t_pb_type_timing {
    e_pb_type_timing_type type;
    openfpga::BasicPort clk_port;  /* src port*/
    openfpga::BasicPort data_port; /* des_port */
    std::string max;
    std::string min;
    bool operator==(const struct t_pb_type_timing& other) const {
      return (type == other.type && clk_port == other.clk_port && data_port == other.data_port &&
              max == other.max && min == other.min);
    }
  } PbTypeTiming;
  /* pb_type name, pb_type index in the mode, port info */
  typedef std::tuple<std::string, size_t, openfpga::BasicPort> EdgePort;
  /* pb_type name, pb_type index in the mode, port name, port pin index */
  typedef std::tuple<std::string, size_t, std::string, size_t> EdgePin;
  /* Max, Min delays */
  typedef std::pair<std::string, std::string> EdgeDelay;
  typedef struct t_pb_type_edge {
    EdgePort src_port;
    EdgePort des_port;
    EdgeDelay delay = {"0", "0"};
    std::vector<std::string> pack_patterns;
    std::string circuit_model_name = "";
  } PbTypeEdge;

 public: /* -- Accessors -- */
  int add_xml_vpr_arch_node(pugi::xml_node& root_node) const;
  std::string lut_primitive_pb_type_name(const size_t& size) const;
  std::string circuit_mode_port_attr_prefix() const;
  std::string lut_comb_delay_variable(const size_t& size) const;
  std::vector<std::string> find_primitive_pb_type_names(const std::string& pmtv_name) const;
  std::string find_primitive_pb_type_name(const std::string& pmtv_name) const;
  bool find_pb_type_port(const std::string& pb_type_name, const std::string& port_name,
                         openfpga::BasicPort& port) const;
  std::vector<size_t> child_pbs(const std::string& mode_name,
                                const std::string child_pb_name) const;
  int add_xml_openfpga_arch_node(pugi::xml_node& root_node) const;
  bool is_arch_primitive_exist(const std::string& name) const;
  /* Global network generation*/
  int build_global_network(const VprArchLayout::e_fabric_type& fb_type,
                           openfpga::ClockNetwork& gntwk) const;
  int build_ckbuf_location_map(const VprArchLayout::e_fabric_type& fb_type,
                               GlobalPortLoc& ckbuf_loc_map) const;
  size_t num_resets() const;
  size_t num_clocks() const;
  /* Used to generate bitstream setting files */
  int add_xml_openfpga_bitstream_setting_node(pugi::xml_node& root_node) const;
  std::string gclock_network_name() const;
  std::string greset_network_name() const;

 private: /* Private utility */
  bool is_physical_primitive_default_in_circuit_model(const VprArchPrimitive& pmtv) const;
  int add_xml_vpr_arch_models(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_tiles(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_layout(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_device(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_switchlist(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_segmentlist(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_directlist(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_complexblocklist(pugi::xml_node& root_node) const;
  int add_xml_vpr_arch_pb_type_tree(pugi::xml_node& root_node,
                                    const std::string& top_pb_type_name) const;
  int add_xml_vpr_arch_pb_type_declaration(pugi::xml_node& root_node,
                                           const std::string& pb_type_name, const bool& is_top,
                                           const size_t& num_pb) const;
  int rec_add_xml_vpr_arch_pb_type(pugi::xml_node& root_node, const std::string& pb_type_name,
                                   const size_t& num_pb) const;
  int add_xml_vpr_arch_mode_interconnects(pugi::xml_node& root_node, const size_t& mode_id) const;
  int add_xml_vpr_arch_primitive_pb_type_timing_node(pugi::xml_node& root_node,
                                                     const std::string& pb_type_name,
                                                     const PbTypeTiming& timing_info) const;
  int add_xml_openfpga_arch_techlib(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_techlib_device(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_techlib_device_logic(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_techlib_device_io(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_techlib_variation(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_circuit_lib(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_config_protocol(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_cb(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_sb(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_routing_segment(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_direct_connection(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_tile_annotation(pugi::xml_node& root_node) const;
  int rec_add_xml_openfpga_arch_pb_type_annotation_physical_mode(
    pugi::xml_node& root_node, const std::string& pb_type_name,
    const std::string& pb_type_path) const;
  int rec_add_xml_openfpga_arch_pb_type_annotation_operating_mode(
    pugi::xml_node& root_node, const std::string& pb_type_name,
    const std::string& pb_type_path) const;
  int add_xml_openfpga_arch_pb_type_annotation_interconnect(pugi::xml_node& root_node,
                                                            const std::string& pb_type_mode,
                                                            const std::string& pb_type_path) const;
  int add_xml_openfpga_arch_pb_type_annotation_operating_mode(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_pb_type_annotation_physical_mode(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_pb_type_annotation(pugi::xml_node& root_node) const;
  int add_xml_openfpga_arch_circuit_model_inv(pugi::xml_node& root_node, const std::string& name,
                                              const bool& is_default) const;
  int add_xml_openfpga_arch_circuit_model_buf(pugi::xml_node& root_node, const std::string& name,
                                              const bool& is_default,
                                              const size_t& drive_strength) const;
  int add_xml_openfpga_arch_circuit_model_or2(pugi::xml_node& root_node, const std::string& name,
                                              const bool& is_default) const;
  int add_xml_openfpga_arch_circuit_model_mux2(pugi::xml_node& root_node, const std::string& name,
                                               const bool& is_default) const;
  int add_xml_openfpga_arch_circuit_model_chan_wire(pugi::xml_node& root_node,
                                                    const std::string& name,
                                                    const bool& is_default) const;
  int add_xml_openfpga_arch_circuit_model_wire(pugi::xml_node& root_node, const std::string& name,
                                               const bool& is_default) const;
  int add_xml_openfpga_arch_circuit_model_mux(pugi::xml_node& root_node, const std::string& name,
                                              const std::string& mux2_name,
                                              const std::string& last_stage_mux2_name,
                                              const std::string& out_buffer_model,
                                              const std::string& const_input_val,
                                              const bool& is_default = false) const;
  int add_xml_openfpga_arch_circuit_model_ccff(pugi::xml_node& root_node) const;
  int build_global_network_tree_taps(const std::string& ntwk_name,
                                     const openfpga::BasicPort& g_port,
                                     const std::vector<VprArchLayout::e_port_type>& port_types,
                                     openfpga::ClockNetwork& gntwk) const;
  int build_global_network_tree_spines(const vtr::Point<size_t>& fabric_size,
                                       const vtr::Point<size_t>& entry_point,
                                       const std::string& ntwk_name,
                                       const openfpga::BasicPort& g_port,
                                       const openfpga::BasicPort& ckbuf_starting_subtile,
                                       const openfpga::BasicPort& ckbuf_out_port,
                                       openfpga::ClockNetwork& gntwk) const;
  int build_global_network_clock_tree(const vtr::Point<size_t>& fabric_size,
                                      const vtr::Point<size_t>& entry_point,
                                      const std::string& ntwk_name,
                                      const openfpga::BasicPort& g_port,
                                      const openfpga::BasicPort& ckbuf_starting_subtile,
                                      const openfpga::BasicPort& ckbuf_out_port,
                                      openfpga::ClockNetwork& gntwk) const;
  int build_global_network_reset_tree(const vtr::Point<size_t>& fabric_size,
                                      const vtr::Point<size_t>& entry_point,
                                      const std::string& ntwk_name,
                                      const openfpga::BasicPort& g_port,
                                      const openfpga::BasicPort& ckbuf_starting_subtile,
                                      const openfpga::BasicPort& ckbuf_out_port,
                                      openfpga::ClockNetwork& gntwk) const;
  std::string gen_gntwk_main_spine_name(const std::string& prefix) const;
  std::string gen_gntwk_branch_spine_name(const std::string& prefix,
                                          const VprArchLayout::e_dir_type& dir,
                                          const vtr::Point<size_t>& tap_point) const;

 public: /* -- Mutators -- */
  /* TODO: Set I/O number constraints */
  void set_num_clocks(const size_t& val);
  void set_num_pclocks(const size_t& val);
  void set_num_config_regions(const size_t& val);
  void set_num_resets(const size_t& val);
  /* Add arch primitives */
  void add_arch_primitive(const VprArchPrimitive& pmtv);
  void set_arch_layout(const VprArchLayout& lyot);
  /* Add pb_type for complex block graph */
  bool add_pb_type(const std::string& name);
  /* Copy an existing pb_type with the same ports */
  bool copy_pb_type(const std::string& name, const std::string& existing_pb_type);
  bool set_pb_type_blif_model(const std::string& name, const std::string& blif_model);
  bool set_pb_type_class(const std::string& name, const e_pb_type_class& val);
  bool set_pb_type_circuit_model(const std::string& name, const std::string& val);
  bool set_pb_type_physical_pb_type(const std::string& name, const std::string& val);
  bool set_pb_type_physical_pb_type_index_factor(const std::string& name, const std::string& val);
  bool set_pb_type_physical_pb_type_index_offset(const std::string& name, const std::string& val);
  bool set_pb_type_mode_bits(const std::string& name, const std::string& val);
  bool add_pb_type_port(const std::string& name, const openfpga::BasicPort& port,
                        const VprArchLayout::e_port_type& type, const bool& logic_equ = false);
  bool set_pb_type_port_class(const std::string& name, const openfpga::BasicPort& port_name,
                              const e_port_class& port_class);
  bool set_pb_type_physical_port_binding(const std::string& name, const std::string& op_port_name,
                                         const openfpga::BasicPort& phy_port,
                                         const std::string& rotate_offset = std::string());
  /* Add timing modeling */
  bool add_pb_type_setup_delay(const std::string& name, const openfpga::BasicPort& data_port,
                               const openfpga::BasicPort& clk_port, const std::string& val);
  bool add_pb_type_hold_delay(const std::string& name, const openfpga::BasicPort& data_port,
                              const openfpga::BasicPort& clk_port, const std::string& val);
  bool add_pb_type_clk2q_delay(const std::string& name, const openfpga::BasicPort& data_port,
                               const openfpga::BasicPort& clk_port, const std::string& max_val,
                               const std::string& min_val);
  bool add_pb_type_comb_delay(const std::string& name, const openfpga::BasicPort& in_port,
                              const openfpga::BasicPort& out_port, const std::string& max_val);
  /* Create top-level subtile as pb_type to the complexblock list */
  int init_top_pb_type(const bool& verbose = false);
  /* Create the architecture primitives as primitive pb_type to the complexblock list */
  int init_primitive_pb_type(const bool& verbose = false);
  /* Add operating mode to a parent pb_type */
  bool add_pb_type_mode(const std::string& pb_type_name, const std::string& mode_name,
                        const bool& is_physical = false);
  bool set_pb_type_mode(const std::string& pb_type_name, const std::string& mode_name,
                        const bool& is_physical = false);
  bool add_pb_type_to_mode(const std::string& mode_name, const std::string& pb_type_name,
                           const size_t& num_pb);
  /* Auto-wiring is only applicable to the following condition:
   * - The mode only contains one pb type
   * - All the inputs and outputs of the child pb type should have the corresponding ports in the
   * same name at the parent pb_type of this mode
   * - All the interconnects will be in the type of direct and timing will be set to 0
   */
  bool auto_wire_pb_type_mode(const std::string& mode_name,
                              const bool& loose_match_port_type = false);
  bool auto_wire_control_signals_pb_type_mode_child(const std::string& mode_name,
                                                    const std::string& child_pb);
  /* Add interconnects */
  bool add_in2in_edge(const std::string& mode_name, const openfpga::BasicPort& src_port,
                      const EdgePort& des_port, const EdgeDelay& delay = {"0", "0"});
  bool add_in2in_edge(const std::string& mode_name, const openfpga::BasicPort& src_port,
                      const EdgePort& des_port, const std::string& circuit_model_name,
                      const EdgeDelay& delay = {"0", "0"});
  bool add_internal_edge(const std::string& mode_name, const EdgePort& src_port,
                         const EdgePort& des_port, const EdgeDelay& delay = {"0", "0"});
  bool add_internal_edge(const std::string& mode_name, const EdgePort& src_port,
                         const EdgePort& des_port, const std::string& circuit_model_name,
                         const EdgeDelay& delay = {"0", "0"});
  bool add_out2out_edge(const std::string& mode_name, const EdgePort& src_port,
                        const openfpga::BasicPort& des_port, const EdgeDelay& delay = {"0", "0"});
  bool add_out2out_edge(const std::string& mode_name, const EdgePort& src_port,
                        const openfpga::BasicPort& des_port, const std::string& circuit_model_name,
                        const EdgeDelay& delay = {"0", "0"});
  /* Set edge delay */
  bool set_in2in_edge_delay(const std::string& mode_name, const openfpga::BasicPort& src_port,
                            const EdgePort& des_port, const EdgeDelay& delay);
  /* Set pack patterns */
  bool set_in2in_edge_pack_pattern(const std::string& mode_name,
                                   const openfpga::BasicPort& src_port, const EdgePort& des_port,
                                   const std::string& pack_pattern);
  bool set_internal_edge_pack_pattern(const std::string& mode_name, const EdgePort& src_port,
                                      const EdgePort& des_port, const std::string& pack_pattern);
  bool set_out2out_edge_pack_pattern(const std::string& mode_name, const EdgePort& src_port,
                                     const openfpga::BasicPort& des_port,
                                     const std::string& pack_pattern);
  int set_arch_primitive_mode_bits(const std::string& name, const std::string& tag,
                                   const std::string& mode_bits);

 public: /* -- Validators -- */
  e_pb_type_interconnect_type str2pb_type_interconnect_type(const std::string& str,
                                                            const bool& verbose = false) const;
  std::string pb_type_interconnect_type2str(const e_pb_type_interconnect_type& type,
                                            const bool& verbose = false) const;
  std::string pb_type_interconnect_type_all2str() const;
  bool valid_pb_type_interconnect_type(const e_pb_type_interconnect_type& type) const;
  void check_valid_pb_type_interconnect_type(const e_pb_type_interconnect_type& type) const;
  std::vector<ArchGen::e_pb_type_interconnect_type> pb_type_interconnect_types() const;

  e_pb_type_class str2pb_type_class(const std::string& str, const bool& verbose = false) const;
  std::string pb_type_class2str(const e_pb_type_class& type, const bool& verbose = false) const;
  std::string pb_type_class_all2str() const;
  bool valid_pb_type_class(const e_pb_type_class& type) const;
  void check_valid_pb_type_class(const e_pb_type_class& type) const;
  std::vector<ArchGen::e_pb_type_class> pb_type_classs() const;

  e_port_class str2port_class(const std::string& str, const bool& verbose = false) const;
  std::string port_class2str(const e_port_class& type, const bool& verbose = false) const;
  std::string port_class_all2str() const;
  bool valid_port_class(const e_port_class& type) const;
  void check_valid_port_class(const e_port_class& type) const;
  std::vector<ArchGen::e_port_class> port_classs() const;

 public: /* -- Public utility -- */
  bool valid_pb_type_port(const std::string& name, const openfpga::BasicPort& port,
                          const bool& verbose) const;

 private: /* -- Internal utility -- */
  size_t find_pb_type_id(const std::string& name) const;
  bool valid_pb_type_id(const size_t& id) const;
  size_t find_pb_mode_id(const std::string& name) const;
  bool valid_pb_mode_id(const size_t& id) const;
  bool valid_pb_mode_child_pb_index(const std::string& mode_name, const std::string& pb_type_name,
                                    const size_t& pb_idx, const bool& verbose) const;
  std::string find_pb_type_port_xml_node_name(const VprArchLayout::e_port_type& type) const;
  bool check_port_type_match(const VprArchLayout::e_port_type& port0_type,
                             const VprArchLayout::e_port_type& port1_type,
                             const bool& loose_match) const;
  int compile_pb_type_mode_interconnects(const size_t& mode_id,
                                         std::map<EdgePin, std::vector<size_t>>& interc) const;
  std::string pb_type_interconnect_name(const EdgePin& edge_pin) const;
  /* Provide a pb type, return its full hierarchical path in the format of <name>[mode].<name>[mode]
   */
  std::string pb_type_hierarchy_to_string(const std::string& pb_type_name) const;

 private: /* -- Internal data -- */
  /* Arch parameters */
  size_t num_clks_;
  size_t num_pclks_;
  size_t num_config_regions_;
  size_t num_rsts_;
  std::array<size_t, 4> num_ios_; /* Side: top, right, bottom, left */

  std::vector<VprArchPrimitive> arch_primitives_;
  VprArchLayout arch_layout_;
  mutable std::map<std::string, bool> outputted_circuit_models_;

  /* Complex block data structure */
  std::map<std::string, size_t>
    pb_type_name2ids_; /* Unique name per id, must be a valid one in te VPR layout */
  std::vector<std::string>
    pb_type_blif_models_; /* Mode names for childs, first mode is always the physical mode */
  std::vector<e_pb_type_class> pb_type_classes_;
  std::vector<std::string> pb_type_parent_modes_;
  std::vector<std::vector<std::string>>
    pb_type_modes_; /* Mode names for childs, first mode is always the physical mode */
  std::vector<std::vector<bool>> pb_type_mode_is_physical_; /* Parent pb_type ids */
  std::vector<std::vector<VprArchLayout::e_port_type>> pb_type_port_types_;
  std::vector<std::vector<bool>> pb_type_port_logic_equi_;
  std::vector<std::vector<e_port_class>> pb_type_port_classes_;
  std::vector<std::string> pb_type_circuit_model_names_;
  std::vector<std::string> pb_type_mode_bits_;
  std::vector<std::string>
    pb_type_physical_index_factors_; /* Only for pb_type in operating modes */
  std::vector<std::string>
    pb_type_physical_index_offsets_;                   /* Only for pb_type in operating modes */
  std::vector<std::string> pb_type_physical_pb_types_; /* Only for pb_type in operating modes */
  std::vector<std::vector<openfpga::BasicPort>> pb_type_ports_;
  std::vector<std::vector<openfpga::BasicPort>>
    pb_type_physical_ports_; /* Only for pb_type in operating modes */
  std::vector<std::vector<std::string>>
    pb_type_physical_pin_rotate_offsets_; /* Only for pb_type in operating modes */
  /* Timing modeling inside pb_type */
  std::vector<std::vector<PbTypeTiming>> pb_type_timing_;

  std::map<std::string, size_t>
    pb_mode_name2ids_; /* Unique name per id, must be a valid one in the VPR layout */
  std::vector<std::string> pb_mode_parents_; /* Parent pb_type ids */
  std::vector<std::map<std::string, size_t>>
    pb_mode_childs_; /* Child pb_type ids to count (num_pb) */
  std::vector<std::vector<PbTypeEdge>> pb_mode_edges_;

  /* Constants */
  std::array<const char*, size_t(e_pb_type_interconnect_type::NUM_TYPES)>
    PB_TYPE_INTERCONNECT_TYPE_STRING_;
  std::array<const char*, size_t(e_pb_type_class::NUM_TYPES)> PB_TYPE_CLASS_STRING_;
  std::array<const char*, size_t(e_port_class::NUM_TYPES)> PORT_CLASS_STRING_;
  std::string LUT_PB_TYPE_NAME_PREFIX_;
  std::string LUT_COMB_DELAY_VAR_PREFIX_;
  std::string LUT_COMB_DELAY_VAR_POSTFIX_;
  std::string CIRCUIT_MODEL_PORT_ATTR_PREFIX_;
  std::string CUSTOM_VERILOG_NETLIST_FILE_;
  std::string CCFF_MODEL_NAME_;
  std::string GCLK_NAME_;
  std::string GCLK_NTWK_NAME_;
  std::string GRST_NAME_;
  std::string GRST_NTWK_NAME_;
};

}  // namespace arcxpress
