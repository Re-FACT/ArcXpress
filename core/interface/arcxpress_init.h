#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arcxpress_context.h"
/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

int arcxpress_init(ArcxpressContext& arcxpress_ctx, const std::string& root_path,
                   const std::string& boot_cfg_file);
} /* end namespace arcxpress */
