#include "vpr_log_parser.h"

#include <fstream>
#include <regex>

#include "arch_error.h"
#include "command_exit_codes.h"
#include "openfpga_scale.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "read_xml_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arcxpress {  // Begin namespace arcxpress

VprLogParser::VprLogParser() {
  clear();
  XML_ROOT_NODE_ = "keywords";
  XML_KW_NODE_ = "keyword";
  XML_KW_ATTR_NAME_ = "name";
  XML_KW_REGEX_NODE_ = "regex";
  XML_KW_ATTR_TYPE_ = "type";
  XML_KW_ATTR_DATA_TYPE_ = "data_type";
  KW_TYPE_STRING_ = {"int", "float", "scientific"};
  KW_DATA_TYPE_STRING_ = {"critical_path_delay"};
}

int VprLogParser::load_keywords_from_xml(const std::string& fname, const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read keywords File");
  /* A clean start */
  clear_keywords();
  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);
    pugi::xml_node xml_root = get_single_child(doc, XML_ROOT_NODE_, loc_data);
    /* Parse keyword one by one */
    for (pugi::xml_node xml_kw : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_kw.name() != std::string(XML_KW_NODE_)) {
        bad_tag(xml_root, loc_data, xml_kw, {XML_KW_NODE_});
      }
      std::string name = get_attribute(xml_kw, XML_KW_ATTR_NAME_, loc_data).as_string();
      std::string type_str = get_attribute(xml_kw, XML_KW_ATTR_TYPE_, loc_data).as_string();
      kw_types_[name] = str2keyword_type(type_str, true);
      std::string data_type_str =
        get_attribute(xml_kw, XML_KW_ATTR_DATA_TYPE_, loc_data, pugiutil::ReqOpt::OPTIONAL)
          .as_string();
      if (!data_type_str.empty()) {
        kw_data_types_[name] = str2data_type(data_type_str, true);
      } else {
        kw_data_types_[name] = e_data_type::NUM_TYPES;
      }
      /* Get regular expression data */
      pugi::xml_node xml_regex = get_single_child(xml_kw, XML_KW_REGEX_NODE_, loc_data);
      kw_regex_[name] = std::string(xml_regex.child_value());
      VTR_LOGV(verbose, " Keyword: %s, Type: %s, Regex: %s\n", name.c_str(), type_str.c_str(),
               kw_regex_[name].c_str());
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname.c_str(), e.line(), "%s", e.what());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  kw_is_valid_ = true;
  return openfpga::CMD_EXEC_SUCCESS;
}

std::vector<std::string> VprLogParser::keywords() const {
  check_keyword_valid();
  std::vector<std::string> ret;
  for (auto pair : stats_) {
    ret.push_back(pair.first);
  }
  return ret;
}

bool VprLogParser::find_critical_path_delay_keyword(std::string& kw) const {
  check_keyword_valid();
  std::vector<std::string> ret;
  for (auto pair : stats_) {
    if (kw_data_types_.at(pair.first) == e_data_type::CPD) {
      ret.push_back(pair.first);
    }
  }
  if (ret.size() == 1) {
    kw = ret[0];
    return true;
  }
  return false;
}

float VprLogParser::value(const std::string& kw) const {
  check_valid();
  auto result = stats_.find(kw);
  if (result == stats_.end()) {
    VTR_LOG_ERROR(
      "Invalid keyword '%s' which is not defined! Please double check your keyword configuration "
      "file\n",
      kw.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return result->second;
}

void VprLogParser::check_valid() const {
  if (!kw_is_valid_ || !result_is_valid_) {
    VTR_LOG_ERROR("VPR log data is invalid!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void VprLogParser::check_keyword_valid() const {
  if (!kw_is_valid_) {
    VTR_LOG_ERROR("Keywords of VPR log data is invalid!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void VprLogParser::check_result_valid() const {
  if (!result_is_valid_) {
    VTR_LOG_ERROR("Results of VPR log data is invalid!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void VprLogParser::clear_keywords() {
  kw_regex_.clear();
  kw_types_.clear();
  kw_data_types_.clear();
  kw_is_valid_ = false;
}

void VprLogParser::clear_results() {
  stats_.clear();
  result_is_valid_ = false;
}

void VprLogParser::clear() {
  clear_keywords();
  clear_results();
}

void VprLogParser::check_valid_keyword_type(const VprLogParser::e_keyword_type& type) const {
  if (!valid_keyword_type(type)) {
    VTR_LOG_ERROR("keyword type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

VprLogParser::e_keyword_type VprLogParser::str2keyword_type(const std::string& str,
                                                            const bool& verbose) const {
  for (int itype = size_t(VprLogParser::e_keyword_type::INT);
       itype < size_t(VprLogParser::e_keyword_type::NUM_TYPES); ++itype) {
    if (str == std::string(KW_TYPE_STRING_[itype])) {
      return static_cast<VprLogParser::e_keyword_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for keyword! Expect %s\n", keyword_type_all2str().c_str());
  return VprLogParser::e_keyword_type::NUM_TYPES;
}

std::string VprLogParser::keyword_type2str(const VprLogParser::e_keyword_type& type,
                                           const bool& verbose) const {
  if (!valid_keyword_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for keyword! Expect %s\n",
                   keyword_type_all2str().c_str());
    return std::string();
  }
  return std::string(KW_TYPE_STRING_[size_t(type)]);
}

std::string VprLogParser::keyword_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(VprLogParser::e_keyword_type::INT);
       itype < size_t(VprLogParser::e_keyword_type::NUM_TYPES); ++itype) {
    full_types += std::string(KW_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool VprLogParser::valid_keyword_type(const e_keyword_type& type) const {
  return type != VprLogParser::e_keyword_type::NUM_TYPES;
}

void VprLogParser::check_valid_data_type(const VprLogParser::e_data_type& type) const {
  if (!valid_data_type(type)) {
    VTR_LOG_ERROR("data type is not valid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

VprLogParser::e_data_type VprLogParser::str2data_type(const std::string& str,
                                                      const bool& verbose) const {
  for (int itype = size_t(VprLogParser::e_data_type::CPD);
       itype < size_t(VprLogParser::e_data_type::NUM_TYPES); ++itype) {
    if (str == std::string(KW_DATA_TYPE_STRING_[itype])) {
      return static_cast<VprLogParser::e_data_type>(itype);
    }
  }
  VTR_LOGV_ERROR(verbose, "Invalid type for data! Expect %s\n", data_type_all2str().c_str());
  return VprLogParser::e_data_type::NUM_TYPES;
}

std::string VprLogParser::data_type2str(const VprLogParser::e_data_type& type,
                                        const bool& verbose) const {
  if (!valid_data_type(type)) {
    VTR_LOGV_ERROR(verbose, "Invalid type for data! Expect %s\n", data_type_all2str().c_str());
    return std::string();
  }
  return std::string(KW_TYPE_STRING_[size_t(type)]);
}

std::string VprLogParser::data_type_all2str() const {
  std::string full_types = "[";
  for (int itype = size_t(VprLogParser::e_data_type::CPD);
       itype < size_t(VprLogParser::e_data_type::NUM_TYPES); ++itype) {
    full_types += std::string(KW_DATA_TYPE_STRING_[itype]) + std::string("|");
  }
  full_types.pop_back();
  full_types += "]";
  return full_types;
}

bool VprLogParser::valid_data_type(const e_data_type& type) const {
  return type != VprLogParser::e_data_type::NUM_TYPES;
}

int VprLogParser::parse(const std::string& fname, const bool& verbose) {
  std::ifstream file(fname);
  std::string line;

  if (file.is_open()) {
    while (std::getline(file, line)) {
      /* For each line, go through all the regex */
      for (auto pair : kw_regex_) {
        std::regex pattern(pair.second);
        std::smatch match;
        if (std::regex_search(line, match, pattern)) {
          /* Depending on the types of the data, the matching number will vary */
          switch (kw_types_[pair.first]) {
            case e_keyword_type::INT:
            case e_keyword_type::FLOAT: {
              VTR_LOGV(verbose, "Match: %s\n", match[1].str().c_str());
              stats_[pair.first] = std::stof(match[1].str());
              break;
            }
            case e_keyword_type::SCIENTIFIC: {
              float digits = std::stof(match[1].str());
              std::string unit = match[2].str();
              stats_[pair.first] = digits * openfpga::string_to_unit(unit);
              break;
            }
            default:
              VTR_LOG_ERROR("Invalid keyword type!\n");
              return openfpga::CMD_EXEC_FATAL_ERROR;
          }
        }
      }
    }
    file.close();
  } else {
    VTR_LOG_ERROR("Unable to open openfpga log file '%s'\n", fname.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  result_is_valid_ = true;
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arcxpress
