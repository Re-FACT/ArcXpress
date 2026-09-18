#include "bitstream_distribution.h"

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

BitstreamDistribution::BitstreamDistribution() { is_valid_ = false; }

size_t BitstreamDistribution::num_regions() const { return num_bits_per_region_.size(); }

size_t BitstreamDistribution::max_region_bitstream_size() const {
  check_valid();
  size_t max_num = 0;
  for (size_t num_bits : num_bits_per_region_) {
    max_num = std::max(num_bits, max_num);
  }
  return max_num;
}

size_t BitstreamDistribution::bitstream_size() const {
  check_valid();
  size_t sum = 0;
  for (size_t num_bits : num_bits_per_region_) {
    sum += num_bits;
  }
  return sum;
}

void BitstreamDistribution::set_region_num_bits(const size_t& region_id, const size_t& num_bits) {
  if (region_id >= num_bits_per_region_.size()) {
    num_bits_per_region_.resize(region_id + 1);
  }
  num_bits_per_region_[region_id] = num_bits;
}

void BitstreamDistribution::clear() {
  num_bits_per_region_.clear();
  is_valid_ = false;
}

bool BitstreamDistribution::validate(const size_t& expect_num_bits) {
  size_t sum = 0;
  for (size_t num_bits : num_bits_per_region_) {
    sum += num_bits;
  }
  if (sum == expect_num_bits) {
    is_valid_ = true;
    return true;
  }
  VTR_LOG(
    "Invalid bitstream distribution where the expected number of bits '%lu' does not match the "
    "internal data '%lu'!\n",
    expect_num_bits, sum);
  return false;
}

void BitstreamDistribution::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Bitstream distribution data is invalid!");
    VTR_LOG("Check the data sources where it is loaded from!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

}  // End namespace arkangel
