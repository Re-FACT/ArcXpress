/********************************************************************
 * This file includes functions to create/load/save projects in arkangel
 *******************************************************************/
#include "arkangel_init.h"

#include <filesystem>

#include "arkangel_constants.h"
#include "arkangel_file_os.h"
#include "arkangel_version.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "openfpga_tokenizer.h"
#include "read_xml_boot_config.h"
#include "vtr_log.h"
#include "vtr_time.h"

/* begin namespace arcxpress */
namespace arcxpress {

/********************************************************************
 * Initialize arkangle data
 * - Device-to-Model mapping
 *******************************************************************/

#ifdef ARKANGEL_LICENSED_BUILD
#include "license_manager.h"
bool init_license_manager(arcxpress::LicenseManager& license_mgr, const std::string& version) {
  const char* lic_path = std::getenv("ARKANGEL_LICENSE_PATH");
  if (lic_path == nullptr) {
    VTR_LOG_ERROR("License: ARKANGEL_LICENSE_PATH not defined\n");
    return false;
  }
  std::filesystem::path lic_fpath = std::string(lic_path);
  std::filesystem::path norm_lic_fpath = lic_fpath.lexically_normal();
  VTR_LOG("Checking out license: %s\n", norm_lic_fpath.string().c_str());
  if (!license_mgr.init(norm_lic_fpath.string(), version)) {
    VTR_LOG_ERROR("License check failed\n");
    return false;
  }
  license_mgr.printLicenseInfo();
  VTR_LOG("License check passd\n");
  return true;
}
#endif

int arkangel_init(ArkangelContext& arkangel_ctx, const std::string& root_path,
                  const std::string& boot_cfg_file) {
  vtr::ScopedStartFinishTimer timer("Initializing...");
  std::string build_version = std::string(LICENSED_BUILD_VERSION);

#ifdef ARKANGEL_LICENSED_BUILD
  if (init_license_manager(arkangel_ctx.mutable_license_manager(), build_version) == false) {
    VTR_LOG_ERROR("License initialization failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (arkangel_ctx.license_manager().verifyBuildVersion(build_version) == false) {
    VTR_LOG_ERROR("License was generated for a different version. Expected version: %s\n",
                  build_version.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  if (!arkangel_ctx.license_manager().checkOut(
        arcxpress::LicenseManager::LicensedFeatureName::AAEE)) {
    VTR_LOG_ERROR("License checkout failed\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif

  /* Load Boot configuration */
  VTR_LOG("Booting...\n");
  int status =
    arcxpress::read_xml_boot_config(boot_cfg_file.c_str(), arkangel_ctx.mutable_boot_config());
  if (openfpga::CMD_EXEC_SUCCESS != status) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Validation */
  status = arkangel_ctx.mutable_boot_config().validate();
  if (!status) {
    VTR_LOG("Invalid booting configuration! Abort\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Initialize tapeout manager */
  arkangel_ctx.mutable_tapeout_manager().load_boot_config(arkangel_ctx.boot_config());
  VTR_LOG("Done\n");

  /* Initialize tape-out related data structure */
  arkangel_ctx.mutable_tapeout_manager().set_kw_root_path(root_path);
  arkangel_ctx.mutable_benchmark_manager().set_kw_root_path(root_path);
  arkangel_ctx.mutable_pdk_manager().set_kw_root_path(root_path);

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // namespace arcxpress
