#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>

namespace arkangel {

/********************************************************************
 * Function declaration
 *******************************************************************/
// Set an system env
int set_system_env_var(const std::string& var, const std::string& val, const bool& append,
                       const bool& overwrite = true);

}  // namespace arkangel
