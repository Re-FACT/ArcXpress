/********************************************************************
 * This file includes functions that outputs a bus group object to XML format
 *******************************************************************/
/* Headers from system goes first */
#include <algorithm>
#include <filesystem>
#include <string>

/* Headers from vtr util library */
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

/* Headers from pin constraint library */
#include "openfpga_digest.h"
#include "pdk_manager_xml_constants.h"
#include "pugixml.hpp"
#include "verilog_constants.h"
#include "write_xml_pdk_manager.h"

namespace arkangel {  // Begin namespace arkangel

/********************************************************************
 * Only the time stamp of the first file that is read in will be outputted!
 * TODO: Consider to aggregate all the authors. Use the earliest created date.
 * Use the latest modified date.
 *******************************************************************/
static int write_xml_pdk_stamps(pugi::xml_node& root_node, const PdkManager& pdk_mgr) {
  int err_code = 0;
  pugi::xml_node stmp_node = root_node.append_child(XML_PDK_STAMP_ROOT_NODE_NAME);

  pugi::xml_node author_node = stmp_node.append_child(XML_PDK_AUTHOR_NODE_NAME);
  author_node.text().set(pdk_mgr.author().c_str());

  pugi::xml_node dates_node = stmp_node.append_child(XML_PDK_DATE_NODE_NAME);
  pugi::xml_node created_node = dates_node.append_child(XML_PDK_CREATE_DATE_NODE_NAME);
  created_node.text().set(pdk_mgr.date_created().c_str());

  pugi::xml_node modified_node = dates_node.append_child(XML_PDK_LAST_MODIFIED_DATE_NODE_NAME);
  modified_node.text().set(pdk_mgr.date_last_modified().c_str());

  pugi::xml_node ver_node = stmp_node.append_child(XML_PDK_TOOL_VERSION_NODE_NAME);
  ver_node.text().set(pdk_mgr.tool_version().c_str());

  return err_code;
}

static int write_xml_sclib_file(pugi::xml_node& root_node, const PdkManager& pdk_mgr,
                                const SclibId& sclib_id, const SclibFileId& f_id) {
  int err_code = 0;
  pugi::xml_node f_node = root_node.append_child(XML_PDK_FILE_NODE_NAME);
  f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_TYPE) =
    pdk_mgr.sclib_file_type_str(sclib_id, f_id).c_str();
  if (!pdk_mgr.sclib_file_process(sclib_id, f_id).empty()) {
    f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_PROCESS) =
      pdk_mgr.sclib_file_process(sclib_id, f_id).c_str();
  }
  if (!pdk_mgr.sclib_file_vdd(sclib_id, f_id).empty()) {
    f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_VDD) =
      pdk_mgr.sclib_file_vdd(sclib_id, f_id).c_str();
  }
  if (!pdk_mgr.sclib_file_temperature(sclib_id, f_id).empty()) {
    f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_TEMP) =
      pdk_mgr.sclib_file_temperature(sclib_id, f_id).c_str();
  }
  if (!pdk_mgr.sclib_file_model_type(sclib_id, f_id).empty()) {
    f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_MODEL) =
      pdk_mgr.sclib_file_model_type(sclib_id, f_id).c_str();
  }
  if (!pdk_mgr.sclib_file_tag(sclib_id, f_id).empty()) {
    f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_TAG) =
      pdk_mgr.sclib_file_tag(sclib_id, f_id).c_str();
  }
  f_node.text().set(pdk_mgr.sclib_file_path(sclib_id, f_id).c_str());

  return err_code;
}

static int write_xml_sclib_file_for_synth(pugi::xml_node& root_node, const PdkManager& pdk_mgr,
                                          const SclibId& sclib_id, const SclibFileId& f_id) {
  int err_code = 0;

  if (pdk_mgr.sclib_file_type(sclib_id, f_id) != PdkManager::e_file_type::LIB) {
    return err_code;
  }

  pugi::xml_node f_node = root_node.append_child(XML_PDK_FILE_NODE_NAME);
  f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_TAG) =
    pdk_mgr.sclib_file_tag(sclib_id, f_id).c_str();
  f_node.text().set(pdk_mgr.sclib_file_path(sclib_id, f_id).c_str());

  return err_code;
}

static int write_xml_tech_file(pugi::xml_node& root_node, const PdkManager& pdk_mgr,
                               const TechfileId& tf_id) {
  int err_code = 0;
  pugi::xml_node f_node = root_node.append_child(XML_PDK_FILE_NODE_NAME);
  f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_TYPE) = pdk_mgr.techfile_type_str(tf_id).c_str();
  if (!pdk_mgr.techfile_metal_stack(tf_id).empty()) {
    f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_METALSTACK) =
      pdk_mgr.techfile_metal_stack(tf_id).c_str();
  }
  if (!pdk_mgr.techfile_corner(tf_id).empty()) {
    f_node.append_attribute(XML_PDK_FILE_ATTRIBUTE_CORNER) = pdk_mgr.techfile_corner(tf_id).c_str();
  }
  f_node.text().set(pdk_mgr.techfile_path(tf_id).c_str());

  return err_code;
}

static int write_xml_design_naming_rules(pugi::xml_node& root_node) {
  int err_code = 0;
  pugi::xml_node dsgn_node = root_node.append_child(XML_PDK_NODE_DESIGN);
  pugi::xml_node dsgn_tb_naming_node =
    dsgn_node.append_child(XML_PDK_NODE_DESIGN_TB_MODULE_POSTFIX);
  dsgn_tb_naming_node.text().set(FORMAL_VERIFICATION_TOP_MODULE_POSTFIX);
  pugi::xml_node dsgn_dut_naming_node =
    dsgn_node.append_child(XML_PDK_NODE_DESIGN_DUT_MODULE_POSTFIX);
  dsgn_dut_naming_node.text().set(FORMAL_VERIFICATION_TOP_MODULE_UUT_NAME);

  return err_code;
}

static int write_xml_corner_map(pugi::xml_node& root_node,
                                const std::map<std::string, std::string>& corner_map) {
  int err_code = 0;
  pugi::xml_node corners_node = root_node.append_child(XML_PDK_NODE_RCCORNERS);
  for (auto result : corner_map) {
    pugi::xml_node corner_node = corners_node.append_child(XML_PDK_NODE_RCCORNERS_CORNER);
    corner_node.append_attribute(XML_PDK_NODE_RCCORNERS_CORNER_ATTRIBUTE_TAG) =
      result.first.c_str();
    corner_node.text().set(result.second.c_str());
  }

  return err_code;
}

/********************************************************************
 * A writer to output a PDK manager object to XML format
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_pdk_manager(const char* fname, const PdkManager& pdk_mgr) {
  vtr::ScopedStartFinishTimer timer("Write pdk data to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_PDK_ROOT_NODE_NAME);
  root_node.append_attribute(XML_PDK_ATTRIBUTE_NAME) = pdk_mgr.name().c_str();

  /* Output time stamp */
  err_code += write_xml_pdk_stamps(root_node, pdk_mgr);

  /* Output root path */
  pugi::xml_node rootpath_node = root_node.append_child(XML_PDK_NODE_ROOT);
  rootpath_node.text().set(pdk_mgr.root().c_str());

  /* Output sclibs */
  pugi::xml_node sclibs_node = root_node.append_child(XML_PDK_NODE_SCLIBS);
  for (SclibId sclib_id : pdk_mgr.sclibs()) {
    pugi::xml_node sclib_node = sclibs_node.append_child(XML_PDK_SCLIBS_NODE_SCLIB);
    sclib_node.append_attribute(XML_PDK_SCLIB_ATTRIBUTE_NAME) =
      pdk_mgr.sclib_name(sclib_id).c_str();
    sclib_node.append_attribute(XML_PDK_SCLIB_ATTRIBUTE_HEIGHT) =
      pdk_mgr.sclib_cell_height(sclib_id);
    sclib_node.append_attribute(XML_PDK_SCLIB_ATTRIBUTE_CPP) = pdk_mgr.sclib_cpp(sclib_id);
    for (SclibFileId f_id : pdk_mgr.sclib_files(sclib_id)) {
      err_code += write_xml_sclib_file(sclib_node, pdk_mgr, sclib_id, f_id);
    }
  }

  /* Output techfile */
  pugi::xml_node tf_node = root_node.append_child(XML_PDK_NODE_TECHFILE);
  for (TechfileId tf_id : pdk_mgr.techfiles()) {
    err_code += write_xml_tech_file(tf_node, pdk_mgr, tf_id);
  }

  out_xml.save_file(fname);

  return err_code;
}

/********************************************************************
 * A writer to output a PDK configuration file for synthesis purpose
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_pdk_config_for_synth(const char* fname, const PdkManager& pdk_mgr) {
  vtr::ScopedStartFinishTimer timer("Write synthesis-oriented pdk data to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_PDK_ROOT_NODE_NAME);

  /* Output root path */
  pugi::xml_node rootpath_node = root_node.append_child(XML_PDK_NODE_ROOT);
  rootpath_node.text().set(pdk_mgr.root().c_str());

  /* Output sclibs */
  pugi::xml_node sclibs_node = root_node.append_child(XML_PDK_NODE_SCLIBS);
  for (SclibId sclib_id : pdk_mgr.sclibs()) {
    pugi::xml_node sclib_node = sclibs_node.append_child(XML_PDK_SCLIBS_NODE_SCLIB);
    sclib_node.append_attribute(XML_PDK_SCLIB_ATTRIBUTE_NAME) =
      pdk_mgr.sclib_name(sclib_id).c_str();
    /* Only output lib files with tag attributes */
    for (SclibFileId f_id : pdk_mgr.sclib_files(sclib_id)) {
      err_code += write_xml_sclib_file_for_synth(sclib_node, pdk_mgr, sclib_id, f_id);
    }
  }

  std::filesystem::path fpath = std::string(fname);
  openfpga::create_directory(fpath.parent_path().string(), true, false);
  out_xml.save_file(fname);

  return err_code;
}

/********************************************************************
 * A writer to output a PDK configuration file for synthesis purpose
 *
 * Return 0 if successful
 * Return 1 if there are more serious bugs in the architecture
 * Return 2 if fail when creating files
 *******************************************************************/
int write_xml_pdk_config_for_qor_extraction(const char* fname, const PdkManager& pdk_mgr,
                                            const std::map<std::string, std::string>& corner_map) {
  vtr::ScopedStartFinishTimer timer("Write QoR-extraction-oriented pdk data to file");

  int err_code = 0;

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_PDK_ROOT_NODE_NAME);

  /* Output root path */
  pugi::xml_node rootpath_node = root_node.append_child(XML_PDK_NODE_ROOT);
  rootpath_node.text().set(pdk_mgr.root().c_str());

  pugi::xml_node sfac_node = root_node.append_child(XML_PDK_NODE_SHRINK_FACTOR);
  sfac_node.text().set(std::to_string(pdk_mgr.shrink_factor()).c_str());

  /* Output sclibs */
  pugi::xml_node sclibs_node = root_node.append_child(XML_PDK_NODE_SCLIBS);
  for (SclibId sclib_id : pdk_mgr.sclibs()) {
    pugi::xml_node sclib_node = sclibs_node.append_child(XML_PDK_SCLIBS_NODE_SCLIB);
    sclib_node.append_attribute(XML_PDK_SCLIB_ATTRIBUTE_NAME) =
      pdk_mgr.sclib_name(sclib_id).c_str();
    /* Only output lib files with tag attributes */
    for (SclibFileId f_id : pdk_mgr.sclib_files(sclib_id)) {
      err_code += write_xml_sclib_file_for_synth(sclib_node, pdk_mgr, sclib_id, f_id);
    }
  }
  /* TODO: Output IP libs if possible */
  /* Design naming rules */
  err_code += write_xml_design_naming_rules(root_node);
  /* Corner tag mapping rules */
  err_code += write_xml_corner_map(root_node, corner_map);

  std::filesystem::path fpath = std::string(fname);
  openfpga::create_directory(fpath.parent_path().string(), true, false);
  out_xml.save_file(fname);

  return err_code;
}

}  // End of namespace arkangel
