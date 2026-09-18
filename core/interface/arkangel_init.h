#ifndef ARKANGEL_INIT_H
#define ARKANGEL_INIT_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#ifdef ARKANGEL_LICENSED_BUILD
#include "license_manager.h"
#endif
/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arkangel */
namespace arkangel {

int arkangel_init(ArkangelContext& arkangel_ctx, const std::string& root_path,
                  const std::string& boot_cfg_file);
} /* end namespace arkangel */

#endif
