/* This is an self-contained floorplanner which can accept options, run floorplanning algorithm and
 * output results */
#include "command_echo.h"
#include "command_exit_codes.h"
#include "command_parser.h"
#include "floorplan_pin_physical_location.h"
#include "floorplan_signal_distribution_network.h"
#include "read_floorplan_params.h"
#include "read_xml_floorplan_strategy.h"
#include "read_xml_io_location_map.h"
#include "report_floorplan.h"
#include "run_floorplan_flow.h"
#include "tile_qor.h"
#include "vtr_log.h"

/** @brief Initialize the options from command-line inputs and organize in the
 * format that is ready for parsing */
static std::vector<std::string> format_argv(const std::string& cmd_name, int argc,
                                            const char** argv) {
  std::vector<std::string> cmd_opts;
  cmd_opts.push_back(cmd_name);
  for (int iarg = 1; iarg < argc; ++iarg) {
    cmd_opts.push_back(std::string(argv[iarg]));
  }
  return cmd_opts;
}

int main(int argc, const char** argv) {
  openfpga::Command cmd("fpga_floorplanner_kisok");

  openfpga::CommandOptionId opt_cpp = cmd.add_option(
    "cpp", true,
    "Specify the Contact Poly Pitch (CPP) as a float number in the unit of [um]. This is the "
    "actual value of the parameter CPP to be used in the floorplanning strategy file");
  cmd.set_option_require_value(opt_cpp, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_sc_height = cmd.add_option(
    "sc_height", true,
    "Specify the standard cell height (SC_HEIGHT) as a float number in the unit of [um]. This is "
    "the actual value of the parameter SC_HEIGHT to be used in the floorplanning strategy file");
  cmd.set_option_require_value(opt_sc_height, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_strtgy =
    cmd.add_option("strategy", true, "Specify the path to the floorplanning strategy file");
  cmd.set_option_require_value(opt_strtgy, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_flrpln_sig_distr_ntwk =
    cmd.add_option("floorplan_signal_distribution_network", false,
                   "Specify if a detailed signal distribution network should be included or not. "
                   "By default, it is off");

  openfpga::CommandOptionId opt_tile_qor =
    cmd.add_option("tile_qor", true, "Specify the path to the Tile Quality-of-Result file");
  cmd.set_option_require_value(opt_tile_qor, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_tile_ref =
    cmd.add_option("tile_ref", true, "Specify the path to the Tile reference file");
  cmd.set_option_require_value(opt_tile_ref, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_dominant_tile =
    cmd.add_option("dominant_tile", false,
                   "Specify the name of dominant tile. Must be a valid one in the tile reference "
                   "file. If not specified, it will be automatically inferred");
  cmd.set_option_require_value(opt_dominant_tile, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_io_coord =
    cmd.add_option("io_coordinates", false, "Specify the path to I/O coordinate file");
  cmd.set_option_require_value(opt_io_coord, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_io_mlayer =
    cmd.add_option("io_metal_layer", false, "Specify the metal layer to place all the I/O pins");
  cmd.set_option_require_value(opt_io_mlayer, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_tile_pinloc = cmd.add_option(
    "tile_pin_location", false,
    "Specify the path to the XML file contains tile-level pin logical location assignments");
  cmd.set_option_require_value(opt_tile_pinloc, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_tile_pin_mlayer = cmd.add_option(
    "tile_pin_metal_layer", false, "Specify the metal layer to place all the tile pins");
  cmd.set_option_require_value(opt_tile_pin_mlayer, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_shaping_result = cmd.add_option(
    "shaping_result", true,
    "Specify the file path to the tile-level shaping results, required by physical design flows");
  cmd.set_option_require_value(opt_shaping_result, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_rpt =
    cmd.add_option("report", true, "Specify the path to output the floorplanning report");
  cmd.set_option_require_value(opt_rpt, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_ntwk_rpt = cmd.add_option(
    "signal_distribution_network_report_file", false,
    "Specify the path to output the signal distribution network floorplanning results");
  cmd.set_option_require_value(opt_ntwk_rpt, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_io_phy_loc_rpt = cmd.add_option(
    "io_physical_location_report", false,
    "Specify the path to report physical location for each I/O pins at fpga_top module");
  cmd.set_option_require_value(opt_io_phy_loc_rpt, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_tile_pin_phy_loc_rpt =
    cmd.add_option("tile_pin_physical_location_report", false,
                   "Specify the path to report physical location for each tile pins");
  cmd.set_option_require_value(opt_tile_pin_phy_loc_rpt, openfpga::OPT_STRING);

#ifdef USE_QT
  openfpga::CommandOptionId opt_result_fig = cmd.add_option(
    "result_figure", false,
    "Specify the file path to the figure (.png) which visualize the floorplanning results");
  cmd.set_option_require_value(opt_result_fig, openfpga::OPT_STRING);
#endif

  openfpga::CommandOptionId opt_shrink_fac =
    cmd.add_option("shrink_factor", false,
                   "Specify the shrink factor required by the PDK in a float number (0.0, 1.0]. By "
                   "default, assume no shrinking (1.0)");
  cmd.set_option_require_value(opt_shrink_fac, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_rpt_precision =
    cmd.add_option("precision", false,
                   "Specify the precision level for numbers in reports. By default, a precision of "
                   "3 is considered");
  cmd.set_option_require_value(opt_rpt_precision, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_img_scale_fac =
    cmd.add_option("figure_scale_factor", false,
                   "Specify the scaling factor required by the PDK in a float number (>1.0). By "
                   "default, assume no scaling (1.0)");
  cmd.set_option_require_value(opt_img_scale_fac, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_verbosity =
    cmd.add_option("verbosity", false,
                   "Show verbose outputs by setting the verbosity level. High values produce more "
                   "detailed outputs");
  cmd.set_option_require_value(opt_verbosity, openfpga::OPT_STRING);

  openfpga::CommandOptionId opt_help = cmd.add_option("help", false, "Show help desk");

  /* Parse the option, to avoid issues, we use the command name to replace the
   * argv[0] */
  std::vector<std::string> cmd_opts = format_argv(cmd.name(), argc, argv);

  openfpga::CommandContext cmd_ctx(cmd);
  if (!parse_command(cmd_opts, cmd, cmd_ctx) || cmd_ctx.option_enable(cmd, opt_help)) {
    /* Echo the command */
    print_command_options(cmd);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  } else {
    /* Let user to confirm selected options */
    print_command_context(cmd, cmd_ctx);
  }

  int verbosity = 0;
  if (cmd_ctx.option_enable(cmd, opt_verbosity)) {
    verbosity = std::stoi(cmd_ctx.option_value(cmd, opt_verbosity));
  }

  /* Parse the fabric key from an XML file */
  float cpp = std::stof(cmd_ctx.option_value(cmd, opt_cpp));
  VTR_LOG("Consider the Contact Poly Pitch (CPP): %g [um]\n", cpp);

  float sc_height = std::stof(cmd_ctx.option_value(cmd, opt_sc_height));
  VTR_LOG("Consider the Standard Cell Height (SC_HEIGHT): %g [um]\n", sc_height);

  arcxpress::FloorplanStrategy flrpln_strtg(cpp, sc_height);

  /* Parse from an XML file */
  std::string strategy_file = cmd_ctx.option_value(cmd, opt_strtgy);
  std::string tref_file = cmd_ctx.option_value(cmd, opt_tile_ref);
  std::string synth_tile_qor_file = cmd_ctx.option_value(cmd, opt_tile_qor);
  std::string shaping_file = cmd_ctx.option_value(cmd, opt_shaping_result);
  std::string rpt_fpath = cmd_ctx.option_value(cmd, opt_rpt);
  /* Read floorplan strategy file */
  VTR_LOGV(verbosity > 0, "Reading strategy from %s\n", strategy_file.c_str());
  int status = arcxpress::read_xml_floorplan_strategy(strategy_file.c_str(), flrpln_strtg);
  if (openfpga::CMD_EXEC_SUCCESS != status) {
    VTR_LOG_ERROR("Reading strategy failed with errors\n");
    return status;
  }
  VTR_LOGV(verbosity > 0, "Done\n");
  VTR_LOGV(verbosity > 0, "Validating strategy...\n");
  if (!flrpln_strtg.validate()) {
    VTR_LOG_ERROR("Strategy is invalid\n");
    return status;
  }
  VTR_LOGV(verbosity > 0, "Done\n");

  /* read tile qor info and reference info */
  arcxpress::TileQor tile_qor;
  VTR_LOGV(verbosity > 0, "Reading tile-level QoR from %s\n", synth_tile_qor_file.c_str());
  status = read_qor_file(tref_file.c_str(), synth_tile_qor_file.c_str(), tile_qor);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Reading tile-level Qor failed with errors\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbosity > 0, "Done\n");

  VTR_LOG("All required file loaded successfully\n");
  VTR_LOG("Run floorplanning...\n");
  std::string dom_tile_name = "";
  if (cmd_ctx.option_enable(cmd, opt_dominant_tile)) {
    dom_tile_name = cmd_ctx.option_value(cmd, opt_dominant_tile);
  }
  status = run_floorplan_flow(flrpln_strtg, tile_qor, dom_tile_name, verbosity > 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Floorplanning failed with errors\n");
    return status;
  }
  VTR_LOG("Done\n");

  /* Default constants */
  std::string prj_name = "default_project";
  std::string family_name = "default_family";
  std::string pdk_name = "custom_pdk";
  std::string run_name = "default_run";
  float shrink_fac = 1.0;
  if (cmd_ctx.option_enable(cmd, opt_shrink_fac)) {
    shrink_fac = std::stof(cmd_ctx.option_value(cmd, opt_shrink_fac));
  }
  VTR_LOG("Consider the shrink factor: %g\n", shrink_fac);

  size_t rpt_precision = 3;
  if (cmd_ctx.option_enable(cmd, opt_rpt_precision)) {
    rpt_precision = std::stoi(cmd_ctx.option_value(cmd, opt_rpt_precision));
  }
  VTR_LOG("Consider the precision in report files: %ld\n", rpt_precision);

  VTR_LOG("Writing shaping results to %s\n", shaping_file.c_str());
  status = report_floorplan(shaping_file, prj_name, family_name, pdk_name, run_name, tile_qor,
                            shrink_fac, rpt_precision, verbosity > 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Writing shaping results failed with errors\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  VTR_LOG("Writing floorplanning results to %s\n", rpt_fpath.c_str());
  status = report_floorplan_to_csv(rpt_fpath.c_str(), prj_name, family_name, pdk_name, run_name,
                                   tile_qor, shrink_fac, rpt_precision, verbosity > 1);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    VTR_LOG_ERROR("Writing floorplanning results failed with errors\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("Done\n");

  if (cmd_ctx.option_enable(cmd, opt_flrpln_sig_distr_ntwk)) {
    VTR_LOG("Run detailed floorplanning on signal distribution network...\n");
    status = floorplan_signal_distribution_network(flrpln_strtg, tile_qor, verbosity > 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Floorplanning failed with errors\n");
      return status;
    }
    VTR_LOG("Done\n");
    if (cmd_ctx.option_enable(cmd, opt_ntwk_rpt)) {
      std::string sig_distr_ntwk_fpath = cmd_ctx.option_value(cmd, opt_ntwk_rpt);
      status =
        report_floorplan_signal_distribution_network_to_xml(tile_qor, sig_distr_ntwk_fpath.c_str());
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        VTR_LOG_ERROR(
          "Writing signal distribution network floorplanning results failed with errors\n");
        return status;
      }
    }
  }

  /* Floorplan the I/O physical location on borderlines */
  if (cmd_ctx.option_enable(cmd, opt_io_coord)) {
    std::string io_coord_fpath = cmd_ctx.option_value(cmd, opt_io_coord);
    if (!cmd_ctx.option_enable(cmd, opt_io_mlayer)) {
      VTR_LOG_ERROR("Missing required option 'io_metal_layer' for I/O detailed floorplanning\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::string io_mlayer = cmd_ctx.option_value(cmd, opt_io_mlayer);
    status = floorplan_io_physical_location(flrpln_strtg, io_coord_fpath, io_mlayer, tile_qor,
                                            verbosity > 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Floorplanning failed with errors\n");
      return status;
    }
    if (cmd_ctx.option_enable(cmd, opt_io_phy_loc_rpt)) {
      std::string io_phy_loc_fpath = cmd_ctx.option_value(cmd, opt_io_phy_loc_rpt);
      status =
        report_floorplan_io_physical_location_to_text(tile_qor, io_phy_loc_fpath, verbosity > 1);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        VTR_LOG_ERROR("Writing I/O physical location results failed with errors\n");
        return status;
      }
    }
  }

  /* Floorplan the tile pin physical location on each module's borderlines */
  if (cmd_ctx.option_enable(cmd, opt_tile_pinloc)) {
    std::string tile_pinloc_fpath = cmd_ctx.option_value(cmd, opt_tile_pinloc);
    if (!cmd_ctx.option_enable(cmd, opt_tile_pin_mlayer)) {
      VTR_LOG_ERROR(
        "Missing required option 'tile_pin_metal_layer' for tile pin detailed floorplanning\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
    std::string tile_pin_mlayer = cmd_ctx.option_value(cmd, opt_tile_pin_mlayer);
    status = floorplan_tile_pin_physical_location(flrpln_strtg, tile_pinloc_fpath, tile_pin_mlayer,
                                                  tile_qor, verbosity > 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Floorplanning failed with errors\n");
      return status;
    }
    if (cmd_ctx.option_enable(cmd, opt_tile_pin_phy_loc_rpt)) {
      std::string tile_pin_phy_loc_fpath = cmd_ctx.option_value(cmd, opt_tile_pin_phy_loc_rpt);
      status = report_floorplan_tile_pin_physical_location_to_text(tile_qor, tile_pin_phy_loc_fpath,
                                                                   verbosity > 1);
      if (status != openfpga::CMD_EXEC_SUCCESS) {
        VTR_LOG_ERROR("Writing tile pin physical location results failed with errors\n");
        return status;
      }
    }
  }
#ifdef USE_QT
  /* Generate figures */
  if (cmd_ctx.option_enable(cmd, opt_result_fig)) {
    std::string fig_fpath = cmd_ctx.option_value(cmd, opt_result_fig);
    float img_scale_fac = 1.0;
    if (cmd_ctx.option_enable(cmd, opt_img_scale_fac)) {
      img_scale_fac = std::stof(cmd_ctx.option_value(cmd, opt_img_scale_fac));
    }
    bool draw_sig_distr_ntwk = cmd_ctx.option_enable(cmd, opt_flrpln_sig_distr_ntwk);
    status =
      draw_floorplan_figure(fig_fpath, tile_qor, draw_sig_distr_ntwk, img_scale_fac, verbosity > 1);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR("Floorplanning visualization failed with errors\n");
      return status;
    }
  }
#endif

  VTR_LOG("Floorplanning for the FPGA fabric completed successfully\n");
  return openfpga::CMD_EXEC_SUCCESS;
}
