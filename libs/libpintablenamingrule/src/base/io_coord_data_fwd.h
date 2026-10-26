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

namespace arcxpress {  // Begin namespace arcxpress

struct io_borderline_id_tag;

typedef vtr::StrongId<io_borderline_id_tag> IoBorderlineId;

/* Short declaration of class */
class IoCoordData;

}  // End of namespace arcxpress
