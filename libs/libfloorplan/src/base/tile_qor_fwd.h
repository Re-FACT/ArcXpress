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

struct sig_distr_ntwk_buf_id_tag;

typedef vtr::StrongId<sig_distr_ntwk_buf_id_tag> SigDistrNtwkBufId;

/* Short declaration of class */
class TileQor;

}  // End of namespace arcxpress
