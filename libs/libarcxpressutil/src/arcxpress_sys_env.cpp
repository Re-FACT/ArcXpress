/********************************************************************
 * This file includes the functions to do file/directory operation
 *******************************************************************/
#include "arcxpress_sys_env.h"

#include <cstdlib>

#include "arcxpress_file_os.h"
#include "command_exit_codes.h"
#include "vtr_log.h"

namespace arcxpress {

int set_system_env_var(const std::string& var, const std::string& val, const bool& append,
                       const bool& overwrite) {
  std::string new_val = val;
  if (append) {
    const char* curr_val = std::getenv(var.c_str());
    if (curr_val != nullptr) {
      new_val += std::string(":") + std::string(curr_val);
    }
  }
  int orw = overwrite ? 1 : 0;
  if (universal_setenv(var.c_str(), new_val.c_str(), orw) == 0) {
    VTR_LOG("Set up system environment on variable '%s'\n", var.c_str());
    return openfpga::CMD_EXEC_SUCCESS;
  }
  VTR_LOG_ERROR(
    "Failed to set up system environment to \n\t%s=%s\nPlease double check your system!\n",
    var.c_str(), new_val.c_str());
  return openfpga::CMD_EXEC_FATAL_ERROR;
}

}  // namespace arcxpress
