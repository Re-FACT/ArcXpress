#pragma once

#include <map>
#include "physical_types.h"

namespace arcxpress { // begin namespace arcxpress

/* Data packet */
struct ArchGenInputParams {
  int height;
  int width;
  std::map<e_side, int> num_inputs;
  std::map<e_side, int> num_outputs;
  int num_clks;
  int num_rsts;
  int num_pclks;
  int num_config_regions;
  bool sync_dp_dimension;
};

} // end namespace arcxpress
