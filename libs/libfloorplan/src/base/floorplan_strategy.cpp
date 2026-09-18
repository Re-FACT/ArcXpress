#include "floorplan_strategy.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "command_exit_codes.h"
#include "openfpga_port_parser.h"
#include "openfpga_tokenizer.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

FloorplanStrategy::FloorplanStrategy(const float& cpp, const float& sc_h) {
  clear_all();
  cpp_ = cpp;
  sc_height_ = sc_h;
  KW_CPP_ = "CPP";
  KW_SC_HEIGHT_ = "SC_HEIGHT";
  KW_MULT_ = '*';

  DISTRIBUTION_NETWORK_TYPE_STRING_ = {"htree", "fishbone"};
}

std::array<e_side, NUM_2D_SIDES> FloorplanStrategy::sides() const { return TOTAL_2D_SIDES; }

std::vector<std::string> FloorplanStrategy::tiles() const {
  std::vector<std::string> ret;
  for (auto pair : tile_util_) {
    ret.push_back(pair.first);
  }
  return ret;
}

float FloorplanStrategy::cpp() const {
  check_valid();
  return cpp_;
}

float FloorplanStrategy::standard_cell_height() const {
  check_valid();
  return sc_height_;
}

float FloorplanStrategy::boundary_cell_width() const {
  check_valid();
  return parse_gap(boundary_cell_width_);
}

float FloorplanStrategy::height_width_ratio() const {
  check_valid();
  return height_width_ratio_;
}

float FloorplanStrategy::tile_default_utilization() const {
  check_valid();
  return tile_default_util_;
}

float FloorplanStrategy::tile_utilization(const std::string& tile) const {
  check_valid();
  auto result = tile_util_.find(tile);
  if (result == tile_util_.end()) {
    return tile_default_util_;
  }
  return result->second;
}

float FloorplanStrategy::tile_horizental_gap() const {
  check_valid();
  return parse_gap(tile_gap_[0]);
}

float FloorplanStrategy::tile_vertical_gap() const {
  check_valid();
  return parse_gap(tile_gap_[1]);
}

float FloorplanStrategy::side_gap(const e_side& side) const {
  check_valid();
  check_valid_side(side);
  return parse_gap(side_gap_[size_t(side)]);
}

void FloorplanStrategy::set_cpp(const float& val) { cpp_ = val; }
void FloorplanStrategy::set_standard_cell_height(const float& val) { sc_height_ = val; }
void FloorplanStrategy::set_boundary_cell_width(const std::string& val) {
  boundary_cell_width_ = val;
}
void FloorplanStrategy::set_height_width_ratio(const float& val) { height_width_ratio_ = val; }
void FloorplanStrategy::set_tile_default_utilization(const float& val) { tile_default_util_ = val; }
void FloorplanStrategy::set_tile_utilization(const std::string& tile, const float& val) {
  tile_util_[tile] = val;
}
void FloorplanStrategy::set_tile_horizental_gap(const std::string& val) { tile_gap_[0] = val; }
void FloorplanStrategy::set_tile_vertical_gap(const std::string& val) { tile_gap_[1] = val; }
void FloorplanStrategy::set_side_gap(const e_side& side, const std::string& val) {
  check_valid_side(side);
  side_gap_[size_t(side)] = val;
}

bool FloorplanStrategy::validate() const {
  is_valid_ = false;
  size_t num_err = 0;
  /* CPP and height should be non zero */
  if (cpp_ == 0.) {
    VTR_LOG_ERROR("Contact poly pitch cannot be zero!\n");
    num_err++;
  }
  if (sc_height_ == 0.) {
    VTR_LOG_ERROR("Standard cell height cannot be zero!\n");
    num_err++;
  }
  if (!valid_horizental_gap_string(boundary_cell_width_)) {
    VTR_LOG_ERROR("Invalid boundary cell width for tiles '%s'! Expect %s*<integer>!\n",
                  boundary_cell_width_.c_str(), KW_CPP_.c_str());
    num_err++;
  }
  if (height_width_ratio_ > 1. || height_width_ratio_ == 0.) {
    VTR_LOG_ERROR("The height/width ratio of tile '%f' is invalid. Expect (0, 1]!\n",
                  height_width_ratio_);
    num_err++;
  }
  if (tile_default_util_ > 1. || tile_default_util_ == 0.) {
    VTR_LOG_ERROR("The default utilization of tile '%f' is invalid. Expect (0, 1]!\n",
                  tile_default_util_);
    num_err++;
  }
  for (auto pair : tile_util_) {
    if (pair.second > 1. || pair.second == 0.) {
      VTR_LOG_ERROR("The utilization '%f' of tile '%s' is invalid. Expect (0, 1]!\n", pair.second,
                    pair.first.c_str());
      num_err++;
    }
  }
  if (!valid_horizental_gap_string(tile_gap_[0])) {
    VTR_LOG_ERROR("Invalid horizental gap definition for tiles '%s'! Expect %s*<integer>!\n",
                  tile_gap_[0].c_str(), KW_CPP_.c_str());
    num_err++;
  }
  if (!valid_vertical_gap_string(tile_gap_[1])) {
    VTR_LOG_ERROR("Invalid vertical gap definition for tiles '%s'! Expect %s*<integer>!\n",
                  tile_gap_[1].c_str(), KW_SC_HEIGHT_.c_str());
    num_err++;
  }
  for (e_side curr_side : sides()) {
    if (curr_side == TOP || curr_side == BOTTOM) {
      if (!valid_vertical_gap_string(side_gap_[size_t(curr_side)])) {
        VTR_LOG_ERROR("Invalid side '%s' gap definition '%s'! Expect %s*<integer>!\n",
                      TOTAL_2D_SIDE_STRINGS[size_t(curr_side)],
                      side_gap_[size_t(curr_side)].c_str(), KW_SC_HEIGHT_.c_str());
        num_err++;
      }
    } else {
      VTR_ASSERT_SAFE(curr_side == LEFT || curr_side == RIGHT);
      if (!valid_horizental_gap_string(side_gap_[size_t(curr_side)])) {
        VTR_LOG_ERROR("Invalid side '%s' gap definition '%s'! Expect %s*<integer>!\n",
                      TOTAL_2D_SIDE_STRINGS[size_t(curr_side)],
                      side_gap_[size_t(curr_side)].c_str(), KW_CPP_.c_str());
        num_err++;
      }
    }
  }
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err == 0;
}

void FloorplanStrategy::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Floorplanning strategy is invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void FloorplanStrategy::check_valid_side(const e_side& side) const {
  if (!valid_side(side)) {
    VTR_LOG_ERROR("Side is not valid! Expect the following:\n");
    for (size_t iside = 0; iside < NUM_2D_SIDES; ++iside) {
      VTR_LOG("\t%s\n", TOTAL_2D_SIDE_STRINGS[iside]);
    }
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

bool FloorplanStrategy::valid_side(const e_side& side) const {
  return size_t(side) < size_t(NUM_2D_SIDES);
}

void FloorplanStrategy::clear_all() {
  cpp_ = 0.;
  sc_height_ = 0.;
  clear();
}

void FloorplanStrategy::clear() {
  height_width_ratio_ = 1.;
  boundary_cell_width_.clear();
  tile_default_util_ = 0.8;
  tile_util_.clear();
  for (size_t igap = 0; igap < tile_gap_.size(); ++igap) {
    tile_gap_[igap].clear();
  }
  for (size_t igap = 0; igap < side_gap_.size(); ++igap) {
    side_gap_[igap].clear();
  }
  is_valid_ = false;
}

float FloorplanStrategy::parse_gap(const std::string& gap_str) const {
  /* Corner case : Only keyword, e.g., SC_HEIGHT or CPP */
  if (gap_str == KW_CPP_) {
    return cpp_;
  }
  if (gap_str == KW_SC_HEIGHT_) {
    return sc_height_;
  }
  /* Regular cases: CPP*<int> or SC_HEIGHT*<int> */
  openfpga::StringToken tokenizer(gap_str);
  std::vector<std::string> tokens = tokenizer.split(KW_MULT_);
  if (tokens.size() != 2) {
    VTR_LOG_ERROR(
      "Invalid gap definition '%s'! Expect [ %s | %s | %s*<integer> | %s*<integer> ]!\n",
      gap_str.c_str(), KW_CPP_.c_str(), KW_SC_HEIGHT_.c_str(), KW_CPP_.c_str(),
      KW_SC_HEIGHT_.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (tokens[0] == KW_CPP_) {
    if (std::all_of(tokens[1].begin(), tokens[1].end(), ::isdigit)) {
      return cpp_ * std::stoi(tokens[1]);
    } else {
      VTR_LOG_ERROR(
        "Invalid gap definition '%s'! Expect [ %s | %s | %s*<integer> | %s*<integer> ]!\n",
        gap_str.c_str(), KW_CPP_.c_str(), KW_SC_HEIGHT_.c_str(), KW_CPP_.c_str(),
        KW_SC_HEIGHT_.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
  }
  if (tokens[0] == KW_SC_HEIGHT_) {
    if (std::all_of(tokens[1].begin(), tokens[1].end(), ::isdigit)) {
      return sc_height_ * std::stoi(tokens[1]);
    } else {
      VTR_LOG_ERROR(
        "Invalid gap definition '%s'! Expect [ %s | %s | %s*<integer> | %s*<integer> ]!\n",
        gap_str.c_str(), KW_CPP_.c_str(), KW_SC_HEIGHT_.c_str(), KW_CPP_.c_str(),
        KW_SC_HEIGHT_.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
  }
  if (tokens[1] == KW_CPP_) {
    if (std::all_of(tokens[0].begin(), tokens[0].end(), ::isdigit)) {
      return cpp_ * std::stoi(tokens[0]);
    } else {
      VTR_LOG_ERROR(
        "Invalid gap definition '%s'! Expect [ %s | %s | %s*<integer> | %s*<integer> ]!\n",
        gap_str.c_str(), KW_CPP_.c_str(), KW_SC_HEIGHT_.c_str(), KW_CPP_.c_str(),
        KW_SC_HEIGHT_.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
  }
  if (tokens[1] == KW_SC_HEIGHT_) {
    if (std::all_of(tokens[0].begin(), tokens[0].end(), ::isdigit)) {
      return sc_height_ * std::stoi(tokens[0]);
    } else {
      VTR_LOG_ERROR(
        "Invalid gap definition '%s'! Expect [ %s | %s | %s*<integer> | %s*<integer> ]!\n",
        gap_str.c_str(), KW_CPP_.c_str(), KW_SC_HEIGHT_.c_str(), KW_CPP_.c_str(),
        KW_SC_HEIGHT_.c_str());
      exit(openfpga::CMD_EXEC_FATAL_ERROR);
    }
  }
  /* Invalid still */
  VTR_LOG_ERROR("Invalid gap definition '%s'! Expect [ %s | %s | %s*<integer> | %s*<integer> ]!\n",
                gap_str.c_str(), KW_CPP_.c_str(), KW_SC_HEIGHT_.c_str(), KW_CPP_.c_str(),
                KW_SC_HEIGHT_.c_str());
  exit(openfpga::CMD_EXEC_FATAL_ERROR);
}

bool FloorplanStrategy::valid_horizental_gap_string(const std::string& gap_str) const {
  /* Corner case : Only keyword, e.g., SC_HEIGHT or CPP */
  if (gap_str == KW_CPP_) {
    return true;
  }
  /* Regular cases: CPP*<int> or SC_HEIGHT*<int> */
  openfpga::StringToken tokenizer(gap_str);
  std::vector<std::string> tokens = tokenizer.split(KW_MULT_);
  if (tokens.size() != 2) {
    return false;
  }
  if (tokens[0] == KW_CPP_) {
    if (std::all_of(tokens[1].begin(), tokens[1].end(), ::isdigit)) {
      return true;
    }
  }
  if (tokens[1] == KW_CPP_) {
    if (std::all_of(tokens[0].begin(), tokens[0].end(), ::isdigit)) {
      return true;
    }
  }
  /* Invalid still */
  return false;
}

bool FloorplanStrategy::valid_vertical_gap_string(const std::string& gap_str) const {
  /* Corner case : Only keyword, e.g., SC_HEIGHT or CPP */
  if (gap_str == KW_SC_HEIGHT_) {
    return true;
  }
  /* Regular cases: CPP*<int> or SC_HEIGHT*<int> */
  openfpga::StringToken tokenizer(gap_str);
  std::vector<std::string> tokens = tokenizer.split(KW_MULT_);
  if (tokens.size() != 2) {
    return false;
  }
  if (tokens[0] == KW_SC_HEIGHT_) {
    if (std::all_of(tokens[1].begin(), tokens[1].end(), ::isdigit)) {
      return true;
    }
  }
  if (tokens[1] == KW_SC_HEIGHT_) {
    if (std::all_of(tokens[0].begin(), tokens[0].end(), ::isdigit)) {
      return true;
    }
  }
  /* Invalid still */
  return false;
}

FloorplanStrategy::e_distribution_network_type FloorplanStrategy::str2distribution_network_type(
  const std::string& str, const bool& verbose) const {
  for (int itype = size_t(FloorplanStrategy::e_distribution_network_type::HTREE);
       itype != size_t(FloorplanStrategy::e_distribution_network_type::NUM_TYPES); ++itype) {
    if (str == std::string(DISTRIBUTION_NETWORK_TYPE_STRING_[itype])) {
      return static_cast<FloorplanStrategy::e_distribution_network_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of distribution network! Expect %s\n",
                 distribution_network_type_all2str().c_str());
  return FloorplanStrategy::e_distribution_network_type::NUM_TYPES;
}

std::string FloorplanStrategy::distribution_network_type2str(
  const FloorplanStrategy::e_distribution_network_type& type, const bool& verbose) const {
  if (!valid_distribution_network_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type of distribution network! Expect %s\n",
                   distribution_network_type_all2str().c_str());
    return std::string();
  }
  return std::string(DISTRIBUTION_NETWORK_TYPE_STRING_[size_t(type)]);
}

std::string FloorplanStrategy::distribution_network_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(FloorplanStrategy::e_distribution_network_type::HTREE);
       itype != size_t(FloorplanStrategy::e_distribution_network_type::NUM_TYPES); ++itype) {
    full_types += std::string(DISTRIBUTION_NETWORK_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool FloorplanStrategy::valid_distribution_network_type(
  const FloorplanStrategy::e_distribution_network_type& distribution_network_type) const {
  return distribution_network_type != FloorplanStrategy::e_distribution_network_type::NUM_TYPES;
}

void FloorplanStrategy::check_valid_distribution_network_type(
  const FloorplanStrategy::e_distribution_network_type& type) const {
  if (!valid_distribution_network_type(type)) {
    VTR_LOG_ERROR("Invalid type of distribution network. Expect %s!",
                  distribution_network_type_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void FloorplanStrategy::add_metal_layer(const std::string& name, const float& len,
                                        const float& width, const float& spacing) {
  MetalLayer mlayer(name, len, width, spacing);
  metal_layers_.push_back(mlayer);
}

bool FloorplanStrategy::add_signal_distribution_network(const std::string& port_str,
                                                        const std::string& type_str,
                                                        const std::string& bufcell,
                                                        const size_t& depth,
                                                        const vtr::Point<size_t>& entry_point) {
  openfpga::PortParser port_parser(port_str);
  openfpga::BasicPort port = port_parser.port();
  if (!port.is_valid()) {
    VTR_LOG_ERROR("Invalid port '%s' for signal distribution network\n",
                  port.to_verilog_string().c_str());
    return false;
  }
  e_distribution_network_type type = str2distribution_network_type(type_str);
  if (!valid_distribution_network_type(type)) {
    VTR_LOG_ERROR("Invalid type of distribution network. Expect %s!",
                  distribution_network_type_all2str().c_str());
    return false;
  }
  SignalDistributionNetwork sig_ntwk(port, type, bufcell, depth, entry_point);
  signal_distribution_network_.push_back(sig_ntwk);
  return true;
}

bool FloorplanStrategy::valid_signal_distribution_network() const {
  return !signal_distribution_network_.empty();
}

std::vector<FloorplanStrategy::SignalDistributionNetwork>
FloorplanStrategy::signal_distribution_networks() const {
  return signal_distribution_network_;
}

bool FloorplanStrategy::valid_metal_layer(const std::string& name) const {
  for (auto mlayer : metal_layers_) {
    if (name == mlayer.name) {
      return true;
    }
  }
  return false;
}

float FloorplanStrategy::metal_layer_length(const std::string& name) const {
  for (auto mlayer : metal_layers_) {
    if (name == mlayer.name) {
      return mlayer.length;
    }
  }
  return 0;
}

float FloorplanStrategy::metal_layer_width(const std::string& name) const {
  for (auto mlayer : metal_layers_) {
    if (name == mlayer.name) {
      return mlayer.width;
    }
  }
  return 0;
}

float FloorplanStrategy::metal_layer_spacing(const std::string& name) const {
  for (auto mlayer : metal_layers_) {
    if (name == mlayer.name) {
      return mlayer.spacing;
    }
  }
  return 0;
}

}  // End namespace arkangel
