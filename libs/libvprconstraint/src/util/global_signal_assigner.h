#pragma once

#include "ckbuf_cell_info.h"
#include "global_port_loc_map.h"
#include "pin_constraints_fwd.h"
#include "pugixml.hpp"

namespace arkangel {
int assign_global_signals(PinConstraints& pcf_info, GlobalPortLoc& global_port_loc,
                          CkbufInfo& ckbuf_info,
                          const bool& include_internal_generated_global_signals,
                          const bool& verbose = true);

int assign_global_reset(GlobalPortLoc& global_port_loc, openfpga::BasicPort& assigned_ckbuf_port,
                        vtr::Point<int>& assigned_location, int& assigned_subtile);

int assign_global_clk(GlobalPortLoc& global_port_loc, openfpga::BasicPort& assigned_ckbuf_port,
                      vtr::Point<int>& assigned_location, int& assigned_subtile);

void log_assign_global_ports_result(const PinConstraints& pcf_info,
                                    const GlobalPortLoc& global_port_loc,
                                    const CkbufInfo& ckbuf_info);

void log_available_global_ports(const PinConstraints& pcf_info,
                                const GlobalPortLoc& global_port_loc);

int generate_global_signal_pcf4dv(const std::string& cell_map_file, const std::string& pcf_file,
                                  const std::string& clock_loc_map_file,
                                  const std::string& final_pcf);

int generate_global_signal_vpr_constraint_file(const std::string& cell_map_file,
                                               const std::string& pcf_file,
                                               const std::string& clock_loc_map_file,
                                               pugi::xml_node& vpr_constr_xml_root,
                                               const std::string& final_pcf);
}  // namespace arkangel
