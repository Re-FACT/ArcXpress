#include "arch_gen.h"

#include <algorithm>
#include <filesystem>

#include "arch_gen_circuit_model_utils.h"
#include "bitstream_setting_xml_constants.h"
#include "command_exit_codes.h"
#include "openfpga_arch_xml_constants.h"
#include "vpr_arch_primitive.h"
#include "vpr_arch_xml_constants.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arcxpress {  // Begin namespace arcxpress

ArchGen::ArchGen() {
  /* Default settings */
  num_clks_ = 8;
  num_pclks_ = 8;
  num_config_regions_ = 32;
  num_rsts_ = 8;
  PB_TYPE_INTERCONNECT_TYPE_STRING_ = {"direct", "mux", "complete"};
  PB_TYPE_CLASS_STRING_ = {"lut", "flipflop"};
  PORT_CLASS_STRING_ = {"lut_in", "lut_out", "D", "Q", "clock"};
  LUT_PB_TYPE_NAME_PREFIX_ = "lut";
  CIRCUIT_MODEL_PORT_ATTR_PREFIX_ = "config_enable";
  LUT_COMB_DELAY_VAR_PREFIX_ = "LUT";
  LUT_COMB_DELAY_VAR_POSTFIX_ = "DELAY";
  CUSTOM_VERILOG_NETLIST_FILE_ = "${CUSTOM_MODULE_VERILOG_FILE}";
  CCFF_MODEL_NAME_ = "EFPGA_CCFF";
  GCLK_NAME_ = "clk";
  GCLK_NTWK_NAME_ = "clk_tree_2lvl";
  GRST_NAME_ = "reset";
  GRST_NTWK_NAME_ = "rst_tree_2lvl";
}

void ArchGen::set_num_clocks(const size_t& val) { num_clks_ = val; }
void ArchGen::set_num_pclocks(const size_t& val) { num_pclks_ = val; }
void ArchGen::set_num_config_regions(const size_t& val) { num_config_regions_ = val; }
void ArchGen::set_num_resets(const size_t& val) { num_rsts_ = val; }

size_t ArchGen::num_clocks() const { return num_clks_; }

size_t ArchGen::num_resets() const { return num_rsts_; }

std::string ArchGen::lut_primitive_pb_type_name(const size_t& size) const {
  return std::string(LUT_PB_TYPE_NAME_PREFIX_) + std::to_string(size);
}

std::string ArchGen::circuit_mode_port_attr_prefix() const {
  return CIRCUIT_MODEL_PORT_ATTR_PREFIX_;
}

std::string ArchGen::lut_comb_delay_variable(const size_t& size) const {
  return std::string("${") + std::string(LUT_COMB_DELAY_VAR_PREFIX_) + std::to_string(size) +
         std::string("_") + std::string(LUT_COMB_DELAY_VAR_POSTFIX_) + std::string("}");
}

int ArchGen::add_xml_vpr_arch_models(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node models_node = root_node.append_child(XML_VPR_ARCH_MODELS_NODE_NAME);

  std::map<std::string, int> model_scoreboard;
  for (auto pmtv : arch_primitives_) {
    auto result = model_scoreboard.find(pmtv.blif_model());
    if (result == model_scoreboard.end()) {
      num_err += pmtv.vpr_model_xml_node(models_node);
      model_scoreboard[pmtv.blif_model()] = 0;
    } else {
      model_scoreboard[pmtv.blif_model()]++;
    }
  }

  return num_err;
}

void ArchGen::add_arch_primitive(const VprArchPrimitive& pmtv) { arch_primitives_.push_back(pmtv); }

void ArchGen::set_arch_layout(const VprArchLayout& lyot) { arch_layout_ = lyot; }

int ArchGen::add_xml_vpr_arch_tiles(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node tiles_node = root_node.append_child(XML_VPR_ARCH_TILES_NODE_NAME);

  num_err += arch_layout_.vpr_tile_xml_node(tiles_node);

  return num_err;
}

int ArchGen::add_xml_vpr_arch_layout(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node layout_node = root_node.append_child(XML_VPR_ARCH_LAYOUT_NODE_NAME);

  num_err += arch_layout_.vpr_layout_xml_node(layout_node);

  return num_err;
}

int ArchGen::add_xml_vpr_arch_device(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node device_node = root_node.append_child(XML_VPR_ARCH_DEVICE_NODE_NAME);

  num_err += arch_layout_.vpr_device_xml_node(device_node);

  return num_err;
}

int ArchGen::add_xml_vpr_arch_switchlist(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node sw_node = root_node.append_child(XML_VPR_ARCH_SWITCHLIST_NODE_NAME);

  num_err += arch_layout_.vpr_switchlist_xml_node(sw_node);

  return num_err;
}

int ArchGen::add_xml_vpr_arch_segmentlist(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node seg_node = root_node.append_child(XML_VPR_ARCH_SEGMENTLIST_NODE_NAME);

  num_err += arch_layout_.vpr_segmentlist_xml_node(seg_node);

  return num_err;
}

int ArchGen::add_xml_vpr_arch_directlist(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node directlist_node = root_node.append_child(XML_VPR_ARCH_DIRECTLIST_NODE_NAME);

  num_err += arch_layout_.vpr_directlist_xml_node(directlist_node);

  return num_err;
}

int ArchGen::add_xml_vpr_arch_node(pugi::xml_node& root_node) const {
  int num_err = 0;
  /* logical models */
  num_err += add_xml_vpr_arch_models(root_node);
  /* tiles */
  num_err += add_xml_vpr_arch_tiles(root_node);
  /* layout */
  num_err += add_xml_vpr_arch_layout(root_node);
  /* device */
  num_err += add_xml_vpr_arch_device(root_node);
  /* switchlist */
  num_err += add_xml_vpr_arch_switchlist(root_node);
  /* segment */
  num_err += add_xml_vpr_arch_segmentlist(root_node);
  /* directlist */
  num_err += add_xml_vpr_arch_directlist(root_node);
  /* complexblocks */
  num_err += add_xml_vpr_arch_complexblocklist(root_node);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

void ArchGen::check_valid_pb_type_interconnect_type(
  const ArchGen::e_pb_type_interconnect_type& type) const {
  if (!valid_pb_type_interconnect_type(type)) {
    VTR_LOG_ERROR("pb_type interconnect type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

ArchGen::e_pb_type_interconnect_type ArchGen::str2pb_type_interconnect_type(
  const std::string& str, const bool& verbose) const {
  for (int itype = size_t(ArchGen::e_pb_type_interconnect_type::DIRECT);
       itype < size_t(ArchGen::e_pb_type_interconnect_type::NUM_TYPES); ++itype) {
    if (str == std::string(PB_TYPE_INTERCONNECT_TYPE_STRING_[itype])) {
      return static_cast<ArchGen::e_pb_type_interconnect_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for pb_type_interconnect! Expect %s\n",
                 pb_type_interconnect_type_all2str().c_str());
  return ArchGen::e_pb_type_interconnect_type::NUM_TYPES;
}

std::string ArchGen::pb_type_interconnect_type2str(const ArchGen::e_pb_type_interconnect_type& type,
                                                   const bool& verbose) const {
  if (!valid_pb_type_interconnect_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for pb_type_interconnect! Expect %s\n",
                   pb_type_interconnect_type_all2str().c_str());
    return std::string();
  }
  return std::string(PB_TYPE_INTERCONNECT_TYPE_STRING_[size_t(type)]);
}

std::string ArchGen::pb_type_interconnect_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(ArchGen::e_pb_type_interconnect_type::DIRECT);
       itype < size_t(ArchGen::e_pb_type_interconnect_type::NUM_TYPES); ++itype) {
    full_types += std::string(PB_TYPE_INTERCONNECT_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::vector<ArchGen::e_pb_type_interconnect_type> ArchGen::pb_type_interconnect_types() const {
  std::vector<e_pb_type_interconnect_type> pb_type_interconnect_types;
  for (int itype = size_t(ArchGen::e_pb_type_interconnect_type::DIRECT);
       itype < size_t(ArchGen::e_pb_type_interconnect_type::NUM_TYPES); ++itype) {
    pb_type_interconnect_types.push_back(static_cast<ArchGen::e_pb_type_interconnect_type>(itype));
  }
  return pb_type_interconnect_types;
}

bool ArchGen::valid_pb_type_interconnect_type(const e_pb_type_interconnect_type& type) const {
  return type != ArchGen::e_pb_type_interconnect_type::NUM_TYPES;
}

bool ArchGen::add_pb_type(const std::string& name) {
  /* Check pb_type name */
  auto result = pb_type_name2ids_.find(name);
  if (result != pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Invalid pb_type name '%s' which is already defined!\n", name.c_str());
    return false;
  }
  size_t new_id = pb_type_modes_.size();
  pb_type_blif_models_.emplace_back();
  pb_type_classes_.emplace_back(e_pb_type_class::NUM_TYPES);
  pb_type_parent_modes_.emplace_back();
  pb_type_modes_.emplace_back();
  pb_type_mode_is_physical_.emplace_back();
  pb_type_circuit_model_names_.emplace_back();
  pb_type_mode_bits_.emplace_back();
  pb_type_ports_.emplace_back();
  pb_type_port_types_.emplace_back();
  pb_type_port_classes_.emplace_back();
  pb_type_port_logic_equi_.emplace_back();
  pb_type_physical_index_factors_.emplace_back();
  pb_type_physical_index_offsets_.emplace_back();
  pb_type_physical_pb_types_.emplace_back();
  pb_type_physical_ports_.emplace_back();
  pb_type_physical_pin_rotate_offsets_.emplace_back();

  pb_type_timing_.emplace_back();

  pb_type_name2ids_[name] = new_id;
  return true;
}

bool ArchGen::set_pb_type_blif_model(const std::string& name, const std::string& blif_model) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  pb_type_blif_models_[pb_type_id] = blif_model;
  return true;
}

bool ArchGen::add_pb_type_port(const std::string& name, const openfpga::BasicPort& port,
                               const VprArchLayout::e_port_type& type, const bool& logic_equ) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Check port name collision */
  for (auto curr_port : pb_type_ports_[pb_type_id]) {
    if (curr_port.get_name() == port.get_name()) {
      VTR_LOG_ERROR("Invalid port '%s' for pb_type name '%s' which is already defined as '%s'!\n",
                    port.to_verilog_string().c_str(), name.c_str(),
                    curr_port.to_verilog_string().c_str());
      return false;
    }
  }
  pb_type_ports_[pb_type_id].push_back(port);
  pb_type_port_types_[pb_type_id].push_back(type);
  pb_type_port_classes_[pb_type_id].push_back(e_port_class::NUM_TYPES);
  pb_type_port_logic_equi_[pb_type_id].push_back(logic_equ);
  pb_type_physical_ports_[pb_type_id].emplace_back();
  pb_type_physical_pin_rotate_offsets_[pb_type_id].emplace_back();
  return true;
}

int ArchGen::init_top_pb_type(const bool& verbose) {
  int num_err = 0;
  VTR_LOGV(verbose, "Adding top-level pb_type from pre-defined tiles...\n");
  /* TODO: Validate the ArchLayout is complete */
  for (auto subtile : arch_layout_.subtiles()) {
    VTR_LOGV(verbose, "Adding top-level pb type from tile '%s'\n", subtile.c_str());
    bool status = add_pb_type(subtile);
    if (!status) {
      num_err++;
      break;
    }
    for (auto port_id : arch_layout_.subtile_ports(subtile)) {
      status = add_pb_type_port(subtile, arch_layout_.subtile_port(subtile, port_id),
                                arch_layout_.subtile_port_type(subtile, port_id),
                                arch_layout_.subtile_port_logic_equivalent(subtile, port_id));
      if (!status) {
        num_err++;
        break;
      }
    }
  }
  VTR_LOGV(verbose, "Done\n");
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}
int ArchGen::init_primitive_pb_type(const bool& verbose) {
  int num_err = 0;
  VTR_LOGV(verbose, "Adding primitive pb types from pre-defined vpr models...\n");
  /* TODO: Validate the ArchLayout is complete */
  for (auto pmtv : arch_primitives_) {
    if (!pmtv.contain_mode_bits()) {
      VTR_LOGV(verbose, "Adding primitive pb type '%s' from model '%s'\n",
               pmtv.pb_type_name().c_str(), pmtv.name().c_str());
      bool status = add_pb_type(pmtv.pb_type_name());
      if (!status) {
        num_err++;
        break;
      }
      std::string blif_m = std::string(".subckt ") + pmtv.blif_model();
      status = set_pb_type_blif_model(pmtv.pb_type_name(), blif_m);
      if (!status) {
        num_err++;
        break;
      }
      for (auto port_id : pmtv.ports()) {
        status = add_pb_type_port(pmtv.pb_type_name(), pmtv.port(port_id), pmtv.port_type(port_id));
        if (!status) {
          num_err++;
          break;
        }
      }
    } else {
      /* Based on the tag-to-mode bits, create different pb_type names */
      for (auto tag : pmtv.mode_bit_tags()) {
        VTR_LOGV(verbose, "Adding primitive pb type '%s' from model '%s'\n",
                 pmtv.pb_type_name(tag).c_str(), pmtv.name().c_str());
        bool status = add_pb_type(pmtv.pb_type_name(tag));
        if (!status) {
          num_err++;
          break;
        }
        std::string blif_m = std::string(".subckt ") + pmtv.blif_model();
        status = set_pb_type_blif_model(pmtv.pb_type_name(tag), blif_m);
        if (!status) {
          num_err++;
          break;
        }
        status = set_pb_type_mode_bits(pmtv.pb_type_name(tag), pmtv.mode_bits(tag));
        if (!status) {
          num_err++;
          break;
        }
        for (auto port_id : pmtv.ports()) {
          status =
            add_pb_type_port(pmtv.pb_type_name(tag), pmtv.port(port_id), pmtv.port_type(port_id));
          if (!status) {
            num_err++;
            break;
          }
        }
      }
    }
  }
  VTR_LOGV(verbose, "Done\n");
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

size_t ArchGen::find_pb_type_id(const std::string& name) const {
  auto result = pb_type_name2ids_.find(name);
  if (result == pb_type_name2ids_.end()) {
    VTR_LOG_ERROR("Invalid pb_type name '%s' which is not defined yet!\n", name.c_str());
    return pb_type_modes_.size();
  }
  return result->second;
}

bool ArchGen::valid_pb_type_id(const size_t& id) const { return id < pb_type_modes_.size(); }

bool ArchGen::valid_pb_type_port(const std::string& name, const openfpga::BasicPort& port,
                                 const bool& verbose) const {
  bool uncontained_port = true;
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  for (auto curr_port : pb_type_ports_[pb_type_id]) {
    if (curr_port.mergeable(port) && curr_port.contained(port)) {
      return true;
    }
    if (curr_port.mergeable(port) && !curr_port.contained(port)) {
      VTR_LOGV_ERROR(
        verbose, "Invalid port '%s' for pb_type name '%s' which has already defined as '%s'!\n",
        port.to_verilog_string().c_str(), name.c_str(), curr_port.to_verilog_string().c_str());
      return false;
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid port '%s' for pb_type name '%s' which is not found!\n",
                 port.to_verilog_string().c_str(), name.c_str());
  return false;
}

bool ArchGen::add_pb_type_setup_delay(const std::string& name, const openfpga::BasicPort& data_port,
                                      const openfpga::BasicPort& clk_port, const std::string& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Ensure data port and clk port are valid */
  bool valid_data_port = valid_pb_type_port(name, data_port, true);
  if (!valid_data_port) {
    return false;
  }
  bool valid_clk_port = valid_pb_type_port(name, clk_port, true);
  if (!valid_clk_port) {
    return false;
  }

  PbTypeTiming t_info;
  t_info.type = e_pb_type_timing_type::SETUP;
  t_info.data_port = data_port;
  t_info.clk_port = clk_port;
  t_info.max = val;
  t_info.min = val;

  /* Avoid duplicated timing */
  bool existed = false;
  for (auto tmp_t_info : pb_type_timing_[pb_type_id]) {
    if (t_info == tmp_t_info) {
      existed = true;
      break;
    }
  }
  if (existed) {
    VTR_LOG_WARN("Skip to add timing info as it is already defined!\n");
    return true;
  }

  pb_type_timing_[pb_type_id].push_back(t_info);
  return true;
}

bool ArchGen::add_pb_type_hold_delay(const std::string& name, const openfpga::BasicPort& data_port,
                                     const openfpga::BasicPort& clk_port, const std::string& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Ensure data port and clk port are valid */
  bool valid_data_port = valid_pb_type_port(name, data_port, true);
  if (!valid_data_port) {
    return false;
  }
  bool valid_clk_port = valid_pb_type_port(name, clk_port, true);
  if (!valid_clk_port) {
    return false;
  }

  PbTypeTiming t_info;
  t_info.type = e_pb_type_timing_type::HOLD;
  t_info.data_port = data_port;
  t_info.clk_port = clk_port;
  t_info.max = val;
  t_info.min = val;
  pb_type_timing_[pb_type_id].push_back(t_info);
  return true;
}

bool ArchGen::add_pb_type_clk2q_delay(const std::string& name, const openfpga::BasicPort& data_port,
                                      const openfpga::BasicPort& clk_port,
                                      const std::string& max_val, const std::string& min_val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Ensure data port and clk port are valid */
  bool valid_data_port = valid_pb_type_port(name, data_port, true);
  if (!valid_data_port) {
    return false;
  }
  bool valid_clk_port = valid_pb_type_port(name, clk_port, true);
  if (!valid_clk_port) {
    return false;
  }
  PbTypeTiming t_info;
  t_info.type = e_pb_type_timing_type::CLK2Q;
  t_info.data_port = data_port;
  t_info.clk_port = clk_port;
  t_info.max = max_val;
  t_info.min = min_val;
  pb_type_timing_[pb_type_id].push_back(t_info);
  return true;
}

bool ArchGen::add_pb_type_comb_delay(const std::string& name, const openfpga::BasicPort& in_port,
                                     const openfpga::BasicPort& out_port,
                                     const std::string& max_val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Ensure data port and clk port are valid */
  bool valid_in_port = valid_pb_type_port(name, in_port, true);
  if (!valid_in_port) {
    return false;
  }
  bool valid_out_port = valid_pb_type_port(name, out_port, true);
  if (!valid_out_port) {
    return false;
  }
  PbTypeTiming t_info;
  t_info.type = e_pb_type_timing_type::COMB;
  t_info.data_port = out_port;
  t_info.clk_port = in_port;
  t_info.max = max_val;
  t_info.min = max_val;
  pb_type_timing_[pb_type_id].push_back(t_info);
  return true;
}

bool ArchGen::add_pb_type_mode(const std::string& pb_type_name, const std::string& mode_name,
                               const bool& is_physical) {
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Check pb_type name */
  auto result = pb_mode_name2ids_.find(mode_name);
  if (result != pb_mode_name2ids_.end()) {
    VTR_LOG_ERROR("Invalid mode name '%s' which is already defined!\n", mode_name.c_str());
    return false;
  }
  size_t new_id = pb_mode_parents_.size();
  pb_mode_parents_.push_back(pb_type_name);
  pb_mode_childs_.emplace_back();
  pb_mode_edges_.emplace_back();
  pb_mode_name2ids_[mode_name] = new_id;

  return set_pb_type_mode(pb_type_name, mode_name, is_physical);
}

size_t ArchGen::find_pb_mode_id(const std::string& name) const {
  auto result = pb_mode_name2ids_.find(name);
  if (result == pb_mode_name2ids_.end()) {
    VTR_LOG_ERROR("Invalid pb_type mode name '%s' which is not defined yet!\n", name.c_str());
    return pb_mode_parents_.size();
  }
  return result->second;
}

bool ArchGen::valid_pb_mode_id(const size_t& id) const { return id < pb_mode_parents_.size(); }

bool ArchGen::add_pb_type_to_mode(const std::string& mode_name, const std::string& pb_type_name,
                                  const size_t& num_pb) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  /* Ensure the child is not duplicated */
  auto result = pb_mode_childs_[pb_mode_id].find(pb_type_name);
  if (result != pb_mode_childs_[pb_mode_id].end()) {
    VTR_LOG_ERROR("The child pb_type '%s' is already defined under the mode '%s'!\n",
                  pb_type_name.c_str(), mode_name.c_str());
    return false;
  }
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }

  pb_mode_childs_[pb_mode_id][pb_type_name] = num_pb;
  pb_type_parent_modes_[pb_type_id] = mode_name;
  return true;
}

bool ArchGen::check_port_type_match(const VprArchLayout::e_port_type& port0_type,
                                    const VprArchLayout::e_port_type& port1_type,
                                    const bool& loose_match) const {
  if (!loose_match) {
    return port0_type == port1_type;
  }
  /* Loose match rules:
   * INPUT could be
   * - CIN
   * - CLOCK
   * - SCANI
   * - RESET
   * OUTPUT could be
   * - CO
   * - SCANO
   */
  std::vector<VprArchLayout::e_port_type> input_types = {
    VprArchLayout::e_port_type::INPUT, VprArchLayout::e_port_type::CI,
    VprArchLayout::e_port_type::SCANI, VprArchLayout::e_port_type::CLOCK,
    VprArchLayout::e_port_type::RESET};
  std::vector<VprArchLayout::e_port_type> output_types = {
    VprArchLayout::e_port_type::OUTPUT,
    VprArchLayout::e_port_type::CO,
    VprArchLayout::e_port_type::SCANO,
  };
  if (std::find(input_types.begin(), input_types.end(), port0_type) != input_types.end() &&
      std::find(input_types.begin(), input_types.end(), port1_type) != input_types.end()) {
    return true;
  }
  if (std::find(output_types.begin(), output_types.end(), port0_type) != output_types.end() &&
      std::find(output_types.begin(), output_types.end(), port1_type) != output_types.end()) {
    return true;
  }
  return false;
}

bool ArchGen::auto_wire_pb_type_mode(const std::string& mode_name,
                                     const bool& loose_match_port_type) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  /* Check if the auto-wiring can be applied */
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  size_t parent_pb_type_id = find_pb_type_id(parent_pb_type);
  if (!valid_pb_type_id(parent_pb_type_id)) {
    VTR_LOG_ERROR("The mode '%s' does not belong to any pb type '%s'!\n", mode_name.c_str(),
                  parent_pb_type.c_str());
    return false;
  }
  if (pb_mode_childs_[pb_mode_id].empty()) {
    VTR_LOG_ERROR("The mode '%s' has no child pb type. Auto-wiring cannot be applied!\n",
                  mode_name.c_str());
    return false;
  }
  if (pb_mode_childs_[pb_mode_id].size() > 1) {
    VTR_LOG_ERROR("The mode '%s' has more 1 child pb type. Auto-wiring cannot be applied!\n",
                  mode_name.c_str());
    return false;
  }
  auto it = pb_mode_childs_[pb_mode_id].begin();
  std::string child_pb_type = it->first;
  size_t child_pb_type_id = find_pb_type_id(child_pb_type);
  if (!valid_pb_type_id(child_pb_type_id)) {
    VTR_LOG_ERROR("The mode '%s' has an invalid child pb_type '%s' which is not defined!\n",
                  mode_name.c_str(), child_pb_type.c_str());
    return false;
  }
  if (it->second != 1) {
    VTR_LOG_ERROR(
      "The mode has a child pb type '%s' whose count is '%lu'. Expect the count to be 1, otherwise "
      "auto-wiring cannot be applied!\n",
      mode_name.c_str(), child_pb_type.c_str(), it->second);
    return false;
  }
  /* Check port names and type matches: all the child pb type port should have one corresponding in
   * the parent pb_type  */
  for (size_t iport = 0; iport < pb_type_port_types_[child_pb_type_id].size(); iport++) {
    VprArchLayout::e_port_type curr_child_port_type = pb_type_port_types_[child_pb_type_id][iport];
    openfpga::BasicPort curr_child_port = pb_type_ports_[child_pb_type_id][iport];

    bool found_matched = false;
    for (size_t jport = 0; jport < pb_type_port_types_[parent_pb_type_id].size(); jport++) {
      VprArchLayout::e_port_type curr_parent_port_type =
        pb_type_port_types_[parent_pb_type_id][jport];
      openfpga::BasicPort curr_parent_port = pb_type_ports_[parent_pb_type_id][jport];
      if (curr_parent_port == curr_child_port &&
          !check_port_type_match(curr_parent_port_type, curr_child_port_type,
                                 loose_match_port_type)) {
        VTR_LOG_ERROR(
          "The mode '%s' has a child pb type '%s' whose port '%s' has a corresponding one (same "
          "port name and width) from the parent pb_type '%s' but in different type! Auto-wiring "
          "cannot be applied!\n",
          mode_name.c_str(), child_pb_type.c_str(), curr_parent_port.to_verilog_string().c_str(),
          curr_child_port.get_name().c_str(), parent_pb_type.c_str());
        return false;
      }
      if (curr_parent_port == curr_child_port &&
          check_port_type_match(curr_parent_port_type, curr_child_port_type,
                                loose_match_port_type)) {
        found_matched = true;
        /* Do the wiring */
        for (auto pin : curr_parent_port.pins()) {
          openfpga::BasicPort parent_pin(curr_parent_port.get_name(), pin, pin);
          openfpga::BasicPort child_pin(curr_child_port.get_name(), pin, pin);
          PbTypeEdge curr_edge;
          if (curr_child_port_type == VprArchLayout::e_port_type::INPUT ||
              curr_child_port_type == VprArchLayout::e_port_type::RESET ||
              curr_child_port_type == VprArchLayout::e_port_type::SCANI ||
              curr_child_port_type == VprArchLayout::e_port_type::CI ||
              curr_child_port_type == VprArchLayout::e_port_type::CLOCK) {
            /* In-to-in connection */
            curr_edge.src_port = {parent_pb_type, 0, parent_pin};
            curr_edge.des_port = {child_pb_type, 0, child_pin};
          } else {
            /* out-to-out connection */
            curr_edge.src_port = {child_pb_type, 0, child_pin};
            curr_edge.des_port = {parent_pb_type, 0, parent_pin};
          }
          pb_mode_edges_[pb_mode_id].push_back(curr_edge);
        }
        break;
      }
      if (curr_parent_port.mergeable(curr_child_port) &&
          curr_parent_port.contained(curr_child_port) &&
          check_port_type_match(curr_parent_port_type, curr_child_port_type,
                                loose_match_port_type)) {
        found_matched = true;
        /* Do the wiring */
        for (auto pin : curr_child_port.pins()) {
          openfpga::BasicPort parent_pin(curr_parent_port.get_name(), pin, pin);
          openfpga::BasicPort child_pin(curr_child_port.get_name(), pin, pin);
          PbTypeEdge curr_edge;
          if (curr_child_port_type == VprArchLayout::e_port_type::INPUT ||
              curr_child_port_type == VprArchLayout::e_port_type::RESET ||
              curr_child_port_type == VprArchLayout::e_port_type::SCANI ||
              curr_child_port_type == VprArchLayout::e_port_type::CI ||
              curr_child_port_type == VprArchLayout::e_port_type::CLOCK) {
            /* In-to-in connection */
            curr_edge.src_port = {parent_pb_type, 0, parent_pin};
            curr_edge.des_port = {child_pb_type, 0, child_pin};
          } else {
            /* out-to-out connection */
            curr_edge.src_port = {child_pb_type, 0, child_pin};
            curr_edge.des_port = {parent_pb_type, 0, parent_pin};
          }
          pb_mode_edges_[pb_mode_id].push_back(curr_edge);
        }
        break;
      }
    }
    if (!found_matched) {
      VTR_LOG_ERROR(
        "The mode '%s' has a child pb type '%s' whose port does not have a corresponding one (same "
        "port type, name and width) from the parent pb_type '%s'. Auto-wiring cannot be applied!\n",
        mode_name.c_str(), child_pb_type.c_str(), curr_child_port.get_name().c_str(),
        parent_pb_type.c_str());
      return false;
    }
  }
  return true;
}

std::vector<std::string> ArchGen::find_primitive_pb_type_names(const std::string& pmtv_name) const {
  std::vector<std::string> ret;
  for (auto pmtv : arch_primitives_) {
    if (pmtv.name() == pmtv_name) {
      return pmtv.pb_type_names();
    }
  }
  return ret;
}

std::string ArchGen::find_primitive_pb_type_name(const std::string& pmtv_name) const {
  std::vector<std::string> ret = find_primitive_pb_type_names(pmtv_name);
  if (ret.empty()) {
    VTR_LOG_ERROR("No valid pb type is defined for vpr primitive model '%s'\n", pmtv_name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (ret.size() > 1) {
    VTR_LOG_ERROR("More than 1 pb type is defined for vpr primitive model '%s'! Expect only 1\n",
                  pmtv_name.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret[0];
}

bool ArchGen::find_pb_type_port(const std::string& pb_type_name, const std::string& port_name,
                                openfpga::BasicPort& port) const {
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  for (auto curr_port : pb_type_ports_[pb_type_id]) {
    if (curr_port.get_name() == port_name) {
      port = curr_port;
      return true;
    }
  }
  return false;
}

bool ArchGen::copy_pb_type(const std::string& name, const std::string& existing_pb_type) {
  size_t src_pb_type_id = find_pb_type_id(existing_pb_type);
  if (!valid_pb_type_id(src_pb_type_id)) {
    return false;
  }
  bool status = add_pb_type(name);
  if (!status) {
    return status;
  }
  /* Copy the ports */
  for (size_t iport = 0; iport < pb_type_port_types_[src_pb_type_id].size(); iport++) {
    status = add_pb_type_port(name, pb_type_ports_[src_pb_type_id][iport],
                              pb_type_port_types_[src_pb_type_id][iport]);
    if (!status) {
      return status;
    }
  }
  return true;
}

std::vector<size_t> ArchGen::child_pbs(const std::string& mode_name,
                                       const std::string child_pb_name) const {
  std::vector<size_t> ret;
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return ret;
  }
  for (auto pair : pb_mode_childs_[pb_mode_id]) {
    if (pair.first == child_pb_name) {
      for (size_t i = 0; i < pair.second; ++i) {
        ret.push_back(i);
      }
    }
  }
  return ret;
}

bool ArchGen::auto_wire_control_signals_pb_type_mode_child(const std::string& mode_name,
                                                           const std::string& child_pb_type) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  /* Check if the auto-wiring can be applied */
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  size_t parent_pb_type_id = find_pb_type_id(parent_pb_type);
  if (!valid_pb_type_id(parent_pb_type_id)) {
    VTR_LOG_ERROR("The mode '%s' does not belong to any pb type '%s'!\n", mode_name.c_str(),
                  parent_pb_type.c_str());
    return false;
  }
  /* Find child pb_type */
  size_t child_pb_type_id = find_pb_type_id(child_pb_type);
  if (!valid_pb_type_id(child_pb_type_id)) {
    VTR_LOG_ERROR("The mode '%s' has an invalid child pb_type '%s' which is not defined!\n",
                  mode_name.c_str(), child_pb_type.c_str());
    return false;
  }
  if (child_pbs(mode_name, child_pb_type).size() != 1) {
    VTR_LOG_ERROR("Require the only 1 child pb_type '%s' in the mode '%s', while multiple",
                  child_pb_type.c_str(), mode_name.c_str());
    return false;
  }
  for (size_t iport = 0; iport < pb_type_port_types_[child_pb_type_id].size(); iport++) {
    VprArchLayout::e_port_type curr_child_port_type = pb_type_port_types_[child_pb_type_id][iport];
    openfpga::BasicPort curr_child_port = pb_type_ports_[child_pb_type_id][iport];
    if (curr_child_port_type != VprArchLayout::e_port_type::RESET &&
        curr_child_port_type != VprArchLayout::e_port_type::SCANI &&
        curr_child_port_type != VprArchLayout::e_port_type::CI &&
        curr_child_port_type != VprArchLayout::e_port_type::CLOCK &&
        curr_child_port_type != VprArchLayout::e_port_type::CO &&
        curr_child_port_type != VprArchLayout::e_port_type::SCANO) {
      continue;
    }

    bool found_matched = false;
    for (size_t jport = 0; jport < pb_type_port_types_[parent_pb_type_id].size(); jport++) {
      VprArchLayout::e_port_type curr_parent_port_type =
        pb_type_port_types_[parent_pb_type_id][jport];
      openfpga::BasicPort curr_parent_port = pb_type_ports_[parent_pb_type_id][jport];
      if (curr_parent_port_type == curr_child_port_type) {
        found_matched = true;
        /* Do the wiring */
        PbTypeEdge curr_edge;
        if (curr_child_port_type == VprArchLayout::e_port_type::RESET ||
            curr_child_port_type == VprArchLayout::e_port_type::SCANI ||
            curr_child_port_type == VprArchLayout::e_port_type::CI ||
            curr_child_port_type == VprArchLayout::e_port_type::CLOCK) {
          /* In-to-in connection */
          curr_edge.src_port = {parent_pb_type, 0, curr_parent_port};
          curr_edge.des_port = {child_pb_type, 0, curr_child_port};
        } else {
          /* out-to-out connection */
          curr_edge.src_port = {child_pb_type, 0, curr_child_port};
          curr_edge.des_port = {parent_pb_type, 0, curr_parent_port};
        }
        pb_mode_edges_[pb_mode_id].push_back(curr_edge);
        break;
      }
    }
    if (!found_matched) {
      VTR_LOG_ERROR(
        "The mode '%s' has a child pb type '%s' whose port does not have a corresponding one (same "
        "port type and width) from the parent pb_type '%s'. Auto-wiring cannot be applied!\n",
        mode_name.c_str(), child_pb_type.c_str(), curr_child_port.get_name().c_str(),
        parent_pb_type.c_str());
      return false;
    }
  }
  return true;
}

bool ArchGen::valid_pb_mode_child_pb_index(const std::string& mode_name,
                                           const std::string& pb_type_name, const size_t& pb_idx,
                                           const bool& verbose) const {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  for (auto pair : pb_mode_childs_[pb_mode_id]) {
    if (pair.first == pb_type_name) {
      return pb_idx < pair.second;
    }
  }
  VTR_LOGV_ERROR(verbose, "Cannot find a child pb_type '%s[%lu]' under mode '%s'\n",
                 pb_type_name.c_str(), pb_idx, mode_name.c_str());
  return false;
}

bool ArchGen::add_in2in_edge(const std::string& mode_name, const openfpga::BasicPort& src_port,
                             const EdgePort& des_port, const std::string& circuit_model_name,
                             const EdgeDelay& delay) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  /* Check if the auto-wiring can be applied */
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  size_t parent_pb_type_id = find_pb_type_id(parent_pb_type);
  if (!valid_pb_type_id(parent_pb_type_id)) {
    VTR_LOG_ERROR("The mode '%s' does not belong to any pb type '%s'!\n", mode_name.c_str(),
                  parent_pb_type.c_str());
    return false;
  }
  if (!valid_pb_type_port(parent_pb_type, src_port, true)) {
    return false;
  }
  std::string des_pb = std::get<0>(des_port);
  size_t des_pb_idx = std::get<1>(des_port);
  openfpga::BasicPort des_pb_port = std::get<2>(des_port);
  if (!valid_pb_mode_child_pb_index(mode_name, des_pb, des_pb_idx, true)) {
    return false;
  }
  if (!valid_pb_type_port(des_pb, des_pb_port, true)) {
    return false;
  }
  PbTypeEdge curr_edge;
  curr_edge.src_port = {parent_pb_type, 0, src_port};
  curr_edge.des_port = des_port;
  curr_edge.delay = delay;
  curr_edge.circuit_model_name = circuit_model_name;
  pb_mode_edges_[pb_mode_id].push_back(curr_edge);
  return true;
}

bool ArchGen::add_in2in_edge(const std::string& mode_name, const openfpga::BasicPort& src_port,
                             const EdgePort& des_port, const EdgeDelay& delay) {
  return add_in2in_edge(mode_name, src_port, des_port, std::string(), delay);
}

bool ArchGen::add_internal_edge(const std::string& mode_name, const EdgePort& src_port,
                                const EdgePort& des_port, const std::string& circuit_model_name,
                                const EdgeDelay& delay) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  std::string src_pb = std::get<0>(src_port);
  size_t src_pb_idx = std::get<1>(src_port);
  openfpga::BasicPort src_pb_port = std::get<2>(src_port);
  if (!valid_pb_mode_child_pb_index(mode_name, src_pb, src_pb_idx, true)) {
    return false;
  }
  if (!valid_pb_type_port(src_pb, src_pb_port, true)) {
    return false;
  }
  std::string des_pb = std::get<0>(des_port);
  size_t des_pb_idx = std::get<1>(des_port);
  openfpga::BasicPort des_pb_port = std::get<2>(des_port);
  if (!valid_pb_mode_child_pb_index(mode_name, des_pb, des_pb_idx, true)) {
    return false;
  }
  if (!valid_pb_type_port(des_pb, des_pb_port, true)) {
    return false;
  }
  PbTypeEdge curr_edge;
  curr_edge.src_port = src_port;
  curr_edge.des_port = des_port;
  curr_edge.delay = delay;
  curr_edge.circuit_model_name = circuit_model_name;
  pb_mode_edges_[pb_mode_id].push_back(curr_edge);
  return true;
}

bool ArchGen::add_internal_edge(const std::string& mode_name, const EdgePort& src_port,
                                const EdgePort& des_port, const EdgeDelay& delay) {
  return add_internal_edge(mode_name, src_port, des_port, std::string(), delay);
}

bool ArchGen::set_in2in_edge_delay(const std::string& mode_name,
                                   const openfpga::BasicPort& src_port, const EdgePort& des_port,
                                   const EdgeDelay& delay) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  for (size_t iedge = 0; iedge < pb_mode_edges_[pb_mode_id].size(); ++iedge) {
    if (pb_mode_edges_[pb_mode_id][iedge].src_port == EdgePort(parent_pb_type, 0, src_port) &&
        pb_mode_edges_[pb_mode_id][iedge].des_port == des_port) {
      pb_mode_edges_[pb_mode_id][iedge].delay = delay;
      return true;
    }
  }
  return false;
}

bool ArchGen::set_pb_type_mode(const std::string& pb_type_name, const std::string& mode_name,
                               const bool& is_physical) {
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  if (pb_type_modes_[pb_type_id].end() !=
      std::find(pb_type_modes_[pb_type_id].begin(), pb_type_modes_[pb_type_id].end(), mode_name)) {
    VTR_LOG_ERROR("The mode '%s' is already defined under pb_type '%s'!\n", mode_name.c_str(),
                  pb_type_name.c_str());
    return false;
  }
  pb_type_modes_[pb_type_id].push_back(mode_name);
  pb_type_mode_is_physical_[pb_type_id].push_back(is_physical);
  return true;
}

bool ArchGen::add_out2out_edge(const std::string& mode_name, const EdgePort& src_port,
                               const openfpga::BasicPort& des_port,
                               const std::string& circuit_model_name, const EdgeDelay& delay) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  /* Check if the auto-wiring can be applied */
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  size_t parent_pb_type_id = find_pb_type_id(parent_pb_type);
  if (!valid_pb_type_id(parent_pb_type_id)) {
    VTR_LOG_ERROR("The mode '%s' does not belong to any pb type '%s'!\n", mode_name.c_str(),
                  parent_pb_type.c_str());
    return false;
  }
  if (!valid_pb_type_port(parent_pb_type, des_port, true)) {
    return false;
  }
  std::string src_pb = std::get<0>(src_port);
  size_t src_pb_idx = std::get<1>(src_port);
  openfpga::BasicPort src_pb_port = std::get<2>(src_port);
  if (!valid_pb_mode_child_pb_index(mode_name, src_pb, src_pb_idx, true)) {
    return false;
  }
  if (!valid_pb_type_port(src_pb, src_pb_port, true)) {
    return false;
  }
  PbTypeEdge curr_edge;
  curr_edge.src_port = src_port;
  curr_edge.des_port = {parent_pb_type, 0, des_port};
  curr_edge.delay = delay;
  curr_edge.circuit_model_name = circuit_model_name;
  pb_mode_edges_[pb_mode_id].push_back(curr_edge);
  return true;
}

bool ArchGen::add_out2out_edge(const std::string& mode_name, const EdgePort& src_port,
                               const openfpga::BasicPort& des_port, const EdgeDelay& delay) {
  return add_out2out_edge(mode_name, src_port, des_port, std::string(), delay);
}

int ArchGen::add_xml_vpr_arch_complexblocklist(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node cmplxblklist_node =
    root_node.append_child(XML_VPR_ARCH_COMPLEXBLOCKLIST_NODE_NAME);

  for (auto subtile : arch_layout_.subtiles()) {
    num_err += add_xml_vpr_arch_pb_type_tree(cmplxblklist_node, subtile);
  }

  return num_err;
}

std::string ArchGen::find_pb_type_port_xml_node_name(
  const VprArchLayout::e_port_type& port_type) const {
  switch (port_type) {
    case VprArchLayout::e_port_type::INPUT:
    case VprArchLayout::e_port_type::RESET:
    case VprArchLayout::e_port_type::SCANI:
    case VprArchLayout::e_port_type::CI:
      return std::string(XML_VPR_ARCH_PB_TYPE_INPUT_NODE_NAME);
    case VprArchLayout::e_port_type::OUTPUT:
    case VprArchLayout::e_port_type::SCANO:
    case VprArchLayout::e_port_type::CO:
      return std::string(XML_VPR_ARCH_PB_TYPE_OUTPUT_NODE_NAME);
    case VprArchLayout::e_port_type::CLOCK:
      return std::string(XML_VPR_ARCH_PB_TYPE_CLOCK_NODE_NAME);
    default:
      VTR_LOG_ERROR("Invalid port type!");
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return std::string();
}

int ArchGen::add_xml_vpr_arch_pb_type_declaration(pugi::xml_node& pb_type_node,
                                                  const std::string& pb_type_name,
                                                  const bool& is_top, const size_t& num_pb) const {
  int num_err = 0;
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    return 1;
  }
  /* Add the xml node for top-level pb type */
  pb_type_node.append_attribute(XML_VPR_ARCH_PB_TYPE_ATTR_NAME) = pb_type_name.c_str();
  if (!is_top) {
    pb_type_node.append_attribute(XML_VPR_ARCH_PB_TYPE_ATTR_NUM_PB) =
      std::to_string(num_pb).c_str();
  }
  /* Primitve should have a blif model */
  bool is_pmtv = pb_type_modes_[pb_type_id].empty() ? true : false;
  if (is_pmtv) {
    pb_type_node.append_attribute(XML_VPR_ARCH_PB_TYPE_ATTR_BLIF_MODEL) =
      pb_type_blif_models_[pb_type_id].c_str();
    if (valid_pb_type_class(pb_type_classes_[pb_type_id])) {
      pb_type_node.append_attribute(XML_VPR_ARCH_PB_TYPE_ATTR_CLASS) =
        pb_type_class2str(pb_type_classes_[pb_type_id]).c_str();
    }
  }
  /* Add ports */
  for (size_t iport = 0; iport < pb_type_port_types_[pb_type_id].size(); ++iport) {
    std::string curr_port_type_str =
      find_pb_type_port_xml_node_name(pb_type_port_types_[pb_type_id][iport]);
    pugi::xml_node curr_port_node = pb_type_node.append_child(curr_port_type_str.c_str());
    curr_port_node.append_attribute(XML_VPR_ARCH_PB_TYPE_PORT_ATTR_NAME) =
      pb_type_ports_[pb_type_id][iport].get_name().c_str();
    curr_port_node.append_attribute(XML_VPR_ARCH_PB_TYPE_PORT_ATTR_NUM_PINS) =
      std::to_string(pb_type_ports_[pb_type_id][iport].get_width()).c_str();
    if (is_pmtv && valid_port_class(pb_type_port_classes_[pb_type_id][iport])) {
      curr_port_node.append_attribute(XML_VPR_ARCH_PB_TYPE_PORT_ATTR_CLASS) =
        port_class2str(pb_type_port_classes_[pb_type_id][iport]).c_str();
    }
    if (!is_top) {
      continue;
    }
    if (pb_type_port_logic_equi_[pb_type_id][iport]) {
      curr_port_node.append_attribute(XML_VPR_ARCH_PB_TYPE_PORT_ATTR_EQUIVALENT) = "full";
    }
    if (pb_type_port_types_[pb_type_id][iport] == VprArchLayout::e_port_type::RESET ||
        pb_type_port_types_[pb_type_id][iport] == VprArchLayout::e_port_type::SCANI) {
      curr_port_node.append_attribute(XML_VPR_ARCH_PB_TYPE_PORT_ATTR_IS_NON_CLOCK_GLOBAL) = "true";
    }
  }
  /* Add primitive pb timing */
  if (is_pmtv) {
    /* delay constant */
    for (auto timing_info : pb_type_timing_[pb_type_id]) {
      num_err +=
        add_xml_vpr_arch_primitive_pb_type_timing_node(pb_type_node, pb_type_name, timing_info);
    }
  }
  return num_err;
}

int ArchGen::add_xml_vpr_arch_primitive_pb_type_timing_node(pugi::xml_node& root_node,
                                                            const std::string& pb_type_name,
                                                            const PbTypeTiming& timing_info) const {
  int num_err = 0;
  if (timing_info.type == e_pb_type_timing_type::SETUP) {
    pugi::xml_node curr_tnode = root_node.append_child(XML_VPR_ARCH_PB_TYPE_TSETUP_NODE_NAME);
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_TSETUP_ATTR_VALUE) = timing_info.max.c_str();
    std::string port = pb_type_name + std::string(".") + timing_info.data_port.to_verilog_string();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_TSETUP_ATTR_PORT) = port.c_str();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_TSETUP_ATTR_CLOCK) =
      timing_info.clk_port.get_name().c_str();
  }
  if (timing_info.type == e_pb_type_timing_type::HOLD) {
    pugi::xml_node curr_tnode = root_node.append_child(XML_VPR_ARCH_PB_TYPE_THOLD_NODE_NAME);
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_THOLD_ATTR_VALUE) = timing_info.max.c_str();
    std::string port = pb_type_name + std::string(".") + timing_info.data_port.to_verilog_string();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_THOLD_ATTR_PORT) = port.c_str();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_THOLD_ATTR_CLOCK) =
      timing_info.clk_port.get_name().c_str();
  } else if (timing_info.type == e_pb_type_timing_type::CLK2Q) {
    pugi::xml_node curr_tnode = root_node.append_child(XML_VPR_ARCH_PB_TYPE_TCLK2Q_NODE_NAME);
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_TCLK2Q_ATTR_MAX) = timing_info.max.c_str();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_TCLK2Q_ATTR_MIN) = timing_info.min.c_str();
    std::string port = pb_type_name + std::string(".") + timing_info.data_port.to_verilog_string();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_TCLK2Q_ATTR_PORT) = port.c_str();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_TCLK2Q_ATTR_CLOCK) =
      timing_info.clk_port.get_name().c_str();
  } else if (timing_info.type == e_pb_type_timing_type::COMB) {
    pugi::xml_node curr_tnode =
      root_node.append_child(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_NODE_NAME);
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_MAX) =
      timing_info.max.c_str();
    /* Min is optional */
    if (!timing_info.min.empty()) {
      curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_MIN) =
        timing_info.min.c_str();
    }
    std::string in_port =
      pb_type_name + std::string(".") + timing_info.clk_port.to_verilog_string();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_IN_PORT) = in_port.c_str();
    std::string out_port =
      pb_type_name + std::string(".") + timing_info.data_port.to_verilog_string();
    curr_tnode.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_OUT_PORT) =
      out_port.c_str();
  }

  return num_err;
}

std::string ArchGen::pb_type_interconnect_name(const EdgePin& edge_pin) const {
  std::string name_str;
  openfpga::BasicPort curr_pb_type(std::get<0>(edge_pin), std::get<1>(edge_pin),
                                   std::get<1>(edge_pin));
  openfpga::BasicPort curr_pin(std::get<2>(edge_pin), std::get<3>(edge_pin), std::get<3>(edge_pin));
  name_str = curr_pb_type.to_verilog_string() + std::string("_") + curr_pin.to_verilog_string();
  return name_str;
}

int ArchGen::compile_pb_type_mode_interconnects(
  const size_t& mode_id, std::map<EdgePin, std::vector<size_t>>& interc) const {
  int num_err = 0;
  if (!valid_pb_mode_id(mode_id)) {
    return 1;
  }
  /* TODO: Compile the edges and identify the type for each ending point of the edge.
   * If multiple edges have a common ending point, then a MUX will be added as the interconnect
   * Otherwise, single driver ending point will be inferred as direct interconnect for each
   */
  for (size_t iedge = 0; iedge < pb_mode_edges_[mode_id].size(); ++iedge) {
    PbTypeEdge edge = pb_mode_edges_[mode_id][iedge];
    for (auto pin : std::get<2>(edge.des_port).pins()) {
      EdgePin curr_pin = std::make_tuple(std::get<0>(edge.des_port), std::get<1>(edge.des_port),
                                         std::get<2>(edge.des_port).get_name(), pin);
      auto result = interc.find(curr_pin);
      if (result == interc.end()) {
        interc[curr_pin].push_back(iedge);
      } else {
        result->second.push_back(iedge);
      }
    }
  }
  return num_err;
}

int ArchGen::add_xml_vpr_arch_mode_interconnects(pugi::xml_node& root_node,
                                                 const size_t& mode_id) const {
  int num_err = 0;
  if (!valid_pb_mode_id(mode_id)) {
    return 1;
  }
  std::map<EdgePin, std::vector<size_t>> interc;
  num_err += compile_pb_type_mode_interconnects(mode_id, interc);
  if (num_err) {
    return 1;
  }

  /* Output interconnect nodes in XML */
  pugi::xml_node interc_node = root_node.append_child(XML_VPR_ARCH_PB_TYPE_INTERCONNECT_NODE_NAME);
  for (auto pair : interc) {
    pugi::xml_node curr_interc_node;
    if (pair.second.empty()) {
      VTR_LOG_ERROR("Invalid connect for pin '%s[%lu].%s' which has no driver!\n",
                    std::get<0>(pair.first).c_str(), std::get<1>(pair.first),
                    openfpga::BasicPort(std::get<2>(pair.first), std::get<3>(pair.first),
                                        std::get<3>(pair.first))
                      .to_verilog_string()
                      .c_str());
      num_err++;
      break;
    } else if (pair.second.size() == 1) {
      curr_interc_node =
        interc_node.append_child(XML_VPR_ARCH_PB_TYPE_INTERCONNECT_DIRECT_NODE_NAME);
    } else {
      curr_interc_node = interc_node.append_child(XML_VPR_ARCH_PB_TYPE_INTERCONNECT_MUX_NODE_NAME);
    }
    /* Output port attribute */
    std::string out_port_str;
    {
      openfpga::BasicPort curr_pb_type(std::get<0>(pair.first), std::get<1>(pair.first),
                                       std::get<1>(pair.first));
      openfpga::BasicPort curr_pin(std::get<2>(pair.first), std::get<3>(pair.first),
                                   std::get<3>(pair.first));
      out_port_str =
        curr_pb_type.to_verilog_string() + std::string(".") + curr_pin.to_verilog_string();
    }
    /* Give a custom name */
    std::string name_str = pb_type_interconnect_name(pair.first);
    curr_interc_node.append_attribute(XML_VPR_ARCH_PB_TYPE_INTERCONNECT_ATTR_NAME) =
      name_str.c_str();
    /* Input port attribute */
    std::string in_port_str;
    for (size_t src_edge_idx : pair.second) {
      openfpga::BasicPort curr_pb_type(std::get<0>(pb_mode_edges_[mode_id][src_edge_idx].src_port),
                                       std::get<1>(pb_mode_edges_[mode_id][src_edge_idx].src_port),
                                       std::get<1>(pb_mode_edges_[mode_id][src_edge_idx].src_port));
      in_port_str +=
        curr_pb_type.to_verilog_string() + std::string(".") +
        std::get<2>(pb_mode_edges_[mode_id][src_edge_idx].src_port).to_verilog_string();
      in_port_str += std::string(" ");
    }
    in_port_str.pop_back(); /* Kick out the last space */
    curr_interc_node.append_attribute(XML_VPR_ARCH_PB_TYPE_INTERCONNECT_ATTR_INPUT) =
      in_port_str.c_str();
    curr_interc_node.append_attribute(XML_VPR_ARCH_PB_TYPE_INTERCONNECT_ATTR_OUTPUT) =
      out_port_str.c_str();
    /* Delay for each edge */
    for (size_t src_edge_idx : pair.second) {
      /* Skip zero delay edges */
      if (pb_mode_edges_[mode_id][src_edge_idx].delay == EdgeDelay{"0", "0"}) {
        continue;
      }
      pugi::xml_node curr_delay_node =
        curr_interc_node.append_child(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_NODE_NAME);
      openfpga::BasicPort curr_pb_type(std::get<0>(pb_mode_edges_[mode_id][src_edge_idx].src_port),
                                       std::get<1>(pb_mode_edges_[mode_id][src_edge_idx].src_port),
                                       std::get<1>(pb_mode_edges_[mode_id][src_edge_idx].src_port));
      std::string edge_src_port_str =
        curr_pb_type.to_verilog_string() + std::string(".") +
        std::get<2>(pb_mode_edges_[mode_id][src_edge_idx].src_port).to_verilog_string();
      curr_delay_node.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_MAX) =
        pb_mode_edges_[mode_id][src_edge_idx].delay.first.c_str();
      if (pb_mode_edges_[mode_id][src_edge_idx].delay.second != std::string("0")) {
        curr_delay_node.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_MIN) =
          pb_mode_edges_[mode_id][src_edge_idx].delay.second.c_str();
      }
      curr_delay_node.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_IN_PORT) =
        edge_src_port_str.c_str();
      curr_delay_node.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_OUT_PORT) =
        out_port_str.c_str();
    }
    /* Pack patterns only apply to direct */
    for (size_t src_edge_idx : pair.second) {
      /* Skip pack patterns if not defined */
      for (std::string pack_pattern_name : pb_mode_edges_[mode_id][src_edge_idx].pack_patterns) {
        openfpga::BasicPort src_pb_type(
          std::get<0>(pb_mode_edges_[mode_id][src_edge_idx].src_port),
          std::get<1>(pb_mode_edges_[mode_id][src_edge_idx].src_port),
          std::get<1>(pb_mode_edges_[mode_id][src_edge_idx].src_port));
        std::string in_port_str =
          src_pb_type.to_verilog_string() + std::string(".") +
          std::get<2>(pb_mode_edges_[mode_id][src_edge_idx].src_port).to_verilog_string();
        openfpga::BasicPort des_pb_type(std::get<0>(pair.first), std::get<1>(pair.first),
                                        std::get<1>(pair.first));
        openfpga::BasicPort des_pin(std::get<2>(pair.first), std::get<3>(pair.first),
                                    std::get<3>(pair.first));
        std::string out_port_str =
          des_pb_type.to_verilog_string() + std::string(".") + des_pin.to_verilog_string();

        pugi::xml_node curr_pp_node =
          curr_interc_node.append_child(XML_VPR_ARCH_PB_TYPE_PACK_PATTERN_NODE_NAME);
        curr_pp_node.append_attribute(XML_VPR_ARCH_PB_TYPE_PACK_PATTERN_ATTR_NAME) =
          pack_pattern_name.c_str();
        curr_pp_node.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_IN_PORT) =
          in_port_str.c_str();
        curr_pp_node.append_attribute(XML_VPR_ARCH_PB_TYPE_DELAY_CONSTANT_ATTR_OUT_PORT) =
          out_port_str.c_str();
      }
    }
  }
  return num_err;
}

int ArchGen::rec_add_xml_vpr_arch_pb_type(pugi::xml_node& root_node,
                                          const std::string& pb_type_name,
                                          const size_t& num_pb) const {
  int num_err = 0;
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    return 1;
  }
  num_err += add_xml_vpr_arch_pb_type_declaration(root_node, pb_type_name, false, num_pb);
  if (num_err) {
    return num_err;
  }
  /* Walk through all the modes */
  for (size_t imode = 0; imode < pb_type_modes_[pb_type_id].size(); ++imode) {
    pugi::xml_node curr_mode_node = root_node.append_child(XML_VPR_ARCH_PB_TYPE_MODE_NODE_NAME);
    std::string mode_name = pb_type_modes_[pb_type_id][imode];
    curr_mode_node.append_attribute(XML_VPR_ARCH_PB_TYPE_MODE_ATTR_NAME) = mode_name.c_str();
    /* TODO: Should have a dedicated flag to handle this */
    if (pb_type_mode_is_physical_[pb_type_id][imode] && pb_type_modes_[pb_type_id].size() > 1) {
      curr_mode_node.append_attribute(XML_VPR_ARCH_PB_TYPE_MODE_ATTR_DISABLE_PACKING) = "true";
    }
    size_t pb_mode_id = find_pb_mode_id(mode_name);
    if (!valid_pb_mode_id(pb_mode_id)) {
      return 1;
    }
    for (auto pair : pb_mode_childs_[pb_mode_id]) {
      pugi::xml_node child_pb_type_node =
        curr_mode_node.append_child(XML_VPR_ARCH_PB_TYPE_NODE_NAME);
      num_err += rec_add_xml_vpr_arch_pb_type(child_pb_type_node, pair.first, pair.second);
    }
    /* Add interconnect */
    num_err += add_xml_vpr_arch_mode_interconnects(curr_mode_node, pb_mode_id);
  }

  return num_err;
}

int ArchGen::add_xml_vpr_arch_pb_type_tree(pugi::xml_node& root_node,
                                           const std::string& top_pb_type_name) const {
  int num_err = 0;
  size_t pb_type_id = find_pb_type_id(top_pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    return 1;
  }
  pugi::xml_node pb_type_node = root_node.append_child(XML_VPR_ARCH_PB_TYPE_NODE_NAME);
  num_err += add_xml_vpr_arch_pb_type_declaration(pb_type_node, top_pb_type_name, true, 1);
  if (num_err) {
    return num_err;
  }
  /* Walk through modes and then go recursive on their child pb_types */
  for (size_t imode = 0; imode < pb_type_modes_[pb_type_id].size(); ++imode) {
    pugi::xml_node curr_mode_node = pb_type_node.append_child(XML_VPR_ARCH_PB_TYPE_MODE_NODE_NAME);
    std::string mode_name = pb_type_modes_[pb_type_id][imode];
    curr_mode_node.append_attribute(XML_VPR_ARCH_PB_TYPE_MODE_ATTR_NAME) = mode_name.c_str();
    /* TODO: Should have a dedicated flag to handle this */
    if (pb_type_mode_is_physical_[pb_type_id][imode] && pb_type_modes_[pb_type_id].size() > 1) {
      curr_mode_node.append_attribute(XML_VPR_ARCH_PB_TYPE_MODE_ATTR_DISABLE_PACKING) = "true";
    }
    size_t pb_mode_id = find_pb_mode_id(mode_name);
    if (!valid_pb_mode_id(pb_mode_id)) {
      return 1;
    }
    for (auto pair : pb_mode_childs_[pb_mode_id]) {
      pugi::xml_node child_pb_type_node =
        curr_mode_node.append_child(XML_VPR_ARCH_PB_TYPE_NODE_NAME);
      num_err += rec_add_xml_vpr_arch_pb_type(child_pb_type_node, pair.first, pair.second);
    }
    /* Add interconnect */
    num_err += add_xml_vpr_arch_mode_interconnects(curr_mode_node, pb_mode_id);
  }

  return num_err;
}

bool ArchGen::set_pb_type_class(const std::string& name, const e_pb_type_class& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  pb_type_classes_[pb_type_id] = val;
  return true;
}

bool ArchGen::set_pb_type_circuit_model(const std::string& name, const std::string& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  pb_type_circuit_model_names_[pb_type_id] = val;
  return true;
}

bool ArchGen::set_pb_type_physical_pb_type(const std::string& name, const std::string& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  pb_type_physical_pb_types_[pb_type_id] = val;
  return true;
}

bool ArchGen::set_pb_type_physical_pb_type_index_factor(const std::string& name,
                                                        const std::string& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  pb_type_physical_index_factors_[pb_type_id] = val;
  return true;
}

bool ArchGen::set_pb_type_physical_pb_type_index_offset(const std::string& name,
                                                        const std::string& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  pb_type_physical_index_offsets_[pb_type_id] = val;
  return true;
}

bool ArchGen::set_pb_type_mode_bits(const std::string& name, const std::string& val) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  pb_type_mode_bits_[pb_type_id] = val;
  return true;
}

bool ArchGen::set_pb_type_port_class(const std::string& name, const openfpga::BasicPort& port,
                                     const e_port_class& port_class) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Check port name collision */
  for (size_t iport = 0; iport < pb_type_ports_[pb_type_id].size(); ++iport) {
    if (pb_type_ports_[pb_type_id][iport].get_name() == port.get_name()) {
      pb_type_port_classes_[pb_type_id][iport] = port_class;
      return true;
    }
  }
  VTR_LOG_ERROR("Invalid port '%s' for pb_type name '%s' which is not defined!\n",
                port.to_verilog_string().c_str(), name.c_str());
  return false;
}

bool ArchGen::set_pb_type_physical_port_binding(const std::string& name,
                                                const std::string& op_port_name,
                                                const openfpga::BasicPort& phy_port,
                                                const std::string& rotate_offset) {
  size_t pb_type_id = find_pb_type_id(name);
  if (!valid_pb_type_id(pb_type_id)) {
    return false;
  }
  /* Check port name collision */
  for (size_t iport = 0; iport < pb_type_ports_[pb_type_id].size(); ++iport) {
    if (pb_type_ports_[pb_type_id][iport].get_name() == op_port_name) {
      pb_type_physical_ports_[pb_type_id][iport] = phy_port;
      if (!rotate_offset.empty()) {
        pb_type_physical_pin_rotate_offsets_[pb_type_id][iport] = rotate_offset;
      }
      return true;
    }
  }
  VTR_LOG_ERROR("Invalid port '%s' for pb_type name '%s' which is not defined!\n",
                op_port_name.c_str(), name.c_str());
  return false;
}

void ArchGen::check_valid_pb_type_class(const ArchGen::e_pb_type_class& type) const {
  if (!valid_pb_type_class(type)) {
    VTR_LOG_ERROR("pb_type class type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

ArchGen::e_pb_type_class ArchGen::str2pb_type_class(const std::string& str,
                                                    const bool& verbose) const {
  for (int itype = size_t(ArchGen::e_pb_type_class::LUT);
       itype < size_t(ArchGen::e_pb_type_class::NUM_TYPES); ++itype) {
    if (str == std::string(PB_TYPE_CLASS_STRING_[itype])) {
      return static_cast<ArchGen::e_pb_type_class>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for pb_type class! Expect %s\n",
                 pb_type_class_all2str().c_str());
  return ArchGen::e_pb_type_class::NUM_TYPES;
}

std::string ArchGen::pb_type_class2str(const ArchGen::e_pb_type_class& type,
                                       const bool& verbose) const {
  if (!valid_pb_type_class(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for pb_type class! Expect %s\n",
                   pb_type_class_all2str().c_str());
    return std::string();
  }
  return std::string(PB_TYPE_CLASS_STRING_[size_t(type)]);
}

std::string ArchGen::pb_type_class_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(ArchGen::e_pb_type_class::LUT);
       itype < size_t(ArchGen::e_pb_type_class::NUM_TYPES); ++itype) {
    full_types += std::string(PB_TYPE_CLASS_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::vector<ArchGen::e_pb_type_class> ArchGen::pb_type_classs() const {
  std::vector<e_pb_type_class> pb_type_classs;
  for (int itype = size_t(ArchGen::e_pb_type_class::LUT);
       itype < size_t(ArchGen::e_pb_type_class::NUM_TYPES); ++itype) {
    pb_type_classs.push_back(static_cast<ArchGen::e_pb_type_class>(itype));
  }
  return pb_type_classs;
}

bool ArchGen::valid_pb_type_class(const e_pb_type_class& type) const {
  return type != ArchGen::e_pb_type_class::NUM_TYPES;
}

void ArchGen::check_valid_port_class(const ArchGen::e_port_class& type) const {
  if (!valid_port_class(type)) {
    VTR_LOG_ERROR("pb_type port class type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

ArchGen::e_port_class ArchGen::str2port_class(const std::string& str, const bool& verbose) const {
  for (int itype = size_t(ArchGen::e_port_class::LUT_IN);
       itype < size_t(ArchGen::e_port_class::NUM_TYPES); ++itype) {
    if (str == std::string(PORT_CLASS_STRING_[itype])) {
      return static_cast<ArchGen::e_port_class>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for pb_type port class! Expect %s\n",
                 port_class_all2str().c_str());
  return ArchGen::e_port_class::NUM_TYPES;
}

std::string ArchGen::port_class2str(const ArchGen::e_port_class& type, const bool& verbose) const {
  if (!valid_port_class(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for pb_type port class! Expect %s\n",
                   port_class_all2str().c_str());
    return std::string();
  }
  return std::string(PORT_CLASS_STRING_[size_t(type)]);
}

std::string ArchGen::port_class_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(ArchGen::e_port_class::LUT_IN);
       itype < size_t(ArchGen::e_port_class::NUM_TYPES); ++itype) {
    full_types += std::string(PORT_CLASS_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::vector<ArchGen::e_port_class> ArchGen::port_classs() const {
  std::vector<e_port_class> port_classs;
  for (int itype = size_t(ArchGen::e_port_class::LUT_IN);
       itype < size_t(ArchGen::e_port_class::NUM_TYPES); ++itype) {
    port_classs.push_back(static_cast<ArchGen::e_port_class>(itype));
  }
  return port_classs;
}

bool ArchGen::valid_port_class(const e_port_class& type) const {
  return type != ArchGen::e_port_class::NUM_TYPES;
}

bool ArchGen::set_in2in_edge_pack_pattern(const std::string& mode_name,
                                          const openfpga::BasicPort& src_port,
                                          const EdgePort& des_port,
                                          const std::string& pack_pattern) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  for (size_t iedge = 0; iedge < pb_mode_edges_[pb_mode_id].size(); ++iedge) {
    if (pb_mode_edges_[pb_mode_id][iedge].src_port == EdgePort(parent_pb_type, 0, src_port) &&
        pb_mode_edges_[pb_mode_id][iedge].des_port == des_port) {
      if (std::find(pb_mode_edges_[pb_mode_id][iedge].pack_patterns.begin(),
                    pb_mode_edges_[pb_mode_id][iedge].pack_patterns.end(),
                    pack_pattern) != pb_mode_edges_[pb_mode_id][iedge].pack_patterns.end()) {
        VTR_LOG_ERROR("Pack pattern '%s' is already defined for edge %s[%lu].%s -> %s[%lu].%s!\n",
                      pack_pattern.c_str(), parent_pb_type.c_str(), 0,
                      src_port.to_verilog_string().c_str(), std::get<0>(des_port).c_str(),
                      std::get<1>(des_port), std::get<2>(des_port).to_verilog_string().c_str());
        return false;
      }
      pb_mode_edges_[pb_mode_id][iedge].pack_patterns.push_back(pack_pattern);
      return true;
    }
  }
  return false;
}

bool ArchGen::set_internal_edge_pack_pattern(const std::string& mode_name, const EdgePort& src_port,
                                             const EdgePort& des_port,
                                             const std::string& pack_pattern) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  for (size_t iedge = 0; iedge < pb_mode_edges_[pb_mode_id].size(); ++iedge) {
    if (pb_mode_edges_[pb_mode_id][iedge].src_port == src_port &&
        pb_mode_edges_[pb_mode_id][iedge].des_port == des_port) {
      if (std::find(pb_mode_edges_[pb_mode_id][iedge].pack_patterns.begin(),
                    pb_mode_edges_[pb_mode_id][iedge].pack_patterns.end(),
                    pack_pattern) != pb_mode_edges_[pb_mode_id][iedge].pack_patterns.end()) {
        VTR_LOG_ERROR("Pack pattern '%s' is already defined for edge %s[%lu].%s -> %s[%lu].%s!\n",
                      pack_pattern.c_str(), std::get<0>(src_port).c_str(), std::get<1>(src_port),
                      std::get<2>(src_port).to_verilog_string().c_str(),
                      std::get<0>(des_port).c_str(), std::get<1>(des_port),
                      std::get<2>(des_port).to_verilog_string().c_str());
        return false;
      }
      pb_mode_edges_[pb_mode_id][iedge].pack_patterns.push_back(pack_pattern);
      return true;
    }
  }
  return false;
}

bool ArchGen::set_out2out_edge_pack_pattern(const std::string& mode_name, const EdgePort& src_port,
                                            const openfpga::BasicPort& des_port,
                                            const std::string& pack_pattern) {
  size_t pb_mode_id = find_pb_mode_id(mode_name);
  if (!valid_pb_mode_id(pb_mode_id)) {
    return false;
  }
  std::string parent_pb_type = pb_mode_parents_[pb_mode_id];
  for (size_t iedge = 0; iedge < pb_mode_edges_[pb_mode_id].size(); ++iedge) {
    if (pb_mode_edges_[pb_mode_id][iedge].des_port == EdgePort(parent_pb_type, 0, des_port) &&
        pb_mode_edges_[pb_mode_id][iedge].src_port == src_port) {
      if (std::find(pb_mode_edges_[pb_mode_id][iedge].pack_patterns.begin(),
                    pb_mode_edges_[pb_mode_id][iedge].pack_patterns.end(),
                    pack_pattern) != pb_mode_edges_[pb_mode_id][iedge].pack_patterns.end()) {
        VTR_LOG_ERROR("Pack pattern '%s' is already defined for edge %s[%lu].%s -> %s[%lu].%s!\n",
                      pack_pattern.c_str(), std::get<0>(src_port).c_str(), std::get<1>(src_port),
                      std::get<2>(src_port).to_verilog_string().c_str(), parent_pb_type.c_str(), 0,
                      des_port.to_verilog_string().c_str());
        return false;
      }
      pb_mode_edges_[pb_mode_id][iedge].pack_patterns.push_back(pack_pattern);
      return true;
    }
  }
  return false;
}

int ArchGen::add_xml_openfpga_arch_techlib_device_logic(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node dmdl_node =
    root_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_NODE_NAME);
  dmdl_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_ATTR_NAME) = "logic";
  dmdl_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_ATTR_TYPE) = "transistor";

  pugi::xml_node dmdl_lib_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_NODE_NAME);
  dmdl_lib_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_TYPE) = "industry";
  dmdl_lib_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_CORNER) = "TOP_TT";
  dmdl_lib_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_REF) = "M";
  dmdl_lib_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_PATH) =
    "${OPENFPGA_PATH}/openfpga_flow/tech/PTM_45nm/45nm.pm";

  pugi::xml_node dmdl_dsgn_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_NODE_NAME);
  dmdl_dsgn_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_ATTR_VDD) = "0.9";
  dmdl_dsgn_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_ATTR_PN_RATIO) =
    "2";

  pugi::xml_node dmdl_pmos_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_PMOS_NODE_NAME);
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_NAME) = "pch";
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_CHAN_LENGTH) =
    "40e-9";
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_MIN_WIDTH) =
    "140e-9";
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_VARIATION) =
    "logic_transistor_var";

  pugi::xml_node dmdl_nmos_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_NMOS_NODE_NAME);
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_NAME) = "nch";
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_CHAN_LENGTH) =
    "40e-9";
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_MIN_WIDTH) =
    "140e-9";
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_VARIATION) =
    "logic_transistor_var";

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_techlib_device_io(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node dmdl_node =
    root_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_NODE_NAME);
  dmdl_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_ATTR_NAME) = "io";
  dmdl_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_ATTR_TYPE) = "transistor";

  pugi::xml_node dmdl_lib_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_NODE_NAME);
  dmdl_lib_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_TYPE) = "academia";
  dmdl_lib_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_REF) = "M";
  dmdl_lib_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_LIB_ATTR_PATH) =
    "${OPENFPGA_PATH}/openfpga_flow/tech/PTM_45nm/45nm.pm";

  pugi::xml_node dmdl_dsgn_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_NODE_NAME);
  dmdl_dsgn_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_ATTR_VDD) = "2.5";
  dmdl_dsgn_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_DESIGN_ATTR_PN_RATIO) =
    "3";

  pugi::xml_node dmdl_pmos_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_PMOS_NODE_NAME);
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_NAME) = "pch_25";
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_CHAN_LENGTH) =
    "270e-9";
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_MIN_WIDTH) =
    "320e-9";
  dmdl_pmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_VARIATION) =
    "io_transistor_var";

  pugi::xml_node dmdl_nmos_node =
    dmdl_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_NMOS_NODE_NAME);
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_NAME) = "nch_25";
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_CHAN_LENGTH) =
    "270e-9";
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_MIN_WIDTH) =
    "320e-9";
  dmdl_nmos_node.append_attribute(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_MODEL_MOS_ATTR_VARIATION) =
    "io_transistor_var";

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_techlib_device(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node dlib_node = root_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_DEVICE_NODE_NAME);
  num_err += add_xml_openfpga_arch_techlib_device_logic(dlib_node);
  num_err += add_xml_openfpga_arch_techlib_device_io(dlib_node);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_techlib_variation(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node vlib_node = root_node.append_child(XML_OPENFPGA_ARCH_VARIATION_LIB_NODE_NAME);

  pugi::xml_node vlib_logic_node = vlib_node.append_child(XML_OPENFPGA_ARCH_VARIATION_NODE_NAME);
  vlib_logic_node.append_attribute(XML_OPENFPGA_ARCH_VARIATION_ATTR_NAME) = "logic_transistor_var";
  vlib_logic_node.append_attribute(XML_OPENFPGA_ARCH_VARIATION_ATTR_ABS_DEVIATION) = "0.1";
  vlib_logic_node.append_attribute(XML_OPENFPGA_ARCH_VARIATION_ATTR_NUM_SIGMA) = "3";

  pugi::xml_node vlib_io_node = vlib_node.append_child(XML_OPENFPGA_ARCH_VARIATION_NODE_NAME);
  vlib_io_node.append_attribute(XML_OPENFPGA_ARCH_VARIATION_ATTR_NAME) = "io_transistor_var";
  vlib_io_node.append_attribute(XML_OPENFPGA_ARCH_VARIATION_ATTR_ABS_DEVIATION) = "0.1";
  vlib_io_node.append_attribute(XML_OPENFPGA_ARCH_VARIATION_ATTR_NUM_SIGMA) = "3";

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_techlib(pugi::xml_node& root_node) const {
  int num_err = 0;
  /* All hardcoded */
  pugi::xml_node techlib_node = root_node.append_child(XML_OPENFPGA_ARCH_TECHLIB_NODE_NAME);
  num_err += add_xml_openfpga_arch_techlib_device(techlib_node);
  num_err += add_xml_openfpga_arch_techlib_variation(techlib_node);
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_inv(pugi::xml_node& root_node,
                                                     const std::string& name,
                                                     const bool& is_default) const {
  int num_err = 0;
  std::string cmdl_name = name;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "inv_buf";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST) =
    CUSTOM_VERILOG_NETLIST_FILE_.c_str();

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TOPOLOGY) =
    "inverter";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_SIZE) = "1";

  pugi::xml_node dev_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_NODE_NAME);
  dev_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_ATTR_DEVICE_MODEL_NAME) =
    "logic";

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "in";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "in_i";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "out";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "out_no";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Delay matrix */
    pugi::xml_node dm_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_NODE_NAME);
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_TYPE) = "rise";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_IN_PORT) = "in";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_OUT_PORT) = "out";
    dm_node.text().set("10e-12");
  }
  { /* Delay matrix */
    pugi::xml_node dm_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_NODE_NAME);
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_TYPE) = "fall";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_IN_PORT) = "in";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_OUT_PORT) = "out";
    dm_node.text().set("10e-12");
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_buf(pugi::xml_node& root_node,
                                                     const std::string& name,
                                                     const bool& is_default,
                                                     const size_t& drive_strength) const {
  int num_err = 0;
  std::string cmdl_name = name;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "inv_buf";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST) =
    CUSTOM_VERILOG_NETLIST_FILE_.c_str();

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TOPOLOGY) = "buffer";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_SIZE) = "1";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_NUM_LEVEL) = "2";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_F_PER_STAGE) =
    std::to_string(drive_strength).c_str();

  pugi::xml_node dev_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_NODE_NAME);
  dev_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_ATTR_DEVICE_MODEL_NAME) =
    "logic";

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "in";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "in_i";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "out";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "out_o";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Delay matrix */
    pugi::xml_node dm_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_NODE_NAME);
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_TYPE) = "rise";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_IN_PORT) = "in";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_OUT_PORT) = "out";
    dm_node.text().set("10e-12");
  }
  { /* Delay matrix */
    pugi::xml_node dm_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_NODE_NAME);
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_TYPE) = "fall";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_IN_PORT) = "in";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_OUT_PORT) = "out";
    dm_node.text().set("10e-12");
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_or2(pugi::xml_node& root_node,
                                                     const std::string& name,
                                                     const bool& is_default) const {
  int num_err = 0;
  std::string cmdl_name = name;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "gate";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST) =
    CUSTOM_VERILOG_NETLIST_FILE_.c_str();

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TOPOLOGY) = "OR";

  pugi::xml_node dev_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_NODE_NAME);
  dev_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_ATTR_DEVICE_MODEL_NAME) =
    "logic";

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "a";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "in0_i";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "b";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "in1_i";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "out";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "out_o";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Delay matrix */
    pugi::xml_node dm_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_NODE_NAME);
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_TYPE) = "rise";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_IN_PORT) = "a b";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_OUT_PORT) = "out";
    dm_node.text().set("10e-12 5e-12");
  }
  { /* Delay matrix */
    pugi::xml_node dm_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_NODE_NAME);
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_TYPE) = "fall";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_IN_PORT) = "a b";
    dm_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DELAY_MATRIX_ATTR_OUT_PORT) = "out";
    dm_node.text().set("10e-12 5e-12");
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_mux2(pugi::xml_node& root_node,
                                                      const std::string& name,
                                                      const bool& is_default) const {
  int num_err = 0;
  std::string cmdl_name = name;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "gate";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST) =
    CUSTOM_VERILOG_NETLIST_FILE_.c_str();

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TOPOLOGY) = "MUX2";

  pugi::xml_node dev_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_NODE_NAME);
  dev_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DEVICE_TECH_ATTR_DEVICE_MODEL_NAME) =
    "logic";

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "in0";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "in1_i";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "in1";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "in0_i";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "sel";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "sel_i";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "out";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "out_o";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_chan_wire(pugi::xml_node& root_node,
                                                           const std::string& name,
                                                           const bool& is_default) const {
  int num_err = 0;
  std::string cmdl_name = name;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "chan_wire";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = "track_seg";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "in";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "out";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* wire parameters */
    pugi::xml_node wpar_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_NODE_NAME);
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_MODEL_TYPE) = "pi";
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_R) = "101";
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_C) = "22.5e-15";
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_NUM_LEVEL) = "1";
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_wire(pugi::xml_node& root_node,
                                                      const std::string& name,
                                                      const bool& is_default) const {
  int num_err = 0;
  std::string cmdl_name = name;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "wire";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "in";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "out";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* wire parameters */
    pugi::xml_node wpar_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_NODE_NAME);
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_MODEL_TYPE) = "pi";
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_R) = "0";
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_C) = "0";
    wpar_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_WIRE_PARAM_ATTR_NUM_LEVEL) = "1";
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_mux(
  pugi::xml_node& root_node, const std::string& name, const std::string& mux2_name,
  const std::string& last_stage_mux2_name, const std::string& out_buffer_model,
  const std::string& const_input_val, const bool& is_default) const {
  int num_err = 0;
  std::string cmdl_name = name;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "mux";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_DUMP_STRUCTURAL_VERILOG) = "true";

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_STRUCTURE) = "tree";
  if (const_input_val == std::string("0") || const_input_val == std::string("1")) {
    dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_ADD_CONST_INPUT) =
      "true";
    dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_CONST_INPUT_VAL) =
      const_input_val.c_str();
  }

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) =
      out_buffer_model.empty() ? "false" : "true";
    if (!out_buffer_model.empty()) {
      out_buf_node.append_attribute(
        XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_CIRCUIT_MODEL_NAME) = out_buffer_model.c_str();
    }
  }

  { /* Pass gate logic */
    pugi::xml_node pgl_node = cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PGL_NODE_NAME);
    pgl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PGL_ATTR_CIRCUIT_MODEL_NAME) =
      mux2_name.c_str();
  }
  if (!last_stage_mux2_name.empty()) { /* Pass gate logic */
    pugi::xml_node pgl_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LSPGL_NODE_NAME);
    pgl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_LSPGL_ATTR_CIRCUIT_MODEL_NAME) =
      last_stage_mux2_name.c_str();
  }

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "in";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "out";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "sram";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "sram";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_model_ccff(pugi::xml_node& root_node) const {
  int num_err = 0;
  std::string cmdl_name = "EFPGA_CCFF";
  bool is_default = true;
  pugi::xml_node cmdl_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_NODE_NAME);
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_TYPE) = "ccff";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_NAME) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_PREFIX) = cmdl_name.c_str();
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_IS_DEFAULT) =
    is_default ? "true" : "false";
  cmdl_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_ATTR_VERILOG_NETLIST) =
    CUSTOM_VERILOG_NETLIST_FILE_.c_str();

  pugi::xml_node dsgn_node =
    cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_NODE_NAME);
  dsgn_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_DESIGN_TECH_ATTR_TYPE) = "cmos";

  { /* Input buffer */
    pugi::xml_node in_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_INPUT_BUFFER_NODE_NAME);
    in_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }
  { /* Output buffer */
    pugi::xml_node out_buf_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_OUTPUT_BUFFER_NODE_NAME);
    out_buf_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_BUFFER_ATTR_EXIST) = "false";
  }

  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "D";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) =
      CIRCUIT_MODEL_PORT_ATTR_PREFIX_.c_str();
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "CFGE";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL) = "true";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL) = "0";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_CONFIG_ENABLE) = "true";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "Q";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "CFGQN";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "output";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "CFGQ";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "clock";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "prog_clk";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "CLK";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL) = "true";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_PROG) = "true";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL) = "0";
  }
  { /* Port */
    pugi::xml_node port_node =
      cmdl_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_NODE_NAME);
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_TYPE) = "input";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_PREFIX) = "prog_reset";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_LIB_NAME) = "RESET_B";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_SIZE) = "1";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_GLOBAL) = "true";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_PROG) = "true";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_IS_RESET) = "true";
    port_node.append_attribute(XML_OPENFPGA_ARCH_CIRCUIT_MODEL_PORT_ATTR_DEFAULT_VAL) = "1";
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_circuit_lib(pugi::xml_node& root_node) const {
  int num_err = 0;
  pugi::xml_node clib_node = root_node.append_child(XML_OPENFPGA_ARCH_CIRCUIT_LIB_NODE_NAME);
  num_err += add_xml_openfpga_arch_circuit_model_inv(clib_node, "INV_WRAPPER", true);
  num_err += add_xml_openfpga_arch_circuit_model_inv(clib_node, "INV_LVT_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_inv(clib_node, "INV_HVT_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_buf(clib_node, "BUFD2_WRAPPER", false, 2);
  num_err += add_xml_openfpga_arch_circuit_model_buf(clib_node, "BUFD2_LVT_WRAPPER", false, 2);
  num_err += add_xml_openfpga_arch_circuit_model_buf(clib_node, "BUFD2_HVT_WRAPPER", false, 2);
  num_err += add_xml_openfpga_arch_circuit_model_buf(clib_node, "BUFD4_WRAPPER", false, 4);
  num_err += add_xml_openfpga_arch_circuit_model_buf(clib_node, "CKBUF_WRAPPER", false, 2);
  num_err += add_xml_openfpga_arch_circuit_model_or2(clib_node, "OR2_LVT_WRAPPER", true);
  num_err += add_xml_openfpga_arch_circuit_model_or2(clib_node, "OR2_HVT_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "PB_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "PB_LS_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "CB_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "CB_LS_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "SB_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "SB_LS_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "LUT_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "CK_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_mux2(clib_node, "CK_LS_MUX2_WRAPPER", false);
  num_err += add_xml_openfpga_arch_circuit_model_chan_wire(clib_node, "chan_segment", true);
  num_err += add_xml_openfpga_arch_circuit_model_wire(clib_node, "direct_interc", true);
  num_err += add_xml_openfpga_arch_circuit_model_mux(clib_node, "pb_mux", "PB_MUX2_WRAPPER", "", "",
                                                     "0", true);
  num_err += add_xml_openfpga_arch_circuit_model_mux(clib_node, "pb_mux_no_const_input",
                                                     "PB_MUX2_WRAPPER", "", "", "");
  num_err += add_xml_openfpga_arch_circuit_model_mux(clib_node, "pb_mux_highload",
                                                     "PB_MUX2_WRAPPER", "", "BUFD4_WRAPPER", "");
  num_err += add_xml_openfpga_arch_circuit_model_mux(clib_node, "cb_mux", "CB_MUX2_WRAPPER",
                                                     "CB_LS_MUX2_WRAPPER", "", "0");
  num_err += add_xml_openfpga_arch_circuit_model_mux(clib_node, "sb_mux", "SB_MUX2_WRAPPER",
                                                     "SB_LS_MUX2_WRAPPER", "", "0");
  num_err += add_xml_openfpga_arch_circuit_model_mux(clib_node, "clk_mux", "CK_MUX2_WRAPPER",
                                                     "CK_LS_MUX2_WRAPPER", "", "0");
  num_err += add_xml_openfpga_arch_circuit_model_ccff(clib_node);
  /* Add circuit model based on vpr arch primitives which represent physical models */
  for (auto pmtv : arch_primitives_) {
    if (!pmtv.is_physical_model()) {
      /* Bypass non-physical models */
      continue;
    }
    /* Skip duplicated models */
    auto result = outputted_circuit_models_.find(pmtv.circuit_model_name());
    if (result != outputted_circuit_models_.end()) {
      continue;
    }
    bool is_default = is_physical_primitive_default_in_circuit_model(pmtv);
    num_err +=
      pmtv.openfpga_circuit_model_xml_node(clib_node, is_default, CUSTOM_VERILOG_NETLIST_FILE_);
    /* register */
    outputted_circuit_models_[pmtv.circuit_model_name()] = true;
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

bool ArchGen::is_physical_primitive_default_in_circuit_model(const VprArchPrimitive& pmtv) const {
  /* If this is the first of the primitive in the same type of circuit model, it should be default
   */
  for (auto curr_pmtv : arch_primitives_) {
    if (curr_pmtv.circuit_model_type() == pmtv.circuit_model_type()) {
      if (curr_pmtv.name() == pmtv.name()) {
        return true;
      }
      break;
    }
  }
  return false;
}

int ArchGen::add_xml_openfpga_arch_config_protocol(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node cfgp_node = root_node.append_child(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_NODE_NAME);
  pugi::xml_node org_node = cfgp_node.append_child(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_NODE_NAME);
  org_node.append_attribute(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_ATTR_TYPE) = "scan_chain";
  org_node.append_attribute(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_ATTR_CIRCUIT_MODEL_NAME) =
    CCFF_MODEL_NAME_.c_str();

  /* FIXME: This is very dirty solution for now. In future, should redesign this!!! */
  if (num_config_regions_ / num_pclks_ == 1) {
    org_node.append_attribute(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_ATTR_NUM_REGIONS) =
      std::to_string(num_config_regions_).c_str();
  } else {
    org_node.append_attribute(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_ATTR_NUM_REGIONS) =
      "${FPGA_SIZE_X}";
  }

  for (size_t iclk = 0; iclk < num_pclks_; ++iclk) {
    std::string pclk_name = std::string("prog_clk[") + std::to_string(iclk) + std::string("]");
    std::string ccff_head_var =
      std::string("${FPGA_PROG_CLK") + std::to_string(iclk) + std::string("_CCFF_HEAD_INDICES}");
    /* FIXME: This is very dirty solution for now. In future, should redesign this!!! */
    if (num_config_regions_ / num_pclks_ == 1) {
      ccff_head_var = std::to_string(iclk);
    }

    pugi::xml_node pclk_node =
      org_node.append_child(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_PROG_CLK_NODE_NAME);
    pclk_node.append_attribute(XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_PROG_CLK_ATTR_PORT) =
      pclk_name.c_str();
    pclk_node.append_attribute(
      XML_OPENFPGA_ARCH_CONFIG_PROTOCOL_ORG_PROG_CLK_ATTR_CCFF_HEAD_INDICES) =
      ccff_head_var.c_str();
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_cb(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node cb_node = root_node.append_child(XML_OPENFPGA_ARCH_CONNECTION_BLOCK_NODE_NAME);
  pugi::xml_node sw_node = cb_node.append_child(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_NODE_NAME);
  sw_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_NAME) =
    arch_layout_.cb_mux_name().c_str();
  sw_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_CIRCUIT_MODEL_NAME) =
    "cb_mux";

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_sb(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node sb_node = root_node.append_child(XML_OPENFPGA_ARCH_SWITCH_BLOCK_NODE_NAME);
  { /* CK MUX */
    pugi::xml_node sw_node = sb_node.append_child(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_NODE_NAME);
    sw_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_NAME) =
      arch_layout_.clk_mux_name().c_str();
    sw_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_CIRCUIT_MODEL_NAME) =
      "clk_mux";
  }
  /* Walk through all the Lx switches */
  for (auto seg_type : arch_layout_.segment_types()) {
    for (auto seg_len : arch_layout_.segment_lengths(seg_type)) {
      pugi::xml_node sw_node =
        sb_node.append_child(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_NODE_NAME);
      sw_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_NAME) =
        arch_layout_.sb_mux_name(seg_type, seg_len).c_str();
      sw_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_BLOCK_SWITCH_ATTR_CIRCUIT_MODEL_NAME) =
        "sb_mux";
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_routing_segment(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node rt_node = root_node.append_child(XML_OPENFPGA_ARCH_ROUTING_SEGMENT_NODE_NAME);
  /* Walk through all the Lx segments */
  for (auto seg_type : arch_layout_.segment_types()) {
    for (auto seg_len : arch_layout_.segment_lengths(seg_type)) {
      pugi::xml_node seg_node =
        rt_node.append_child(XML_OPENFPGA_ARCH_ROUTING_SEGMENT_SEGMENT_NODE_NAME);
      seg_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_SEGMENT_SEGMENT_ATTR_NAME) =
        arch_layout_.segment_name(seg_type, seg_len).c_str();
      seg_node.append_attribute(XML_OPENFPGA_ARCH_ROUTING_SEGMENT_SEGMENT_ATTR_CIRCUIT_MODEL_NAME) =
        "chan_segment";
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_direct_connection(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node dc_node = root_node.append_child(XML_OPENFPGA_ARCH_DIRECT_CONNECTION_NODE_NAME);
  for (auto idc : arch_layout_.direct_connects()) {
    pugi::xml_node d_node =
      dc_node.append_child(XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_NODE_NAME);
    d_node.append_attribute(XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_ATTR_NAME) =
      arch_layout_.direct_connect_name(idc).c_str();
    if (arch_layout_.direct_connect_is_feedback_connection(idc)) {
      d_node.append_attribute(XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_ATTR_CIRCUIT_MODEL_NAME) =
        "cb_mux";
      d_node.append_attribute(XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_ATTR_TYPE) = "part_of_cb";
    } else {
      d_node.append_attribute(XML_OPENFPGA_ARCH_DIRECT_CONNECTION_DIRECT_ATTR_CIRCUIT_MODEL_NAME) =
        "direct_interc";
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_tile_annotation(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node ta_node = root_node.append_child(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_NODE_NAME);
  for (auto tile_name : arch_layout_.tiles()) { /* Add physical equivalent site syntax */
    for (auto stile_name : arch_layout_.subtiles(tile_name)) {
      std::string site_name = arch_layout_.subtile_physical_equivalent_site(tile_name, stile_name);
      if (site_name.empty()) {
        VTR_LOG_ERROR("Fail to find a physical equivalent site for subtile '%s' under tile '%s'\n",
                      stile_name.c_str(), tile_name.c_str());
        num_err++;
        continue;
      }
      pugi::xml_node pes_node =
        ta_node.append_child(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_NODE_NAME);
      pes_node.append_attribute(
        XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_ATTR_TILE) = tile_name.c_str();
      pes_node.append_attribute(
        XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_ATTR_SUBTILE) =
        site_name.c_str();
      pes_node.append_attribute(
        XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_PHYSICAL_EQUIVALENT_SITE_ATTR_SITE) = site_name.c_str();
    }
  }
  {
    pugi::xml_node gp_node =
      ta_node.append_child(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_NODE_NAME);
    gp_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_NAME) =
      GCLK_NAME_.c_str();
    gp_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_IS_CLOCK) = "true";
    gp_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_DEFAULT_VAL) = "0";
    gp_node.append_attribute(
      XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_CLOCK_ARCH_TREE_NAME) =
      GCLK_NTWK_NAME_.c_str();
    /* List all the tile ports */
    for (auto tile : arch_layout_.tiles()) {
      /* TODO: Bypass non-I/O tile as openfpga has a bug here */
      if (!arch_layout_.is_io_tile(tile)) {
        continue;
      }
      /* Walk through all the required ports */
      for (auto port : arch_layout_.find_tile_ports_by_type(
             tile, arcxpress::VprArchLayout::e_port_type::CLOCK)) {
        pugi::xml_node tile_node =
          gp_node.append_child(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_NODE_NAME);
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_NAME) =
          tile.c_str();
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_PORT) =
          port.get_name().c_str();
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_X) = "-1";
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_Y) = "-1";
      }
    }
  }
  {
    pugi::xml_node gp_node =
      ta_node.append_child(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_NODE_NAME);
    gp_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_NAME) =
      GRST_NAME_.c_str();
    gp_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_IS_RESET) = "true";
    gp_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_DEFAULT_VAL) = "1";
    gp_node.append_attribute(
      XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_GLOBAL_PORT_ATTR_CLOCK_ARCH_TREE_NAME) =
      GRST_NTWK_NAME_.c_str();
    /* List all the tile ports */
    for (auto tile : arch_layout_.tiles()) {
      /* TODO: Bypass non I/O tile as openfpga has a bug here */
      if (!arch_layout_.is_io_tile(tile)) {
        continue;
      }
      /* Walk through all the required ports */
      for (auto port : arch_layout_.find_tile_ports_by_type(
             tile, arcxpress::VprArchLayout::e_port_type::RESET)) {
        pugi::xml_node tile_node =
          gp_node.append_child(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_NODE_NAME);
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_NAME) =
          tile.c_str();
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_PORT) =
          port.get_name().c_str();
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_X) = "-1";
        tile_node.append_attribute(XML_OPENFPGA_ARCH_TILE_ANNOTATIONS_TILE_ATTR_Y) = "-1";
      }
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_pb_type_annotation_interconnect(
  pugi::xml_node& root_node, const std::string& phy_mode, const std::string& pb_type_path) const {
  int num_err = 0;
  size_t mode_id = find_pb_mode_id(phy_mode);
  if (!valid_pb_mode_id(mode_id)) {
    return 1;
  }
  std::map<EdgePin, std::vector<size_t>> interc;
  num_err += compile_pb_type_mode_interconnects(mode_id, interc);
  if (num_err) {
    return 1;
  }

  /* Output interconnect nodes in XML */
  for (auto pair : interc) {
    /* Give a custom name */
    std::string name_str = pb_type_interconnect_name(pair.first);
    /* Check the circuit model names, all the edges should have the same one */
    std::string curr_cmdl;
    for (size_t src_edge_idx : pair.second) {
      /* Skip zero delay edges */
      if (pb_mode_edges_[mode_id][src_edge_idx].circuit_model_name.empty()) {
        continue;
      }
      if (!curr_cmdl.empty() &&
          curr_cmdl != pb_mode_edges_[mode_id][src_edge_idx].circuit_model_name) {
        VTR_LOG_ERROR("Circuit model is different on the incoming edges for interconnect '%s'!\n",
                      name_str.c_str());
        return 1;
      }
      curr_cmdl = pb_mode_edges_[mode_id][src_edge_idx].circuit_model_name;
    }
    if (!curr_cmdl.empty()) {
      pugi::xml_node curr_pb_type_node =
        root_node.append_child(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_NODE_NAME);
      curr_pb_type_node.append_attribute(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_NAME) =
        pb_type_path.c_str();
      pugi::xml_node curr_interc_node = curr_pb_type_node.append_child(
        XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_INTERCONNECT_NODE_NAME);
      curr_interc_node.append_attribute(
        XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_INTERCONNECT_ATTR_NAME) = name_str.c_str();
      curr_interc_node.append_attribute(
        XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_INTERCONNECT_ATTR_CIRCUIT_MODEL_NAME) =
        curr_cmdl.c_str();
    }
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::rec_add_xml_openfpga_arch_pb_type_annotation_physical_mode(
  pugi::xml_node& root_node, const std::string& pb_type_name,
  const std::string& pb_type_path) const {
  int num_err = 0;
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    VTR_LOG_ERROR("Invalid pb_type '%s' which is not yet defined!\n", pb_type_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Primitive pb_type, find mode bits and */
  if (pb_type_modes_[pb_type_id].empty()) {
    pugi::xml_node pb_node =
      root_node.append_child(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_NODE_NAME);
    pb_node.append_attribute(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_NAME) =
      pb_type_path.c_str();
    if (pb_type_circuit_model_names_[pb_type_id].empty()) {
      VTR_LOG_ERROR("The primitive pb_type '%s' is not binded to any circuit model!\n",
                    pb_type_path.c_str());
      num_err++;
    }
    pb_node.append_attribute(
      XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_CIRCUIT_MODEL_NAME) =
      pb_type_circuit_model_names_[pb_type_id].c_str();
    if (!pb_type_mode_bits_[pb_type_id].empty()) {
      pb_node.append_attribute(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_MODE_BITS) =
        pb_type_mode_bits_[pb_type_id].c_str();
    }
    return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
  }
  /* Walk through modes and skip operating modes */
  size_t phy_mode_id = 0;
  for (size_t imode = 0; imode < pb_type_modes_[pb_type_id].size(); ++imode) {
    if (pb_type_mode_is_physical_[pb_type_id][imode]) {
      phy_mode_id = imode;
      break;
    }
  }
  /* Output pb_type physical mode definition */
  if (pb_type_modes_[pb_type_id].size() == 1) {
    phy_mode_id = 0;
  } else {
    pugi::xml_node pb_node =
      root_node.append_child(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_NODE_NAME);
    pb_node.append_attribute(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_NAME) =
      pb_type_path.c_str();
    pb_node.append_attribute(
      XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_MODE_NAME) =
      pb_type_modes_[pb_type_id][phy_mode_id].c_str();
  }
  std::string phy_mode = pb_type_modes_[pb_type_id][phy_mode_id];
  size_t pb_mode_id = find_pb_mode_id(phy_mode);
  if (!valid_pb_mode_id(pb_mode_id)) {
    VTR_LOG_ERROR("The physical mode '%s' is not yet defined!\n", phy_mode.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Output interconnect circuit model names */
  num_err +=
    add_xml_openfpga_arch_pb_type_annotation_interconnect(root_node, phy_mode, pb_type_path);
  /* Go recursively */
  for (auto pair : pb_mode_childs_[pb_mode_id]) {
    std::string curr_pb_type_path =
      pb_type_path + std::string("[") + phy_mode + std::string("].") + pair.first;
    num_err += rec_add_xml_openfpga_arch_pb_type_annotation_physical_mode(root_node, pair.first,
                                                                          curr_pb_type_path);
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::rec_add_xml_openfpga_arch_pb_type_annotation_operating_mode(
  pugi::xml_node& root_node, const std::string& pb_type_name,
  const std::string& pb_type_path) const {
  int num_err = 0;
  size_t pb_type_id = find_pb_type_id(pb_type_name);
  if (!valid_pb_type_id(pb_type_id)) {
    VTR_LOG_ERROR("Invalid pb_type '%s' which is not yet defined!\n", pb_type_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Primitive pb_type, find mode bits and */
  if (pb_type_modes_[pb_type_id].empty() && pb_type_circuit_model_names_[pb_type_id].empty()) {
    pugi::xml_node pb_node =
      root_node.append_child(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_NODE_NAME);
    pb_node.append_attribute(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_NAME) =
      pb_type_path.c_str();
    if (!pb_type_mode_bits_[pb_type_id].empty()) {
      pb_node.append_attribute(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_MODE_BITS) =
        pb_type_mode_bits_[pb_type_id].c_str();
    }
    /* TODO: Add physical pb type binding */
    if (pb_type_physical_pb_types_[pb_type_id].empty()) {
      VTR_LOG_ERROR(
        "Invalid operating pb_type '%s' which is not binded to any physical pb_type !\n",
        pb_type_name.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::string hie_str = pb_type_hierarchy_to_string(pb_type_physical_pb_types_[pb_type_id]);
    if (hie_str.empty()) {
      VTR_LOG_ERROR("Invalid operating pb_type '%s' whose hierarchical path is not correct!\n",
                    pb_type_name.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    pb_node.append_attribute(
      XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_PB_TYPE_NAME) = hie_str.c_str();
    if (!pb_type_physical_index_factors_[pb_type_id].empty()) {
      pb_node.append_attribute(
        XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_PB_TYPE_INDEX_FACTOR) =
        pb_type_physical_index_factors_[pb_type_id].c_str();
    }
    if (!pb_type_physical_index_offsets_[pb_type_id].empty()) {
      pb_node.append_attribute(
        XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PB_TYPE_ATTR_PHYSICAL_PB_TYPE_INDEX_OFFSET) =
        pb_type_physical_index_offsets_[pb_type_id].c_str();
    }
    for (size_t iport = 0; iport < pb_type_physical_ports_[pb_type_id].size(); ++iport) {
      if (!pb_type_physical_ports_[pb_type_id][iport].is_valid()) {
        continue;
      }
      pugi::xml_node pb_port_node =
        pb_node.append_child(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_NODE_NAME);
      pb_port_node.append_attribute(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_ATTR_NAME) =
        pb_type_ports_[pb_type_id][iport].get_name().c_str();
      pb_port_node.append_attribute(
        XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_ATTR_PHYSICAL_MODE_PORT) =
        pb_type_physical_ports_[pb_type_id][iport].to_verilog_string().c_str();
      if (!pb_type_physical_pin_rotate_offsets_[pb_type_id][iport].empty()) {
        pb_port_node.append_attribute(
          XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_PORT_ATTR_PHYSICAL_MODE_PIN_ROTATE_OFFSET) =
          pb_type_physical_pin_rotate_offsets_[pb_type_id][iport].c_str();
      }
    }
    return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
  }
  /* Walk through modes and skip physical modes */
  size_t phy_mode_id = 0;
  for (size_t imode = 0; imode < pb_type_modes_[pb_type_id].size(); ++imode) {
    /* If there is only 1 mode here, we will dive to the next level */
    if (pb_type_modes_[pb_type_id].size() > 1 && pb_type_mode_is_physical_[pb_type_id][imode]) {
      continue;
    }
    std::string op_mode = pb_type_modes_[pb_type_id][imode];
    size_t pb_mode_id = find_pb_mode_id(op_mode);
    if (!valid_pb_mode_id(pb_mode_id)) {
      VTR_LOG_ERROR("The operating mode '%s' is not yet defined!\n", op_mode.c_str());
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }

    /* Go recursively */
    for (auto pair : pb_mode_childs_[pb_mode_id]) {
      std::string curr_pb_type_path =
        pb_type_path + std::string("[") + op_mode + std::string("].") + pair.first;
      num_err += rec_add_xml_openfpga_arch_pb_type_annotation_operating_mode(root_node, pair.first,
                                                                             curr_pb_type_path);
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_pb_type_annotation_physical_mode(
  pugi::xml_node& root_node) const {
  int num_err = 0;

  /* Starting from top-level pb_type and focus on physical modes
   * If there is more than 1 mode, a physical mode should be defined
   * If there is only 1 mode, the mode is the physical mode
   * If there is 0 mode, this is a primitive pb_type
   */
  for (auto subtile : arch_layout_.subtiles()) {
    /* Walk thru in a recursive way */
    num_err +=
      rec_add_xml_openfpga_arch_pb_type_annotation_physical_mode(root_node, subtile, subtile);
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_pb_type_annotation_operating_mode(
  pugi::xml_node& root_node) const {
  int num_err = 0;

  /* Starting from top-level pb_type and focus on physical modes
   * If there is more than 1 mode, a physical mode should be defined
   * If there is only 1 mode, the mode is the physical mode
   * If there is 0 mode, this is a primitive pb_type
   */
  for (auto subtile : arch_layout_.subtiles()) {
    /* Walk thru in a recursive way */
    num_err +=
      rec_add_xml_openfpga_arch_pb_type_annotation_operating_mode(root_node, subtile, subtile);
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_pb_type_annotation(pugi::xml_node& root_node) const {
  int num_err = 0;

  pugi::xml_node pta_node = root_node.append_child(XML_OPENFPGA_ARCH_PB_TYPE_ANNOTATIONS_NODE_NAME);
  /* Walk through pb type tree (physical mode tree) and do the following:
   * - add physical mode definition
   * - find primitive pb_type and add circuit model and mode bit definition
   * - Walk through pb type tree (physical mode tree) and add interconnect definition
   */
  num_err += add_xml_openfpga_arch_pb_type_annotation_physical_mode(pta_node);
  /* Walk through pb type tree (operating mode tree) and add physical pb_type binding
   * (including port binding) and mode bits */
  num_err += add_xml_openfpga_arch_pb_type_annotation_operating_mode(pta_node);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_arch_node(pugi::xml_node& root_node) const {
  int num_err = 0;

  num_err += add_xml_openfpga_arch_techlib(root_node);
  num_err += add_xml_openfpga_arch_circuit_lib(root_node);
  num_err += add_xml_openfpga_arch_config_protocol(root_node);
  num_err += add_xml_openfpga_arch_cb(root_node);
  num_err += add_xml_openfpga_arch_sb(root_node);
  num_err += add_xml_openfpga_arch_routing_segment(root_node);
  num_err += add_xml_openfpga_arch_direct_connection(root_node);
  num_err += add_xml_openfpga_arch_tile_annotation(root_node);
  num_err += add_xml_openfpga_arch_pb_type_annotation(root_node);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

bool ArchGen::is_arch_primitive_exist(const std::string& name) const {
  for (auto pmtv : arch_primitives_) {
    if (pmtv.name() == name) {
      return true;
    }
  }
  return false;
}

int ArchGen::set_arch_primitive_mode_bits(const std::string& name, const std::string& tag,
                                          const std::string& mode_bits) {
  int num_err = 0;
  bool found_match = false;
  for (auto& pmtv : arch_primitives_) {
    if (pmtv.name() != name) {
      continue;
    }
    found_match = true;
    bool status = pmtv.set_mode_bits(tag, mode_bits);
    if (!status) {
      VTR_LOG_ERROR("Failed when adding mode_bits '%s' under tag '%s' for the primitive '%s'!\n",
                    mode_bits.c_str(), tag.c_str(), name.c_str());
      num_err++;
    }
    break;
  }
  if (!found_match) {
    VTR_LOG_ERROR("The primitive '%s' is not yet defined!\n", name.c_str());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

std::string ArchGen::pb_type_hierarchy_to_string(const std::string& pb_type_name) const {
  /* Backtrace to the top-level pb type */
  std::string curr_pb_type_name = pb_type_name;
  std::string ret = pb_type_name;
  while (1) {
    size_t curr_pb_type_id = find_pb_type_id(curr_pb_type_name);
    if (!valid_pb_type_id(curr_pb_type_id)) {
      VTR_LOG_ERROR("Pb type '%s' is not defined!\n", curr_pb_type_name.c_str());
      return std::string(); /* Invalid output which should be captured by who call this function! */
    }
    std::string curr_parent_mode = pb_type_parent_modes_[curr_pb_type_id];
    /* When reaching the top, there is no more parent mode */
    if (curr_parent_mode.empty()) {
      break;
    }
    size_t curr_pb_mode_id = find_pb_mode_id(curr_parent_mode);
    if (!valid_pb_mode_id(curr_pb_mode_id)) {
      VTR_LOG_ERROR("Parent mode '%s' of Pb type '%s' is not defined!\n", curr_parent_mode.c_str(),
                    curr_pb_type_name.c_str());
      return std::string(); /* Invalid output which should be captured by who call this function! */
    }
    std::string curr_parent_pb_type = pb_mode_parents_[curr_pb_mode_id];
    size_t curr_parent_pb_type_id = find_pb_type_id(curr_parent_pb_type);
    if (!valid_pb_type_id(curr_parent_pb_type_id)) {
      VTR_LOG_ERROR("Parent pb type '%s' is not defined!\n", curr_parent_pb_type.c_str());
      return std::string(); /* Invalid output which should be captured by who call this function! */
    }
    /* Update the hierarch string */
    ret = curr_parent_pb_type + std::string("[") + curr_parent_mode + std::string("].") + ret;
    /* Go to one level up */
    curr_pb_type_name = curr_parent_pb_type;
  }
  return ret;
}

std::string ArchGen::gen_gntwk_main_spine_name(const std::string& prefix) const {
  return prefix + std::string("_l0");
}

std::string ArchGen::gen_gntwk_branch_spine_name(const std::string& prefix,
                                                 const VprArchLayout::e_dir_type& dir,
                                                 const vtr::Point<size_t>& tap_point) const {
  std::string ret = prefix + std::string("_l1");
  if (dir == VprArchLayout::e_dir_type::INC) {
    ret += std::string("_N_");
  } else if (dir == VprArchLayout::e_dir_type::DEC) {
    ret += std::string("_S_");
  }
  ret += std::string("x") + std::to_string(tap_point.x());
  ret += std::string("y") + std::to_string(tap_point.y());
  return ret;
}

int ArchGen::build_global_network_tree_taps(
  const std::string& ntwk_name, const openfpga::BasicPort& g_port,
  const std::vector<VprArchLayout::e_port_type>& port_types, openfpga::ClockNetwork& gntwk) const {
  int num_err = 0;

  openfpga::ClockTreeId gtree_id = gntwk.find_tree(ntwk_name);
  if (!gntwk.valid_tree_id(gtree_id)) {
    VTR_LOG_ERROR("Fail to find the existing global signal network '%s' to add taps!\n",
                  ntwk_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Tap all the I/O, CLB clocks */
  for (size_t ipin : g_port.pins()) {
    /* The destination tile could be I/O or CLB. For only CLK port */
    openfpga::BasicPort from_pin(g_port.get_name(), ipin, ipin);
    for (auto port_type : port_types) {
      for (auto tile : arch_layout_.tiles_contain_ports(port_type)) {
        for (size_t id : arch_layout_.subtile_indices_contain_ports(tile, port_type)) {
          for (openfpga::BasicPort to_pin : arch_layout_.subtile_pins(tile, id, port_type)) {
            if (port_type == VprArchLayout::e_port_type::INPUT &&
                !arch_layout_.subtile_pin_driven_by_greset(tile, id, to_pin)) {
              continue;
            }
            openfpga::BasicPort to_tile(tile, id, id);
            std::string to_pin_str =
              to_tile.to_verilog_string() + std::string(".") + to_pin.to_verilog_string();
            gntwk.add_tree_tap(gtree_id, from_pin, to_pin_str);
          }
        }
      }
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::build_global_network_tree_spines(const vtr::Point<size_t>& fabric_size,
                                              const vtr::Point<size_t>& entry_point,
                                              const std::string& ntwk_name,
                                              const openfpga::BasicPort& g_port,
                                              const openfpga::BasicPort& ckbuf_starting_subtile,
                                              const openfpga::BasicPort& ckbuf_out_port,
                                              openfpga::ClockNetwork& gntwk) const {
  int num_err = 0;

  openfpga::ClockTreeId gtree_id = gntwk.create_tree(ntwk_name, g_port);
  if (!gntwk.valid_tree_id(gtree_id)) {
    VTR_LOG_ERROR("Fail to create the global signal network '%s'!\n", ntwk_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Create main spine */
  std::string m_spn_name = gen_gntwk_main_spine_name(g_port.get_name());
  openfpga::ClockSpineId m_spn_id = gntwk.create_spine(m_spn_name);
  if (!gntwk.valid_spine_id(m_spn_id)) {
    VTR_LOG_ERROR("Fail to create the main spine for global signal network '%s'!\n",
                  ntwk_name.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  gntwk.set_spine_start_point(m_spn_id, vtr::Point<int>(entry_point.x(), entry_point.y()));
  gntwk.set_spine_end_point(
    m_spn_id, vtr::Point<int>(entry_point.x() + fabric_size.x() - 2, entry_point.y()));
  gntwk.set_spine_parent_tree(m_spn_id, gtree_id);
  /* Add switch point */
  for (size_t itap = 0; itap < fabric_size.x() - 1; ++itap) {
    /* Add north and south spines */
    for (auto dir : {VprArchLayout::e_dir_type::INC, VprArchLayout::e_dir_type::DEC}) {
      std::string b_spn_name = gen_gntwk_branch_spine_name(
        g_port.get_name(), dir, vtr::Point<size_t>(itap, entry_point.y()));
      openfpga::ClockSpineId b_spn_id = gntwk.create_spine(b_spn_name);
      gntwk.set_spine_parent_tree(b_spn_id, gtree_id);
      if (!gntwk.valid_spine_id(b_spn_id)) {
        VTR_LOG_ERROR("Fail to create the branch spine for global signal network '%s'!\n",
                      ntwk_name.c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      if (dir == VprArchLayout::e_dir_type::INC) {
        gntwk.set_spine_start_point(b_spn_id, vtr::Point<int>(itap, entry_point.y() + 1));
        gntwk.set_spine_end_point(b_spn_id, vtr::Point<int>(itap, fabric_size.y() - 1));
      } else if (dir == VprArchLayout::e_dir_type::DEC) {
        gntwk.set_spine_start_point(b_spn_id, vtr::Point<int>(itap, entry_point.y()));
        gntwk.set_spine_end_point(b_spn_id, vtr::Point<int>(itap, 0));
      }
      openfpga::ClockSwitchPointId sw_id =
        gntwk.add_spine_switch_point(m_spn_id, b_spn_id, vtr::Point<int>(itap, entry_point.y()));
      /* Add internal driver */
      if (itap == 0) {
        for (size_t ipin = 0; ipin < g_port.pins().size(); ++ipin) {
          size_t curr_pin = g_port.pins()[ipin];
          size_t ckbuf_pin_offset = ipin % (g_port.pins().size() / 2);
          /* Get ckbuf subtile index in the arch */
          openfpga::BasicPort from_tile = ckbuf_starting_subtile;
          from_tile.set_lsb(from_tile.get_lsb() + ckbuf_pin_offset);
          from_tile.set_msb(from_tile.get_msb() + ckbuf_pin_offset);
          /* Get ckbuf output port */
          openfpga::BasicPort from_pin = ckbuf_out_port;
          std::string from_pin_str =
            from_tile.to_verilog_string() + std::string(".") + from_pin.to_verilog_string();
          openfpga::BasicPort to_pin(g_port.get_name(), curr_pin, curr_pin);
          std::string to_pin_str = to_pin.to_verilog_string();
          gntwk.add_spine_switch_point_internal_driver(m_spn_id, sw_id, from_pin_str, to_pin_str);
        }
      }
    }
    /* Add intermediate driver */
    if (itap == 0) {
      for (size_t ipin = 0; ipin < g_port.pins().size(); ++ipin) {
        size_t curr_pin = g_port.pins()[ipin];
        size_t ckbuf_pin_offset = ipin % (g_port.pins().size() / 2);
        /* Get ckbuf subtile index in the arch */
        openfpga::BasicPort from_tile = ckbuf_starting_subtile;
        from_tile.set_lsb(from_tile.get_lsb() + ckbuf_pin_offset);
        from_tile.set_msb(from_tile.get_msb() + ckbuf_pin_offset);
        /* Get ckbuf output port */
        openfpga::BasicPort from_pin = ckbuf_out_port;
        std::string from_pin_str =
          from_tile.to_verilog_string() + std::string(".") + from_pin.to_verilog_string();
        openfpga::BasicPort to_pin(g_port.get_name(), curr_pin, curr_pin);
        std::string to_pin_str = to_pin.to_verilog_string();
        gntwk.add_spine_intermediate_driver(m_spn_id, vtr::Point<int>(itap, entry_point.y()),
                                            from_pin_str, to_pin_str);
      }
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::build_global_network_clock_tree(const vtr::Point<size_t>& fabric_size,
                                             const vtr::Point<size_t>& entry_point,
                                             const std::string& ntwk_name,
                                             const openfpga::BasicPort& g_port,
                                             const openfpga::BasicPort& ckbuf_starting_subtile,
                                             const openfpga::BasicPort& ckbuf_out_port,
                                             openfpga::ClockNetwork& gntwk) const {
  int num_err = 0;

  num_err += build_global_network_tree_spines(fabric_size, entry_point, ntwk_name, g_port,
                                              ckbuf_starting_subtile, ckbuf_out_port, gntwk);
  std::vector<VprArchLayout::e_port_type> tap_port_types{VprArchLayout::e_port_type::CLOCK};
  num_err += build_global_network_tree_taps(ntwk_name, g_port, tap_port_types, gntwk);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::build_global_network_reset_tree(const vtr::Point<size_t>& fabric_size,
                                             const vtr::Point<size_t>& entry_point,
                                             const std::string& ntwk_name,
                                             const openfpga::BasicPort& g_port,
                                             const openfpga::BasicPort& ckbuf_starting_subtile,
                                             const openfpga::BasicPort& ckbuf_out_port,
                                             openfpga::ClockNetwork& gntwk) const {
  int num_err = 0;

  num_err += build_global_network_tree_spines(fabric_size, entry_point, ntwk_name, g_port,
                                              ckbuf_starting_subtile, ckbuf_out_port, gntwk);
  std::vector<VprArchLayout::e_port_type> tap_port_types{VprArchLayout::e_port_type::RESET,
                                                         VprArchLayout::e_port_type::INPUT};
  num_err += build_global_network_tree_taps(ntwk_name, g_port, tap_port_types, gntwk);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::build_global_network(const VprArchLayout::e_fabric_type& fb_type,
                                  openfpga::ClockNetwork& gntwk) const {
  int num_err = 0;

  if (!arch_layout_.valid_fabric_type(fb_type)) {
    VTR_LOG_ERROR("Invalid fabric type. Expect [dp | ultimate]!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Get the dimension of the array */
  size_t height = arch_layout_.fabric_height(fb_type);
  size_t width = arch_layout_.fabric_width(fb_type);
  VTR_LOG("Height: %lu, Width: %lu when building clock network\n", height, width);
  /* Find the clock/reset sources point. Currently, only support the entry point on the left side at
   * the middle point */
  vtr::Point<size_t> clk_entry_point(0, height / 2 - 1);
  vtr::Point<size_t> rst_entry_point(0, height / 2 - 1);
  /* Set all the basic information */
  gntwk.set_default_segment_name(arch_layout_.clk_segment_name());
  gntwk.set_default_tap_switch_name(arch_layout_.cb_mux_name());
  gntwk.set_default_driver_switch_name(arch_layout_.clk_mux_name());
  /* Create the clock network */
  openfpga::BasicPort gck_port(GCLK_NAME_, num_clocks());
  openfpga::BasicPort ckbuf_out_port = arch_layout_.ckbuf_output_port();
  size_t ckbuf_starting_idx = arch_layout_.ckbuf_subtile_starting_index();
  openfpga::BasicPort gck_ckbuf_starting_subtile = arch_layout_.ckbuf_subtile(ckbuf_starting_idx);
  num_err += build_global_network_clock_tree(vtr::Point<size_t>(width, height), clk_entry_point,
                                             GCLK_NTWK_NAME_, gck_port, gck_ckbuf_starting_subtile,
                                             ckbuf_out_port, gntwk);
  /* Create the reset network */
  openfpga::BasicPort grst_port(GRST_NAME_, num_resets());
  openfpga::BasicPort grst_ckbuf_starting_subtile =
    arch_layout_.ckbuf_subtile(ckbuf_starting_idx + num_clocks() / 2);
  num_err += build_global_network_reset_tree(vtr::Point<size_t>(width, height), rst_entry_point,
                                             GRST_NTWK_NAME_, grst_port,
                                             grst_ckbuf_starting_subtile, ckbuf_out_port, gntwk);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::build_ckbuf_location_map(const VprArchLayout::e_fabric_type& fb_type,
                                      GlobalPortLoc& ckbuf_loc_map) const {
  int num_err = 0;

  if (!arch_layout_.valid_fabric_type(fb_type)) {
    VTR_LOG_ERROR("Invalid fabric type. Expect [dp | ultimate]!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Get the dimension of the array */
  size_t height = arch_layout_.fabric_height(fb_type);
  size_t width = arch_layout_.fabric_width(fb_type);
  VTR_LOG("Height: %lu, Width: %lu when building ckbuf location map\n", height, width);
  /* Find the clock/reset sources point. Currently, only support the entry point on the left side at
   * the middle point */
  vtr::Point<int> clk_entry_point(1, height / 2 - 1);
  vtr::Point<int> rst_entry_point(1, height / 2 - 1);

  openfpga::BasicPort gck_port(GCLK_NAME_, num_clocks());
  size_t ckbuf_starting_idx = arch_layout_.ckbuf_subtile_starting_index();
  for (auto pin : gck_port.pins()) {
    openfpga::BasicPort curr_ck_port(gck_port.get_name(), pin, pin);
    vtr::Point<int> curr_entry(clk_entry_point.x(), clk_entry_point.y());
    if (pin < gck_port.pins().size() / 2) {
      curr_entry.set_y(curr_entry.y() + 1);
    }
    size_t ckbuf_pin_offset = pin % (gck_port.pins().size() / 2);
    ckbuf_loc_map.create_port_info_instance(curr_ck_port, curr_entry,
                                            ckbuf_starting_idx + ckbuf_pin_offset, "clock");
  }

  openfpga::BasicPort grst_port(GRST_NAME_, num_resets());
  size_t grst_ckbuf_starting_idx = ckbuf_starting_idx + num_clocks() / 2;
  for (auto pin : grst_port.pins()) {
    openfpga::BasicPort curr_rst_port(grst_port.get_name(), pin, pin);
    size_t ckbuf_pin_offset = pin % (grst_port.pins().size() / 2);
    vtr::Point<int> curr_entry(rst_entry_point.x(), rst_entry_point.y());
    if (pin < grst_port.pins().size() / 2) {
      curr_entry.set_y(curr_entry.y() + 1);
    }
    ckbuf_loc_map.create_port_info_instance(curr_rst_port, curr_entry,
                                            grst_ckbuf_starting_idx + ckbuf_pin_offset, "reset");
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchGen::add_xml_openfpga_bitstream_setting_node(pugi::xml_node& root_node) const {
  int num_err = 0;
  /* Walk through all the arch primitives which require a mode-select bitstream setting */
  for (auto pmtv : arch_primitives_) {
    if (!pmtv.require_mode_select_parameters()) {
      continue;
    }
    /* Get the pb_type name for each primitive and output info in XML nodes */
    for (std::string pb_type_name : pmtv.pb_type_names()) {
      size_t pb_type_id = find_pb_type_id(pb_type_name);
      if (!valid_pb_type_id(pb_type_id)) {
        VTR_LOG_ERROR("Invalid pb_type '%s' which is not yet defined!\n", pb_type_name.c_str());
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
      /* Find all the parameters and organize in the XML node */
      for (std::string ms_param : pmtv.mode_select_parameters()) {
        std::string ms_param_key = pmtv.mode_select_parameter_key(pb_type_name);
        pugi::xml_node bs_node = root_node.append_child(XML_PB_TYPE_NODE_NAME);
        bs_node.append_attribute(XML_PB_TYPE_ATTRIBUTE_NAME) =
          pb_type_hierarchy_to_string(pb_type_name).c_str();
        bs_node.append_attribute(XML_PB_TYPE_ATTRIBUTE_SOURCE) = "eblif";
        bs_node.append_attribute(XML_PB_TYPE_ATTRIBUTE_CONTENT) = ms_param.c_str();
        bs_node.append_attribute(XML_PB_TYPE_ATTRIBUTE_IS_MODE_SELECT_BITSTREAM) = "true";
        bs_node.append_attribute(XML_PB_TYPE_ATTRIBUTE_BITSTREAM_OFFSET) =
          std::to_string(pmtv.mode_select_offset(ms_param, ms_param_key)).c_str();
      }
    }
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

std::string ArchGen::gclock_network_name() const { return GCLK_NTWK_NAME_; }
std::string ArchGen::greset_network_name() const { return GRST_NTWK_NAME_; }

}  // End namespace arcxpress
