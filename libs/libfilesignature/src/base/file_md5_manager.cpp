#include "file_md5_manager.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "command_exit_codes.h"
#ifdef USE_OPENSSL3_API
#include "openssl/evp.h"
#else
#include "openssl/md5.h"
#endif
#include "vtr_assert.h"
#include "vtr_log.h"
#include "yaml-cpp/yaml.h"

namespace arkangel {  // Begin namespace arkangel

FileMd5Manager::FileMd5Manager() { clear(); }

void FileMd5Manager::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("File Md5 data is invalid!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void FileMd5Manager::clear() {
  file_paths_.clear();
  file_ref_md5_.clear();
  file_md5_.clear();
  file_md5_status_.clear();
  is_valid_ = false;
}

int FileMd5Manager::check_md5(const bool& verbose) {
  /* Must have all the reference md5 ready  */
  if (file_ref_md5_.empty() && !file_paths_.empty()) {
    VTR_LOG_ERROR("Missing reference Md5 which must be load from files before checking any Md5!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Generate md5 for all the files and compare one by one */
  int status = gen_md5(verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Check current data is valid. Deposit invalid status for the checking */
  check_valid();
  is_valid_ = false;
  int num_err = 0;
  for (size_t i = 0; i < file_md5_.size(); ++i) {
    if (file_md5_[i] != file_ref_md5_[i]) {
      VTR_LOG_ERROR("Md5 mismatches for file '%s'!\n\tReference Md5: %s\n\tCalculated Md5: %s\n",
                    file_paths_[i].c_str(), file_ref_md5_[i].c_str(), file_md5_[i].c_str());
      file_md5_status_[i] = e_status::MD5_FAIL;
      num_err++;
    } else {
      VTR_LOGV(verbose, "Md5 check passed for file '%s'\n", file_paths_[i].c_str());
      file_md5_status_[i] = e_status::MD5_PASS;
    }
  }
  if (num_err) {
    VTR_LOG_ERROR("Found %lu Md5 mismatches\n", num_err);
  } else {
    VTR_LOG("Md5 checks passed\n");
    is_valid_ = true;
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

void FileMd5Manager::add_file(const std::string& val) {
  file_paths_.push_back(val);
  file_ref_md5_.emplace_back();
  file_md5_.emplace_back();
  file_md5_status_.emplace_back(FileMd5Manager::e_status::FRESH);
  is_valid_ = true;
}

int FileMd5Manager::gen_md5(const bool& verbose) {
  /* TODO: Use taskflow or TBB */
  /* Check current data is valid. Deposit invalid status for the checking */
  check_valid();
  is_valid_ = false;
  size_t num_err = 0;
  for (size_t i = 0; i < file_paths_.size(); ++i) {
    int status = gen_md5_for_file(i, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Failed to generate Md5 for file '%s'!\n", file_paths_[i].c_str());
      num_err++;
    }
    file_md5_status_[i] = e_status::MD5_GENERATED;
  }
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

void FileMd5Manager::reserve(const size_t& count) {
  file_paths_.reserve(count);
  file_ref_md5_.reserve(count);
  file_md5_.reserve(count);
  file_md5_status_.reserve(count);
}

int FileMd5Manager::load_md5_from_yaml_file(const std::string& fpath, const bool& verbose) {
  int num_err = 0;
  /* Clear all the Md5 files */
  clear();
  /* Read yaml */
  if (!std::filesystem::exists(fpath)) {
    VTR_LOG_ERROR("YAML Md5 file '%s' does not exist!\n", fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Reading Md5 file '%s' ...\n", fpath.c_str());
  YAML::Node config = YAML::LoadFile(fpath);
  /* Sanity checks, expect only MAP node */
  reserve(config.size());
  for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
    file_paths_.push_back(it->first.as<std::string>());
    file_ref_md5_.push_back(it->second.as<std::string>());
    file_md5_.emplace_back();
    file_md5_status_.push_back(e_status::READY_FOR_CHECK);
  }
  VTR_LOGV(verbose, "Found %ld files with md5 in the list\n", file_paths_.size());
  VTR_LOGV(verbose, "Done\n");
  VTR_LOGV(verbose, "Validating file paths\n");
  /* Check each file exists */
  for (std::string curr_fpath : file_paths_) {
    if (!std::filesystem::exists(curr_fpath)) {
      VTR_LOG_ERROR("File '%s' does not exist!\n", curr_fpath.c_str());
      num_err++;
    }
  }
  VTR_LOGV(verbose, "Done\n");
  if (num_err == 0) {
    is_valid_ = true;
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

int FileMd5Manager::write_md5_to_yaml_file(const std::string& fpath, const bool& verbose) const {
  int num_err = 0;
  check_valid();
  YAML::Node config;
  for (size_t i = 0; i < file_paths_.size(); ++i) {
    config[file_paths_[i]] = file_md5_[i];
  }
  try {
    std::ofstream fout(fpath);
    if (fout.is_open()) {
      fout << config;
      fout.close();
      VTR_LOGV(verbose, "File Md5 data written to '%s'\n", fpath.c_str());
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

int FileMd5Manager::gen_md5_for_file(const size_t& f_idx, const bool& verbose) {
  /* TODO: Valid file index */
  std::string fpath = file_paths_[f_idx];
  std::ifstream file(fpath, std::ios::in | std::ios::binary | std::ios::ate);

  if (!std::filesystem::exists(fpath)) {
    VTR_LOG_ERROR("File does not exist at location: %s!\n", fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  if (!file.is_open()) {
    VTR_LOG_ERROR("Cannot open file '%s'!\n", fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  // Get file size
  VTR_LOGV(verbose, "Generating Md5 for file '%s'\n", fpath.c_str());
  long file_size = file.tellg();
  VTR_LOGV(verbose, "File size: %lu bytes\n", file_size);

  std::string line;
  size_t line_cnt;

#ifdef USE_OPENSSL3_API
  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  const EVP_MD* md = EVP_md5();
  EVP_DigestInit_ex2(mdctx, md, nullptr);
  while (std::getline(file, line)) {
    VTR_LOGV(verbose, "Reading line %lu\n", line_cnt);
    EVP_DigestUpdate(mdctx, line.c_str(), line.length());
    line_cnt++;
  }
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;
  EVP_DigestFinal_ex(mdctx, md_value, &md_len);
  EVP_MD_CTX_free(mdctx);
#else
  MD5_CTX md5Context;
  MD5_Init(&md5Context);
  while (std::getline(file, line)) {
    VTR_LOGV(verbose, "Reading line %lu\n", line_cnt);
    MD5_Update(&md5Context, line.c_str(), file.gcount());
    line_cnt++;
  }
  unsigned int md_len = MD5_DIGEST_LENGTH;
  unsigned char md_value[md_len];
  MD5_Final(md_value, &md5Context);
#endif

  std::stringstream ss;
  VTR_LOGV(verbose, "Md5 before filliing: %s\n", md_value);
  for (unsigned int i = 0; i < md_len; ++i) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)md_value[i];
  }
  VTR_LOGV(verbose, "Md5 after filliing: %s\n", ss.str().c_str());

  file_md5_[f_idx] = ss.str();
  VTR_LOGV(verbose, "Done\n");

  file.close();

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arkangel
