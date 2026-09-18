#include "arch_io_constraints.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>

#include "arch_util.h"
#include "command_exit_codes.h"
#include "side_manager.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

ArchIoConstraints::ArchIoConstraints() {
  is_valid_ = false;
  IO_DIRECTION_STRING_ = {"input", "output"};
}

std::string ArchIoConstraints::side2str(const e_side& type, const bool& verbose) const {
  if (!valid_side(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid side! Expect %s\n", side_all2str().c_str());
    return std::string();
  }
  SideManager side_mgr(type);
  return side_mgr.to_string();
}

std::string ArchIoConstraints::io_direction2str(const e_io_direction& type,
                                                const bool& verbose) const {
  if (!valid_io_direction(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid I/O direction! Expect %s\n", io_direction_all2str().c_str());
    return std::string();
  }
  return IO_DIRECTION_STRING_[size_t(type)];
}

std::string ArchIoConstraints::side_all2str() const {
  std::string all_side_str = "[";
  for (size_t iside = 0; iside < size_t(NUM_2D_SIDES); iside++) {
    SideManager side_mgr(iside);
    all_side_str += side_mgr.to_string() + std::string("|");
  }
  all_side_str.pop_back();
  all_side_str += "]";
  return all_side_str;
}

e_side ArchIoConstraints::str2side(const std::string& side_str, const bool& verbose) const {
  e_side curr_side = NUM_2D_SIDES;
  for (size_t iside = 0; iside < size_t(NUM_2D_SIDES); iside++) {
    SideManager side_mgr(iside);
    if (side_str == side_mgr.to_string()) {
      curr_side = side_mgr.get_side();
      break;
    }
  }
  if (!valid_side(curr_side)) {
    VTR_LOGV_ERROR(verbose, "Invalid side '%s'. Expect %s\n", side_str.c_str(),
                   side_all2str().c_str());
  }
  return curr_side;
}

ArchIoConstraints::e_io_direction ArchIoConstraints::str2io_direction(const std::string& str,
                                                                      const bool& verbose) const {
  for (int itype = size_t(ArchIoConstraints::e_io_direction::INPUT);
       itype != size_t(ArchIoConstraints::e_io_direction::NUM_TYPES); ++itype) {
    if (str == std::string(IO_DIRECTION_STRING_[itype])) {
      return static_cast<ArchIoConstraints::e_io_direction>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type of I/O direction! Expect %s\n",
                 io_direction_all2str().c_str());
  return ArchIoConstraints::e_io_direction::NUM_TYPES;
}

std::vector<ArchIoConstraints::e_io_direction> ArchIoConstraints::io_directions() const {
  std::vector<ArchIoConstraints::e_io_direction> ret;
  for (int itype = size_t(ArchIoConstraints::e_io_direction::INPUT);
       itype != size_t(ArchIoConstraints::e_io_direction::NUM_TYPES); ++itype) {
    ret.push_back(static_cast<ArchIoConstraints::e_io_direction>(itype));
  }
  return ret;
}

std::string ArchIoConstraints::io_direction_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(ArchIoConstraints::e_io_direction::INPUT);
       itype != size_t(ArchIoConstraints::e_io_direction::NUM_TYPES); ++itype) {
    full_types += std::string(IO_DIRECTION_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool ArchIoConstraints::valid_side(const e_side& f_type) const { return f_type != NUM_2D_SIDES; }

bool ArchIoConstraints::valid_io_direction(const ArchIoConstraints::e_io_direction& f_type) const {
  return f_type != ArchIoConstraints::e_io_direction::NUM_TYPES;
}

bool ArchIoConstraints::add_num_io_per_tile_setting(const std::string& side_str,
                                                    const std::string& func,
                                                    const std::string& var_name,
                                                    const std::string& msb_var_name,
                                                    const size_t& max) {
  e_io_direction io_dir = str2io_direction(func, true);
  if (!valid_io_direction(io_dir)) {
    return false;
  }
  e_side curr_side = str2side(side_str, true);
  if (curr_side == NUM_2D_SIDES) {
    return false;
  }
  auto result = name2ids_.find(var_name);
  if (result != name2ids_.end()) {
    VTR_LOG_ERROR(
      "Variable '%s' already defined in the I/O settings! Duplicated definition is not allowed\n",
      var_name.c_str());
    return false;
  }
  /* Avoid duplicated msb var names */
  if (!msb_var_name.empty() &&
      msb_var_names_.end() !=
        std::find(msb_var_names_.begin(), msb_var_names_.end(), msb_var_name)) {
    VTR_LOG_ERROR(
      "MSB variable '%s' already defined in the I/O settings! Duplicated definition is not "
      "allowed\n",
      msb_var_name.c_str());
    return false;
  }
  /* Assign new id */
  size_t new_id = msb_var_names_.size();
  name2ids_[var_name] = new_id;
  msb_var_names_.push_back(msb_var_name);
  max_per_tile_.push_back(max);
  min_per_tile_.push_back(1);
  /* Register in fast look-up */
  var_lookup_[size_t(curr_side)][size_t(io_dir)].push_back(var_name);
  return true;
}

bool ArchIoConstraints::add_io_type_setting(const std::string& side_str,
                                            const std::string& var_name) {
  e_side curr_side = str2side(side_str, true);
  if (!valid_side(curr_side)) {
    return false;
  }
  arch_io_types_[size_t(curr_side)] = var_name;
  return true;
}

bool ArchIoConstraints::validate(const bool& verbose) const {
  size_t num_err = 0;
  /* Each side should have valid io vars */
  for (size_t iside = 0; iside < arch_io_types_.size(); ++iside) {
    if (arch_io_types_[iside].empty()) {
      VTR_LOG_ERROR("Side '%s' has no variables for I/O existence\n", SideManager(iside).c_str());
      num_err++;
    }
  }
  for (size_t iside = 0; iside < var_lookup_.size(); ++iside) {
    for (size_t itype = 0; itype < var_lookup_[iside].size(); ++itype) {
      if (var_lookup_[iside][itype].empty()) {
        VTR_LOG_ERROR(
          "Side '%s' has no variables for I/O ('%s') parameters\n", SideManager(iside).c_str(),
          io_direction2str(static_cast<ArchIoConstraints::e_io_direction>(itype), true).c_str());
        num_err++;
      }
      /* For each side, there should be only 1 variable without a max */
      size_t num_unconstrained_io = 0;
      for (auto var_name : var_lookup_[iside][itype]) {
        auto result = name2ids_.find(var_name);
        if (result == name2ids_.end()) {
          VTR_LOG_ERROR(
            "Invalid variable '%s' on side '%s' and direction '%s' which is not defined!\n",
            var_name.c_str(), SideManager(iside).c_str(),
            io_direction2str(static_cast<ArchIoConstraints::e_io_direction>(itype), true).c_str());
          return false;
        }
        size_t var_idx = result->second;
        if (max_per_tile_[var_idx] == 0) {
          num_unconstrained_io++;
        }
        if (min_per_tile_[var_idx] == 0) {
          VTR_LOG_ERROR(
            "The I/O (direction '%s') variables on side '%s' has a minimum value of 0. Expect to "
            "be 1\n",
            SideManager(iside).c_str(),
            io_direction2str(static_cast<ArchIoConstraints::e_io_direction>(itype), true).c_str());
          num_err++;
        }
      }
      if (num_unconstrained_io == 0) {
        VTR_LOG_ERROR(
          "The I/O (direction '%s') variables on side '%s' has no unconstrained type\n",
          SideManager(iside).c_str(),
          io_direction2str(static_cast<ArchIoConstraints::e_io_direction>(itype), true).c_str());
        num_err++;
      }
      if (num_unconstrained_io > 1) {
        VTR_LOG_ERROR(
          "The I/O (direction '%s') variables on side '%s' has more than 1 unconstrained type\n",
          SideManager(iside).c_str(),
          io_direction2str(static_cast<ArchIoConstraints::e_io_direction>(itype), true).c_str());
        num_err++;
      }
    }
  }
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err == 0;
}

void ArchIoConstraints::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Invalid file type!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void ArchIoConstraints::check_valid_side(const e_side& side) const {
  if (!valid_side(side)) {
    VTR_LOG_ERROR("Invalid side! Expect %s\n", side_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void ArchIoConstraints::check_valid_io_direction(const e_io_direction& type) const {
  if (!valid_io_direction(type)) {
    VTR_LOG_ERROR("Invalid I/O direction! Expect %s\n", io_direction_all2str().c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

size_t ArchIoConstraints::io_num_by_side(const e_side& io_side, const size_t& io_num_per_tile,
                                         const int& height, const int& width) const {
  if (io_side == LEFT || io_side == RIGHT) {
    return io_num_per_tile * height;
  }
  VTR_ASSERT(io_side == TOP || io_side == BOTTOM);
  return io_num_per_tile * width;
}

size_t ArchIoConstraints::io_num_per_tile_by_side(const e_side& io_side, const size_t& io_num,
                                                  const int& height, const int& width) const {
  if (io_side == LEFT || io_side == RIGHT) {
    if (io_num % height != 0) {
      VTR_LOG(
        "To meet the constraints, the actual number of I/Os on '%s' side will be larger than the "
        "user specification\n",
        side2str(io_side).c_str());
    }
    return std::ceil((float)io_num / (float)height);
  }
  VTR_ASSERT(io_side == TOP || io_side == BOTTOM);
  if (io_num % width != 0) {
    VTR_LOG(
      "To meet the constraints, the actual number of I/Os on '%s' side will be larger than the "
      "user specification\n",
      side2str(io_side).c_str());
  }
  return std::ceil((float)io_num / (float)width);
}

size_t ArchIoConstraints::min_io_num(const e_side& io_side, const e_io_direction& io_dir,
                                     const int& height, const int& width) const {
  size_t num_io = 0;
  check_valid();
  check_valid_side(io_side);
  check_valid_io_direction(io_dir);
  for (std::string var_name : var_lookup_[size_t(io_side)][size_t(io_dir)]) {
    size_t var_idx = name2ids_.at(var_name);
    num_io += min_per_tile_[var_idx];
  }
  return io_num_by_side(io_side, num_io, height, width);
}

std::vector<std::string> ArchIoConstraints::var_names(const e_side& io_side,
                                                      const e_io_direction& io_type) const {
  check_valid();
  check_valid_side(io_side);
  check_valid_io_direction(io_type);
  return var_lookup_[size_t(io_side)][size_t(io_type)];
}

std::vector<std::string> ArchIoConstraints::var_names_constrained(
  const e_side& io_side, const e_io_direction& io_type) const {
  check_valid();
  check_valid_side(io_side);
  check_valid_io_direction(io_type);
  std::vector<std::string> ret;
  for (std::string var_name : var_lookup_[size_t(io_side)][size_t(io_type)]) {
    size_t var_idx = name2ids_.at(var_name);
    if (max_per_tile_[var_idx] != 0) {
      ret.push_back(var_name);
    }
  }
  return ret;
}

std::vector<std::string> ArchIoConstraints::var_names_unconstrained(
  const e_side& io_side, const e_io_direction& io_type) const {
  check_valid();
  check_valid_side(io_side);
  check_valid_io_direction(io_type);
  std::vector<std::string> ret;
  for (std::string var_name : var_lookup_[size_t(io_side)][size_t(io_type)]) {
    size_t var_idx = name2ids_.at(var_name);
    if (max_per_tile_[var_idx] == 0) {
      ret.push_back(var_name);
    }
  }
  return ret;
}

std::string ArchIoConstraints::io_type_var_name(const e_side& io_side) const {
  check_valid();
  check_valid_side(io_side);
  return arch_io_types_[size_t(io_side)];
}

int ArchIoConstraints::find_and_assign_arch_io_type_vars(
  const e_side& io_side, std::map<std::string, std::string>& io_vars, const bool& verbose) const {
  check_valid();
  check_valid_side(io_side);
  /* clear start */
  io_vars.clear();
  /* Exception: if the io num is set to 0, no more computing is needed. Early exit */
  VTR_LOGV(verbose, "%s side I/O are all removed\n", side2str(io_side).c_str());
  io_vars[io_type_var_name(io_side)] = std::string(EMPTY_BLOCK_NAME);
  return openfpga::CMD_EXEC_SUCCESS;
}

int ArchIoConstraints::find_and_assign_arch_io_vars(
  const e_side& io_side, const e_io_direction& io_type, const size_t& io_num, const int& height,
  const int& width, std::map<std::string, std::string>& io_vars, const bool& verbose) const {
  check_valid();
  check_valid_side(io_side);
  check_valid_io_direction(io_type);
  /* clear start */
  io_vars.clear();
  /* This is a constraint forced by OpenFPGA which does not support I/O on corners */
  int io_height = height - 2;
  int io_width = width - 2;
  /* Check if the io num hit the lower bound. If so, follow the minimum arrangement */
  if (io_num <= min_io_num(io_side, io_type, io_height, io_width)) {
    VTR_LOG_WARN(
      "Consider the minimum number '%ld' for I/O '%s' on '%s' side, rather than the constraint "
      "'%ld'\n",
      min_io_num(io_side, io_type, io_height, io_width), io_direction2str(io_type).c_str(),
      side2str(io_side).c_str(), io_num);
    for (std::string var_name : var_names(io_side, io_type)) {
      size_t var_idx = name2ids_.at(var_name);
      io_vars[var_name] = std::to_string(min_per_tile_[var_idx]);
      std::string io_msb_var = msb_var_names_[var_idx];
      if (io_msb_var.empty()) {
        continue;
      }
      io_vars[io_msb_var] = std::to_string(min_per_tile_[var_idx] - 1);
    }
    return openfpga::CMD_EXEC_SUCCESS;
  }
  /* Now show respect to the upper bound per type. And spread the I/O count, subject to the
   * constraint */
  size_t num_assigned_io = 0;
  for (std::string var_name : var_names_constrained(io_side, io_type)) {
    size_t var_idx = name2ids_.at(var_name);
    io_vars[var_name] = std::to_string(max_per_tile_[var_idx]);
    /* Count the number */
    num_assigned_io += io_num_by_side(io_side, max_per_tile_[var_idx], io_height, io_width);
    std::string io_msb_var = msb_var_names_[var_idx];
    if (io_msb_var.empty()) {
      continue;
    }
    io_vars[io_msb_var] = std::to_string(max_per_tile_[var_idx] - 1);
  }
  size_t curr_io_per_tile =
    io_num_per_tile_by_side(io_side, io_num - num_assigned_io, io_height, io_width);
  if (curr_io_per_tile < 1) {
    VTR_LOG_ERROR("Invalid number of I/O which is lower than the minimum number\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  for (std::string var_name : var_names_unconstrained(io_side, io_type)) {
    size_t var_idx = name2ids_.at(var_name);
    io_vars[var_name] = std::to_string(curr_io_per_tile);
    std::string io_msb_var = msb_var_names_[var_idx];
    if (io_msb_var.empty()) {
      continue;
    }
    io_vars[io_msb_var] = std::to_string(curr_io_per_tile - 1);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arkangel
