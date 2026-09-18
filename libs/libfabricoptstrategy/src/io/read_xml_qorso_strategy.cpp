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
#include "qorso_strategy_xml_constants.h"
#include "read_xml_qorso_strategy.h"
#include "read_xml_util.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

static int read_xml_power_strategy_task(pugi::xml_node& xml_node,
                                        const pugiutil::loc_data& loc_data, QorsoStrategy& qs_strtg,
                                        const std::string& s_name, const bool& verbose) {
  /* Create task with a name */
  std::string t_name =
    get_attribute(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_ATTR_NAME, loc_data).as_string();
  int status = qs_strtg.add_power_task(s_name, t_name);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Designs */
  pugi::xml_node xml_designs =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_DESIGNS, loc_data);
  /* Parse designs one by one */
  for (pugi::xml_node xml_design : xml_designs.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_design.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_TASK_DESIGNS_NODE_DESIGN)) {
      bad_tag(xml_design, loc_data, xml_designs,
              {XML_QORSOSTRATEGY_STRATEGY_TASK_DESIGNS_NODE_DESIGN});
    }
    std::string dsgn_str = std::string(xml_design.child_value());
    status = qs_strtg.add_power_task_design(s_name, t_name, dsgn_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* Method */
  pugi::xml_node xml_mthd =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_METHOD, loc_data);
  std::string mthd_str = std::string(xml_mthd.child_value());
  status = qs_strtg.set_power_task_method(s_name, t_name, mthd_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* App var */
  pugi::xml_node xml_appvar =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_APPVAR, loc_data);
  /* Parse variable one by one */
  for (pugi::xml_node xml_var : xml_appvar.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_var.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_NODE_VARIABLE)) {
      bad_tag(xml_var, loc_data, xml_appvar, {XML_QORSOSTRATEGY_STRATEGY_NODE_VARIABLE});
    }
    std::string var_name =
      get_attribute(xml_var, XML_QORSOSTRATEGY_STRATEGY_VARIABLE_ATTR_NAME, loc_data).as_string();
    std::string var_val_str = std::string(xml_var.child_value());
    status = qs_strtg.add_power_task_app_variable(s_name, t_name, var_name, var_val_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* sdc var */
  pugi::xml_node xml_sdcvar =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_SDCVAR, loc_data);
  /* Parse variable one by one */
  for (pugi::xml_node xml_var : xml_sdcvar.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_var.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_NODE_VARIABLE)) {
      bad_tag(xml_var, loc_data, xml_sdcvar, {XML_QORSOSTRATEGY_STRATEGY_NODE_VARIABLE});
    }
    std::string var_name =
      get_attribute(xml_var, XML_QORSOSTRATEGY_STRATEGY_VARIABLE_ATTR_NAME, loc_data).as_string();
    std::string var_val_str = std::string(xml_var.child_value());
    status = qs_strtg.add_power_task_sdc_variable(s_name, t_name, var_name, var_val_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* sdc */
  pugi::xml_node xml_sdc =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_SDC, loc_data);
  for (pugi::xml_node xml_file : xml_sdc.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_TASK_SDC_NODE_FILE)) {
      bad_tag(xml_file, loc_data, xml_sdc, {XML_QORSOSTRATEGY_STRATEGY_TASK_SDC_NODE_FILE});
    }
    std::string sdc_f_str = std::string(xml_file.child_value());
    status = qs_strtg.add_power_task_sdc_file(s_name, t_name, sdc_f_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* report command */
  pugi::xml_node xml_rptcmd =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_REPORT_COMMAND, loc_data);
  std::string rptcmd_str = std::string(xml_rptcmd.child_value());
  status = qs_strtg.set_power_task_report_command(s_name, t_name, rptcmd_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  bool rptcmd_ts =
    get_attribute(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_REPORT_COMMAND_ATTR_REMOVE_TIME_STAMP,
                  loc_data, pugiutil::ReqOpt::OPTIONAL)
      .as_bool(false);
  status = qs_strtg.set_power_task_remove_time_stamp(s_name, t_name, rptcmd_ts);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_power_strategy(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                   QorsoStrategy& qs_strtg, const std::string& s_name,
                                   const bool& verbose) {
  /* Summary */
  pugi::xml_node xml_smry =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_NODE_SUMMARY, loc_data);

  std::string power_data_type =
    get_attribute(xml_smry, XML_QORSOSTRATEGY_STRATEGY_SUMMARY_ATTR_DATA_TYPE, loc_data)
      .as_string();
  int status = qs_strtg.set_summary_data(s_name, power_data_type);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Power tasks */
  for (pugi::xml_node xml_task : xml_node.children()) {
    if (xml_task.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_NODE_TASK)) {
      continue;
    }
    status = read_xml_power_strategy_task(xml_task, loc_data, qs_strtg, s_name, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_timing_strategy_task(pugi::xml_node& xml_node,
                                         const pugiutil::loc_data& loc_data,
                                         QorsoStrategy& qs_strtg, const std::string& s_name,
                                         const bool& verbose) {
  /* Create task with a name */
  std::string t_name =
    get_attribute(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_ATTR_NAME, loc_data).as_string();
  int status = qs_strtg.add_timing_task(s_name, t_name);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Designs */
  pugi::xml_node xml_designs =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_DESIGNS, loc_data);
  std::string auto_infer_type =
    get_attribute(xml_designs,
                  XML_QORSOSTRATEGY_STRATEGY_TASK_DESIGNS_ATTR_AUTO_INFER_FROM_TILE_TYPE, loc_data,
                  pugiutil::ReqOpt::OPTIONAL)
      .as_string();
  if (!auto_infer_type.empty()) {
    status =
      qs_strtg.set_timing_task_design_auto_infer_from_tile_type(s_name, t_name, auto_infer_type);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  } else {
    /* Parse designs one by one */
    for (pugi::xml_node xml_design : xml_designs.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_design.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_TASK_DESIGNS_NODE_DESIGN)) {
        bad_tag(xml_design, loc_data, xml_designs,
                {XML_QORSOSTRATEGY_STRATEGY_TASK_DESIGNS_NODE_DESIGN});
      }
      std::string dsgn_str = std::string(xml_design.child_value());
      status = qs_strtg.add_timing_task_design(s_name, t_name, dsgn_str);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  }

  /* Parse paths */
  pugi::xml_node xml_path =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_PATH, loc_data);
  /* Parse path attributes */
  std::string maxdly_str =
    get_attribute(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_MAX_DELAY, loc_data)
      .as_string();
  status = qs_strtg.set_timing_task_max_delay(s_name, t_name, maxdly_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string mindly_str =
    get_attribute(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_MIN_DELAY, loc_data)
      .as_string();
  status = qs_strtg.set_timing_task_min_delay(s_name, t_name, mindly_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string dlytype_str =
    get_attribute(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_DELAY_TYPE, loc_data)
      .as_string();
  status = qs_strtg.set_timing_task_delay_type(s_name, t_name, dlytype_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string clkgroup_str =
    get_attribute(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_CLOCK_GROUP, loc_data)
      .as_string();
  status = qs_strtg.set_timing_task_clock_group(s_name, t_name, clkgroup_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string nworst_str =
    get_attribute(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_NWORST, loc_data).as_string();
  status = qs_strtg.set_timing_task_nworst(s_name, t_name, nworst_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string maxpaths_str =
    get_attribute(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_ATTR_MAX_PATHS, loc_data)
      .as_string();
  status = qs_strtg.set_timing_task_max_paths(s_name, t_name, maxpaths_str);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* From pins */
  pugi::xml_node xml_from_pin =
    get_single_child(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_FROM_PINS, loc_data);
  for (pugi::xml_node xml_pin : xml_from_pin.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_pin.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_PIN)) {
      bad_tag(xml_pin, loc_data, xml_from_pin, {XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_PIN});
    }
    std::string pin_str = std::string(xml_pin.child_value());
    status = qs_strtg.add_timing_task_from_pin(s_name, t_name, pin_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* to pins */
  pugi::xml_node xml_to_pin =
    get_single_child(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_TO_PINS, loc_data);
  for (pugi::xml_node xml_pin : xml_to_pin.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_pin.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_PIN)) {
      bad_tag(xml_pin, loc_data, xml_to_pin, {XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_PIN});
    }
    std::string pin_str = std::string(xml_pin.child_value());
    status = qs_strtg.add_timing_task_to_pin(s_name, t_name, pin_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  /* through pins */
  pugi::xml_node xml_thru_pin =
    get_single_child(xml_path, XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_THRU_PINS, loc_data,
                     pugiutil::ReqOpt::OPTIONAL);
  if (xml_thru_pin) {
    for (pugi::xml_node xml_pin : xml_thru_pin.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_pin.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_PIN)) {
        bad_tag(xml_pin, loc_data, xml_thru_pin, {XML_QORSOSTRATEGY_STRATEGY_TASK_PATH_NODE_PIN});
      }
      std::string pin_str = std::string(xml_pin.child_value());
      status = qs_strtg.add_timing_task_thru_pin(s_name, t_name, pin_str);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  }

  /* sdc */
  pugi::xml_node xml_sdc =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_TASK_NODE_SDC, loc_data);
  for (pugi::xml_node xml_file : xml_sdc.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_TASK_SDC_NODE_FILE)) {
      bad_tag(xml_file, loc_data, xml_sdc, {XML_QORSOSTRATEGY_STRATEGY_TASK_SDC_NODE_FILE});
    }
    std::string sdc_f_str = std::string(xml_file.child_value());
    status = qs_strtg.add_timing_task_sdc_file(s_name, t_name, sdc_f_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_timing_strategy(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                    QorsoStrategy& qs_strtg, const std::string& s_name,
                                    const bool& verbose) {
  /* Summary */
  pugi::xml_node xml_smry =
    get_single_child(xml_node, XML_QORSOSTRATEGY_STRATEGY_NODE_SUMMARY, loc_data);

  std::string timing_data =
    get_attribute(xml_smry, XML_QORSOSTRATEGY_STRATEGY_SUMMARY_ATTR_TIMING_DATA, loc_data)
      .as_string();
  int status = qs_strtg.set_summary_data(s_name, timing_data);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  std::string timing_data_type =
    get_attribute(xml_smry, XML_QORSOSTRATEGY_STRATEGY_SUMMARY_ATTR_DATA_TYPE, loc_data)
      .as_string();
  status = qs_strtg.set_summary_data_type(s_name, timing_data_type);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  float sfac = get_attribute(xml_smry, XML_QORSOSTRATEGY_STRATEGY_SUMMARY_ATTR_SCALE_FACTOR,
                             loc_data, pugiutil::ReqOpt::OPTIONAL)
                 .as_float(1.0);
  status = qs_strtg.set_summary_data_scale_factor(s_name, sfac);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Summary variables */
  for (pugi::xml_node xml_var : xml_smry.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_var.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_NODE_VARIABLE)) {
      bad_tag(xml_smry, loc_data, xml_var, {XML_QORSOSTRATEGY_STRATEGY_NODE_VARIABLE});
    }
    std::string var_arch_name =
      get_attribute(xml_var, XML_QORSOSTRATEGY_STRATEGY_VARIABLE_ATTR_NAME, loc_data).as_string();
    std::string var_disp_name = std::string(xml_var.child_value());
    VTR_LOGV(verbose, "Add summary variable pair ('%s', '%s') to timing strategy '%s'\n",
             var_arch_name.c_str(), var_disp_name.c_str(), s_name.c_str());
    status = qs_strtg.add_arch_variable_pair(s_name, var_arch_name, var_disp_name);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* Timing tasks */
  for (pugi::xml_node xml_task : xml_node.children()) {
    if (xml_task.name() != std::string(XML_QORSOSTRATEGY_STRATEGY_NODE_TASK)) {
      continue;
    }
    status = read_xml_timing_strategy_task(xml_task, loc_data, qs_strtg, s_name, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_strategy(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                             QorsoStrategy& qs_strtg, const bool& verbose) {
  std::string s_name =
    get_attribute(xml_node, XML_QORSOSTRATEGY_STRATEGY_ATTR_NAME, loc_data).as_string();
  std::string s_type =
    get_attribute(xml_node, XML_QORSOSTRATEGY_STRATEGY_ATTR_TYPE, loc_data).as_string();
  VTR_LOGV(verbose, "Parsing strategy '%s' whose type is '%s'...\n", s_name.c_str(),
           s_type.c_str());
  int status = qs_strtg.add_strategy(s_type, s_name);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Summary */
  auto s_type_enum = qs_strtg.str2strategy_type(s_type);
  if (s_type_enum == QorsoStrategy::e_strategy_type::TIMING ||
      s_type_enum == QorsoStrategy::e_strategy_type::TIMING_GRST ||
      s_type_enum == QorsoStrategy::e_strategy_type::TIMING_GCLK) {
    status = read_xml_timing_strategy(xml_node, loc_data, qs_strtg, s_name, verbose);
  } else if (s_type_enum == QorsoStrategy::e_strategy_type::POWER) {
    status = read_xml_power_strategy(xml_node, loc_data, qs_strtg, s_name, verbose);
  } else {
    VTR_LOG_ERROR("Invalid type of strategy '%s'. Expect %s\n", s_name.c_str(),
                  qs_strtg.strategy_type_all2str().c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return status;
}

static int read_xml_global_var_sweep(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                     QorsoStrategy& qs_strtg, const bool& verbose) {
  std::string v_name =
    get_attribute(xml_node, XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_NAME, loc_data).as_string();

  int start =
    get_attribute(xml_node, XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_START, loc_data).as_int();

  int end = get_attribute(xml_node, XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_END, loc_data).as_int();

  int step = get_attribute(xml_node, XML_QORSOSTRATEGY_GLOBAL_VAR_SWEEP_ATTR_STEP, loc_data,
                           pugiutil::ReqOpt::OPTIONAL)
               .as_int(1);

  return qs_strtg.add_strategy_sweep_variable(v_name, start, end, step);
}

static int read_xml_global_var_constant(pugi::xml_node& xml_node,
                                        const pugiutil::loc_data& loc_data, QorsoStrategy& qs_strtg,
                                        const bool& verbose) {
  std::string v_name =
    get_attribute(xml_node, XML_QORSOSTRATEGY_GLOBAL_VAR_CONSTANT_ATTR_NAME, loc_data).as_string();

  int value =
    get_attribute(xml_node, XML_QORSOSTRATEGY_GLOBAL_VAR_CONSTANT_ATTR_VALUE, loc_data).as_int();

  return qs_strtg.add_strategy_constant_variable(v_name, value);
}

static int read_xml_global_var(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                               QorsoStrategy& qs_strtg, const bool& verbose) {
  size_t num_err = 0;
  for (pugi::xml_node xml_var : xml_node.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_var.name() == std::string(XML_QORSOSTRATEGY_GLOBAL_VAR_NODE_SWEEP)) {
      int status = read_xml_global_var_sweep(xml_var, loc_data, qs_strtg, verbose);
      if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
        num_err++;
      }
    } else if (xml_var.name() == std::string(XML_QORSOSTRATEGY_GLOBAL_VAR_NODE_CONSTANT)) {
      int status = read_xml_global_var_constant(xml_var, loc_data, qs_strtg, verbose);
      if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
        num_err++;
      }
    } else {
      bad_tag(
        xml_node, loc_data, xml_var,
        {XML_QORSOSTRATEGY_GLOBAL_VAR_NODE_SWEEP, XML_QORSOSTRATEGY_GLOBAL_VAR_NODE_CONSTANT});
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_qorso_strategy(const char* fname, QorsoStrategy& qs_strtg, const bool& append,
                            const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read Qorso Strategy File");
  size_t num_err = 0;

  if (!append) {
    qs_strtg.clear_strategies();
  }

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_QORSOSTRATEGY_ROOT_NODE_NAME, loc_data);

    /* Parse strategy */
    for (pugi::xml_node xml_node : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_node.name() == std::string(XML_QORSOSTRATEGY_NODE_STRATEGY)) {
        int status = read_xml_strategy(xml_node, loc_data, qs_strtg, verbose);
        if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
          num_err++;
        }
      } else if (xml_node.name() == std::string(XML_QORSOSTRATEGY_NODE_GLOBAL_VAR)) {
        int status = read_xml_global_var(xml_node, loc_data, qs_strtg, verbose);
        if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
          num_err++;
        }
      } else {
        bad_tag(xml_root, loc_data, xml_node,
                {XML_QORSOSTRATEGY_NODE_STRATEGY, XML_QORSOSTRATEGY_NODE_GLOBAL_VAR});
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
