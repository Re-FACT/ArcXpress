#ifndef WRITE_XML_BENCHMARK_MANAGER_H
#define WRITE_XML_BENCHMARK_MANAGER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "benchmark_manager.h"

/********************************************************************
 * Function declaration
 *******************************************************************/
namespace arkangel {  // Begin namespace arkangel

int write_xml_benchmark_manager(const char* fname, const BenchmarkManager& bm_mgr);

}  // End of namespace arkangel

#endif
