/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML of project file to the associated
 * data structures
 *******************************************************************/
#include <filesystem>
#include <string>

/* Headers from pugi XML library */
#include "pugixml.hpp"
#include "pugixml_util.hpp"

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_time.h"

/* Headers from libarchfpga */
#include "arch_error.h"
#include "benchmark_manager_xml_constants.h"
#include "command_exit_codes.h"
#include "pugixml_util.hpp"
#include "read_xml_benchmark_manager.h"
#include "read_xml_util.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * Parse XML codes of a <stamp> to an object of ProjectManager
 *******************************************************************/
static void read_xml_stamp(pugi::xml_node& xml_stamp, const pugiutil::loc_data& loc_data,
                           BenchmarkManager& bm_mgr, const std::string& fname) {
  pugi::xml_node xml_author = get_single_child(xml_stamp, XML_SUITE_AUTHOR_NODE_NAME, loc_data);
  std::string author_str = std::string(xml_author.child_value());

  pugi::xml_node xml_dates = get_single_child(xml_stamp, XML_SUITE_DATE_NODE_NAME, loc_data);
  pugi::xml_node xml_created_date =
    get_single_child(xml_dates, XML_SUITE_CREATE_DATE_NODE_NAME, loc_data);
  std::string date_created_str = std::string(xml_created_date.child_value());

  pugi::xml_node xml_last_modified_date =
    get_single_child(xml_dates, XML_SUITE_LAST_MODIFIED_DATE_NODE_NAME, loc_data);
  std::string date_last_modified_str = std::string(xml_last_modified_date.child_value());

  pugi::xml_node xml_tool_version =
    get_single_child(xml_stamp, XML_SUITE_TOOL_VERSION_NODE_NAME, loc_data);
  std::string tool_version_str = std::string(xml_tool_version.child_value());

  bm_mgr.add_file_info(fname, author_str, date_created_str, date_last_modified_str,
                       tool_version_str);
}

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_benchmark_design_sources(pugi::xml_node& xml_root,
                                             const pugiutil::loc_data& loc_data,
                                             BenchmarkManager& bm_mgr, const BenchmarkId& bm_id) {
  for (pugi::xml_node xml_source : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_source.name() != std::string(XML_SUITE_BENCHMARK_FILE_NODE_NAME)) {
      bad_tag(xml_root, loc_data, xml_source, {XML_SUITE_BENCHMARK_FILE_NODE_NAME});
    }
    std::string ftype =
      get_attribute(xml_source, XML_SUITE_BENCHMARK_FILE_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string fname = std::string(xml_source.child_value());
    bm_mgr.add_design_source(bm_id, ftype, fname);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_benchmark_cocotb_sources(pugi::xml_node& xml_root,
                                             const pugiutil::loc_data& loc_data,
                                             BenchmarkManager& bm_mgr, const BenchmarkId& bm_id) {
  for (pugi::xml_node xml_source : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_source.name() != std::string(XML_SUITE_BENCHMARK_FILE_NODE_NAME)) {
      bad_tag(xml_root, loc_data, xml_source, {XML_SUITE_BENCHMARK_FILE_NODE_NAME});
    }
    std::string ftype =
      get_attribute(xml_source, XML_SUITE_BENCHMARK_FILE_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string fname = std::string(xml_source.child_value());
    bm_mgr.add_cocotb_source(bm_id, ftype, fname);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_benchmark_constraints(pugi::xml_node& xml_root,
                                          const pugiutil::loc_data& loc_data,
                                          BenchmarkManager& bm_mgr, const BenchmarkId& bm_id) {
  for (pugi::xml_node xml_source : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_source.name() != std::string(XML_SUITE_BENCHMARK_FILE_NODE_NAME)) {
      bad_tag(xml_root, loc_data, xml_source, {XML_SUITE_BENCHMARK_FILE_NODE_NAME});
    }
    std::string ftype =
      get_attribute(xml_source, XML_SUITE_BENCHMARK_FILE_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string fname = std::string(xml_source.child_value());
    bm_mgr.add_constraint(bm_id, ftype, fname);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes of a <device> to an object of ProjectManager
 *******************************************************************/
static int read_xml_benchmark(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                              BenchmarkManager& bm_mgr) {
  std::string st_name =
    get_attribute(xml_root, XML_SUITE_BENCHMARK_ATTRIBUTE_SUITE, loc_data).as_string();
  BenchmarkSuiteId st_id = bm_mgr.create_suite(st_name);
  /* Get name */
  std::string bm_name =
    get_attribute(xml_root, XML_SUITE_BENCHMARK_ATTRIBUTE_NAME, loc_data).as_string();
  BenchmarkId bm_id = bm_mgr.add_benchmark(st_id, bm_name);

  pugi::xml_node xml_tm =
    get_single_child(xml_root, XML_SUITE_BENCHMARK_TOPMODULE_NODE_NAME, loc_data);
  std::string tm_str = std::string(xml_tm.child_value());
  bm_mgr.set_top_module(bm_id, tm_str);

  /* Get design sources */
  pugi::xml_node xml_design =
    get_single_child(xml_root, XML_SUITE_BENCHMARK_DESIGN_NODE_NAME, loc_data);
  read_xml_benchmark_design_sources(xml_design, loc_data, bm_mgr, bm_id);

  /* Get cocotb sources */
  pugi::xml_node xml_cocotb =
    get_single_child(xml_root, XML_SUITE_BENCHMARK_COCOTB_NODE_NAME, loc_data);
  read_xml_benchmark_cocotb_sources(xml_cocotb, loc_data, bm_mgr, bm_id);

  /* Get constraint sources */
  pugi::xml_node xml_constr =
    get_single_child(xml_root, XML_SUITE_BENCHMARK_CONSTRAINTS_NODE_NAME, loc_data);
  read_xml_benchmark_constraints(xml_constr, loc_data, bm_mgr, bm_id);

  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_benchmark_manager(const char* fname, BenchmarkManager& bm_mgr, const bool& overwrite) {
  vtr::ScopedStartFinishTimer timer("Read benchmark file");
  /* Check file existence */
  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File does not exist at location: %s!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  size_t num_err = 0;

  /* Overwrite if required */
  if (overwrite) {
    bm_mgr.clear();
  }

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_SUITE_ROOT_NODE_NAME, loc_data);

    /* Parse stamps */
    pugi::xml_node xml_stamp_root =
      get_single_child(xml_root, XML_SUITE_STAMP_ROOT_NODE_NAME, loc_data);
    read_xml_stamp(xml_stamp_root, loc_data, bm_mgr, std::string(fname));

    /* Get suite name and add benchmark one by one */
    for (pugi::xml_node xml_bm : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_bm.name() == std::string(XML_SUITE_BENCHMARK_ROOT_NODE_NAME)) {
        int status = read_xml_benchmark(xml_bm, loc_data, bm_mgr);
        if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
          num_err++;
        }
      }
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname, e.line(), "%s", e.what());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  if (num_err) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arcxpress
