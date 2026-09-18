#include "benchmark_packet.h"

#include <filesystem>

#include "command_exit_codes.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

std::vector<std::string> BenchmarkPacket::files_by_type(
  const BenchmarkManager::e_file_type& f_type) const {
  std::vector<std::string> ret;
  for (size_t ifile = 0; ifile < files_.size(); ++ifile) {
    if (file_types_[ifile] == f_type) {
      ret.push_back(files_[ifile]);
    }
  }
  return ret;
}

std::vector<std::string> BenchmarkPacket::design_sources() const {
  return files_by_type(BenchmarkManager::e_file_type::VERILOG);
}

std::string BenchmarkPacket::post_synth_netlist() const {
  std::vector<std::string> ret = files_by_type(BenchmarkManager::e_file_type::BLIF);
  if (ret.empty()) {
    return std::string();
  }
  if (ret.size() > 1) {
    VTR_LOG("Multiple post-synthesis netlists are found! Expect only 1\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret[0];
}

std::string BenchmarkPacket::sdc() const {
  std::vector<std::string> ret = files_by_type(BenchmarkManager::e_file_type::SDC);
  if (ret.empty()) {
    return std::string();
  }
  if (ret.size() > 1) {
    VTR_LOG("Multiple SDCs are found! Expect only 1\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret[0];
}

std::string BenchmarkPacket::pcf() const {
  std::vector<std::string> ret = files_by_type(BenchmarkManager::e_file_type::PCF);
  if (ret.empty()) {
    return std::string();
  }
  if (ret.size() > 1) {
    VTR_LOG("Multiple PCFs are found! Expect only 1\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret[0];
}

std::string BenchmarkPacket::pcf_gclk() const {
  std::vector<std::string> ret = files_by_type(BenchmarkManager::e_file_type::PCF_GCLK);
  if (ret.empty()) {
    return std::string();
  }
  if (ret.size() > 1) {
    VTR_LOG("Multiple PCFs are found! Expect only 1\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return ret[0];
}

bool BenchmarkPacket::empty() const { return files_.empty(); }

bool BenchmarkPacket::valid_for_post_synth() const {
  if (post_synth_netlist().empty()) {
    VTR_LOG_ERROR("Required post-synthesis netlist is not defined!\n");
    return false;
  }
  return true;
}

void BenchmarkPacket::add_file(const BenchmarkManager::e_file_type& f_type,
                               const std::string& fname) {
  files_.push_back(fname);
  file_types_.push_back(f_type);
}

}  // End namespace arkangel
