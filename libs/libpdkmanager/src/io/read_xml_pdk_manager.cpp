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
#include "pdk_manager_xml_constants.h"
#include "pugixml_util.hpp"
#include "read_xml_pdk_manager.h"
#include "read_xml_util.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

/********************************************************************
 * Parse XML codes of a <stamp> to an object of ProjectManager
 *******************************************************************/
static void read_xml_stamp(pugi::xml_node& xml_stamp, const pugiutil::loc_data& loc_data,
                           PdkManager& pdk_mgr, const std::string& fname) {
  pugi::xml_node xml_author = get_single_child(xml_stamp, XML_PDK_AUTHOR_NODE_NAME, loc_data);
  std::string author_str = std::string(xml_author.child_value());

  pugi::xml_node xml_dates = get_single_child(xml_stamp, XML_PDK_DATE_NODE_NAME, loc_data);
  pugi::xml_node xml_created_date =
    get_single_child(xml_dates, XML_PDK_CREATE_DATE_NODE_NAME, loc_data);
  std::string date_created_str = std::string(xml_created_date.child_value());

  pugi::xml_node xml_last_modified_date =
    get_single_child(xml_dates, XML_PDK_LAST_MODIFIED_DATE_NODE_NAME, loc_data);
  std::string date_last_modified_str = std::string(xml_last_modified_date.child_value());

  pugi::xml_node xml_tool_version =
    get_single_child(xml_stamp, XML_PDK_TOOL_VERSION_NODE_NAME, loc_data);
  std::string tool_version_str = std::string(xml_tool_version.child_value());

  pdk_mgr.set_author(author_str);
  pdk_mgr.set_date_created(date_created_str);
  pdk_mgr.set_date_last_modified(date_last_modified_str);
  pdk_mgr.set_tool_version(tool_version_str);
}

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_pdk_sclib_file(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                                   PdkManager& pdk_mgr, const SclibId& sclib_id) {
  for (pugi::xml_node xml_source : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_source.name() != std::string(XML_PDK_FILE_NODE_NAME)) {
      bad_tag(xml_root, loc_data, xml_source, {XML_PDK_FILE_NODE_NAME});
    }
    std::string ftype =
      get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string fname = std::string(xml_source.child_value());
    SclibFileId f_id = pdk_mgr.add_sclib_file(sclib_id, ftype, fname);
    /* Add other properties */
    std::string process_str = get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_PROCESS, loc_data,
                                            pugiutil::ReqOpt::OPTIONAL)
                                .as_string();
    pdk_mgr.set_sclib_file_process(sclib_id, f_id, process_str);
    std::string vdd_str =
      get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_VDD, loc_data, pugiutil::ReqOpt::OPTIONAL)
        .as_string();
    pdk_mgr.set_sclib_file_vdd(sclib_id, f_id, vdd_str);
    std::string temp_str =
      get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_TEMP, loc_data, pugiutil::ReqOpt::OPTIONAL)
        .as_string();
    pdk_mgr.set_sclib_file_temperature(sclib_id, f_id, temp_str);
    std::string model_str =
      get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_MODEL, loc_data, pugiutil::ReqOpt::OPTIONAL)
        .as_string();
    pdk_mgr.set_sclib_file_model_type(sclib_id, f_id, model_str);
    std::string tag_str =
      get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_TAG, loc_data, pugiutil::ReqOpt::OPTIONAL)
        .as_string();
    pdk_mgr.set_sclib_file_tag(sclib_id, f_id, tag_str);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_techfile(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                             PdkManager& pdk_mgr) {
  for (pugi::xml_node xml_source : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_source.name() != std::string(XML_PDK_FILE_NODE_NAME)) {
      bad_tag(xml_root, loc_data, xml_source, {XML_PDK_FILE_NODE_NAME});
    }
    std::string ftype =
      get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_TYPE, loc_data).as_string();
    std::string fname = std::string(xml_source.child_value());
    TechfileId f_id = pdk_mgr.add_techfile(ftype, fname);
    /* Add other properties */
    std::string m_stack_str = get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_METALSTACK, loc_data,
                                            pugiutil::ReqOpt::OPTIONAL)
                                .as_string();
    pdk_mgr.set_techfile_metal_stack(f_id, m_stack_str);
    std::string corner_str =
      get_attribute(xml_source, XML_PDK_FILE_ATTRIBUTE_CORNER, loc_data, pugiutil::ReqOpt::OPTIONAL)
        .as_string();
    pdk_mgr.set_techfile_corner(f_id, corner_str);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes of a <device> to an object of ProjectManager
 *******************************************************************/
static int read_xml_sclib(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                          PdkManager& pdk_mgr) {
  std::string name = get_attribute(xml_root, XML_PDK_SCLIB_ATTRIBUTE_NAME, loc_data).as_string();
  SclibId sclib_id = pdk_mgr.create_sclib(name);

  float height = get_attribute(xml_root, XML_PDK_SCLIB_ATTRIBUTE_HEIGHT, loc_data).as_float();
  pdk_mgr.set_sclib_cell_height(sclib_id, height);
  float cpp = get_attribute(xml_root, XML_PDK_SCLIB_ATTRIBUTE_CPP, loc_data).as_float();
  pdk_mgr.set_sclib_cpp(sclib_id, cpp);

  return read_xml_pdk_sclib_file(xml_root, loc_data, pdk_mgr, sclib_id);
}

static int read_xml_pdk_common_parameters(pugi::xml_node& xml_root,
                                          const pugiutil::loc_data& loc_data, PdkManager& pdk_mgr) {
  /* Parse root path */
  pugi::xml_node xml_root_path = get_single_child(xml_root, XML_PDK_NODE_ROOT, loc_data);
  std::string root_path = std::string(xml_root_path.child_value());
  pdk_mgr.set_root(root_path);

  /* Parse shrink factor, optional */
  pugi::xml_node xml_sfac =
    get_single_child(xml_root, XML_PDK_NODE_SHRINK_FACTOR, loc_data, pugiutil::ReqOpt::OPTIONAL);
  if (xml_sfac) {
    float sfac = std::stof(std::string(xml_sfac.child_value()));
    pdk_mgr.set_shrink_factor(sfac);
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_pdk_manager(const char* fname, PdkManager& pdk_mgr) {
  vtr::ScopedStartFinishTimer timer("Read PDK file");
  /* Check file existence */
  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File does not exist at location: %s!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  size_t num_err = 0;

  pdk_mgr.clear();

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_PDK_ROOT_NODE_NAME, loc_data);

    /* Parse name */
    pdk_mgr.set_name(get_attribute(xml_root, XML_PDK_ATTRIBUTE_NAME, loc_data).as_string());

    /* Parse stamps */
    pugi::xml_node xml_stamp_root =
      get_single_child(xml_root, XML_PDK_STAMP_ROOT_NODE_NAME, loc_data);
    read_xml_stamp(xml_stamp_root, loc_data, pdk_mgr, std::string(fname));

    int status = read_xml_pdk_common_parameters(xml_root, loc_data, pdk_mgr);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }

    /* Parse std cell libs */
    pugi::xml_node xml_sclibs = get_single_child(xml_root, XML_PDK_NODE_SCLIBS, loc_data);
    for (pugi::xml_node xml_sclib : xml_sclibs.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_sclib.name() == std::string(XML_PDK_SCLIBS_NODE_SCLIB)) {
        status = read_xml_sclib(xml_sclib, loc_data, pdk_mgr);
        if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
          num_err++;
        }
      }
    }
    /* Parse tech files */
    pugi::xml_node xml_tf = get_single_child(xml_root, XML_PDK_NODE_TECHFILE, loc_data);
    status = read_xml_techfile(xml_tf, loc_data, pdk_mgr);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
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

}  // End of namespace arkangel
