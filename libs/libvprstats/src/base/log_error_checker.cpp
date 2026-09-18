#include "log_error_checker.h"

#include <fstream>
#include <iostream>

#include "arch_error.h"
#include "command_exit_codes.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "read_xml_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arkangel {  // Begin namespace arkangel

LogErrorChecker::LogErrorChecker() {
  clear();
  XML_ROOT_NODE_ = "keywords";
  XML_KW_NODE_ = "keyword";
  XML_KW_ATTR_NAME_ = "name";
  XML_KW_ATTR_IGNORECASE_ = "ignore_case";
  XML_KW_REGEX_NODE_ = "regex";
}

int LogErrorChecker::load_keywords_from_xml(const std::string& fname, const bool& verbose) {
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
      bool icase =
        get_attribute(xml_kw, XML_KW_ATTR_IGNORECASE_, loc_data, pugiutil::ReqOpt::OPTIONAL)
          .as_bool(false);
      /* Get regular expression data */
      pugi::xml_node xml_regex = get_single_child(xml_kw, XML_KW_REGEX_NODE_, loc_data);
      kw_regex_[name] = std::string(xml_regex.child_value());
      kw_ignore_case_[name] = icase;
      VTR_LOGV(verbose, " Keyword: %s, Regex: %s\n", name.c_str(), kw_regex_[name].c_str());
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname.c_str(), e.line(), "%s", e.what());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  is_valid_ = true;

  return openfpga::CMD_EXEC_SUCCESS;
}

std::vector<std::string> LogErrorChecker::error_messages() const {
  check_valid();
  std::vector<std::string> ret;
  for (size_t ierr = 0; ierr < err_lines_.size(); ++ierr) {
    std::string err_line =
      std::string("[LINE " + std::to_string(err_line_no_[ierr]) + "] " + err_lines_[ierr]);
    ret.push_back(err_line);
  }
  return ret;
}

size_t LogErrorChecker::num_errors() const {
  check_valid();
  return err_lines_.size();
}

void LogErrorChecker::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("Log data is invalid!\n");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void LogErrorChecker::clear_keywords() {
  kw_regex_.clear();
  kw_ignore_case_.clear();
}

void LogErrorChecker::clear_results() {
  err_lines_.clear();
  err_line_no_.clear();
  is_valid_ = false;
}

void LogErrorChecker::clear() {
  clear_keywords();
  clear_results();
}

bool LogErrorChecker::search_and_log_err(const std::string& line, const std::regex& pattern,
                                         const size_t& line_cnt) {
  if (std::regex_search(line, pattern)) {
    err_lines_.push_back(line);
    err_line_no_.push_back(line_cnt);
    return true;
  }
  return false;
}

int LogErrorChecker::parse(const std::string& fname, const bool& verbose) {
  std::ifstream file(fname);
  std::string line;
  size_t line_cnt = 0;

  /* Ensure a clean start */
  clear_results();

  if (file.is_open()) {
    while (std::getline(file, line)) {
      for (auto pair : kw_regex_) {
        bool found_match = false;
        if (kw_ignore_case_[pair.first]) {
          std::regex pattern(pair.second, std::regex_constants::icase);
          found_match = search_and_log_err(line, pattern, line_cnt);
        } else {
          std::regex pattern(pair.second);
          found_match = search_and_log_err(line, pattern, line_cnt);
        }
        if (found_match) {
          break;
        }
      }
      line_cnt++;
    }
    file.close();
    VTR_LOGV(verbose, "Checked %lu lines\n", line_cnt);
    VTR_LOGV(verbose, "Found %lu errors\n", err_lines_.size());
  } else {
    VTR_LOG_ERROR("Unable to open openfpga log file '%s'\n", fname.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  is_valid_ = true;
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arkangel
