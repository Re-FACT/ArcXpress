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
#include "io_coord_xml_constants.h"
#include "pugixml_util.hpp"
#include "read_xml_io_coordinates.h"
#include "read_xml_util.h"
#include "vtr_log.h"

namespace arcxpress {  // Begin namespace arcxpress

/********************************************************************
 * Parse XML codes of a <design_source> to an object of ProjectManager
 *******************************************************************/
static int read_xml_io_coord_per_node(pugi::xml_node& xml_root, const pugiutil::loc_data& loc_data,
                                      IoCoordData& io_coord) {
  for (pugi::xml_node xml_coord : xml_root.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_coord.name() != std::string(XML_IOCOORD_NODE_NAME)) {
      bad_tag(xml_root, loc_data, xml_coord, {XML_IOCOORD_NODE_NAME});
    }
    int coord_x = get_attribute(xml_coord, XML_IOCOORD_ATTR_X, loc_data).as_int();
    int coord_y = get_attribute(xml_coord, XML_IOCOORD_ATTR_Y, loc_data).as_int();
    io_coord.add_coord(vtr::Point<size_t>(coord_x, coord_y));
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

/********************************************************************
 * Parse XML codes to an object of BenchmarkManager
 *******************************************************************/
int read_xml_io_coordinates(const char* fname, IoCoordData& io_coord, const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read I/O coordinate File");
  size_t num_err = 0;

  io_coord.clear();

  if (!std::filesystem::exists(std::string(fname))) {
    VTR_LOG_ERROR("File '%s' does not exist!\n", fname);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);

    pugi::xml_node xml_root = get_single_child(doc, XML_IOCOORD_ROOT_NODE_NAME, loc_data);

    /* Parse tech files */
    int status = read_xml_io_coord_per_node(xml_root, loc_data, io_coord);
    if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
      num_err++;
    }
    VTR_LOGV(verbose, "Found %lu I/O coordinates\n", io_coord.num_coordinates());
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
