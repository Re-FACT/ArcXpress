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
#include "command_exit_codes.h"
#include "pugixml_util.hpp"
#include "read_xml_tapeout_manager_arch_template.h"
#include "read_xml_util.h"
#include "tapeout_manager_arch_template_xml_constants.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

static int read_xml_flow_settings_synth(pugi::xml_node& xml_synth,
                                        const pugiutil::loc_data& loc_data,
                                        TapeoutManager& to_mgr) {
  int num_err = 0;

  for (pugi::xml_node xml_syn_pdk : xml_synth.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_syn_pdk.name() != std::string(XML_TAPEOUT_FLOWSET_SYNTH_PDK_NODE_NAME)) {
      bad_tag(xml_syn_pdk, loc_data, xml_synth, {XML_TAPEOUT_FLOWSET_SYNTH_PDK_NODE_NAME});
    }
    /* Get pdk name as the key to synth setting map */
    std::string pdk_name =
      get_attribute(xml_syn_pdk, XML_TAPEOUT_FLOWSET_SYNTH_PDK_ATTRIBUTE_NAME, loc_data)
        .as_string();

    pugi::xml_node xml_sdc_root =
      get_single_child(xml_syn_pdk, XML_TAPEOUT_FLOWSET_SYNTH_SDC_NODE_NAME, loc_data);
    std::string sdc_root_str = std::string(xml_sdc_root.child_value());
    bool status = to_mgr.set_synth_sdc_root_path(pdk_name, sdc_root_str);
    if (!status) {
      num_err++;
    }

    pugi::xml_node xml_synth_strategy =
      get_single_child(xml_syn_pdk, XML_TAPEOUT_FLOWSET_SYNTH_STRATEGY_NODE_NAME, loc_data);
    std::string synth_strategy_str = std::string(xml_synth_strategy.child_value());
    status = to_mgr.set_synth_strategy_file(pdk_name, synth_strategy_str);
    if (!status) {
      num_err++;
    }

    pugi::xml_node xml_ccff_max_fanout =
      get_single_child(xml_syn_pdk, XML_TAPEOUT_FLOWSET_CCFFMAXFANOUT_NODE_NAME, loc_data);
    std::string ccff_max_fanout_str = std::string(xml_ccff_max_fanout.child_value());
    status = to_mgr.set_ccff_max_fanout(pdk_name, std::stoi(ccff_max_fanout_str));
    if (!status) {
      num_err++;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_flow_settings_floorplan(pugi::xml_node& xml_flrpln,
                                            const pugiutil::loc_data& loc_data,
                                            TapeoutManager& to_mgr) {
  int num_err = 0;

  for (pugi::xml_node xml_flrpln_pdk : xml_flrpln.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_flrpln_pdk.name() != std::string(XML_TAPEOUT_FLOWSET_FLOORPLAN_PDK_NODE_NAME)) {
      bad_tag(xml_flrpln_pdk, loc_data, xml_flrpln, {XML_TAPEOUT_FLOWSET_FLOORPLAN_PDK_NODE_NAME});
    }
    /* Get pdk name as the key to synth setting map */
    std::string pdk_name =
      get_attribute(xml_flrpln_pdk, XML_TAPEOUT_FLOWSET_FLOORPLAN_PDK_ATTRIBUTE_NAME, loc_data)
        .as_string();

    pugi::xml_node xml_flrpln_strategy =
      get_single_child(xml_flrpln_pdk, XML_TAPEOUT_FLOWSET_FLOORPLAN_STRATEGY_NODE_NAME, loc_data);
    std::string flrpln_strategy_str = std::string(xml_flrpln_strategy.child_value());
    bool status = to_mgr.set_floorplan_strategy_file(pdk_name, flrpln_strategy_str);
    if (!status) {
      num_err++;
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_flow_settings_qorso(pugi::xml_node& xml_qorso,
                                        const pugiutil::loc_data& loc_data,
                                        TapeoutManager& to_mgr) {
  int num_err = 0;

  for (pugi::xml_node xml_qtask : xml_qorso.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_qtask.name() != std::string(XML_TAPEOUT_FLOWSET_QORSO_TASK_NODE_NAME)) {
      bad_tag(xml_qtask, loc_data, xml_qorso, {XML_TAPEOUT_FLOWSET_QORSO_TASK_NODE_NAME});
    }
    /* Parse data */
    std::string qtask_name =
      get_attribute(xml_qtask, XML_TAPEOUT_FLOWSET_QORSO_TASK_ATTRIBUTE_NAME, loc_data).as_string();
    std::string qtask_type =
      get_attribute(xml_qtask, XML_TAPEOUT_FLOWSET_QORSO_TASK_ATTRIBUTE_TYPE, loc_data).as_string();

    bool status = to_mgr.add_qorso_task(qtask_name, qtask_type);
    if (!status) {
      num_err++;
      break;
    }

    /* Parse for each supported PDK */
    for (pugi::xml_node xml_qtask_pdk : xml_qtask.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_qtask_pdk.name() != std::string(XML_TAPEOUT_FLOWSET_QORSO_TASK_PDK_NODE_NAME)) {
        bad_tag(xml_qtask_pdk, loc_data, xml_qtask, {XML_TAPEOUT_FLOWSET_QORSO_TASK_PDK_NODE_NAME});
      }
      std::string qtask_pdk_name =
        get_attribute(xml_qtask_pdk, XML_TAPEOUT_FLOWSET_QORSO_TASK_PDK_ATTRIBUTE_NAME, loc_data)
          .as_string();
      pugi::xml_node xml_config =
        get_single_child(xml_qtask_pdk, XML_TAPEOUT_FLOWSET_QORSO_TASK_CONFIG_NODE_NAME, loc_data);
      std::string config_str = std::string(xml_config.child_value());
      status = to_mgr.add_qorso_task_config_by_pdk(qtask_name, qtask_pdk_name, config_str);
      if (!status) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_flow_settings(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                                  TapeoutManager& to_mgr) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  /* Synthesis settings */
  pugi::xml_node xml_synth =
    get_single_child(xml_root, XML_TAPEOUT_FLOWSET_SYNTH_NODE_NAME, loc_data);
  status = read_xml_flow_settings_synth(xml_synth, loc_data, to_mgr);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Parsing node '%s' failed with errors\n", XML_TAPEOUT_FLOWSET_SYNTH_NODE_NAME);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Floorplan settings */
  pugi::xml_node xml_flrpln =
    get_single_child(xml_root, XML_TAPEOUT_FLOWSET_FLOORPLAN_NODE_NAME, loc_data);
  status = read_xml_flow_settings_floorplan(xml_flrpln, loc_data, to_mgr);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                  XML_TAPEOUT_FLOWSET_FLOORPLAN_NODE_NAME);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Qorso settings */
  pugi::xml_node xml_qorso =
    get_single_child(xml_root, XML_TAPEOUT_FLOWSET_QORSO_NODE_NAME, loc_data);
  status = read_xml_flow_settings_qorso(xml_qorso, loc_data, to_mgr);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Parsing node '%s' failed with errors\n", XML_TAPEOUT_FLOWSET_QORSO_NODE_NAME);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_built_in_corner_for_pdk(pugi::xml_node& xml_root,
                                            const pugiutil::loc_data& loc_data,
                                            TapeoutManager& to_mgr, const std::string& pdk_name) {
  for (pugi::xml_node xml_child : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_child.name() != std::string(XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_NODE_NAME)) {
      bad_tag(xml_child, loc_data, xml_root, {XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_NODE_NAME});
    }
    /* Parse data */
    std::string corner_name =
      get_attribute(xml_child, XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_ATTRIBUTE_NAME, loc_data)
        .as_string();
    /* Sclib, spef, and arch timing data is in a dedicated XML node */
    pugi::xml_node xml_sclib =
      get_single_child(xml_child, XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_SCLIB_NODE_NAME, loc_data);
    pugi::xml_node xml_spef =
      get_single_child(xml_child, XML_TAPEOUT_BUILTIN_CORNER_MAP_CORNER_SPEF_NODE_NAME, loc_data);
    bool status = to_mgr.add_builtin_corner_tag(pdk_name, corner_name, xml_sclib.child_value(),
                                                xml_spef.child_value());
    if (!status) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_built_in_corner_map(pugi::xml_node& xml_root,
                                        const pugiutil::loc_data& loc_data,
                                        TapeoutManager& to_mgr) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;

  for (pugi::xml_node xml_pdk_node : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_pdk_node.name() != std::string(XML_TAPEOUT_BUILTIN_CORNER_MAP_PDK_NODE_NAME)) {
      bad_tag(xml_pdk_node, loc_data, xml_root, {XML_TAPEOUT_BUILTIN_CORNER_MAP_PDK_NODE_NAME});
    }
    /* Parse data */
    std::string pdk_name =
      get_attribute(xml_pdk_node, XML_TAPEOUT_BUILTIN_CORNER_MAP_PDK_ATTRIBUTE_NAME, loc_data)
        .as_string();
    std::string pdk_default =
      get_attribute(xml_pdk_node, XML_TAPEOUT_BUILTIN_CORNER_MAP_PDK_ATTRIBUTE_DEFAULT, loc_data)
        .as_string();
    to_mgr.set_default_builtin_corner(pdk_name, pdk_default);
    /* Parse corners */
    status = read_xml_built_in_corner_for_pdk(xml_pdk_node, loc_data, to_mgr, pdk_name);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_num_io_settings(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                    TapeoutManager& to_mgr) {
  std::string var_name =
    get_attribute(xml_node, XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_NAME, loc_data)
      .as_string();
  std::string msb_var_name =
    get_attribute(xml_node, XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_MSB, loc_data,
                  pugiutil::ReqOpt::OPTIONAL)
      .as_string();
  std::string side_str =
    get_attribute(xml_node, XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_SIDE, loc_data)
      .as_string();
  std::string type_str =
    get_attribute(xml_node, XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_TYPE, loc_data)
      .as_string();
  int io_max = get_attribute(xml_node, XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_ATTRIBUTE_MAX, loc_data,
                             pugiutil::ReqOpt::OPTIONAL)
                 .as_int(0);
  bool status =
    to_mgr.add_num_io_per_tile_setting(side_str, type_str, var_name, msb_var_name, io_max);
  return status ? openfpga::CMD_EXEC_SUCCESS : openfpga::CMD_EXEC_FATAL_ERROR;
}

static int read_xml_io_type_settings(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                     TapeoutManager& to_mgr) {
  std::string side_str =
    get_attribute(xml_node, XML_TAPEOUT_ARCH_IO_SETTINGS_IO_TYPE_ATTRIBUTE_SIDE, loc_data)
      .as_string();
  std::string var_name =
    get_attribute(xml_node, XML_TAPEOUT_ARCH_IO_SETTINGS_IO_TYPE_ATTRIBUTE_NAME, loc_data)
      .as_string();
  bool status = to_mgr.add_io_type_setting(side_str, var_name);
  return status ? openfpga::CMD_EXEC_SUCCESS : openfpga::CMD_EXEC_FATAL_ERROR;
}

static int read_xml_io_settings(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                                TapeoutManager& to_mgr) {
  int num_err;
  for (pugi::xml_node xml_child : xml_node.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_child.name() == std::string(XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_NODE_NAME)) {
      int status = read_xml_num_io_settings(xml_child, loc_data, to_mgr);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    } else if (xml_child.name() == std::string(XML_TAPEOUT_ARCH_IO_SETTINGS_IO_TYPE_NODE_NAME)) {
      int status = read_xml_io_type_settings(xml_child, loc_data, to_mgr);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    } else {
      bad_tag(xml_child, loc_data, xml_node,
              {XML_TAPEOUT_ARCH_IO_SETTINGS_NUM_IO_NODE_NAME,
               XML_TAPEOUT_ARCH_IO_SETTINGS_IO_TYPE_NODE_NAME});
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_arch_gen(pugi::xml_node& xml_node, const pugiutil::loc_data& loc_data,
                             TapeoutManager& to_mgr) {
  int num_err = 0;
  for (pugi::xml_node xml_file : xml_node.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
      bad_tag(xml_file, loc_data, xml_node, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
    }
    std::string f_type =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string f_path = std::string(xml_file.child_value());
    bool status = to_mgr.set_arch_gen_bin_path(f_type, f_path);
    if (!status) {
      num_err++;
    }
  }
  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_arch_templates(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                                   TapeoutManager& to_mgr) {
  int num_err = 0;
  size_t route_w = get_attribute(xml_root, XML_TAPEOUT_ARCH_ATTRIBUTE_W, loc_data).as_int();
  to_mgr.set_route_chan_width(route_w);

  /* arch generator */
  pugi::xml_node xml_arch_gen =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_ARCH_GENERATOR_NODE_NAME, loc_data);
  int arch_gen_status = read_xml_arch_gen(xml_arch_gen, loc_data, to_mgr);
  if (arch_gen_status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* io settings */
  pugi::xml_node xml_io_settings =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_IO_SETTINGS_NODE_NAME, loc_data);
  int io_status = read_xml_io_settings(xml_io_settings, loc_data, to_mgr);
  if (io_status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Log config settings */
  pugi::xml_node xml_of_stats_conf =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_OPENFPGA_STATS_CONF_NODE_NAME, loc_data);
  to_mgr.set_openfpga_stats_conf_file(std::string(xml_of_stats_conf.child_value()));
  pugi::xml_node xml_of_error_conf =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_OPENFPGA_ERROR_CONF_NODE_NAME, loc_data);
  to_mgr.set_openfpga_error_conf_file(std::string(xml_of_error_conf.child_value()));
  pugi::xml_node xml_ys_error_conf =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_YOSYS_ERROR_CONF_NODE_NAME, loc_data);
  to_mgr.set_yosys_error_conf_file(std::string(xml_ys_error_conf.child_value()));

  /* For arch timing */
  pugi::xml_node xml_timing =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_TIMING_NODE_NAME, loc_data);
  std::string arch_timing_pdk_tmpl =
    get_attribute(xml_timing, XML_TAPEOUT_ARCH_TIMING_ATTRIBUTE_PDK_TEMPLATE, loc_data).as_string();
  to_mgr.set_arch_timing_template_pdk(arch_timing_pdk_tmpl);
  std::string arch_timing_tmpl =
    get_attribute(xml_timing, XML_TAPEOUT_ARCH_TIMING_ATTRIBUTE_FILE_TEMPLATE, loc_data)
      .as_string();
  to_mgr.set_arch_timing_template_file(arch_timing_tmpl);
  for (pugi::xml_node xml_pdk : xml_timing.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_pdk.name() != std::string(XML_TAPEOUT_ARCH_TIMING_PDK_NODE_NAME)) {
      bad_tag(xml_timing, loc_data, xml_pdk, {XML_TAPEOUT_ARCH_TIMING_PDK_NODE_NAME});
    }
    /* Basic attributes */
    std::string pdk_name =
      get_attribute(xml_pdk, XML_TAPEOUT_ARCH_TIMING_PDK_ATTRIBUTE_NAME, loc_data).as_string();
    for (pugi::xml_node xml_file : xml_pdk.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
        bad_tag(xml_timing, loc_data, xml_file, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
      }
      std::string f_type =
        get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
      std::string f_path = std::string(xml_file.child_value());
      bool status = to_mgr.add_arch_timing_file(pdk_name, f_type, f_path);
      if (!status) {
        num_err++;
      }
    }
  }

  /* For dp data */
  pugi::xml_node xml_dp_arch = get_single_child(xml_root, XML_TAPEOUT_ARCH_DP_NODE_NAME, loc_data);
  for (pugi::xml_node xml_file : xml_dp_arch.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
      bad_tag(xml_dp_arch, loc_data, xml_file, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
    }
    std::string f_type =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string f_path = std::string(xml_file.child_value());
    bool status = to_mgr.add_dp_arch_file(f_type, f_path);
    if (!status) {
      num_err++;
    }
  }
  /* For ultimate data */
  pugi::xml_node xml_ut_arch =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_ULTIMATE_NODE_NAME, loc_data);
  for (pugi::xml_node xml_file : xml_ut_arch.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
      bad_tag(xml_ut_arch, loc_data, xml_file, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
    }
    std::string f_type =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string f_path = std::string(xml_file.child_value());
    bool status = to_mgr.add_ultimate_arch_file(f_type, f_path);
    if (!status) {
      num_err++;
    }
  }
  /* For openfpga settings */
  pugi::xml_node xml_ofsettings =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_OPENFPGASETTING_NODE_NAME, loc_data);
  for (pugi::xml_node xml_file : xml_ofsettings.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
      bad_tag(xml_ofsettings, loc_data, xml_file, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
    }
    std::string f_type =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string f_path = std::string(xml_file.child_value());
    bool status = to_mgr.add_openfpga_setting_file(f_type, f_path);
    if (!status) {
      num_err++;
    }
  }
  /* For openfpga shell scripts */
  pugi::xml_node xml_ofshellscript =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_OPENFPGASCRIPT_NODE_NAME, loc_data);
  for (pugi::xml_node xml_file : xml_ofshellscript.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
      bad_tag(xml_ofshellscript, loc_data, xml_file, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
    }
    std::string f_type =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string f_path = std::string(xml_file.child_value());
    bool status = to_mgr.add_openfpga_script_file(f_type, f_path);
    if (!status) {
      num_err++;
    }
  }
  /* For yosys scripts */
  pugi::xml_node xml_ysscript =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_YOSYSSCRIPT_NODE_NAME, loc_data);
  for (pugi::xml_node xml_file : xml_ysscript.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
      bad_tag(xml_ysscript, loc_data, xml_file, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
    }
    std::string f_type =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string f_path = std::string(xml_file.child_value());
    bool status = to_mgr.add_yosys_script_file(f_type, f_path);
    if (!status) {
      num_err++;
    }
  }

  /* For netlist flags */
  pugi::xml_node xml_flags =
    get_single_child(xml_root, XML_TAPEOUT_NETLIST_FLAGS_NODE_NAME, loc_data);
  for (pugi::xml_node xml_flag_pdk : xml_flags.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_flag_pdk.name() != std::string(XML_TAPEOUT_NETLIST_FLAGS_PDK_NODE_NAME)) {
      bad_tag(xml_flag_pdk, loc_data, xml_flags, {XML_TAPEOUT_NETLIST_FLAGS_PDK_NODE_NAME});
    }
    std::string flag_pdk_name =
      get_attribute(xml_flag_pdk, XML_TAPEOUT_NETLIST_FLAGS_PDK_ATTRIBUTE_NAME, loc_data)
        .as_string();
    for (pugi::xml_node xml_flag : xml_flag_pdk.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_flag.name() != std::string(XML_TAPEOUT_NETLIST_FLAGS_FLAG_NODE_NAME)) {
        bad_tag(xml_flag, loc_data, xml_flag_pdk, {XML_TAPEOUT_NETLIST_FLAGS_FLAG_NODE_NAME});
      }
      std::string f_name =
        get_attribute(xml_flag, XML_TAPEOUT_NETLIST_FLAGS_FLAG_ATTRIBUTE_NAME, loc_data)
          .as_string();
      std::string f_val = std::string(xml_flag.child_value());
      bool status = to_mgr.add_netlist_flag(flag_pdk_name, f_name, f_val);
      if (!status) {
        num_err++;
      }
    }
  }

  /* For third party ip files */
  pugi::xml_node xml_ips = get_single_child(xml_root, XML_TAPEOUT_ARCH_IP_NODE_NAME, loc_data);
  for (pugi::xml_node xml_file : xml_ips.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_file.name() != std::string(XML_TAPEOUT_ARCH_FILE_NODE_NAME)) {
      bad_tag(xml_ips, loc_data, xml_file, {XML_TAPEOUT_ARCH_FILE_NODE_NAME});
    }
    std::string f_type =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string f_fmt =
      get_attribute(xml_file, XML_TAPEOUT_ARCH_ATTRIBUTE_FORMAT, loc_data,
                    pugiutil::ReqOpt::OPTIONAL)
        .as_string(to_mgr.ip_file_format2str(TapeoutManager::e_ip_file_format::AUTO).c_str());
    std::string f_path = std::string(xml_file.child_value());
    bool status = to_mgr.add_ip_file(f_type, f_path, f_fmt);
    if (!status) {
      num_err++;
    }
  }

  /* For netlist makeup key files */
  pugi::xml_node xml_nlist_makeup_key =
    get_single_child(xml_root, XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_NODE_NAME, loc_data);
  std::string nlist_makeup_key_root =
    get_attribute(xml_nlist_makeup_key, XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_ATTRIBUTE_ROOT,
                  loc_data)
      .as_string();
  to_mgr.set_netlist_makeup_key_root_path(nlist_makeup_key_root);

  for (pugi::xml_node xml_pdk : xml_nlist_makeup_key.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_pdk.name() != std::string(XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_NODE_NAME)) {
      bad_tag(xml_pdk, loc_data, xml_nlist_makeup_key,
              {XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_NODE_NAME});
    }
    /* Basic attributes */
    std::string pdk_name =
      get_attribute(xml_pdk, XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_ATTRIBUTE_NAME, loc_data)
        .as_string();
    for (pugi::xml_node xml_file : xml_pdk.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_file.name() ==
          std::string(XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_NETLIST_MAKEUP_CONFIG_NODE_NAME)) {
        std::string f_path = std::string(xml_file.child_value());
        bool status = to_mgr.add_netlist_makeup_config_file(pdk_name, f_path);
        if (!status) {
          num_err++;
        }
      } else if (xml_file.name() ==
                 std::string(
                   XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_CCFF_MAKEUP_BUF_CONFIG_NODE_NAME)) {
        std::string f_path = std::string(xml_file.child_value());
        bool status = to_mgr.add_ccff_makeup_buf_config_file(pdk_name, f_path);
        if (!status) {
          num_err++;
        }
      } else {
        bad_tag(xml_file, loc_data, xml_pdk,
                {XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_CCFF_MAKEUP_BUF_CONFIG_NODE_NAME});
      }
    }
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_tapeout_manager_arch_template(const char* fname, TapeoutManager& to_mgr) {
  vtr::ScopedStartFinishTimer timer("Read architecture template for project file");
  /* Check file existence */
  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File does not exist at location: %s!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t num_err = 0;

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_TAPEOUT_ARCH_ROOT_NODE_NAME, loc_data);

    /* Parse flow settings */
    pugi::xml_node xml_flow_set =
      get_single_child(xml_root, XML_TAPEOUT_FLOWSET_NODE_NAME, loc_data);
    int flset_status = read_xml_flow_settings(xml_flow_set, loc_data, to_mgr);
    if (flset_status == openfpga::CMD_EXEC_FATAL_ERROR) {
      VTR_LOG_ERROR("Parsing node '%s' failed with errors\n", XML_TAPEOUT_FLOWSET_NODE_NAME);
      num_err++;
    }
    /* Parse built-in corners */
    pugi::xml_node xml_bcorner =
      get_single_child(xml_root, XML_TAPEOUT_BUILTIN_CORNER_MAP_NODE_NAME, loc_data);
    int bcorner_status = read_xml_built_in_corner_map(xml_bcorner, loc_data, to_mgr);
    if (bcorner_status == openfpga::CMD_EXEC_FATAL_ERROR) {
      VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                    XML_TAPEOUT_BUILTIN_CORNER_MAP_NODE_NAME);
      num_err++;
    }

    /* Parse arch templates */
    pugi::xml_node xml_arch_tmpl = get_single_child(xml_root, XML_TAPEOUT_ARCH_NODE_NAME, loc_data);
    int arch_status = read_xml_arch_templates(xml_arch_tmpl, loc_data, to_mgr);
    if (arch_status == openfpga::CMD_EXEC_FATAL_ERROR) {
      VTR_LOG_ERROR("Parsing node '%s' failed with errors\n", XML_TAPEOUT_ARCH_NODE_NAME);
      num_err++;
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
