/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <string>

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

/* Headers from pin constraint library */
#include "benchmark_manager_xml_constants.h"
#include "pugixml.hpp"
#include "write_xml_benchmark_manager.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * Only the time stamp of the first file that is read in will be outputted!
 * TODO: Consider to aggregate all the authors. Use the earliest created date.
 * Use the latest modified date.
 *******************************************************************/
static int write_xml_benchmark_stamps(pugi::xml_node& root_node, const BenchmarkManager& bm_mgr) {
  int err_code = 0;
  pugi::xml_node stmp_node = root_node.append_child(XML_SUITE_STAMP_ROOT_NODE_NAME);

  pugi::xml_node author_node = stmp_node.append_child(XML_SUITE_AUTHOR_NODE_NAME);
  author_node.text().set(bm_mgr.authors()[0].c_str());

  pugi::xml_node dates_node = stmp_node.append_child(XML_SUITE_DATE_NODE_NAME);
  pugi::xml_node created_node = dates_node.append_child(XML_SUITE_CREATE_DATE_NODE_NAME);
  created_node.text().set(bm_mgr.date_created()[0].c_str());

  pugi::xml_node modified_node = dates_node.append_child(XML_SUITE_LAST_MODIFIED_DATE_NODE_NAME);
  modified_node.text().set(bm_mgr.date_last_modified()[0].c_str());

  pugi::xml_node ver_node = stmp_node.append_child(XML_SUITE_TOOL_VERSION_NODE_NAME);
  ver_node.text().set(bm_mgr.tool_version()[0].c_str());

  return err_code;
}

int write_xml_benchmark_info(pugi::xml_node& root_node, const BenchmarkManager& bm_mgr,
                             const BenchmarkSuiteId& st_id, const BenchmarkId& bm_id) {
  int err_code = 0;
  pugi::xml_node bm_node = root_node.append_child(XML_SUITE_BENCHMARK_ROOT_NODE_NAME);
  bm_node.append_attribute(XML_SUITE_BENCHMARK_ATTRIBUTE_NAME) =
    bm_mgr.benchmark_name(bm_id).c_str();
  bm_node.append_attribute(XML_SUITE_BENCHMARK_ATTRIBUTE_SUITE) = bm_mgr.suite_name(st_id).c_str();

  /* top module */
  pugi::xml_node tm_node = bm_node.append_child(XML_SUITE_BENCHMARK_TOPMODULE_NODE_NAME);
  tm_node.text().set(bm_mgr.top_module(bm_id).c_str());

  /* Design sources */
  pugi::xml_node dsgn_node = bm_node.append_child(XML_SUITE_BENCHMARK_DESIGN_NODE_NAME);
  for (BenchmarkDesignSourceId f_id : bm_mgr.design_sources(bm_id)) {
    pugi::xml_node f_node = dsgn_node.append_child(XML_SUITE_BENCHMARK_FILE_NODE_NAME);
    f_node.append_attribute(XML_SUITE_BENCHMARK_FILE_ATTRIBUTE_TYPE) =
      bm_mgr.design_source_type_str(bm_id, f_id).c_str();
    f_node.text().set(bm_mgr.design_source_file(bm_id, f_id).c_str());
  }

  /* Cocotb sources */
  pugi::xml_node cctb_node = bm_node.append_child(XML_SUITE_BENCHMARK_COCOTB_NODE_NAME);
  for (BenchmarkCocotbSourceId f_id : bm_mgr.cocotb_sources(bm_id)) {
    pugi::xml_node f_node = cctb_node.append_child(XML_SUITE_BENCHMARK_FILE_NODE_NAME);
    f_node.append_attribute(XML_SUITE_BENCHMARK_FILE_ATTRIBUTE_TYPE) =
      bm_mgr.cocotb_type_str(bm_id, f_id).c_str();
    f_node.text().set(bm_mgr.cocotb_file(bm_id, f_id).c_str());
  }

  /* Constraints */
  pugi::xml_node cnstr_node = bm_node.append_child(XML_SUITE_BENCHMARK_CONSTRAINTS_NODE_NAME);
  for (BenchmarkConstraintId f_id : bm_mgr.constraints(bm_id)) {
    pugi::xml_node f_node = cnstr_node.append_child(XML_SUITE_BENCHMARK_FILE_NODE_NAME);
    f_node.append_attribute(XML_SUITE_BENCHMARK_FILE_ATTRIBUTE_TYPE) =
      bm_mgr.constraint_type_str(bm_id, f_id).c_str();
    f_node.text().set(bm_mgr.constraint_file(bm_id, f_id).c_str());
  }

  return err_code;
}

/********************************************************************
 * A writer to output a strategy manager object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_benchmark_manager(const char* fname, const BenchmarkManager& bm_mgr) {
  vtr::ScopedStartFinishTimer timer("Write benchmark data to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_SUITE_ROOT_NODE_NAME);

  /* Output time stamp */
  err_code += write_xml_benchmark_stamps(root_node, bm_mgr);

  /* Output benchmarks suite by suite */
  for (BenchmarkSuiteId st_id : bm_mgr.suites()) {
    for (BenchmarkId bm_id : bm_mgr.benchmarks(st_id)) {
      err_code += write_xml_benchmark_info(root_node, bm_mgr, st_id, bm_id);
    }
  }

  out_xml.save_file(fname);

  return err_code;
}

}  // End of namespace arcxpress
