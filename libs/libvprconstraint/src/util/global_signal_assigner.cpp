
#include "global_signal_assigner.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "read_xml_ckbuf_cell.h"
#include "read_xml_clk_loc_map.h"
#include "read_xml_pin_constraints.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "write_xml_pin_constraints.h"
#include "write_xml_utils.h"
#include "write_xml_vpr_constraint.h"

namespace arkangel {

int assign_global_clk(GlobalPortLoc& global_port_loc, openfpga::BasicPort& assigned_ckbuf_port,
                      vtr::Point<int>& assigned_location, int& assigned_subtile) {
  for (auto port_id : global_port_loc.global_ports()) {
    if (global_port_loc.is_global_port_clk(port_id) &&
        !global_port_loc.is_global_port_assigned(port_id)) {
      assigned_ckbuf_port = global_port_loc.get_global_port_name(port_id);
      assigned_location = global_port_loc.get_global_port_coord(port_id);
      assigned_subtile = global_port_loc.get_global_port_subtile(port_id);
      global_port_loc.set_global_port_assigned_status(port_id, true);
      return openfpga::CMD_EXEC_SUCCESS;
    }
  }
  return openfpga::CMD_EXEC_NONE;
}

int assign_global_reset(GlobalPortLoc& global_port_loc, openfpga::BasicPort& assigned_ckbuf_port,
                        vtr::Point<int>& assigned_location, int& assigned_subtile) {
  for (auto port_id : global_port_loc.global_ports()) {
    if (global_port_loc.is_global_port_reset(port_id) &&
        !global_port_loc.is_global_port_assigned(port_id)) {
      assigned_ckbuf_port = global_port_loc.get_global_port_name(port_id);
      assigned_location = global_port_loc.get_global_port_coord(port_id);
      assigned_subtile = global_port_loc.get_global_port_subtile(port_id);
      global_port_loc.set_global_port_assigned_status(port_id, true);
      return openfpga::CMD_EXEC_SUCCESS;
    }
  }
  return openfpga::CMD_EXEC_NONE;
}

void log_assign_global_ports_result(const PinConstraints& pcf_info,
                                    const GlobalPortLoc& global_port_loc,
                                    const CkbufInfo& ckbuf_info) {
  for (auto ckbuf_id : ckbuf_info.ckbufs()) {
    std::string ckbuf_name = ckbuf_info.get_ckbuf_name(ckbuf_id);
    auto assigned_ckbuf_port = ckbuf_info.get_ckbuf_assigned_port(ckbuf_id);
    VTR_LOG("Ckbuf net %s is constraint to port %s \n", ckbuf_name.c_str(),
            assigned_ckbuf_port.to_verilog_string().c_str());
  }
}
void log_available_global_ports(const PinConstraints& pcf_info,
                                const GlobalPortLoc& global_port_loc) {
  int available_port_num = 0;
  for (auto id : global_port_loc.global_ports()) {
    auto global_port = global_port_loc.get_global_port_name(id);
    std::string constrainted_net_in_pcf = pcf_info.pin_net(global_port);
    if (constrainted_net_in_pcf.empty()) {
      // no constrained net found for this port, which means this port is not used
      available_port_num++;
    } else {
      std::string global_port_name = global_port.to_verilog_string();
      VTR_LOG("Global signal %s is constrained to port %s\n", constrainted_net_in_pcf.c_str(),
              global_port_name.c_str());
    }
  }
  VTR_LOG("The number of available global ports is: %d\n", available_port_num);
}

/* Assign global signals based on the pin-constraint from user
 * - all the unmapped global ports will be set to OPEN
 * - Optional: internal generated global signals will be assigned to available (not yet mapped)
 * global ports In such scenario, the internally generated global signal may not be considered. For
 * instance, testbench generation
 */
int assign_global_signals(PinConstraints& pcf_info, GlobalPortLoc& global_port_loc,
                          CkbufInfo& ckbuf_info,
                          const bool& include_internal_generated_global_signals,
                          const bool& verbose) {
  // update assignment status of ports in global_port_loc using pcf_info
  for (auto id : global_port_loc.global_ports()) {
    std::string constrainted_net_in_pcf =
      pcf_info.pin_net(global_port_loc.get_global_port_name(id));
    if (pcf_info.unconstrained_net(constrainted_net_in_pcf) ||
        pcf_info.unmapped_net(constrainted_net_in_pcf)) {
      // no constrained net found for this port, which means this port is not used
      global_port_loc.set_global_port_assigned_status(id, false);
    } else {
      global_port_loc.set_global_port_assigned_status(id, true);
    }
  }
  if (verbose) {
    log_available_global_ports(pcf_info, global_port_loc);
  }

  if (include_internal_generated_global_signals) {
    // assign global port for ckbuf
    for (auto ckbuf_id : ckbuf_info.ckbufs()) {
      std::string ckbuf_name = ckbuf_info.get_ckbuf_name(ckbuf_id);
      openfpga::BasicPort assigned_ckbuf_port;
      vtr::Point<int> assigned_location(-1, -1);
      int assigned_subtile;
      if (ckbuf_info.is_ckbuf_clk(ckbuf_id)) {
        int status = assign_global_clk(global_port_loc, assigned_ckbuf_port, assigned_location,
                                       assigned_subtile);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          VTR_LOG_ERROR("No global clock port resource available for ckbuf %s!\n",
                        ckbuf_name.c_str());
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
      } else {
        int status = assign_global_reset(global_port_loc, assigned_ckbuf_port, assigned_location,
                                         assigned_subtile);
        if (status != openfpga::CMD_EXEC_SUCCESS) {
          VTR_LOG_ERROR("No global reset port resource available for ckbuf %s\n",
                        ckbuf_name.c_str());
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }
      }

      pcf_info.create_pin_constraint(assigned_ckbuf_port, ckbuf_name);
      ckbuf_info.set_ckbuf_assigned_port(ckbuf_id, assigned_ckbuf_port);
      ckbuf_info.set_ckbuf_assigned_location(ckbuf_id, assigned_location);
      ckbuf_info.set_ckbuf_assigned_subtile(ckbuf_id, assigned_subtile);
    }
  }
  /*set unused port to be OPEN*/
  for (auto port_id : global_port_loc.global_ports()) {
    if (!global_port_loc.is_global_port_assigned(port_id)) {
      pcf_info.create_pin_constraint(global_port_loc.get_global_port_name(port_id),
                                     std::string(PIN_CONSTRAINT_OPEN_NET));
    }
  }

  if (verbose) {
    log_assign_global_ports_result(pcf_info, global_port_loc, ckbuf_info);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Generate the PCF XML file in design verification purpose
 * Here, a simple auto-completion on the PCF is accomplished
 * No internally generated global signals are assigned
 * Note that this can NOT be used to generate regular PCF for bitstream generation!
 */
int generate_global_signal_pcf4dv(const std::string& cell_map_file, const std::string& pcf_file,
                                  const std::string& clock_loc_map_file,
                                  const std::string& final_pcf) {
  vtr::ScopedStartFinishTimer timer(
    "Generating auto-completed PCF without considering internally generated global signals...");

  /*get ckbuf atom info from cell_map file */
  CkbufInfo ckbuf_info;
  if (set_up_ckbuf_info(cell_map_file, ckbuf_info) != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR(
      "Failed to get ckbuf information from cell map file! "
      "file\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* get global ports format from clock_loc_map file   */
  GlobalPortLoc global_port_loc;
  if (set_up_global_ports(clock_loc_map_file, global_port_loc) != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to get global ports from clock location map file \n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* get pin_constraint_info from pcf file. This map gives the information of where the clk and
   * reset are bound */
  VTR_LOG("Reading PCF file from: %s\n", pcf_file.c_str());
  if (!std::filesystem::exists(pcf_file)) {
    VTR_LOG_ERROR("PCF file does not exist: %s\n", pcf_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  PinConstraints pcf_info = read_xml_pin_constraints(pcf_file.c_str());
  /* FIXME: This bug should be fixed in the libpcf of OpenFPGA!!!!
   * Assign default value to all the signals without a specific value */
  int global_port_num = global_port_loc.global_port_num();
  pcf_info.reserve_pin_constraints(global_port_num);

  for (auto ipcf : pcf_info.pin_constraints()) {
    std::string curr_net = pcf_info.net(ipcf);
    if (pcf_info.unconstrained_net(curr_net)) {
      continue;
    }
    if (!pcf_info.valid_net_default_value(ipcf)) {
      pcf_info.set_net_default_value(ipcf, std::string("0"));
    }
  }
  std::map<std::string, std::string> pin_constraint_info;

  int status = assign_global_signals(pcf_info, global_port_loc, ckbuf_info, false);

  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Assign global ports failed!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }

  // write final pcf
  write_xml_pin_constraints(final_pcf.c_str(), pcf_info);
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Generate the PCF XML file in bitstream generation purpose
 * Here, an auto-completion on the PCF is accomplished where internally generated global signals are
 * assigned Note that this can NOT be used to generate regular PCF for design verification!
 */
int generate_global_signal_vpr_constraint_file(const std::string& cell_map_file,
                                               const std::string& pcf_file,
                                               const std::string& clock_loc_map_file,
                                               pugi::xml_node& vpr_constr_xml_root,
                                               const std::string& final_pcf) {
  vtr::ScopedStartFinishTimer timer("Generating vpr constraints from global signal assignment...");

  /*get ckbuf atom info from cell_map file */
  CkbufInfo ckbuf_info;
  if (set_up_ckbuf_info(cell_map_file, ckbuf_info) != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR(
      "Failed to get ckbuf information from cell map file! "
      "file\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* get global ports format from clock_loc_map file   */
  GlobalPortLoc global_port_loc;
  if (set_up_global_ports(clock_loc_map_file, global_port_loc) != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Failed to get global ports from clock location map file \n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* get pin_constraint_info from pcf file. This map gives the information of where the clk and
   * reset are bound */
  VTR_LOG("Reading PCF file from: %s\n", pcf_file.c_str());
  if (!std::filesystem::exists(pcf_file)) {
    VTR_LOG_ERROR("PCF file does not exist: %s\n", pcf_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  PinConstraints pcf_info = read_xml_pin_constraints(pcf_file.c_str());
  /* FIXME: This bug should be fixed in the libpcf of OpenFPGA!!!!
   * Assign default value to all the signals without a specific value */
  int global_port_num = global_port_loc.global_port_num();
  pcf_info.reserve_pin_constraints(global_port_num);

  for (auto ipcf : pcf_info.pin_constraints()) {
    std::string curr_net = pcf_info.net(ipcf);
    if (pcf_info.unconstrained_net(curr_net)) {
      continue;
    }
    if (!pcf_info.valid_net_default_value(ipcf)) {
      pcf_info.set_net_default_value(ipcf, std::string("0"));
    }
  }
  std::map<std::string, std::string> pin_constraint_info;

  int status = assign_global_signals(pcf_info, global_port_loc, ckbuf_info, true);

  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Assign global ports failed!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }

  // write final pcf
  write_xml_pin_constraints(final_pcf.c_str(), pcf_info);

  // write vpr constraint file
  write_xml_vpr_constraints_file(vpr_constr_xml_root, ckbuf_info, global_port_loc, pcf_info);
  return openfpga::CMD_EXEC_SUCCESS;
}
}  // namespace arkangel
