/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML of project file to the associated
 * data structures
 *******************************************************************/
#include <string>

/* Headers from pugi XML library */
#include "pugixml.hpp"
#include "pugixml_util.hpp"

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_time.h"

/* Headers from libarchfpga */
#include "arch_error.h"
#include "command_exit_codes.h"
#include "pugixml_util.hpp"
#include "read_xml_synth_strategy.h"
#include "read_xml_util.h"
#include "synth_strategy_xml_constants.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

static int read_xml_strategy(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                             FabricSynthStrategy& syn_strtg, size_t& num_defaults,
                             const bool& verbose) {
  std::string s_name =
    get_attribute(xml_node, XML_SYNTHSTRATEGY_STRATEGY_ATTR_NAME, loc_data).as_string();
  VTR_LOGV(verbose, "Parsing strategy '%s'...\n", s_name.c_str());
  int status = syn_strtg.add_strategy(s_name);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  bool s_default = get_attribute(xml_node, XML_SYNTHSTRATEGY_STRATEGY_ATTR_DEFAULT, loc_data,
                                 pugiutil::ReqOpt::OPTIONAL)
                     .as_bool(false);
  if (s_default) {
    status = syn_strtg.set_default(s_name);
    if (num_defaults > 0) {
      VTR_LOG_ERROR("More than 1 default strategy is defined. Expect only 1\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    num_defaults++;
  }
  /* Design patterns */
  pugi::xml_node xml_dsgn =
    get_single_child(xml_node, XML_SYNTHSTRATEGY_STRATEGY_NODE_DESIGNS, loc_data);
  for (pugi::xml_node xml_pattern : xml_dsgn.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_pattern.name() != std::string(XML_SYNTHSTRATEGY_STRATEGY_DESIGNS_NODE_PATTERN)) {
      bad_tag(xml_dsgn, loc_data, xml_pattern, {XML_SYNTHSTRATEGY_STRATEGY_DESIGNS_NODE_PATTERN});
    }
    std::string pattern_str = std::string(xml_pattern.child_value());
    VTR_LOGV(verbose, "Add design pattern '%s' to strategy '%s'\n", pattern_str.c_str(),
             s_name.c_str());
    status = syn_strtg.add_design_pattern(s_name, pattern_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Target libs */
  pugi::xml_node xml_tlibs =
    get_single_child(xml_node, XML_SYNTHSTRATEGY_STRATEGY_NODE_TARGETLIB, loc_data);
  for (pugi::xml_node xml_lib : xml_tlibs.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_lib.name() != std::string(XML_SYNTHSTRATEGY_STRATEGY_TARGETLIB_NODE_LIB)) {
      bad_tag(xml_tlibs, loc_data, xml_lib, {XML_SYNTHSTRATEGY_STRATEGY_TARGETLIB_NODE_LIB});
    }
    std::string tlib_str = std::string(xml_lib.child_value());
    status = syn_strtg.add_target_lib(s_name, tlib_str);
    VTR_LOGV(verbose, "Add target lib '%s' to strategy '%s'\n", tlib_str.c_str(), s_name.c_str());
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* compile options */
  pugi::xml_node xml_compile =
    get_single_child(xml_node, XML_SYNTHSTRATEGY_STRATEGY_NODE_COMPILE, loc_data);
  std::string cmpl_type =
    get_attribute(xml_compile, XML_SYNTHSTRATEGY_STRATEGY_COMPILE_ATTR_TYPE, loc_data).as_string();
  status = syn_strtg.set_compile_type(s_name, cmpl_type);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string cmpl_opt =
    get_attribute(xml_compile, XML_SYNTHSTRATEGY_STRATEGY_COMPILE_ATTR_OPTIMIZATION, loc_data)
      .as_string();
  status = syn_strtg.set_compile_optimization_target(s_name, cmpl_opt);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  std::string cmpl_effort =
    get_attribute(xml_compile, XML_SYNTHSTRATEGY_STRATEGY_COMPILE_ATTR_EFFORT, loc_data)
      .as_string();
  status = syn_strtg.set_compile_optimization_effort(s_name, cmpl_effort);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* sdc */
  pugi::xml_node xml_sdc =
    get_single_child(xml_node, XML_SYNTHSTRATEGY_STRATEGY_NODE_SDC, loc_data);
  for (pugi::xml_node xml_file : xml_sdc.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_SYNTHSTRATEGY_STRATEGY_SDC_NODE_FILE)) {
      bad_tag(xml_file, loc_data, xml_sdc, {XML_SYNTHSTRATEGY_STRATEGY_SDC_NODE_FILE});
    }
    std::string sdc_f_str = std::string(xml_file.child_value());
    status = syn_strtg.add_sdc(s_name, sdc_f_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_synth_strategy(const char* fname, FabricSynthStrategy& syn_strtg,
                            const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read Synthesis Strategy File");
  size_t num_err = 0;

  syn_strtg.clear_strategies();

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_SYNTHSTRATEGY_ROOT_NODE_NAME, loc_data);

    /* Parse strategy */
    size_t num_defaults = 0;
    for (pugi::xml_node xml_node : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_node.name() != std::string(XML_SYNTHSTRATEGY_NODE_STRATEGY)) {
        bad_tag(xml_root, loc_data, xml_node, {XML_SYNTHSTRATEGY_NODE_STRATEGY});
      }
      int status = read_xml_strategy(xml_node, loc_data, syn_strtg, num_defaults, verbose);
      if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
        num_err++;
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

}  // End of namespace arkangel
