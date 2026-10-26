#include "vpr_block_usage_parser.h"

#include "arch_error.h"
#include "command_exit_codes.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "read_xml_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arcxpress {  // Begin namespace arcxpress

VprBlockUsageParser::VprBlockUsageParser() {
  clear();
  XML_ROOT_NODE_ = "block_usage_report";
  XML_NET_NODE_ = "nets";
  XML_NET_ATTR_NUM_ = "num";
  XML_INPUTPINS_NODE_ = "input_pins";
  XML_INPUTPINS_ATTR_NUM_ = "num";
  XML_OUTPUTPINS_NODE_ = "output_pins";
  XML_OUTPUTPINS_ATTR_NUM_ = "num";
  XML_BLOCKS_NODE_ = "blocks";
  XML_BLOCKS_ATTR_NUM_ = "num";
  XML_BLOCK_NODE_ = "block";
  XML_BLOCK_ATTR_TYPE_ = "type";
  XML_BLOCK_ATTR_USAGE_ = "usage";
}

size_t VprBlockUsageParser::num_nets() const {
  check_valid();
  return num_nets_;
}

size_t VprBlockUsageParser::num_blocks() const {
  check_valid();
  size_t num_blks = 0;
  for (auto pair : block_stats_) {
    num_blks += pair.second;
  }
  return num_blks;
}

size_t VprBlockUsageParser::num_inputs() const {
  check_valid();
  return num_inputs_;
}

size_t VprBlockUsageParser::num_outputs() const {
  check_valid();
  return num_outputs_;
}

size_t VprBlockUsageParser::block_usage(const std::string& type) const {
  auto result = block_stats_.find(type);
  if (result == block_stats_.end()) {
    VTR_LOG_ERROR(
      "Invalid block type '%s' for usage query! Please double check the consistency between arch "
      "and log parser\n",
      type.c_str());
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return result->second;
}

std::vector<std::string> VprBlockUsageParser::block_types() const {
  std::vector<std::string> ret;
  for (auto pair : block_stats_) {
    ret.push_back(pair.first);
  }
  return ret;
}

int VprBlockUsageParser::parse(const std::string& fname, const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read block usage File");
  /* A clean start */
  clear();
  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  size_t num_blks = 0;
  size_t expected_num_blks = 0;
  try {
    loc_data = pugiutil::load_xml(doc, fname);
    pugi::xml_node xml_root = get_single_child(doc, XML_ROOT_NODE_, loc_data);
    /* Parse net info */
    pugi::xml_node xml_nets = get_single_child(xml_root, XML_NET_NODE_, loc_data);
    num_nets_ = get_attribute(xml_nets, XML_NET_ATTR_NUM_, loc_data).as_int();
    /* Parse num inputs info */
    pugi::xml_node xml_inputs = get_single_child(xml_root, XML_INPUTPINS_NODE_, loc_data);
    num_inputs_ = get_attribute(xml_inputs, XML_INPUTPINS_ATTR_NUM_, loc_data).as_int();
    /* Parse num inputs info */
    pugi::xml_node xml_outputs = get_single_child(xml_root, XML_OUTPUTPINS_NODE_, loc_data);
    num_outputs_ = get_attribute(xml_outputs, XML_OUTPUTPINS_ATTR_NUM_, loc_data).as_int();
    /* Parse block stats */
    pugi::xml_node xml_blks = get_single_child(xml_root, XML_BLOCKS_NODE_, loc_data);
    expected_num_blks = get_attribute(xml_blks, XML_BLOCKS_ATTR_NUM_, loc_data).as_int();
    for (pugi::xml_node xml_blk : xml_blks.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_blk.name() != std::string(XML_BLOCK_NODE_)) {
        bad_tag(xml_blks, loc_data, xml_blk, {XML_BLOCK_NODE_});
      }
      std::string blk_type = get_attribute(xml_blk, XML_BLOCK_ATTR_TYPE_, loc_data).as_string();
      size_t blk_usage = get_attribute(xml_blk, XML_BLOCK_ATTR_USAGE_, loc_data).as_int();
      block_stats_[blk_type] = blk_usage;
      VTR_LOGV(verbose, "Found block type '%s' with a usage of '%lu'\n", blk_type.c_str(),
               blk_usage);
      num_blks += blk_usage;
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname.c_str(), e.line(), "%s", e.what());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Sanity checks */
  if (num_blks != expected_num_blks) {
    VTR_LOG_ERROR(
      "Mismatch between the number of blocks (%lu) defined and the actual count of blocks (%lu)!\n",
      expected_num_blks, num_blks);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  is_valid_ = true; /* Flip when parsing finished */
  return openfpga::CMD_EXEC_SUCCESS;
}

void VprBlockUsageParser::check_valid() const {
  if (!is_valid_) {
    VTR_LOG_ERROR("VPR block usage data is invalid!");
    exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
}

void VprBlockUsageParser::clear() {
  num_nets_ = 0;
  num_inputs_ = 0;
  num_outputs_ = 0;
  block_stats_.clear();
  is_valid_ = false;
}

}  // End namespace arcxpress
