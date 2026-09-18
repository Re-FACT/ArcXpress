/************************************************************************
 * A header file for BenchmarkManager class, including critical data declaration
 * Please include this file only for using any BenchmarkManager data structure
 * Refer to benchmark_mananger.h for more details
 ***********************************************************************/

/************************************************************************
 * Create strong id for BenchmarkManager to avoid illegal type casting
 ***********************************************************************/
#pragma once

#include "vtr_strong_id.h"

namespace arkangel {  // Begin namespace arkangel

struct ip_file_id_tag;
struct qorso_task_id_tag;

typedef vtr::StrongId<ip_file_id_tag> IpFileId;
typedef vtr::StrongId<qorso_task_id_tag> QorsoTaskId;

/* Short declaration of class */
class TapeoutManager;

}  // End of namespace arkangel
