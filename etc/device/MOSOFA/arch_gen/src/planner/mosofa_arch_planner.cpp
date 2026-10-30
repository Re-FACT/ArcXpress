#include <filesystem>
#include <cctype>
#include <cmath>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "mosofa_arch_planner.h"
#include "side_manager.h"

namespace MOSOFA {  // Begin namespace MOSOFA

ArchPlanner::ArchPlanner(const arcxpress::ArchGenInputParams& params) {
  reset();
  if (height != use_default()) {
    core_height_ = params.height;
    is_valid_ = false;
  } 
  if (width != use_default()) {
    core_width_ = params.width;
    is_valid_ = false;
  }
  if (sync_dp_dimension) {
    dp_core_height_ = core_height_;
    dp_core_width_ = core_width_;
    is_valid_ = false;
  }
  if (num_clks != use_default()) {
    VTR_LOG("Will use custom number of clocks: %d\n", num_clks);
    num_clks_ = params.num_clks;
    is_valid_ = false;
  } 
  if (num_pclks != use_default()) {
    VTR_LOG("Will use custom number of programming clocks: %d\n", num_pclks);
    num_pclks_ = params.num_pclks;
    is_valid_ = false;
  } 
  if (num_regions != use_default()) {
    VTR_LOG("Will use custom number of configurable regions: %d\n", num_regions);
    num_config_regions_ = params.num_regions;
    is_valid_ = false;
  } 
  if (num_rsts != use_default()) {
    VTR_LOG("Will use custom number of resets: %d\n", num_rsts);
    num_rsts_ = params.num_rsts;
    is_valid_ = false;
  } 
  if (!num_inputs.empty()) {
    for (auto pair : params.num_inputs) {
      if (pair.second != use_default_num_io()) {
        num_inputs_[pair.first] = pair.second;
        is_valid_ = false;
      }
    }
  }
  if (!num_outputs.empty()) {
    for (auto pair : params.num_outputs) {
      if (pair.second != use_default_num_io()) {
        num_outputs_[pair.first] = pair.second;
        is_valid_ = false;
      }
    }
  }
  plan(true);
}

int ArchPlanner::use_default() const {
  return 0;
}

int ArchPlanner::use_default_num_io() const {
  return -1;
}

void ArchPlanner::reset() {
  num_input_per_tile_ = { 
   {TOP, 12},
   {RIGHT, 12},
   {BOTTOM, 108},
   {LEFT, 12}
  };
  num_output_per_tile_ = {
   {TOP, 12},
   {RIGHT, 12},
   {BOTTOM, 108},
   {LEFT, 12}
  };

  dp_core_height_ = 4;
  dp_core_width_ = 4;
  core_height_ = 12;
  core_width_ = 12;
  num_clks_ = 1;
  num_pclks_ = 1;
  num_config_regions_ = 12;
  num_rsts_ = 0;
  max_array_size_ = 4096;
  num_fle_per_clb_ = 8;
  plan_io_num();
  is_valid_ = true;
}

int ArchPlanner::num_fle_per_clb() const {
  return num_fle_per_clb_;
}

int ArchPlanner::max_lut_size() const {
  return MAX_LUT_SIZE_;
}

void ArchPlanner::plan_io_num() {
  for (e_side curr_side : TOTAL_2D_SIDES}) {
    num_inputs_[curr_side] = core_width_ * (num_input_per_tile_[curr_side]);
    num_outputs_[curr_side] = core_width_ * (num_output_per_tile_[curr_side]);
  }
}

void ArchPlanner::plan(const bool& verbose) {
  /* Sanity checks on the core fabric size */
  if (core_height_ < 2) {
    VTR_LOG_ERROR("Core height should be at least 2 but provided '%lu'!\n", core_height_);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (core_width_ < 2) {
    VTR_LOG_ERROR("Core width should be at least 2 but provided '%lu'!\n", core_width_);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (core_height_ * core_width_ * num_fle_per_clb_ > max_array_size_) {
    VTR_LOG_ERROR("Core dimension %lu x %lu exceeds the resource limit (%lu)!\n", core_width_, core_height_, max_array_size_ / num_fle_per_clb_);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  /* If the core height and width is smaller than the dp height and width. Set the dp to follow the core dimension */
  if (core_height_ < dp_core_height_) {
    VTR_LOG("Set dp height to be same as core height (=%ld) for consistency\n", core_height_);
    dp_core_height_ = core_height_;
  }
  if (core_width_ < dp_core_width_) {
    VTR_LOG("Set dp width to be same as core width (=%ld) for consistency\n", core_width_);
    dp_core_width_ = core_width_;
  }
  /* Size clocks and rsts */
  if (num_clks_ < 1) {
    VTR_LOG_ERROR("Number of clocks should be at least 1 but provided '%lu'!\n", num_clks_);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (num_pclks_ < 1) {
    VTR_LOG_ERROR("Number of programming clocks should be at least 1 but provided '%lu'!\n", num_pclks_);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  if (num_config_regions_ < 1) {
    VTR_LOG_ERROR("Number of configurable regions should be at least 1 but provided '%lu'!\n", num_config_regions_);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }

  if (num_rsts_ < 1) {
    VTR_LOG_ERROR("Number of resets should be at least 1 but provided '%lu'!\n", num_rsts_);
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  /* Determine the number of I/Os */
  /* Plan inputs:
   * - All the output with clock output should be at least 1
   * - Only the bottom side has 1 programmable delay chain per input tile
   */
  for (e_side curr_side : TOTAL_2D_SIDES) {
    if (curr_side == TOP || curr_side == BOTTOM) {
      if (num_inputs_[curr_side] <= core_width_) {
        num_input_per_tile_[curr_side] = 1;
      } else {
        num_input_per_tile_[curr_side] = std::ceil((float)(num_inputs_[curr_side]) / (float)core_width_); 
      }
      if (num_input_per_tile_[curr_side] % core_width_ != 0) {
        VTR_LOG(
          "To meet the constraints, the actual number of I/Os on '%s' side will be larger than the "
          "user specification\n",
          SideManager(curr_side).c_str());
      }
    } else {
      VTR_ASSERT_SAFE(curr_side == LEFT || curr_side == RIGHT);
      if (num_inputs_[curr_side] <= core_height_) {
        num_input_per_tile_[curr_side] = 1;
      } else {
        num_input_per_tile_[curr_side] = std::ceil((float)(num_inputs_[curr_side]) / (float)core_height_); 
      }
      if (num_input_per_tile_[curr_side] % core_height_ != 0) {
        VTR_LOG(
          "To meet the constraints, the actual number of I/Os on '%s' side will be larger than the "
          "user specification\n",
          SideManager(curr_side).c_str());
      }
    }
  }
  /* Plan outputs */
  for (e_side curr_side : TOTAL_2D_SIDES) {
    if (curr_side == TOP || curr_side == BOTTOM) {
      if (num_outputs_[curr_side] <= core_width_) {
        num_output_per_tile_[curr_side] = 1;
      } else {
        num_output_per_tile_[curr_side] = std::ceil((float)(num_outputs_[curr_side]) / (float)core_width_); 
      }
      if (num_output_per_tile_[curr_side] % core_width_ != 0) {
        VTR_LOG(
          "To meet the constraints, the actual number of I/Os on '%s' side will be larger than the "
          "user specification\n",
          SideManager(curr_side).c_str());
      }
    } else {
      VTR_ASSERT_SAFE(curr_side == LEFT || curr_side == RIGHT);
      if (num_outputs_[curr_side] <= core_height_) {
        num_output_per_tile_[curr_side] = 1;
      } else {
        num_output_per_tile_[curr_side] = std::ceil((float)(num_outputs_[curr_side]) / (float)core_height_); 
      }
      if (num_output_per_tile_[curr_side] % core_height_ != 0) {
        VTR_LOG(
          "To meet the constraints, the actual number of I/Os on '%s' side will be larger than the "
          "user specification\n",
          SideManager(curr_side).c_str());
      }
    }
  }
  plan_io_num();
  is_valid_ = true;
}

void ArchPlanner::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Intend to arch planner access data when it is still dirty!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

size_t ArchPlanner::dp_height() const {
  check_valid();
  return dp_core_height_ + 2;
}

size_t ArchPlanner::dp_width() const {
  check_valid();
  return dp_core_width_ + 2;
}

size_t ArchPlanner::height() const {
  check_valid();
  return core_height_ + 2;
}

size_t ArchPlanner::width() const {
  check_valid();
  return core_width_ + 2;
}

size_t ArchPlanner::num_input_per_tile(const e_side& io_side) const {
  check_valid();
  auto result = num_input_per_tile_.find(io_side);
  if (result == num_input_per_tile_.end()) {
    VTR_LOG_ERROR("Invalid I/O side for Arch planner!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return result->second;
}

size_t ArchPlanner::num_output_per_tile(const e_side& io_side) const {
  check_valid();
  auto result = num_output_per_tile_.find(io_side);
  if (result == num_output_per_tile_.end()) {
    VTR_LOG_ERROR("Invalid I/O side for Arch planner!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return result->second;
}

size_t ArchPlanner::num_clocks() const {
  check_valid();
  return num_clks_; 
}

size_t ArchPlanner::num_pclocks() const {
  check_valid();
  return num_pclks_; 
}

size_t ArchPlanner::num_config_regions() const {
  check_valid();
  return num_config_regions_; 
}

size_t ArchPlanner::num_resets() const {
  check_valid();
  return num_rsts_; 
}


}  // End namespace MOSOFA
