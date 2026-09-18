#include "tile_ref.h"

#include <algorithm>
#include <filesystem>
#include <regex>

#include "arch_util.h"
#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

TileRef::TileRef() {
  init();
  is_valid_ = false;
}

void TileRef::init() {
  grid_type_ids_.push_back(0);
  grid_types_.push_back(std::string(EMPTY_BLOCK_NAME));
}

void TileRef::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Tile reference data is invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void TileRef::clear() {
  name2ids_.clear();
  grid_type_ids_.clear();
  grid_height_.clear();
  grid_width_.clear();
  instances_.clear();
  grid_types_.clear();
  init();
  is_valid_ = false;
}

std::vector<std::string> TileRef::tiles() const {
  check_valid();
  std::vector<std::string> ret;
  ret.reserve(name2ids_.size());
  for (auto pair : name2ids_) {
    ret.push_back(pair.first);
  }
  return ret;
}

bool TileRef::valid_internal_index(const size_t& idx) const { return idx < instances_.size(); }

size_t TileRef::find_internal_index(const std::string& tile) const {
  auto result = name2ids_.find(tile);
  if (result == name2ids_.end()) {
    VTR_LOG_ERROR("Invalid tile name '%s' which does not exist!\n", tile.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (!valid_internal_index(result->second)) {
    VTR_LOG_ERROR("Invalid tile index '%lu' which does not exist!\n", result->second);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return result->second;
}

size_t TileRef::num_instances(const std::string& tile) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  return instances_[t_idx].size();
}

// parse instance coordinate of tile
int TileRef::parse_tile_coord(const std::string& inst_name, vtr::Point<size_t>& coord,
                              const bool& verbose) const {
  std::regex pattern(R"_(.*_(\d+)__(\d+)_?)_");
  std::smatch match;
  if (std::regex_match(inst_name, match, pattern)) {
    int first = std::stoi(match[1]);
    int second = std::stoi(match[2]);
    coord.set_x(first);
    coord.set_y(second);
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOGV_WARN(verbose,
                "Invalid instance name '%s' which does not match the pattern tile_<int>__<int>_\n",
                inst_name.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

int TileRef::instance_coordinates(const std::string& tile, std::vector<vtr::Point<size_t>>& coords,
                                  const bool& verbose) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  for (std::string inst : instances_[t_idx]) {
    vtr::Point<size_t> coord(-1, -1);
    int status = parse_tile_coord(inst, coord);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      // could be direct_interc_0_. Give a warn for it and error out.
      VTR_LOGV_WARN(verbose, "Invalid instance name found for tile '%s'\n", tile.c_str());
      return openfpga::CMD_EXEC_MINOR_ERROR;
    }
    coords.push_back(coord);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int TileRef::tile_instance_and_coord_pairs(const std::string& tile,
                                           std::vector<std::string>& instance_names,
                                           std::vector<vtr::Point<size_t>>& coords,
                                           const bool& verbose) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  for (std::string inst : instances_[t_idx]) {
    vtr::Point<size_t> coord(-1, -1);
    int status = parse_tile_coord(inst, coord);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      // could be direct_interc_0_. Give a warn for it and error out.
      VTR_LOGV_WARN(verbose, "Invalid instance name found for tile '%s'\n", tile.c_str());
      return openfpga::CMD_EXEC_MINOR_ERROR;
    }
    instance_names.push_back(inst);
    coords.push_back(coord);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

std::string TileRef::type(const std::string& tile) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  return grid_types_[grid_type_ids_[t_idx]];
}

bool TileRef::add_tile(const std::string& tile, const int& height, const int& width) {
  /* Register */
  auto result = name2ids_.find(tile);
  if (result != name2ids_.end()) {
    VTR_LOG_ERROR(
      "Intend to overwrite an existing tile '%s'. Caused by duplicated tile definition. Please "
      "double check your tile reference data!\n",
      tile.c_str());
    return false;
  }
  name2ids_[tile] = instances_.size();
  grid_type_ids_.emplace_back(0);
  if (height < 1) {
    VTR_LOG_ERROR("Invalid height(=%d) for tile '%s'. The minimum height is 1\n", height,
                  tile.c_str());
    return false;
  }
  grid_height_.emplace_back(height);
  if (width < 1) {
    VTR_LOG_ERROR("Invalid width(=%d) for tile '%s'. The minimum width is 1\n", width,
                  tile.c_str());
    return false;
  }
  grid_width_.emplace_back(width);
  instances_.emplace_back();
  is_valid_ = true;
  return true;
}

bool TileRef::set_type(const std::string& tile, const std::string& type_str) {
  is_valid_ = false;
  size_t t_idx = find_internal_index(tile);
  auto it = std::find(grid_types_.begin(), grid_types_.end(), type_str);
  if (grid_types_.end() == it) {
    grid_type_ids_[t_idx] = grid_types_.size();
    grid_types_.push_back(type_str);
  } else {
    grid_type_ids_[t_idx] = std::distance(grid_types_.begin(), it);
  }
  is_valid_ = true;
  return true;
}

bool TileRef::add_instance(const std::string& tile, const std::string& inst) {
  is_valid_ = false;
  size_t t_idx = find_internal_index(tile);
  /* Sanity checks on duplicated instance names */
  std::vector<std::string> inst_l = instances_[t_idx];
  if (inst_l.end() != std::find(inst_l.begin(), inst_l.end(), inst)) {
    VTR_LOG_ERROR("Instance '%s' already exists for tile '%s'. Duplication is not allowed!\n",
                  inst.c_str(), tile.c_str());
    return false;
  }
  instances_[t_idx].push_back(inst);
  is_valid_ = true;
  return true;
}

size_t TileRef::num_tiles() const {
  check_valid();
  return name2ids_.size();
}

std::string TileRef::custom_tile_name(const std::string& tile) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  std::string ret = tile + std::string("_");
  ret += grid_types_[grid_type_ids_[t_idx]] + std::string("_");
  ret += std::string("id") + std::to_string(t_idx) + std::string("_");
  return ret;
}

size_t TileRef::grid_height(const std::string& tile) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  return grid_height_[t_idx];
}

std::vector<size_t> TileRef::grid_height_offsets(const std::string& tile) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  std::vector<size_t> ret;
  for (size_t ofs = 0; ofs < size_t(grid_height_[t_idx]); ++ofs) {
    ret.push_back(ofs);
  }
  return ret;
}

size_t TileRef::grid_width(const std::string& tile) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  return grid_width_[t_idx];
}

std::vector<size_t> TileRef::grid_width_offsets(const std::string& tile) const {
  check_valid();
  size_t t_idx = find_internal_index(tile);
  std::vector<size_t> ret;
  for (size_t ofs = 0; ofs < size_t(grid_width_[t_idx]); ++ofs) {
    ret.push_back(ofs);
  }
  return ret;
}

bool TileRef::check_equivalent(const TileRef& ref, const bool& verbose) const {
  /* Both should be valid */
  check_valid();
  ref.check_valid();
  VTR_LOGV(verbose, "Checking consistency on tile names and types\n");
  /* Check tile names and types match */
  for (std::string tile : tiles()) {
    if (!ref.tile_exist(tile)) {
      VTR_LOG_ERROR("Mismatch found: required tile '%s' does not exist!\n", tile.c_str());
      return false;
    }
    if (type(tile) != ref.type(tile)) {
      VTR_LOG_ERROR(
        "Mismatch found: tile '%s' has different types '%s' from current and '%s' from reference. "
        "Require to be same\n",
        tile.c_str(), type(tile).c_str(), ref.type(tile).c_str());
      return false;
    }
  }
  VTR_LOGV(verbose, "Consistency checks passed\n");
  return true;
}

bool TileRef::tile_exist(const std::string& tile) const {
  check_valid();
  auto result = name2ids_.find(tile);
  return result != name2ids_.end();
}

}  // End namespace arkangel
