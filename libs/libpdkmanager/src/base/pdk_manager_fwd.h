/************************************************************************
 * A header file for BenchmarkManager class, including critical data declaration
 * Please include this file only for using any BenchmarkManager data structure
 * Refer to benchmark_mananger.h for more details
 ***********************************************************************/

/************************************************************************
 * Create strong id for BenchmarkManager to avoid illegal type casting
 ***********************************************************************/
#ifndef PDK_MANAGER_FWD_H
#define PDK_MANAGER_FWD_H

#include "vtr_strong_id.h"

namespace arkangel {  // Begin namespace arkangel

struct sclib_id_tag;
struct sclib_file_id_tag;
struct techfile_id_tag;

typedef vtr::StrongId<sclib_id_tag> SclibId;
typedef vtr::StrongId<sclib_file_id_tag> SclibFileId;
typedef vtr::StrongId<techfile_id_tag> TechfileId;

/* Short declaration of class */
class PdkManager;

}  // End of namespace arkangel

#endif
