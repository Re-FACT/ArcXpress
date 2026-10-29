/********************************************************************
 * Test the project manager by loading data and output data
 *******************************************************************/
/* Headers from vtrutils */
#include "file_md5_manager.h"
#include "vtr_assert.h"
#include "vtr_log.h"

int main(int argc, char** argv) {
  VTR_ASSERT(argc > 2);
  int status = 0;

  arcxpress::FileMd5Manager md5_mgr4gen;

  /* Last arg is to output file. All the others except the first are the input files */

  /* Parse from an XML file */
  VTR_LOG("Run md5 generation for file: %s.\n", argv[1]);
  md5_mgr4gen.add_file(argv[1]);
  status = md5_mgr4gen.gen_md5(true);
  if (0 != status) {
    return status;
  }

  VTR_LOG("Write the md5 to file: %s.\n", argv[2]);
  status = md5_mgr4gen.write_md5_to_yaml_file(argv[2], true);
  if (0 != status) {
    return status;
  }

  /* Read back and check md5 */
  arcxpress::FileMd5Manager md5_mgr4chk;
  VTR_LOG("Load the md5 from file: %s.\n", argv[2]);
  status = md5_mgr4chk.load_md5_from_yaml_file(argv[2], true);
  if (0 != status) {
    return status;
  }
  VTR_LOG("Run md5 check for file: %s.\n", argv[1]);
  status = md5_mgr4chk.check_md5(true);
  if (0 != status) {
    return status;
  }

  return status;
}
