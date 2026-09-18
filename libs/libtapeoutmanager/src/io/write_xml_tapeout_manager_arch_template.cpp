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
#include "pugixml.hpp"
#include "tapeout_manager_arch_template_xml_constants.h"
#include "write_xml_tapeout_manager_arch_template.h"

namespace arkangel {  // Begin namespace arkangel

static int write_xml_flow_settings_synth(pugi::xml_node& flset_node, const TapeoutManager& to_mgr,
                                         const bool& keep_reserved_word) {
  int err_code = 0;
  pugi::xml_node synth_node = flset_node.append_child(XML_TAPEOUT_FLOWSET_SYNTH_NODE_NAME);

  /* For each PDK, add a new node */
  for (std::string pdk_name : to_mgr.synth_pdk_names()) {
    pugi::xml_node pdk_node = synth_node.append_child(XML_TAPEOUT_FLOWSET_SYNTH_PDK_NODE_NAME);
    pdk_node.append_attribute(XML_TAPEOUT_FLOWSET_SYNTH_PDK_ATTRIBUTE_NAME) = pdk_name.c_str();

    pugi::xml_node sdc_node = pdk_node.append_child(XML_TAPEOUT_FLOWSET_SYNTH_SDC_NODE_NAME);
    sdc_node.text().set(to_mgr.synth_sdc_root_path(pdk_name, keep_reserved_word).c_str());

    pugi::xml_node strategy_node =
      pdk_node.append_child(XML_TAPEOUT_FLOWSET_SYNTH_STRATEGY_NODE_NAME);
    strategy_node.text().set(to_mgr.synth_strategy_file(pdk_name, keep_reserved_word).c_str());

    pugi::xml_node ccffmaxfanout_node =
      pdk_node.append_child(XML_TAPEOUT_FLOWSET_CCFFMAXFANOUT_NODE_NAME);
    ccffmaxfanout_node.text().set(std::to_string(to_mgr.ccff_max_fanout(pdk_name)).c_str());
  }

  return err_code;
}

static int write_xml_flow_settings_floorplan(pugi::xml_node& flset_node,
                                             const TapeoutManager& to_mgr,
                                             const bool& keep_reserved_word) {
  int err_code = 0;
  pugi::xml_node flrpln_node = flset_node.append_child(XML_TAPEOUT_FLOWSET_FLOORPLAN_NODE_NAME);

  /* For each PDK, add a new node */
  for (std::string pdk_name : to_mgr.floorplan_pdk_names()) {
    pugi::xml_node pdk_node = flrpln_node.append_child(XML_TAPEOUT_FLOWSET_FLOORPLAN_PDK_NODE_NAME);
    pdk_node.append_attribute(XML_TAPEOUT_FLOWSET_FLOORPLAN_PDK_ATTRIBUTE_NAME) = pdk_name.c_str();

    pugi::xml_node strategy_node =
      pdk_node.append_child(XML_TAPEOUT_FLOWSET_FLOORPLAN_STRATEGY_NODE_NAME);
    strategy_node.text().set(to_mgr.floorplan_strategy_file(pdk_name, keep_reserved_word).c_str());
  }

  return err_code;
}

static int write_xml_flow_settings_qorso(pugi::xml_node& flset_node, const TapeoutManager& to_mgr,
                                         const bool& keep_reserved_word) {
  int err_code = 0;
  pugi::xml_node qorso_node = flset_node.append_child(XML_TAPEOUT_FLOWSET_QORSO_NODE_NAME);

  for (auto qtask_id : to_mgr.qorso_tasks()) {
    pugi::xml_node task_node = qorso_node.append_child(XML_TAPEOUT_FLOWSET_QORSO_TASK_NODE_NAME);
    task_node.append_attribute(XML_TAPEOUT_FLOWSET_QORSO_TASK_ATTRIBUTE_NAME) = qtask_id.c_str();
    task_node.append_attribute(XML_TAPEOUT_FLOWSET_QORSO_TASK_ATTRIBUTE_TYPE) =
      to_mgr.qorso_task_type_str(qtask_id).c_str();

    /* Add per pdk nodes */
    for (std::string qtask_pdk : to_mgr.qorso_task_pdk_names(qtask_id)) {
      pugi::xml_node pdk_node =
        task_node.append_child(XML_TAPEOUT_FLOWSET_QORSO_TASK_PDK_NODE_NAME);
      pdk_node.append_attribute(XML_TAPEOUT_FLOWSET_QORSO_TASK_PDK_ATTRIBUTE_NAME) =
        qtask_pdk.c_str();

      pugi::xml_node config_node =
        pdk_node.append_child(XML_TAPEOUT_FLOWSET_QORSO_TASK_CONFIG_NODE_NAME);
      config_node.text().set(
        to_mgr.qorso_task_config(qtask_id, qtask_pdk, keep_reserved_word).c_str());
    }
  }

  return err_code;
}

static int write_xml_flow_settings(pugi::xml_node& root_node, const TapeoutManager& to_mgr,
                                   const bool& keep_reserved_word) {
  int err_code = 0;

  pugi::xml_node flset_node = root_node.append_child(XML_TAPEOUT_FLOWSET_NODE_NAME);
  err_code += write_xml_flow_settings_synth(flset_node, to_mgr, keep_reserved_word);
  err_code += write_xml_flow_settings_floorplan(flset_node, to_mgr, keep_reserved_word);
  err_code += write_xml_flow_settings_qorso(flset_node, to_mgr, keep_reserved_word);

  return err_code;
}

/********************************************************************
 * A writer to output a strategy manager object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_tapeout_manager_arch_template(const char* fname, const TapeoutManager& to_mgr,
                                            const bool& keep_reserved_word) {
  vtr::ScopedStartFinishTimer timer("Write arch template data to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_TAPEOUT_ARCH_ROOT_NODE_NAME);

  /* Output flow settings */
  err_code += write_xml_flow_settings(root_node, to_mgr, keep_reserved_word);

  /* Output arch templates */
  pugi::xml_node arch_node = root_node.append_child(XML_TAPEOUT_ARCH_NODE_NAME);
  arch_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_W) =
    std::to_string(to_mgr.route_chan_width()).c_str();

  pugi::xml_node arch_gen_node = arch_node.append_child(XML_TAPEOUT_ARCH_ARCH_GENERATOR_NODE_NAME);
  for (std::string ftype : to_mgr.arch_gen_file_types()) {
    pugi::xml_node file_node = arch_gen_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) = ftype.c_str();
    file_node.text().set(to_mgr.raw_arch_gen_bin_path(ftype).c_str());
  }

  pugi::xml_node of_stats_conf_node =
    arch_node.append_child(XML_TAPEOUT_ARCH_OPENFPGA_STATS_CONF_NODE_NAME);
  of_stats_conf_node.text().set(to_mgr.openfpga_stats_conf_file(true).c_str());

  pugi::xml_node of_error_conf_node =
    arch_node.append_child(XML_TAPEOUT_ARCH_OPENFPGA_ERROR_CONF_NODE_NAME);
  of_error_conf_node.text().set(to_mgr.openfpga_error_conf_file(true).c_str());

  pugi::xml_node ys_error_conf_node =
    arch_node.append_child(XML_TAPEOUT_ARCH_YOSYS_ERROR_CONF_NODE_NAME);
  ys_error_conf_node.text().set(to_mgr.yosys_error_conf_file(true).c_str());

  /* For arch timing */
  pugi::xml_node arch_timing_node = arch_node.append_child(XML_TAPEOUT_ARCH_TIMING_NODE_NAME);
  arch_timing_node.append_attribute(XML_TAPEOUT_ARCH_TIMING_ATTRIBUTE_PDK_TEMPLATE) =
    to_mgr.arch_timing_template_pdk().c_str();
  arch_timing_node.append_attribute(XML_TAPEOUT_ARCH_TIMING_ATTRIBUTE_FILE_TEMPLATE) =
    to_mgr.arch_timing_template_file().c_str();
  for (std::string fpdk : to_mgr.arch_timing_file_pdks()) {
    pugi::xml_node fpdk_node = arch_timing_node.append_child(XML_TAPEOUT_ARCH_TIMING_PDK_NODE_NAME);
    fpdk_node.append_attribute(XML_TAPEOUT_ARCH_TIMING_PDK_ATTRIBUTE_NAME) = fpdk.c_str();
    for (std::string ftype : to_mgr.arch_timing_file_types(fpdk)) {
      pugi::xml_node file_node = fpdk_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
      file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) = ftype.c_str();
      std::string fname = to_mgr.arch_timing_file_by_type(fpdk, ftype, keep_reserved_word);
      file_node.text().set(fname.c_str());
    }
  }

  /* For dp arch */
  pugi::xml_node dp_arch_node = arch_node.append_child(XML_TAPEOUT_ARCH_DP_NODE_NAME);
  for (TapeoutManager::e_arch_file_type ftype : to_mgr.arch_file_types()) {
    pugi::xml_node file_node = dp_arch_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) =
      to_mgr.arch_file_type2str(ftype).c_str();
    std::string fname = to_mgr.dp_arch_file(ftype, keep_reserved_word);
    file_node.text().set(fname.c_str());
  }
  /* For ultimate arch */
  pugi::xml_node ut_arch_node = arch_node.append_child(XML_TAPEOUT_ARCH_ULTIMATE_NODE_NAME);
  for (TapeoutManager::e_arch_file_type ftype : to_mgr.arch_file_types()) {
    pugi::xml_node file_node = ut_arch_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) =
      to_mgr.arch_file_type2str(ftype).c_str();
    std::string fname = to_mgr.ultimate_arch_file(ftype, keep_reserved_word);
    file_node.text().set(fname.c_str());
  }
  /* For openfpga settings */
  pugi::xml_node ofsetting_node =
    arch_node.append_child(XML_TAPEOUT_ARCH_OPENFPGASETTING_NODE_NAME);
  for (TapeoutManager::e_arch_file_type ftype : to_mgr.openfpga_setting_file_types()) {
    pugi::xml_node file_node = ofsetting_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) =
      to_mgr.openfpga_setting_file_type2str(ftype).c_str();
    std::string fname = to_mgr.openfpga_setting_file(ftype, keep_reserved_word);
    file_node.text().set(fname.c_str());
  }
  /* For openfpga shell scripts */
  pugi::xml_node ofscript_node = arch_node.append_child(XML_TAPEOUT_ARCH_OPENFPGASCRIPT_NODE_NAME);
  for (TapeoutManager::e_script_type ftype : to_mgr.openfpga_script_file_types()) {
    pugi::xml_node file_node = ofscript_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) =
      to_mgr.script_type2str(ftype).c_str();
    std::string fname = to_mgr.openfpga_script_file(ftype, keep_reserved_word);
    file_node.text().set(fname.c_str());
  }
  /* For yosys scripts */
  pugi::xml_node ysscript_node = arch_node.append_child(XML_TAPEOUT_ARCH_YOSYSSCRIPT_NODE_NAME);
  for (TapeoutManager::e_script_type ftype : to_mgr.yosys_script_file_types()) {
    pugi::xml_node file_node = ysscript_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) =
      to_mgr.script_type2str(ftype).c_str();
    std::string fname = to_mgr.yosys_script_file(ftype, keep_reserved_word);
    file_node.text().set(fname.c_str());
  }
  /* For netlist flags */
  pugi::xml_node flags_node = arch_node.append_child(XML_TAPEOUT_NETLIST_FLAGS_NODE_NAME);
  for (auto pdk_name : to_mgr.netlist_flags_pdk_names()) {
    pugi::xml_node flags_pdk_node =
      flags_node.append_child(XML_TAPEOUT_NETLIST_FLAGS_PDK_NODE_NAME);
    flags_pdk_node.append_attribute(XML_TAPEOUT_NETLIST_FLAGS_PDK_NODE_NAME) = pdk_name.c_str();
    for (auto f_name : to_mgr.netlist_flags(pdk_name)) {
      pugi::xml_node flag_node =
        flags_pdk_node.append_child(XML_TAPEOUT_NETLIST_FLAGS_FLAG_NODE_NAME);
      flag_node.append_attribute(XML_TAPEOUT_NETLIST_FLAGS_FLAG_ATTRIBUTE_NAME) = f_name.c_str();
      std::string f_val = to_mgr.flag_value(pdk_name, f_name);
      flag_node.text().set(f_val.c_str());
    }
  }

  /* For ips */
  pugi::xml_node ip_node = arch_node.append_child(XML_TAPEOUT_ARCH_IP_NODE_NAME);
  for (auto f_id : to_mgr.ip_files()) {
    pugi::xml_node file_node = ip_node.append_child(XML_TAPEOUT_ARCH_FILE_NODE_NAME);
    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_TYPE) =
      to_mgr.ip_file_type_str(f_id).c_str();

    file_node.append_attribute(XML_TAPEOUT_ARCH_ATTRIBUTE_FORMAT) =
      to_mgr.ip_file_format_str(f_id).c_str();

    std::string fname = to_mgr.ip_file(f_id, keep_reserved_word);
    file_node.text().set(fname.c_str());
  }

  /* For netlist makeup key root */
  pugi::xml_node nlist_makeup_key_node =
    arch_node.append_child(XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_NODE_NAME);
  nlist_makeup_key_node.append_attribute(XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_ATTRIBUTE_ROOT) =
    to_mgr.netlist_makeup_key_root_path(keep_reserved_word).c_str();
  for (auto pdk_name : to_mgr.ccff_makeup_pdk_names()) {
    pugi::xml_node cm_pdk_node =
      flags_node.append_child(XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_NODE_NAME);
    cm_pdk_node.append_attribute(XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_ATTRIBUTE_NAME) =
      pdk_name.c_str();

    if (to_mgr.pdk_require_netlist_makeup_config(pdk_name)) {
      pugi::xml_node nm_cfg_node = cm_pdk_node.append_child(
        XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_NETLIST_MAKEUP_CONFIG_NODE_NAME);
      std::string f_val = to_mgr.netlist_makeup_config_file(pdk_name, false);
      nm_cfg_node.text().set(f_val.c_str());
    }
    pugi::xml_node cm_cfg_node = cm_pdk_node.append_child(
      XML_TAPEOUT_ARCH_NETLIST_MAKEUP_KEY_PDK_CCFF_MAKEUP_BUF_CONFIG_NODE_NAME);
    std::string f_val = to_mgr.ccff_makeup_buf_config_file(pdk_name, false);
    cm_cfg_node.text().set(f_val.c_str());
  }

  out_xml.save_file(fname);

  return err_code;
}

}  // End of namespace arkangel
