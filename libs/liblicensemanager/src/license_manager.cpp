#include "license_manager.h"

#include <openssl/md5.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "arcxpress_constants.h"

/* begin namespace arcxpress */
namespace arcxpress {  // namespace arcxpress begins

LicenseManager::LicenseManager() {}

LicenseManager::LicenseManager(std::string lic_path, std::string version) : LicenseManager() {
  init(lic_path, version);
}

LicenseManager::~LicenseManager() {}

bool LicenseManager::init(const std::string &lic_path, const std::string &version) {
  try {
    if (lic_path.empty()) {
      throw LicenseFatalException();
    }
    if (lic_path.size() > LCC_API_MAX_LICENSE_DATA_LENGTH) {
      throw LicenseFatalException();
    }
    if (version.size() > LCC_API_VERSION_LENGTH) {
      throw LicenseFatalException();
    }
    std::filesystem::path lic_path_fp = lic_path;
    if (!std::filesystem::exists(lic_path_fp)) {
      std::cout << "[ARKANGEL ERROR] License file does not exist or is not a valid file"
                << std::endl;
      throw LicenseFatalException();
    }
    std::filesystem::path lic_md5_path_fp = lic_path + std::string(".md5");
    if (!std::filesystem::exists(lic_md5_path_fp)) {
      std::cout << "[ARKANGEL ERROR] License md5 file is missing" << std::endl;
      throw LicenseFatalException();
    }
    if (!verifyLicFile(lic_path)) {
      throw LicenseFatalException();
    }

    // license location information
    _lic_location.license_data_type = LICENSE_PATH;
    strcpy(_lic_location.licenseData, lic_path.c_str());

    // caller information
    // note: as of 2024.09.24 lcc has not impelmented caller info check; we are using this field to
    // pass the version information to the later checkout functioin to verify by ourselves
    strcpy(_caller_info.feature_name, "AAEE");
    strcpy(_caller_info.version, version.c_str());
    // not used yet, make sure it is 0 to avoid any issues
    _caller_info.magic = 0;
    return true;
  } catch (LicenseFatalException const &ex) {
    std::cout << "[ARKANGEL ERROR] License: initialization exception captured: " << ex.what()
              << std::endl;
    return false;
  }
}

void LicenseManager::printLicenseInfo() {
  std::cout << _open_license_bsd_3_clause_info << std::endl;
  std::cout << "[ARKANGEL INFO] License: information" << std::endl;
  std::cout << "\tFile Location: " << _lic_location.licenseData << std::endl;
  LCC_EVENT_TYPE result = acquire_license(&_caller_info, &_lic_location, &_lic_info);
  std::cout << "\tArkAngel Build: " << _lic_info.proprietary_data << std::endl;
  std::cout << "\tExpire Date: " << _lic_info.expiry_date << std::endl;
  // not implemented yet
  // cout << "Lincense version: " << _lic_info.license_version << std::endl;
  std::cout << "\tLicense Expired: "
            << (_lic_info.has_expiry ? (_lic_info.days_left > 0 ? "N" : "Y") : "N") << std::endl;
  return;
}

bool LicenseManager::verifyBuildVersion(const std::string &version) {
  if (version.size() > LCC_API_VERSION_LENGTH) {
    std::cout << "[ARKANGEL ERROR] Version string too long" << std::endl;
    return false;
  }
  strcpy(_caller_info.version, version.c_str());
  LCC_EVENT_TYPE result = acquire_license(&_caller_info, &_lic_location, &_lic_info);
  // version in caller info is not used yet in licensecc, we currently checking the proprietary data
  // so we don't care the result here
  std::string licensed_version = _lic_info.proprietary_data;
  // the proprietary data is in format of VERSION-BUILDDATE, for instance: "2024R1-20240930"
  std::string::size_type pos = licensed_version.find("-");
  if (pos == std::string::npos) {
    std::cout
      << "[ARKANGEL ERROR] License: invalid license build version provided in verify build version"
      << std::endl;
    return false;
  } else {
    std::string licensed_build = licensed_version.substr(0, pos);
    if (licensed_build != version) {
      std::cout << "[ARKANGEL ERROR] License: version check failed: run-time version <" << version
                << "> does not match licensed version <" << licensed_build << ">" << std::endl;
      return false;
    } else {
      return true;
    }
  }

  // dummy return
  return false;
}

// check if the license file is valid as originally issued
std::string LicenseManager::calculateMD5(const std::string &file_name) {
  unsigned char result[MD5_DIGEST_LENGTH];
  std::ifstream file(file_name, std::ios::in | std::ios::binary | std::ios::ate);
  long fileSize = file.tellg();
  char *memBlock = new char[fileSize];
  file.seekg(0, std::ios::beg);
  file.read(memBlock, fileSize);
  file.close();
  MD5((unsigned char *)memBlock, fileSize, result);
  delete[] memBlock;
  std::string md5String;
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    md5String += ss.str();
  }
  return md5String;
}

bool LicenseManager::verifyLicFile(const std::string &lic_path) {
  std::string lic_md5_path = lic_path + ".md5";
  std::ifstream md5_file(lic_md5_path, std::ios::binary);
  // each digist result is represented by 2 characters, refer to calculateMD5 function
  int md5_digest_char_length = MD5_DIGEST_LENGTH * 2;
  char result[md5_digest_char_length];
  int bytesRead = 0;
  while ((bytesRead = md5_file.read(result, sizeof(result)).gcount()) > 0) {
    break;
  }
  std::string md5_file_content = std::string(result);

  bool signature_verified = false;
  // for some special purpose like we deliver a special build to GUI/contractor
  // in such case, we want to make sure the license file is only for this unique build only
  // also build should guarantee the signature is same to the .md5 file
  if (std::string(LICENSE_FILE_SIGNATURE).length() > 0) {
    if (0 != strncmp(md5_file_content.c_str(), LICENSE_FILE_SIGNATURE, md5_digest_char_length)) {
      signature_verified = false;
    } else {
      signature_verified = true;
    }
  } else {
    signature_verified = true;
  }

  std::string calulated_md5 = calculateMD5(lic_path);
  if (signature_verified) {
    if (0 != strncmp(md5_file_content.c_str(), calulated_md5.c_str(), md5_digest_char_length)) {
      signature_verified = false;
    }
  }

  if (!signature_verified) {
    std::cout << "[ARKANGEL ERROR] License: license file corrupted, please contact Rapid Flex to "
                 "obtain an original one"
              << std::endl;
    return false;
  } else {
    std::cout << "[ARKANGEL INFO] License: signature <" << calulated_md5 << "> checked"
              << std::endl;
    return true;
  }
}

bool LicenseManager::checkOut(const LicensedFeatureName &featureName) {
  return checkOut(licensedFeatureNameMap[featureName]);
}

bool LicenseManager::checkOut(const std::string &featureName) {
  if (featureName.size() > LCC_API_FEATURE_NAME_SIZE) {
    std::cout << "[ARKANGEL ERROR] License: checkout feature name <" << featureName << "> too long"
              << std::endl;
    return false;
  }
  strcpy(_caller_info.feature_name, featureName.c_str());
  size_t pc_id_sz = LCC_API_PC_IDENTIFIER_SIZE + 1;
  char pc_identifier[LCC_API_PC_IDENTIFIER_SIZE + 1];

  LCC_EVENT_TYPE result = acquire_license(&_caller_info, &_lic_location, &_lic_info);
  if (result == LICENSE_OK) {
    std::cout << "[ARKANGEL INFO] License: checkout feature <" << featureName << "> successfully"
              << std::endl;
    return true;
  } else {
    std::cout << "[ARKANGEL ERROR] License: checkout feature <" << featureName
              << "> failed: " << licEventDescriptionType.at(result) << std::endl;
    return false;
  }
}

}  // namespace arcxpress
