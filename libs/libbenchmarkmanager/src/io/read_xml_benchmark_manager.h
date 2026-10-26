#ifndef READ_XML_BENCHMARK_MANAGER_H
#define READ_XML_BENCHMARK_MANAGER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "benchmark_manager.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"

/********************************************************************
 * Function declaration
 *******************************************************************/

namespace arcxpress {  // Begin namespace arcxpress

int read_xml_benchmark_manager(const char* fname, BenchmarkManager& bm_mgr, const bool& overwrite);

}  // End of namespace arcxpress

#endif
