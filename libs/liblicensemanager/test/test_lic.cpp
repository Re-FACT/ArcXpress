#include <iostream>

#include "license_manager.h"

// test the LicenseManager class
int main() {
  int status = 0;
  std::cout << "======== Test License Manager Begin ========" << std::endl;
  char* lic_path = getenv("ARKANGEL_LICENSE_PATH");
  if (lic_path == nullptr) {
    std::cout << "Environment variable ARKANGEL_LICENSE_PATH not defined" << std::endl;
    return 1;
  }

  try {
    // to test valid license file make sure the version is working
    std::string version_to_check = "2025R1";
    arcxpress::LicenseManager lic_manager{std::string(lic_path), version_to_check};

    // test license info print
    lic_manager.printLicenseInfo();

    // test verify build version
    std::cout << "---------------------------------" << std::endl;
    std::cout << "- Test  License By Feature Name -" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    std::cout << ">>>> Verifying build version: " << version_to_check << " ..." << std::endl;
    if (!lic_manager.verifyBuildVersion(version_to_check)) {
      std::cout << ">>>> Failed to verify build version" << std::endl;
      status = 1;
    } else {
      std::cout << ">>>> Build version verified successfully" << std::endl;
    }

    std::vector<std::string> features = {"AAEE", "EVAL", "PROTOTYPE", "FULL"};
    for (const auto& feature : features) {
      std::cout << "---------------------------------" << std::endl;
      std::cout << ">>>> Checking out feature: " << feature << " ..." << std::endl;
      if (!lic_manager.checkOut(feature)) {
        std::cout << ">>>> Failed to checkout license" << std::endl;
        status = 1;
      } else {
        std::cout << ">>>> License checkout successfully" << std::endl;
      }
    }

    std::cout << "---------------------------------" << std::endl;
    std::cout << "- Test  License  By Feature Tag -" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    std::vector<arcxpress::LicenseManager::LicensedFeatureName> feature_tags = {
      arcxpress::LicenseManager::LicensedFeatureName::AAEE,
      arcxpress::LicenseManager::LicensedFeatureName::EVAL,
      arcxpress::LicenseManager::LicensedFeatureName::PROTOTYPE,
      arcxpress::LicenseManager::LicensedFeatureName::FULL};
    for (const auto& feature_tag : feature_tags) {
      std::cout << "---------------------------------" << std::endl;
      std::cout << ">>>> Checking out feature: " << lic_manager.licensedFeatureNameMap[feature_tag]
                << " ..." << std::endl;
      if (!lic_manager.checkOut(feature_tag)) {
        std::cout << ">>>> Failed to checkout license" << std::endl;
        status = 1;
      } else {
        std::cout << ">>>> License checkout successfully" << std::endl;
      }
    }

  } catch (arcxpress::LicenseManager::LicenseFatalException const& ex) {
    std::cout << "[LICENSE TEST] License checkout exception captured: " << ex.what() << std::endl;
    status = 1;
  }

  std::cout << "======== Test  License Manager  End ========" << std::endl;
  return status;
}
