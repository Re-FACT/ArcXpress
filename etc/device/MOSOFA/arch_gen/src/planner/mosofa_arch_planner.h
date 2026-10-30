#pragma once

#include <map>
#include <string>
#include <vector>
#include "physical_types.h"
#include "arch_gen_input_params.h"

namespace MOSOFA { // namespace MOSOFA begins

/**
 * 	Architecture parameter generator
 */
class ArchPlanner {
 public: /* -- Constructor -- */
  ArchPlanner(const ArchGenInputParams& params);
 public: /* -- TYPES -- */
 public: /* -- Accessors -- */
   int use_default() const;
   int use_default_num_io() const;
   size_t height() const;
   size_t width() const;
   size_t dp_height() const;
   size_t dp_width() const;
   size_t num_clocks() const;
   size_t num_pclocks() const;
   size_t num_config_regions() const;
   size_t num_resets() const;
   void check_valid() const;
   size_t num_input_per_tile(const e_side& io_side) const;
   size_t num_output_per_tile(const e_side& io_side) const;
   size_t num_io_per_tile(const e_side& io_side) const;
 private: /* -- Mutators -- */
   void reset();
   /* Plan all the paramters and be ready for access*/
   void plan(const bool& verbose);
   void plan_io_num();
 private: /* -- Internal data -- */
  /* Constants */
  size_t dp_core_height_;
  size_t dp_core_width_;
  size_t core_height_;
  size_t core_width_;
  size_t num_clks_;
  size_t num_rsts_;
  size_t num_pclks_;
  size_t num_config_regions_;
  std::map<e_side, size_t> num_inputs_;
  std::map<e_side, size_t> num_outputs_;
  std::map<e_side, size_t> num_input_per_tile_;
  std::map<e_side, size_t> num_output_per_tile_;
  size_t max_array_size_;
  size_t num_fle_per_clb_;

  bool is_valid_;
};

}  // end namespace MOSOFA
