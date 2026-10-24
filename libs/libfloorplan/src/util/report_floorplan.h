#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

#include "tile_qor.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int report_floorplan(const std::string& shaping_rpt, const std::string& proj_name,
                     const std::string& family, const std::string& pdk_name,
                     const std::string& run_name, const TileQor& tile_qor,
                     const float& shrink_factor, const size_t& precision, const bool& verbose);

int report_floorplan_to_csv(const std::string& rpt_fpath, const std::string& proj_name,
                            const std::string& family, const std::string& pdk_name,
                            const std::string& run_name, const TileQor& tile_qor,
                            const float& shrink_factor, const size_t& precision,
                            const bool& verbose);
#ifdef USE_QT
int draw_floorplan_figure(const std::string& fpath, const TileQor& tile_qor,
                          const bool& mark_sig_distr_ntwk, const float& scale_fac,
                          const bool& verbose);
#endif

int report_floorplan_shaping_txt(const TileQor& tile_qor, const std::string& file_name,
                                 const size_t& precision);

int report_floorplan_signal_distribution_network_to_xml(const TileQor& tile_qor,
                                                        const std::string& fname);
int report_floorplan_io_physical_location_to_text(const TileQor& tile_qor,
                                                  const std::string& rpt_fpath,
                                                  const bool& verbose);
int report_floorplan_tile_pin_physical_location_to_text(const TileQor& tile_qor,
                                                        const std::string& rpt_fpath,
                                                        const bool& verbose);
}  // End of namespace arcxpress
