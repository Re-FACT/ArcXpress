#pragma once

#include <map>
#include <string>
#include <vector>

#include "benchmark_manager.h"

/* namespace arkangel begins */
namespace arkangel {

/**
 * A packet means the necessary files for a benchmark
 * - Verilog files for synthesis inputs
 * - BLIF file as post-synthesis inputs
 * - Design constraints, such as pcf and sdc
 * - Cocotb testbenches
 */
class BenchmarkPacket {
 public: /* -- Accessors -- */
  std::vector<std::string> design_sources() const;
  std::string post_synth_netlist() const;
  std::string sdc() const;
  std::string pcf() const;
  std::string pcf_gclk() const;
  /* Check if there are any buses */
  bool empty() const;

 public: /* -- Mutators -- */
  void add_file(const BenchmarkManager::e_file_type& f_type, const std::string& fname);

 public: /* -- Validators -- */
  bool valid_for_post_synth() const;

 private: /* -- Internal Accessors -- */
  std::vector<std::string> files_by_type(const BenchmarkManager::e_file_type& f_type) const;

 private: /* -- Internal data -- */
  /* Basic information */
  std::vector<std::string> files_; /* From which file the benchmarks are loaded from */
  std::vector<BenchmarkManager::e_file_type>
    file_types_; /* From which file the benchmarks are loaded from */
};

}  // namespace arkangel
