#include "command_exit_codes.h"
#include "read_floorplan_params.h"
#include "read_xml_floorplan_strategy.h"
#include "report_floorplan.h"
#include "run_floorplan_flow.h"
#include "tile_qor.h"
#ifdef USE_QT
#include "floorplan_qt.h"
#endif
int main(int argc, char** argv) {
  VTR_ASSERT(argc > 2);

  arcxpress::FloorplanStrategy flrpln_strtg(0.19, 0.98);
  /* Last arg is to output file. All the others except the first are the input files */

  /* Parse from an XML file */
  std::string strategy_file;
  std::string tref_file;
  std::string synth_tile_qor_file;
  int status = 0;
  for (size_t iarg = 1; iarg < argc - 1; ++iarg) {
    VTR_LOG("Load the floorplan input file: %s.\n", argv[iarg]);
    if (std::strcmp(argv[iarg], "--floorplan_strategy_file") == 0) {
      strategy_file = argv[iarg + 1];
      iarg++;
    }
    if (std::strcmp(argv[iarg], "--tref_file") == 0) {
      tref_file = argv[iarg + 1];
      iarg++;
    }
    if (std::strcmp(argv[iarg], "--tile_qor_file") == 0) {
      synth_tile_qor_file = argv[iarg + 1];
      iarg++;
    }
    /* all the above file should be set here */
  }

  /*read floorplan strategy file*/
  status = arcxpress::read_xml_floorplan_strategy(strategy_file.c_str(), flrpln_strtg);
  if (0 != status) {
    return status;
  }
  /* Validation */
  status = flrpln_strtg.validate();
  if (!status) {
    return status;
  }

  /* read tile qor info and reference info */
  arcxpress::TileQor tile_qor;
  status = read_qor_file(tref_file.c_str(), synth_tile_qor_file.c_str(), tile_qor);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  status = run_floorplan_flow(flrpln_strtg, tile_qor, "", true);
  if (status != openfpga::CMD_EXEC_SUCCESS) return status;
  status = report_floorplan("shaping.txt", "proj_name_", "family_name_", "pdk_mgr_name",
                            "run_name_", tile_qor, 1, 2, true);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#ifdef USE_QT
  status = show_floorplan_data_window("shaping.txt", tile_qor, 1, 2);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
#endif
  std::string rpt_fpath = "output/floorplan_result.csv";
  status = report_floorplan_to_csv(rpt_fpath.c_str(), "proj_name_", "family_name_", "pdk_mgr_name",
                                   "run_name_", tile_qor, 1, 2, true);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return 0;
}
