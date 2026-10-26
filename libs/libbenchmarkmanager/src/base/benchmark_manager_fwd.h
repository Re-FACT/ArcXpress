/************************************************************************
 * A header file for BenchmarkManager class, including critical data declaration
 * Please include this file only for using any BenchmarkManager data structure
 * Refer to benchmark_mananger.h for more details
 ***********************************************************************/

/************************************************************************
 * Create strong id for BenchmarkManager to avoid illegal type casting
 ***********************************************************************/
#ifndef BENCHMARK_MANAGER_FWD_H
#define BENCHMARK_MANAGER_FWD_H

#include "vtr_strong_id.h"

namespace arcxpress {  // Begin namespace arcxpress

struct benchmark_suite_id_tag;
struct benchmark_id_tag;
struct benchmark_design_source_id_tag;
struct benchmark_cocotb_source_id_tag;
struct benchmark_constraint_id_tag;

typedef vtr::StrongId<benchmark_suite_id_tag> BenchmarkSuiteId;
typedef vtr::StrongId<benchmark_id_tag> BenchmarkId;
typedef vtr::StrongId<benchmark_design_source_id_tag> BenchmarkDesignSourceId;
typedef vtr::StrongId<benchmark_constraint_id_tag> BenchmarkConstraintId;
typedef vtr::StrongId<benchmark_cocotb_source_id_tag> BenchmarkCocotbSourceId;

/* Short declaration of class */
class BenchmarkManager;

}  // End of namespace arcxpress

#endif
