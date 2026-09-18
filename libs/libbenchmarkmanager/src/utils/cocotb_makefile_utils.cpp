/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
#include "cocotb_makefile_utils.h"

#include <fstream>
#include <iostream>
#include <regex>

#include "command_exit_codes.h"
#include "openfpga_tokenizer.h"
#include "vtr_log.h"

/* begin namespace arkangel */
namespace arkangel {

/********************************************************************
 * Extract the cell sim files defined in the cocotb makefile for a given benchmark
 * For example, the cell sim file may be shown as
 *   VERILOG_SOURCES += ${CELL_LIB_HOME}/cell_sim_pcnt.v
 * The extracted file name will be cell_sim_pcnt.v
 *******************************************************************/
std::vector<std::string> extract_benchmark_cocotb_makefile_cell_sim_files(
  const BenchmarkManager& bm_mgr, const BenchmarkId& bm_id, const bool& verbose) {
  std::vector<std::string> fnames;
  std::string mk_fname = bm_mgr.cocotb_makefile(bm_id);

  std::ifstream file(mk_fname);
  std::string line;
  size_t line_cnt = 0;

  std::string cell_lib_kw = "CELL_LIB_HOME";
  std::string vlog_src_kw = "VERILOG_SOURCES";
  std::string delim = "/";
  if (file.is_open()) {
    while (std::getline(file, line)) {
      /* FIXME: This is not a good solution to trim the end-of-line at head and back */
      std::string proc_line = line;
      proc_line.erase(std::remove(proc_line.begin(), proc_line.end(), '\n'), proc_line.cend());
      proc_line.erase(std::remove(proc_line.begin(), proc_line.end(), '\r'), proc_line.cend());
      std::regex patternA(vlog_src_kw, std::regex_constants::icase);
      std::regex patternB(cell_lib_kw, std::regex_constants::icase);
      if (std::regex_search(proc_line, patternA) && std::regex_search(proc_line, patternB)) {
        openfpga::StringToken tokenizer(proc_line);
        std::vector<std::string> tokens = tokenizer.split(delim);
        if (tokens.size() != 2) {
          VTR_LOG("Invalid '%s' definition for cocotb makefile '%s'!\n%s\nExpect only 1 '%s'\n",
                  cell_lib_kw.c_str(), mk_fname.c_str(), proc_line.c_str(), delim.c_str());
          exit(openfpga::CMD_EXEC_FATAL_ERROR);
        }
        fnames.push_back(tokens[1]);
      }
      line_cnt++;
    }
    file.close();
    VTR_LOGV(verbose, "Parsed %lu lines of cocotb makefile '%s'\n", line_cnt, mk_fname.c_str());
  } else {
    VTR_LOG_ERROR("Unable to open cocotb makefile '%s'\n", mk_fname.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }

  return fnames;
}

}  // namespace arkangel
