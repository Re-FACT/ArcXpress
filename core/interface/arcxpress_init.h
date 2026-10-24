#ifndef ARCXPRESS_INIT_H
#define ARCXPRESS_INIT_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "arkangel_context.h"
#ifdef ARCXPRESS_LICENSED_BUILD
#include "license_manager.h"
#endif
/********************************************************************
 * Function declaration
 *******************************************************************/

/* begin namespace arcxpress */
namespace arcxpress {

int arkangel_init(ArkangelContext& arkangel_ctx, const std::string& root_path,
                  const std::string& boot_cfg_file);
} /* end namespace arcxpress */

#endif
