#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArcXpress benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class BitstreamDistribution {
 public: /* -- Constructors -- */
  BitstreamDistribution();

 public: /* -- Accessors -- */
  size_t num_regions() const;
  size_t max_region_bitstream_size() const;
  size_t bitstream_size() const;

 public: /* -- Mutators -- */
  void set_region_num_bits(const size_t& region_id, const size_t& num_bits);
  /* Clear methods */
  void clear();

 public: /* -- Validator -- */
  bool validate(const size_t& expect_num_bits);
  void check_valid() const;

 private: /* -- Internal data -- */
  std::vector<size_t> num_bits_per_region_;
  mutable bool is_valid_;
};

}  // namespace arcxpress
