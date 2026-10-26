#include "generate_pin_table_naming_rules.h"

#include "command_exit_codes.h"
#include "io_coord_data.h"
#include "read_xml_io_coordinates.h"
#include "write_xml_pin_table_naming_rules.h"

namespace arcxpress {  // Begin namespace arcxpress

/* Generate a pin table naming rule based on the input I/O coordinates. */
int generate_pin_table_naming_rules(const std::string& io_coord_fname, const std::string& out_fname,
                                    const bool& verbose) {
  /* Read the io coordinate file */
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  IoCoordData io_coords;
  status = read_xml_io_coordinates(io_coord_fname.c_str(), io_coords, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Compute the borderlines */
  status = io_coords.compute_borders(verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* Write the pin table naming rules */
  status = write_xml_pin_table_naming_rules(out_fname.c_str(), io_coords, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End namespace arcxpress
