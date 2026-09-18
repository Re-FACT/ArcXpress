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
#include "bitstream_distribution_xml_constants.h"
#include "command_exit_codes.h"
#include "pugixml_util.hpp"
#include "read_xml_bitstream_distribution.h"
#include "read_xml_util.h"
#include "vtr_log.h"

namespace arkangel {  // Begin namespace arkangel

static int read_xml_region(pugi::xml_node& xml_region, const pugiutil::loc_data& loc_data,
                           BitstreamDistribution& bit_dist) {
  int num_err = 0;

  pugi::xml_node xml_region_node =
    get_single_child(xml_region, XML_BITSTREAM_DISTRIBUTION_NODE_REGION, loc_data);

  size_t region_id =
    get_attribute(xml_region_node, XML_BITSTREAM_DISTRIBUTION_REGION_ATTR_ID, loc_data).as_int();
  size_t num_bits =
    get_attribute(xml_region_node, XML_BITSTREAM_DISTRIBUTION_REGION_ATTR_NUM_OF_BITS, loc_data)
      .as_int();
  bit_dist.set_region_num_bits(region_id, num_bits);

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

static int read_xml_block(pugi::xml_node& xml_region, const pugiutil::loc_data& loc_data,
                          BitstreamDistribution& bit_dist) {
  int num_err = 0;

  pugi::xml_node xml_block_node =
    get_single_child(xml_region, XML_BITSTREAM_DISTRIBUTION_NODE_BLOCK, loc_data);

  size_t num_bits =
    get_attribute(xml_block_node, XML_BITSTREAM_DISTRIBUTION_BLOCK_ATTR_NUM_OF_BITS, loc_data)
      .as_int();
  bool status = bit_dist.validate(num_bits);
  if (!status) {
    num_err++;
  }

  return num_err ? openfpga::CMD_EXEC_FATAL_ERROR : openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_bitstream_distribution(const std::string& fname, BitstreamDistribution& bit_dist) {
  vtr::ScopedStartFinishTimer timer("Read bitstream distribution file");
  /* Check file existence */
  if (!std::filesystem::exists(fname)) {
    VTR_LOG_ERROR("File does not exist at location: %s!\n", fname.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  size_t num_err = 0;

  bit_dist.clear();

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname.c_str());

    pugi::xml_node xml_root =
      get_single_child(doc, XML_BITSTREAM_DISTRIBUTION_ROOT_NODE_NAME, loc_data);

    /* Parse regions and blocks */
    for (pugi::xml_node xml_region : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_region.name() == std::string(XML_BITSTREAM_DISTRIBUTION_NODE_REGIONS)) {
        int region_status = read_xml_region(xml_region, loc_data, bit_dist);
        if (region_status == openfpga::CMD_EXEC_FATAL_ERROR) {
          VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                        XML_BITSTREAM_DISTRIBUTION_NODE_REGIONS);
          num_err++;
        }
      } else if (xml_region.name() == std::string(XML_BITSTREAM_DISTRIBUTION_NODE_BLOCKS)) {
        int region_status = read_xml_block(xml_region, loc_data, bit_dist);
        if (region_status == openfpga::CMD_EXEC_FATAL_ERROR) {
          VTR_LOG_ERROR("Parsing node '%s' failed with errors\n",
                        XML_BITSTREAM_DISTRIBUTION_NODE_BLOCKS);
          num_err++;
        }
      } else {
        bad_tag(xml_region, loc_data, xml_root,
                {XML_BITSTREAM_DISTRIBUTION_NODE_REGIONS, XML_BITSTREAM_DISTRIBUTION_NODE_BLOCKS});
      }
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname.c_str(), e.line(), "%s", e.what());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  if (num_err) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arkangel
