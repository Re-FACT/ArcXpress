#include "boot_config.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

BootConfig::BootConfig() {
  is_valid_ = false;
  BOOT_TYPE_STRING_ = {"build", "install"};
  BIN_TYPE_STRING_ = {"openfpga",
                      "fabric_key_assistant",
                      "pin_table_generator",
                      "netlist_makeup",
                      "ccff_makeup",
                      "netlist_synthesis",
                      "frontend",
                      "frontend_cell_lib",
                      "qorso",
                      "bus_group_generator",
                      "cocotb_util",
                      "cocotb_application_sim_util",
                      "cocotb_force_bitstream",
                      "benchmark_simulation_cocotb_makefile_inc",
                      "cocotb_makefile_inc",
                      "python",
                      "python_site_packages",
                      "python_cocotb_config",
                      "libpython_loc"};
}

std::string BootConfig::author() const {
  check_valid();
  return author_;
}

std::string BootConfig::date_created() const {
  check_valid();
  return date_created_;
}

std::string BootConfig::date_last_modified() const {
  check_valid();
  return date_last_modified_;
}

std::string BootConfig::tool_version() const {
  check_valid();
  return tool_version_;
}

BootConfig::e_boot_type BootConfig::boot_type() const {
  check_valid();
  return boot_type_;
}

std::string BootConfig::bin_relpath(const BootConfig::e_bin_type& type) const {
  check_valid();
  check_valid_bin_type(type);
  return bin_relpaths_[size_t(type)];
}

void BootConfig::set_author(const std::string& val) { author_ = val; }

void BootConfig::set_date_created(const std::string& val) { date_created_ = val; }

void BootConfig::set_date_last_modified(const std::string& val) { date_last_modified_ = val; }

void BootConfig::set_tool_version(const std::string& val) { tool_version_ = val; }

void BootConfig::set_boot_type(const std::string& type_str) {
  boot_type_ = str2boot_type(type_str, true);
}

void BootConfig::set_bin_relpath(const std::string& type_str, const std::string& val) {
  e_bin_type type = str2bin_type(type_str, true);
  check_valid_bin_type(type);
  bin_relpaths_[size_t(type)] = val;
}

bool BootConfig::validate() const {
  is_valid_ = false;
  /* Must have at least 1 time stamp */
  size_t num_err = 0;
  for (size_t itype = size_t(BootConfig::e_bin_type::OPENFPGA);
       itype < size_t(BootConfig::e_bin_type::NUM_TYPES); itype++) {
    if (bin_relpaths_[itype].empty()) {
      VTR_LOG_ERROR("Binary executable path of '%s' is not valid!\n", BIN_TYPE_STRING_[itype]);
      num_err++;
    }
  }
  if (!valid_boot_type(boot_type_)) {
    VTR_LOG_ERROR("Boot type is not valid!");
    num_err++;
  }
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err == 0;
}

void BootConfig::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Booting configuration data is invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void BootConfig::check_valid_bin_type(const BootConfig::e_bin_type& type) const {
  if (!valid_bin_type(type)) {
    VTR_LOG_ERROR("Bin type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

BootConfig::e_bin_type BootConfig::str2bin_type(const std::string& str, const bool& verbose) const {
  for (int itype = size_t(BootConfig::e_bin_type::OPENFPGA);
       itype < size_t(BootConfig::e_bin_type::NUM_TYPES); ++itype) {
    if (str == std::string(BIN_TYPE_STRING_[itype])) {
      return static_cast<BootConfig::e_bin_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for binary file relative path! Expect %s\n",
                 bin_type_all2str().c_str());
  return BootConfig::e_bin_type::NUM_TYPES;
}

std::string BootConfig::bin_type2str(const BootConfig::e_bin_type& type,
                                     const bool& verbose) const {
  if (!valid_bin_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for binary file relative paths! Expect %s\n",
                   bin_type_all2str().c_str());
    return std::string();
  }
  return std::string(BIN_TYPE_STRING_[size_t(type)]);
}

std::string BootConfig::bin_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(BootConfig::e_bin_type::OPENFPGA);
       itype < size_t(BootConfig::e_bin_type::NUM_TYPES); ++itype) {
    full_types += std::string(BIN_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

std::vector<BootConfig::e_bin_type> BootConfig::bin_types() const {
  std::vector<e_bin_type> bin_types;
  for (int itype = size_t(BootConfig::e_bin_type::OPENFPGA);
       itype < size_t(BootConfig::e_bin_type::NUM_TYPES); ++itype) {
    bin_types.push_back(static_cast<BootConfig::e_bin_type>(itype));
  }
  return bin_types;
}

bool BootConfig::valid_bin_type(const e_bin_type& type) const {
  return type != BootConfig::e_bin_type::NUM_TYPES;
}

void BootConfig::clear() {
  author_.clear();
  date_created_.clear();
  date_last_modified_.clear();
  tool_version_.clear();
  boot_type_ = e_boot_type::NUM_TYPES;
  for (size_t itype = 0; itype < bin_relpaths_.size(); itype++) {
    bin_relpaths_[itype].clear();
  }
  is_valid_ = false;
}

void BootConfig::check_valid_boot_type(const BootConfig::e_boot_type& type) const {
  if (!valid_boot_type(type)) {
    VTR_LOG_ERROR("Boot type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

BootConfig::e_boot_type BootConfig::str2boot_type(const std::string& str,
                                                  const bool& verbose) const {
  for (int itype = size_t(BootConfig::e_boot_type::BUILD);
       itype < size_t(BootConfig::e_boot_type::NUM_TYPES); ++itype) {
    if (str == std::string(BOOT_TYPE_STRING_[itype])) {
      return static_cast<BootConfig::e_boot_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for bootary file relative path! Expect %s\n",
                 boot_type_all2str().c_str());
  return BootConfig::e_boot_type::NUM_TYPES;
}

std::string BootConfig::boot_type2str(const BootConfig::e_boot_type& type,
                                      const bool& verbose) const {
  if (!valid_boot_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for boot file relative paths! Expect %s\n",
                   boot_type_all2str().c_str());
    return std::string();
  }
  return std::string(BOOT_TYPE_STRING_[size_t(type)]);
}

std::string BootConfig::boot_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(BootConfig::e_boot_type::BUILD);
       itype < size_t(BootConfig::e_boot_type::NUM_TYPES); ++itype) {
    full_types += std::string(BOOT_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool BootConfig::valid_boot_type(const e_boot_type& type) const {
  return type != BootConfig::e_boot_type::NUM_TYPES;
}

std::string BootConfig::boot_type_str() const { return boot_type2str(boot_type_, true); }

}  // End namespace arkangel
