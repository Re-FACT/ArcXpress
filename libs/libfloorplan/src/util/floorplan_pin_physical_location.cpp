#include "floorplan_pin_physical_location.h"

#include <cmath>
#include <filesystem>

#include "arch_error.h"
#include "command_exit_codes.h"
#include "fabric_pin_physical_location_xml_constants.h"
#include "openfpga_port_parser.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "read_xml_util.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace arcxpress {  // Begin namespace arcxpress

static int read_xml_one_io_location(pugi::xml_node& xml_io, const pugiutil::loc_data& loc_data,
                                    TileQor& tile_qor) {
  openfpga::PortParser port_parser(get_attribute(xml_io, "pad", loc_data).as_string());

  int x_coord = get_attribute(xml_io, "x", loc_data).as_int();
  int y_coord = get_attribute(xml_io, "y", loc_data).as_int();
  int z_coord = get_attribute(xml_io, "z", loc_data).as_int();

  /* Sanity checks */
  if (x_coord < 0 || y_coord < 0 || z_coord < 0) {
    archfpga_throw(loc_data.filename_c_str(), loc_data.line(xml_io),
                   "Invalid coordinate (x, y, z) = (%d, %d, %d)! Expect zero "
                   "or a positive integer!\n",
                   x_coord, y_coord, z_coord);
  }
  if (port_parser.port().get_width() != 1) {
    archfpga_throw(loc_data.filename_c_str(), loc_data.line(xml_io),
                   "I/O (%s) does not have a port size of 1!\n",
                   get_attribute(xml_io, "pad", loc_data).as_string());
  }
  int status = tile_qor.add_io_logical_coordinate(
    port_parser.port(), vtr::Point<size_t>(size_t(x_coord), size_t(y_coord)), size_t(z_coord));
  return status;
}

static int read_xml_io_coordinate_to_tile_qor(const std::string& fname, TileQor& tile_qor,
                                              const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read I/O Location Map");

  if (!std::filesystem::exists(fname)) {
    VTR_LOG_ERROR("Required file '%s' does not exist\n", fname.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);
    /* TODO: Share the XML constants as OpenFPGA parser */
    pugi::xml_node xml_root = get_single_child(doc, "io_coordinates", loc_data);

    for (pugi::xml_node xml_io : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_io.name() != std::string("io")) {
        bad_tag(xml_io, loc_data, xml_root, {"io"});
      }
      int status = read_xml_one_io_location(xml_io, loc_data, tile_qor);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname.c_str(), e.line(), "%s", e.what());
  }

  VTR_LOGV(verbose, "Done\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

/* Main function to run a detailed floorplanning on global signal distribution network */
int floorplan_io_physical_location(const FloorplanStrategy& flrpln_strtg,
                                   const std::string& io_coord_fpath,
                                   const std::string& io_metal_layer, TileQor& tile_qor,
                                   const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Read I/O location from XML file and add to tile_qor */
  VTR_LOGV(verbose, "Reading I/O coordinates from file '%s'...\n", io_coord_fpath.c_str());
  status = read_xml_io_coordinate_to_tile_qor(io_coord_fpath, tile_qor, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Reading I/O coordinate file failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");

  /* Check I/O metal layer is valid or not */
  if (!flrpln_strtg.valid_metal_layer(io_metal_layer)) {
    VTR_LOG_ERROR(
      "The metal layer '%s' specified for I/O floorplanning is not a valid one defined in the "
      "floorplanning strategy!\n",
      io_metal_layer.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbose, "Build data I/O physical location...\n");
  status = tile_qor.build_data_io_physical_location(
    io_metal_layer, flrpln_strtg.metal_layer_length(io_metal_layer),
    flrpln_strtg.metal_layer_width(io_metal_layer),
    flrpln_strtg.metal_layer_spacing(io_metal_layer), verbose);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Build data I/O physical location failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");

  VTR_LOGV(verbose, "Build global I/O physical location...\n");
  status = tile_qor.build_global_io_physical_location(
    io_metal_layer, flrpln_strtg.metal_layer_length(io_metal_layer),
    flrpln_strtg.metal_layer_width(io_metal_layer),
    flrpln_strtg.metal_layer_spacing(io_metal_layer), verbose);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Build global I/O physical location failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");

  return status;
}

static int read_xml_one_tile_pin_location(pugi::xml_node& xml_module,
                                          const pugiutil::loc_data& loc_data, TileQor& tile_qor) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  std::string tile_name =
    get_attribute(xml_module, XML_MODULE_ATTRIBUTE_NAME, loc_data).as_string();
  for (pugi::xml_node xml_loc : xml_module.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_loc.name() != std::string(XML_MODULE_PINLOC_NODE_NAME)) {
      bad_tag(xml_loc, loc_data, xml_module, {XML_MODULE_PINLOC_NODE_NAME});
    }
    /* Get detailed pin location info */
    openfpga::PortParser port_parser(
      get_attribute(xml_loc, XML_MODULE_PINLOC_ATTRIBUTE_PIN, loc_data).as_string());
    if (port_parser.port().get_width() != 1) {
      archfpga_throw(loc_data.filename_c_str(), loc_data.line(xml_loc),
                     "Tile pin (%s) does not have a port size of 1!\n",
                     get_attribute(xml_loc, XML_MODULE_PINLOC_ATTRIBUTE_PIN, loc_data).as_string());
    }
    std::string side_str =
      get_attribute(xml_loc, XML_MODULE_PINLOC_ATTRIBUTE_SIDE, loc_data).as_string();
    status = tile_qor.add_tile_pin_logical_location(tile_name, port_parser.port(), side_str);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }
  return status;
}

static int read_xml_tile_pin_location_to_tile_qor(const std::string& fname, TileQor& tile_qor,
                                                  const bool& verbose) {
  vtr::ScopedStartFinishTimer timer("Read tile pin location info");

  if (!std::filesystem::exists(fname)) {
    VTR_LOG_ERROR("Required file '%s' does not exist\n", fname.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Parse the file */
  pugi::xml_document doc;
  pugiutil::loc_data loc_data;

  try {
    loc_data = pugiutil::load_xml(doc, fname);
    /* TODO: Share the XML constants as OpenFPGA parser */
    pugi::xml_node xml_root = get_single_child(doc, XML_PINLOC_ROOT_NAME, loc_data);

    for (pugi::xml_node xml_child : xml_root.children()) {
      /* Error out if the XML child has an invalid name! */
      if (xml_child.name() != std::string(XML_MODULE_NODE_NAME)) {
        bad_tag(xml_child, loc_data, xml_root, {XML_MODULE_NODE_NAME});
      }
      int status = read_xml_one_tile_pin_location(xml_child, loc_data, tile_qor);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }
    }
  } catch (pugiutil::XmlError& e) {
    archfpga_throw(fname.c_str(), e.line(), "%s", e.what());
  }

  VTR_LOGV(verbose, "Done\n");
  return openfpga::CMD_EXEC_SUCCESS;
}

int floorplan_tile_pin_physical_location(const FloorplanStrategy& flrpln_strtg,
                                         const std::string& tile_pinloc_fpath,
                                         const std::string& tile_pin_metal_layer, TileQor& tile_qor,
                                         const bool& verbose) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  /* Read I/O location from XML file and add to tile_qor */
  VTR_LOGV(verbose, "Reading tile pin logical location from file '%s'...\n",
           tile_pinloc_fpath.c_str());
  status = read_xml_tile_pin_location_to_tile_qor(tile_pinloc_fpath, tile_qor, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Reading tile pin logical location file failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");

  /* Check I/O metal layer is valid or not */
  if (!flrpln_strtg.valid_metal_layer(tile_pin_metal_layer)) {
    VTR_LOG_ERROR(
      "The metal layer '%s' specified for tile pin is not a valid one defined in the floorplanning "
      "strategy!\n",
      tile_pin_metal_layer.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  VTR_LOGV(verbose, "Build tile pin physical location...\n");
  status = tile_qor.build_tile_pin_physical_location(
    tile_pin_metal_layer, flrpln_strtg.metal_layer_length(tile_pin_metal_layer),
    flrpln_strtg.metal_layer_width(tile_pin_metal_layer),
    flrpln_strtg.metal_layer_spacing(tile_pin_metal_layer), verbose);
  if (status == openfpga::CMD_EXEC_FATAL_ERROR) {
    VTR_LOG_ERROR("Build tile pin physical location failed with errors!\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Done\n");

  return status;
}

}  // End of namespace arcxpress
