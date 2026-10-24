#pragma once
#include <string>

namespace arcxpress {
/* Forward declaration */
const std::string DEFAULT_VIRTUAL_CLOCK4STA = "virtual_clock";

// New function to update timing_gclk strategy file in-place (overwrites original)
int update_timing_gclk_strategy_file(const std::string& template_file, int num_clocks,
                                     int num_resets);

// Legacy function for backward compatibility (kept but deprecated)
int generate_timing_gclk_strategy_file(const std::string& template_file,
                                       const std::string& output_file, int num_clocks,
                                       int num_resets);

int generate_constrain_io_file(const std::string& constran_io_file_path,
                               const std::string& final_gclk_pcf,
                               const std::string& clock_loc_map_file,
                               const std::string& clock_period, const bool& verbose);

int generate_disable_config(const std::string& disable_config_file_path);

}  // namespace arcxpress