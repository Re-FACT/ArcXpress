#ifndef ARCXPRESS_LICENSE_MANAGER_H
#define ARCXPRESS_LICENSE_MANAGER_H

#include <string.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "./licensecc/include/licensecc/licensecc.h"
#if defined(_WIN32) || defined(_WIN64)
#undef IN
#undef ABSOLUTE
#endif

/* namespace arcxpress begins */
namespace arcxpress {

class LicenseManager {
 public:
  enum class LicensedFeatureName { AAEE, EVAL, PROTOTYPE, FULL };

  // feature name's string length should be less than "LCC_API_FEATURE_NAME_SIZE"
  std::map<LicenseManager::LicensedFeatureName, std::string> licensedFeatureNameMap = {
    {LicenseManager::LicensedFeatureName::AAEE, "AAEE"},
    {LicenseManager::LicensedFeatureName::EVAL, "EVAL"},
    {LicenseManager::LicensedFeatureName::PROTOTYPE, "PROTOTYPE"},
    {LicenseManager::LicensedFeatureName::FULL, "FULL"}};

  // note: make sure this is in sync with the LCC_EVENT_TYPE enum in licensecc/datatypes.h
  const std::unordered_map<int, std::string> licEventDescriptionType = {
    {LICENSE_OK, "OK "},
    {LICENSE_FILE_NOT_FOUND, "License file not found"},
    {LICENSE_SERVER_NOT_FOUND, "License server not reachable"},
    {ENVIRONMENT_VARIABLE_NOT_DEFINED, "License file environment variable not defined"},
    {FILE_FORMAT_NOT_RECOGNIZED, "License file not recognized"},
    {LICENSE_MALFORMED, "License file information not readable. "},
    {PRODUCT_NOT_LICENSED, "Featrue is not licensed"},
    {PRODUCT_EXPIRED, "License expired"},
    {LICENSE_CORRUPTED, "Signature in provided license file doesn't match with current software"},
    {IDENTIFIERS_MISMATCH,
     "Identifier in provided license file doesn't match the calculated identifier in current "
     "software"}};

  LicenseManager(std::string licensedPathName, std::string version);

  bool init(const std::string &lic_path, const std::string &version);
  bool checkOut(const std::string &featureName);
  bool checkOut(const LicensedFeatureName &featureName);
  void printLicenseInfo();
  bool verifyBuildVersion(const std::string &version);
  std::string calculateMD5(const std::string &);
  bool verifyLicFile(const std::string &);

  struct LicenseFatalException : public std::exception {
    const char *what() const throw() { return "License was not acquired due to a fatal error"; }
  };

  struct LicenseCorrectableException : public std::exception {
    const char *what() const throw() {
      return "License was not acquired due to a correctable error";
    }
  };

  std::map<std::string, LicensedFeatureName> licensedFeatureNameEnumMap = {
    {"AAEE", LicenseManager::LicensedFeatureName::AAEE},
    {"EVAL", LicenseManager::LicensedFeatureName::EVAL},
    {"PROTOTYPE", LicenseManager::LicensedFeatureName::PROTOTYPE},
    {"FULL", LicenseManager::LicensedFeatureName::FULL}};

  ~LicenseManager();
  LicenseManager();

 private:
  LicenseLocation _lic_location;
  CallerInformations _caller_info;
  LicenseInfo _lic_info;
  // below copyright info and disclaimer are required by open-license-manager
  std::string _open_license_bsd_3_clause_info =
    "\n********************************************************************************\nBSD "
    "3-Clause License\n\nCopyright (c) 2020, open-license-manager\nAll rights "
    "reserved.\n\nRedistribution and use in source and binary forms, with or "
    "without\nmodification, are permitted provided that the following conditions are met:\n\n1. "
    "Redistributions of source code must retain the above copyright notice, this\n   list of "
    "conditions and the following disclaimer.\n\n2. Redistributions in binary form must reproduce "
    "the above copyright notice,\n   this list of conditions and the following disclaimer in the "
    "documentation\n   and/or other materials provided with the distribution.\n\n3. Neither the "
    "name of the copyright holder nor the names of its\n   contributors may be used to endorse or "
    "promote products derived from\n   this software without specific prior written "
    "permission.\n\nTHIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS "
    "IS\"\nAND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\nIMPLIED "
    "WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\nDISCLAIMED. IN NO "
    "EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\nFOR ANY DIRECT, INDIRECT, "
    "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\nDAMAGES (INCLUDING, BUT NOT LIMITED TO, "
    "PROCUREMENT OF SUBSTITUTE GOODS OR\nSERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS "
    "INTERRUPTION) HOWEVER\nCAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT "
    "LIABILITY,\nOR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\nOF "
    "THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH "
    "DAMAGE.\n********************************************************************************\n";
};

}  // namespace arcxpress

#endif
