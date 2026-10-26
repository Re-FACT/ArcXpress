#include "arch_aggregator.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "yaml-cpp/yaml.h"

namespace arcxpress {  // Begin namespace arcxpress

ArchAggregator::ArchAggregator() {
  vars_.clear();
  vars_1toN_.clear();
  vars_remove_.clear();
}

ArchAggregator::ArchAggregator(const std::string& var_fpath, const bool& verbose) {
  load_variables(var_fpath, true, false, verbose);
}

int ArchAggregator::aggregate(const std::string& arch_fpath, const std::string& output_fpath,
                              const bool& verbose) const {
  int num_err = 0;
  /* Now read the line of arch file by apply variable replacement */
  if (!std::filesystem::exists(arch_fpath)) {
    VTR_LOG_ERROR("Architecture file '%s' does not exist!\n", arch_fpath.c_str());
    num_err++;
  }
  std::ifstream i_arch_f(arch_fpath);
  std::ofstream o_arch_f(output_fpath);
  if (!i_arch_f.is_open()) {
    VTR_LOG_ERROR("Fail to open the input architecture file '%s'!\n!", arch_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!o_arch_f.is_open()) {
    VTR_LOG_ERROR("Fail to open the output architecture file '%s'!\n", output_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Converting arch file '%s' to '%s'\n", arch_fpath.c_str(),
           output_fpath.c_str());
  size_t line_cnt = 0;
  size_t match_cnt = 0;
  std::string line;
  while (std::getline(i_arch_f, line)) {
    std::string o_line = line;
    /* 1-to-N replacement has the highest priority. Once a match is found, the line will be
     * outputted multiple time */
    bool found_1toN = false;
    for (auto kw : vars_1toN_) {
      std::string curr_var = to_var_format(kw.first);
      size_t start_pos = o_line.find(curr_var);
      if (start_pos == std::string::npos) {
        continue;
      }
      for (std::string val : kw.second) {
        std::string temp_line = line;
        temp_line.replace(start_pos, curr_var.length(), val);
        o_arch_f << temp_line << std::endl;
      }
      found_1toN = true;
      break;
    }
    if (!found_1toN) {
      bool found_var = false;
      for (auto kw : vars_) {
        std::string curr_var = to_var_format(kw.first);
        size_t start_pos = o_line.find(curr_var);
        while (start_pos != std::string::npos) {
          found_var = true;
          o_line.replace(start_pos, curr_var.length(), kw.second);
          match_cnt++;
          /* Try next */
          start_pos = o_line.find(curr_var);
        }
      }
      if (found_var) {
        o_arch_f << o_line << std::endl;
        continue;
      }
      /* Last check before output: if any pattern in the removal var list, we should not output */
      bool line_to_remove = false;
      for (auto kw : vars_remove_) {
        std::string curr_var = to_var_format(kw);
        size_t start_pos = o_line.find(curr_var);
        if (start_pos != std::string::npos) {
          line_to_remove = true;
          break;
        }
      }
      for (auto kw : lines_remove_) {
        std::string curr_var = kw;
        size_t start_pos = o_line.find(curr_var);
        if (start_pos != std::string::npos) {
          line_to_remove = true;
          break;
        }
      }
      if (line_to_remove) {
        continue;
      }
      o_arch_f << o_line << std::endl;
    }
    line_cnt++;
  }
  VTR_LOGV(verbose, "Processed %lu lines with %lu matches\n", line_cnt, match_cnt);
  i_arch_f.close();
  o_arch_f.close();
  VTR_LOGV(verbose, "Done\n");
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

void ArchAggregator::add_variable(const std::string& name, const std::string& val) {
  vars_[name] = val;
}

void ArchAggregator::add_1toN_variable(const std::string& name, const std::string& val) {
  vars_1toN_[name].push_back(val);
}

void ArchAggregator::add_remove_variable(const std::string& name) { vars_remove_.push_back(name); }
void ArchAggregator::add_remove_line(const std::string& name) { lines_remove_.push_back(name); }

void ArchAggregator::add_variable(const std::string& name, const size_t& val) {
  vars_[name] = std::to_string(val);
}

int ArchAggregator::load_variables(const std::string& var_fpath, const bool& overwrite,
                                   const bool& show_diff, const bool& verbose) {
  int num_err = 0;
  /* A clean start */
  if (overwrite) {
    vars_.clear();
  }
  /* Read yaml */
  if (!std::filesystem::exists(var_fpath)) {
    VTR_LOG_ERROR("Variable file '%s' does not exist!\n", var_fpath.c_str());
    num_err++;
  }
  VTR_LOGV(verbose, "Reading variable file '%s' ...\n", var_fpath.c_str());
  try {
    YAML::Node config = YAML::LoadFile(var_fpath);
    /* TODO: Sanity checks, expect only MAP node */
    if (show_diff) {
      VTR_LOG("\nVariable\tBefore Annotation\tAfter Annotation\tDiff (\%)\n");
    }
    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
      std::string key = it->first.as<std::string>();
      std::string new_val = it->second.as<std::string>();
      auto result = vars_.find(key);
      if (result != vars_.end()) {
        std::string old_val = result->second;
        if (show_diff) {
          float diff = std::stof(new_val) / std::stof(old_val) - 1.;
          VTR_LOG("%s\t%s\t%s\t%.2f\%\n", key.c_str(), old_val.c_str(), new_val.c_str(),
                  diff * 100.);
        }
      }
      vars_[key] = new_val;
    }
    if (show_diff) {
      VTR_LOG("\n");
    }
    VTR_LOGV(verbose, "Found %ld variables\n", vars_.size());
    VTR_LOGV(verbose, "Done\n");
  } catch (const YAML::NonUniqueMapKey& e) {
    VTR_LOG_ERROR("%s at LINE%lu\n", e.what(), e.mark.line + 1);
    num_err++;
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int ArchAggregator::write_variables(const std::string& var_fpath, const bool& verbose) const {
  int num_err = 0;
  YAML::Node config;
  for (auto pair : vars_) {
    config[pair.first] = pair.second;
  }
  std::filesystem::path fpath = var_fpath;
  openfpga::create_directory(fpath.parent_path().string(), true, verbose);
  try {
    std::ofstream fout(fpath.string());
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "arch timing variables are written to '%s'\n", fpath.c_str());
    } else {
      VTR_LOG_ERROR("Error opening file '%s'\n", fpath.c_str());
      num_err++;
    }
  } catch (const YAML::Exception& e) {
    VTR_LOG_ERROR("YAML Exception: %s\n", e.what());
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

void ArchAggregator::clear() {
  vars_.clear();
  vars_1toN_.clear();
  vars_remove_.clear();
}

std::string ArchAggregator::to_var_format(const std::string& name) const {
  return std::string("${") + name + std::string("}");
}

}  // End namespace arcxpress
