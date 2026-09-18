#include "report_floorplan.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "arkangel_report_util.h"
#include "command_exit_codes.h"
#include "openfpga_digest.h"
#include "openfpga_reserved_words.h"
#include "pugixml.hpp"
#include "rapidcsv.h"
#include "sig_distr_ntwk_xml_constants.h"
#include "vtr_log.h"
#include "vtr_time.h"
#ifdef USE_QT
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPen>
#endif

namespace arkangel {  // Begin namespace arkangel

/** Show a summary about the floorplanning results on the screen
 * - List the height, width, utilization of tiles
 * - Show the height, width and utlization of the eFPGA (pre-shrink)
 * - Show the height, width and utlization of the eFPGA (post-shrink)
 */
static int report_csv_floorplan_tile(rapidcsv::Document& doc, size_t& row_idx,
                                     const TileQor& tile_qor, const float& shrink_factor,
                                     const size_t& precision, const bool& verbose) {
  int status = openfpga::CMD_EXEC_SUCCESS;

  /* Header line */
  std::string tag_name = "Tile";
  std::string tag_height = "Height (um)";
  std::string tag_width = "Width (um)";
  std::string tag_cell_area = "Cell Area (um^2)";
  std::string tag_preshrink_area = "Pre-shrink Area (um^2)";
  std::string tag_postshrink_area = "Post-shrink Area (um^2)";
  std::string tag_target_util = "Target Utilization";
  std::string tag_actual_util = "Utilization";

  std::vector<std::string> tag_list = {tag_name, tag_height, tag_width, tag_actual_util};
  if (verbose) {
    tag_list = {tag_name,        tag_height,    tag_width,          tag_target_util,
                tag_actual_util, tag_cell_area, tag_preshrink_area, tag_postshrink_area};
  }
  doc.InsertRow(row_idx, tag_list);
  row_idx++;

  /* Add data lines */
  for (std::string tile : tile_qor.tiles()) {
    std::vector<std::string> data_row;
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    data_row.push_back(tile);
    data_row.push_back(to_string_with_precision(tile_qor.get_tile_height(tile), precision));
    data_row.push_back(to_string_with_precision(tile_qor.get_tile_width(tile), precision));
    data_row.push_back(to_string_with_precision(tile_qor.utilization(tile), precision));
    if (verbose) {
      data_row.push_back(
        to_string_with_precision(tile_qor.get_tile_actual_utilization(tile), precision));
      data_row.push_back(to_string_with_precision(tile_qor.cell_area(tile), precision));
      data_row.push_back(to_string_with_precision(tile_qor.get_tile_actual_area(tile), precision));
      data_row.push_back(to_string_with_precision(
        tile_qor.get_tile_actual_area(tile) * shrink_factor * shrink_factor, precision));
    }
    doc.InsertRow(row_idx, data_row);
    row_idx++;
  }

  return status;
}

/** Show a summary about the floorplanning results on the screen
 * - List the height, width, utilization of tiles
 * - Show the height, width and utlization of the eFPGA (pre-shrink)
 * - Show the height, width and utlization of the eFPGA (post-shrink)
 */
static int report_floorplan_tile(const TileQor& tile_qor, const float& shrink_factor,
                                 const size_t& precision, const bool& verbose) {
  int status = openfpga::CMD_EXEC_SUCCESS;

  /* Header line */
  std::string tag_name = "Tile";
  std::string tag_height = "Height (um)";
  std::string tag_width = "Width (um)";
  std::string tag_cell_area = "Cell Area (um^2)";
  std::string tag_preshrink_area = "Pre-shrink Area (um^2)";
  std::string tag_postshrink_area = "Post-shrink Area (um^2)";
  std::string tag_target_util = "Target Utilization";
  std::string tag_actual_util = "Utilization";

  /* Check max string size for each column */
  std::map<std::string, size_t> col_max_char;
  col_max_char[tag_name] = tag_name.size();
  for (std::string tile : tile_qor.tiles()) {
    if (col_max_char[tag_name] < tile.size()) {
      col_max_char[tag_name] = tile.size();
    }
  }
  col_max_char[tag_height] = tag_height.size();
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    if (col_max_char[tag_height] <
        to_string_with_precision(tile_qor.get_tile_height(tile), precision).size()) {
      col_max_char[tag_height] =
        to_string_with_precision(tile_qor.get_tile_height(tile), precision).size();
    }
  }
  col_max_char[tag_width] = tag_width.size();
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    if (col_max_char[tag_width] <
        to_string_with_precision(tile_qor.get_tile_width(tile), precision).size()) {
      col_max_char[tag_width] =
        to_string_with_precision(tile_qor.get_tile_width(tile), precision).size();
    }
  }
  col_max_char[tag_cell_area] = tag_cell_area.size();
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    if (col_max_char[tag_cell_area] <
        to_string_with_precision(tile_qor.cell_area(tile), precision).size()) {
      col_max_char[tag_cell_area] =
        to_string_with_precision(tile_qor.cell_area(tile), precision).size();
    }
  }
  col_max_char[tag_preshrink_area] = tag_preshrink_area.size();
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    /* TODO: The unit should be applied inside the method!!! */
    if (col_max_char[tag_preshrink_area] <
        to_string_with_precision(tile_qor.get_tile_actual_area(tile), precision).size()) {
      col_max_char[tag_preshrink_area] =
        to_string_with_precision(tile_qor.get_tile_actual_area(tile), precision).size();
    }
  }
  col_max_char[tag_postshrink_area] = tag_postshrink_area.size();
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    /* TODO: The unit should be applied inside the method!!! */
    if (col_max_char[tag_postshrink_area] <
        to_string_with_precision(
          tile_qor.get_tile_actual_area(tile) * shrink_factor * shrink_factor, precision)
          .size()) {
      col_max_char[tag_postshrink_area] =
        to_string_with_precision(
          tile_qor.get_tile_actual_area(tile) * shrink_factor * shrink_factor, precision)
          .size();
    }
  }

  col_max_char[tag_target_util] = tag_target_util.size();
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    /* Consider a tailing '%' character*/
    if (col_max_char[tag_target_util] <
        to_string_with_precision(100.0 * tile_qor.utilization(tile), precision).size() + 1) {
      col_max_char[tag_target_util] =
        to_string_with_precision(tile_qor.utilization(tile), precision).size() + 1;
    }
  }

  col_max_char[tag_actual_util] = tag_actual_util.size();
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    /* Consider a tailing '%' character*/
    if (col_max_char[tag_actual_util] <
        to_string_with_precision(100.0 * tile_qor.get_tile_actual_utilization(tile), precision)
            .size() +
          1) {
      col_max_char[tag_actual_util] =
        to_string_with_precision(tile_qor.get_tile_actual_utilization(tile), precision).size() + 1;
    }
  }

  /* Print header line */
  std::vector<std::string> tag_list = {tag_name, tag_height, tag_width, tag_actual_util};
  if (verbose) {
    tag_list = {tag_name,        tag_height,    tag_width,          tag_actual_util,
                tag_target_util, tag_cell_area, tag_preshrink_area, tag_postshrink_area};
  }
  for (std::string tag : tag_list) {
    std::string resized_tag = tag;
    resized_tag.resize(col_max_char[tag], ' ');
    VTR_LOG("%s\t", resized_tag.c_str());
  }
  VTR_LOG("\n");
  /* Print data lines */
  for (std::string tile : tile_qor.tiles()) {
    /* Bypass non-tile modules  */
    if (tile.find("tile", 0) == std::string::npos) {
      continue; /* Bypass non-tile modules */
    }
    /* Column name */
    std::string resized_name = tile;
    resized_name.resize(col_max_char[tag_name], ' ');
    VTR_LOG("%s\t", resized_name.c_str());
    std::string resized_height =
      to_string_with_precision(tile_qor.get_tile_height(tile), precision);
    resized_height.resize(col_max_char[tag_height], ' ');
    VTR_LOG("%s\t", resized_height.c_str());
    std::string resized_width = to_string_with_precision(tile_qor.get_tile_width(tile), precision);
    resized_width.resize(col_max_char[tag_width], ' ');
    VTR_LOG("%s\t", resized_width.c_str());
    std::string resized_actual_util =
      to_string_with_precision(100.0 * tile_qor.get_tile_actual_utilization(tile), precision) +
      std::string("%");
    resized_actual_util.resize(col_max_char[tag_actual_util], ' ');
    VTR_LOG("%s\t", resized_actual_util.c_str());
    if (!verbose) {
      /* End the line */
      VTR_LOG("\n");
      continue;
    }
    std::string resized_target_util =
      to_string_with_precision(100.0 * tile_qor.utilization(tile), precision) + std::string("%");
    resized_target_util.resize(col_max_char[tag_target_util], ' ');
    VTR_LOG("%s\t", resized_target_util.c_str());
    /* Verbose output on more data */
    std::string resized_cell_area = to_string_with_precision(tile_qor.cell_area(tile), precision);
    resized_cell_area.resize(col_max_char[tag_cell_area], ' ');
    VTR_LOG("%s\t", resized_cell_area.c_str());

    std::string resized_preshrink_area =
      to_string_with_precision(tile_qor.get_tile_actual_area(tile), precision);
    resized_preshrink_area.resize(col_max_char[tag_preshrink_area], ' ');
    VTR_LOG("%s\t", resized_preshrink_area.c_str());

    std::string resized_postshrink_area = to_string_with_precision(
      tile_qor.get_tile_actual_area(tile) * shrink_factor * shrink_factor, precision);
    resized_postshrink_area.resize(col_max_char[tag_postshrink_area], ' ');
    VTR_LOG("%s\t", resized_postshrink_area.c_str());

    /* End the line */
    VTR_LOG("\n");
  }
  return status;
}

/** Show a summary about the floorplanning results on the screen
 * - Show the height, width and utlization of the eFPGA (pre-shrink)
 * - Show the height, width and utlization of the eFPGA (post-shrink)
 */
static int report_csv_floorplan_fpga(rapidcsv::Document& doc, size_t& row_idx,
                                     const TileQor& tile_qor, const float& shrink_factor,
                                     const size_t& precision, const bool& verbose) {
  int status = openfpga::CMD_EXEC_SUCCESS;

  float efpga_preshrink_h = tile_qor.efpga_height();
  float efpga_preshrink_w = tile_qor.efpga_width();
  float efpga_preshrink_area = efpga_preshrink_h * efpga_preshrink_w / 1e6;

  float efpga_postshrink_h = efpga_preshrink_h * shrink_factor;
  float efpga_postshrink_w = efpga_preshrink_w * shrink_factor;
  float efpga_postshrink_area = efpga_postshrink_h * efpga_postshrink_w / 1e6;

  float avg_target_util = 100.0 * tile_qor.average_target_utilization();
  float avg_util = 100.0 * tile_qor.average_actual_utilization();

  /* Header line */
  std::string tag_name = "Estimated eFPGA Layout";
  std::string tag_height = "Height (um)";
  std::string tag_width = "Width (um)";
  std::string tag_area = "Area (mm^2)";
  std::string tag_target_util = "Target Utilization";
  std::string tag_actual_util = "Utilization";

  std::string tag_preshrink = "Pre-shrink";
  std::string tag_postshrink = "Post-shrink";

  std::vector<std::string> tag_list = {tag_name, tag_height, tag_width, tag_area};
  if (verbose) {
    tag_list = {tag_name, tag_height, tag_width, tag_area, tag_target_util, tag_actual_util};
  }
  doc.InsertRow(row_idx, tag_list);
  row_idx++;

  /* Data lines */
  {
    std::vector<std::string> data_row;
    data_row.push_back(tag_preshrink);
    data_row.push_back(to_string_with_precision(efpga_preshrink_h, precision));
    data_row.push_back(to_string_with_precision(efpga_preshrink_w, precision));
    data_row.push_back(to_string_with_precision(efpga_preshrink_area, precision));
    if (verbose) {
      data_row.push_back(to_string_with_precision(avg_target_util, precision) + std::string("%"));
      data_row.push_back(to_string_with_precision(avg_util, precision) + std::string("%"));
    }
    doc.InsertRow(row_idx, data_row);
    row_idx++;
  }

  {
    std::vector<std::string> data_row;
    data_row.push_back(tag_postshrink);
    data_row.push_back(to_string_with_precision(efpga_postshrink_h, precision));
    data_row.push_back(to_string_with_precision(efpga_postshrink_w, precision));
    data_row.push_back(to_string_with_precision(efpga_postshrink_area, precision));
    if (verbose) {
      data_row.push_back(to_string_with_precision(avg_target_util, precision) + std::string("%"));
      data_row.push_back(to_string_with_precision(avg_util, precision) + std::string("%"));
    }
    doc.InsertRow(row_idx, data_row);
    row_idx++;
  }

  return status;
}

/** Show a summary about the floorplanning results on the screen
 * - Show the height, width and utlization of the eFPGA (pre-shrink)
 * - Show the height, width and utlization of the eFPGA (post-shrink)
 */
static int report_floorplan_fpga(const TileQor& tile_qor, const float& shrink_factor,
                                 const size_t& precision, const bool& verbose) {
  int status = openfpga::CMD_EXEC_SUCCESS;

  float efpga_preshrink_h = tile_qor.efpga_height();
  float efpga_preshrink_w = tile_qor.efpga_width();
  float efpga_preshrink_area = efpga_preshrink_h * efpga_preshrink_w / 1e6;

  float efpga_postshrink_h = efpga_preshrink_h * shrink_factor;
  float efpga_postshrink_w = efpga_preshrink_w * shrink_factor;
  float efpga_postshrink_area = efpga_postshrink_h * efpga_postshrink_w / 1e6;

  float avg_target_util = 100.0 * tile_qor.average_target_utilization();
  float avg_util = 100.0 * tile_qor.average_actual_utilization();

  /* Header line */
  std::string tag_name = "Estimated eFPGA Layout";
  std::string tag_height = "Height (um)";
  std::string tag_width = "Width (um)";
  std::string tag_area = "Area (mm^2)";
  std::string tag_target_util = "Target Utilization";
  std::string tag_actual_util = "Utilization";

  std::string tag_preshrink = "Pre-shrink";
  std::string tag_postshrink = "Post-shrink";

  /* Check max string size for each column */
  std::map<std::string, size_t> col_max_char;
  col_max_char[tag_name] = std::max({tag_name.size(), tag_preshrink.size(), tag_postshrink.size()});
  col_max_char[tag_height] =
    std::max({tag_height.size(), to_string_with_precision(efpga_preshrink_h, precision).size(),
              to_string_with_precision(efpga_postshrink_h, precision).size()});
  col_max_char[tag_width] =
    std::max({tag_width.size(), to_string_with_precision(efpga_preshrink_w, precision).size(),
              to_string_with_precision(efpga_postshrink_w, precision).size()});
  col_max_char[tag_area] =
    std::max({tag_area.size(), to_string_with_precision(efpga_preshrink_area, precision).size(),
              to_string_with_precision(efpga_postshrink_area, precision).size()});
  col_max_char[tag_actual_util] =
    std::max({tag_actual_util.size(), to_string_with_precision(avg_util, precision).size() + 1});
  col_max_char[tag_target_util] = std::max(
    {tag_target_util.size(), to_string_with_precision(avg_target_util, precision).size() + 1});

  /* Print header line */
  std::vector<std::string> tag_list = {tag_name, tag_height, tag_width, tag_area};
  if (verbose) {
    tag_list = {tag_name, tag_height, tag_width, tag_area, tag_target_util, tag_actual_util};
  }
  for (std::string tag : tag_list) {
    std::string resized_tag = tag;
    resized_tag.resize(col_max_char[tag], ' ');
    VTR_LOG("%s\t", resized_tag.c_str());
  }
  VTR_LOG("\n");
  /* Print data lines */
  /* Pre-shrink */
  {
    std::string resized_name = tag_preshrink;
    resized_name.resize(col_max_char[tag_name], ' ');
    VTR_LOG("%s\t", resized_name.c_str());

    std::string resized_height = to_string_with_precision(efpga_preshrink_h, precision);
    resized_height.resize(col_max_char[tag_height], ' ');
    VTR_LOG("%s\t", resized_height.c_str());

    std::string resized_width = to_string_with_precision(efpga_preshrink_w, precision);
    resized_width.resize(col_max_char[tag_width], ' ');
    VTR_LOG("%s\t", resized_width.c_str());

    std::string resized_area = to_string_with_precision(efpga_preshrink_area, precision);
    resized_area.resize(col_max_char[tag_area], ' ');
    VTR_LOG("%s\t", resized_area.c_str());

    if (verbose) {
      std::string resized_target_util =
        to_string_with_precision(avg_target_util, precision) + std::string("%");
      resized_target_util.resize(col_max_char[tag_target_util], ' ');
      VTR_LOG("%s\t", resized_target_util.c_str());

      std::string resized_actual_util =
        to_string_with_precision(avg_util, precision) + std::string("%");
      resized_actual_util.resize(col_max_char[tag_actual_util], ' ');
      VTR_LOG("%s\t", resized_actual_util.c_str());
    }

    /* End the line */
    VTR_LOG("\n");
  }

  /* Post-shrink */
  {
    std::string resized_name = tag_postshrink;
    resized_name.resize(col_max_char[tag_name], ' ');
    VTR_LOG("%s\t", resized_name.c_str());

    std::string resized_height = to_string_with_precision(efpga_postshrink_h, precision);
    resized_height.resize(col_max_char[tag_height], ' ');
    VTR_LOG("%s\t", resized_height.c_str());

    std::string resized_width = to_string_with_precision(efpga_postshrink_w, precision);
    resized_width.resize(col_max_char[tag_width], ' ');
    VTR_LOG("%s\t", resized_width.c_str());

    std::string resized_area = to_string_with_precision(efpga_postshrink_area, precision);
    resized_area.resize(col_max_char[tag_area], ' ');
    VTR_LOG("%s\t", resized_area.c_str());

    if (verbose) {
      std::string resized_target_util =
        to_string_with_precision(avg_target_util, precision) + std::string("%");
      resized_target_util.resize(col_max_char[tag_target_util], ' ');
      VTR_LOG("%s\t", resized_target_util.c_str());

      std::string resized_actual_util =
        to_string_with_precision(avg_util, precision) + std::string("%");
      resized_actual_util.resize(col_max_char[tag_actual_util], ' ');
      VTR_LOG("%s\t", resized_actual_util.c_str());
    }

    /* End the line */
    VTR_LOG("\n");
  }
  VTR_LOGV(verbose, "\n");

  return status;
}

/** Show a summary about the floorplanning results on the screen
 * - List the height, width, utilization of tiles
 * - Show the height, width and utlization of the eFPGA (pre-shrink)
 * - Show the height, width and utlization of the eFPGA (post-shrink)
 */
int report_floorplan(const std::string& shaping_rpt, const std::string& proj_name,
                     const std::string& family, const std::string& pdk_name,
                     const std::string& run_name, const TileQor& tile_qor,
                     const float& shrink_factor, const size_t& precision, const bool& verbose) {
  VTR_LOG(
    "The following tile-level floorplanning results are based on gate-level netlists along with "
    "floorplanning strategies\n");
  VTR_LOGV(!verbose, "Turn on verbose mode for more details about post-shrink area\n");
  VTR_LOG("\n");
  VTR_LOGV(verbose, "eFPGA project name: %s\n", proj_name.c_str());
  VTR_LOGV(verbose, "architecture family: %s\n", family.c_str());
  VTR_LOGV(verbose, "PDK name: %s\n", pdk_name.c_str());
  VTR_LOGV(verbose, "netlist run: %s\n", run_name.c_str());
  VTR_LOGV(verbose, "Shrink factor: %.2g\n", shrink_factor);
  VTR_LOGV(verbose, "\n\n");

  int status = report_floorplan_shaping_txt(tile_qor, shaping_rpt, precision);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("\n");

  status = report_floorplan_tile(tile_qor, shrink_factor, precision, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOG("\n");
  status = report_floorplan_fpga(tile_qor, shrink_factor, precision, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

#ifdef USE_QT
static int draw_signal_distribution_network_on_floorplan_figure(QPainter& painter,
                                                                const TileQor& tile_qor,
                                                                const float& scale_fac,
                                                                const bool& verbose) {
  for (auto ntwk_name : tile_qor.signal_distribution_networks()) {
    VTR_LOGV(verbose, "Drawing signal distribution network '%s'\n", ntwk_name.c_str());
    /* Walk through all the buffers */
    for (auto ntwk_buf : tile_qor.signal_distribution_network_buffers(ntwk_name)) {
      /* Draw buffer with the given coordinate */
      vtr::Point<float> buf_ctr_loc =
        tile_qor.signal_distribution_network_buffer_physical_location(ntwk_name, ntwk_buf);
      /* Get triangle coordinates */
      QPolygonF triangle;
      /* TODO: The mark size should be significant enough to see on figure. Also it should be
       * smaller than a gap to reflect its physical size. */
      float mark_size = tile_qor.horizontal_tile_gap() * scale_fac / 2.;
      triangle << QPoint(buf_ctr_loc.x() * scale_fac, buf_ctr_loc.y() * scale_fac - mark_size)
               << QPoint(buf_ctr_loc.x() * scale_fac - mark_size,
                         buf_ctr_loc.y() * scale_fac + mark_size)
               << QPoint(buf_ctr_loc.x() * scale_fac + mark_size,
                         buf_ctr_loc.y() * scale_fac + mark_size);
      VTR_LOGV(verbose, "Drawing signal distribution network buffer at (x=%g, y=%g)\n",
               buf_ctr_loc.x(), buf_ctr_loc.y());
      painter.setBrush(Qt::blue);
      painter.drawPolygon(triangle);
      /* Draw connects to its fanout tiles */
      if (tile_qor.is_signal_distribution_network_leaf_buffer(ntwk_name, ntwk_buf)) {
        for (auto leaf_tile_coord :
             tile_qor.signal_distribution_network_buffer_leaf_tiles(ntwk_name, ntwk_buf)) {
          vtr::Point<float> leaf_tile_ctr_loc(tile_qor.efpga_physical_center_x(leaf_tile_coord),
                                              tile_qor.efpga_physical_center_y(leaf_tile_coord));
          VTR_LOGV(verbose,
                   "Drawing connections between signal distribution network buffer at (x=%g, y=%g) "
                   "and sink tile[%lu][%lu]\n",
                   buf_ctr_loc.x(), buf_ctr_loc.y(), leaf_tile_coord.x(), leaf_tile_coord.y());
          QLineF line(buf_ctr_loc.x() * scale_fac, buf_ctr_loc.y() * scale_fac,
                      leaf_tile_ctr_loc.x() * scale_fac, leaf_tile_ctr_loc.y() * scale_fac);
          QPen pen(Qt::red, 3, Qt::SolidLine);
          painter.setPen(pen);
          painter.drawLine(line);
        }
        continue;
      }
      /* Draw connects to its fanout buffer */
      for (auto next_buf :
           tile_qor.signal_distribution_network_buffer_fanouts(ntwk_name, ntwk_buf)) {
        vtr::Point<float> next_buf_ctr_loc =
          tile_qor.signal_distribution_network_buffer_physical_location(ntwk_name, next_buf);
        VTR_LOGV(verbose,
                 "Drawing connections between signal distribution network buffers from (x=%g, "
                 "y=%g) to (x=%g, y=%g)\n",
                 buf_ctr_loc.x(), buf_ctr_loc.y(), next_buf_ctr_loc.x(), next_buf_ctr_loc.y());
        QLineF line(buf_ctr_loc.x() * scale_fac, buf_ctr_loc.y() * scale_fac,
                    next_buf_ctr_loc.x() * scale_fac, next_buf_ctr_loc.y() * scale_fac);
        QPen pen(Qt::blue, 10, Qt::SolidLine);
        painter.setPen(pen);
        painter.drawLine(line);
      }
    }
  }
  return openfpga::CMD_EXEC_SUCCESS;
}

int draw_floorplan_figure(const std::string& fpath, const TileQor& tile_qor,
                          const bool& mark_sig_distr_ntwk, const float& scale_fac,
                          const bool& verbose) {
  int status = openfpga::CMD_EXEC_SUCCESS;
  int argc = 0;
  QGuiApplication app(argc, nullptr);
  /* Create a new image with white background */
  float ratio = scale_fac;
  QImage image(tile_qor.efpga_width() * ratio, tile_qor.efpga_height() * ratio,
               QImage::Format_RGB32);
  image.fill(Qt::white);
  /* Initialize the painter */
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  /* Setup the pen and brush */
  painter.setPen(QPen(Qt::white, 2));
  painter.setBrush(Qt::gray);

  QTextOption text_opt(Qt::AlignCenter);
  text_opt.setWrapMode(QTextOption::WordWrap);

  /* Start drawing tiles */
  for (auto tile : tile_qor.tiles()) {
    std::vector<std::string> inst;
    std::vector<vtr::Point<size_t>> coords;
    int l_status = tile_qor.tile_instance_and_coord_pairs(tile, inst, coords, verbose);
    if (l_status != openfpga::CMD_EXEC_SUCCESS) continue;
    VTR_ASSERT_SAFE(inst.size() == coords.size());
    for (size_t iinst = 0; iinst < inst.size(); ++iinst) {
      vtr::Point<size_t> lgk_coord = coords[iinst];
      vtr::Point<float> phy_loc(tile_qor.efpga_physical_start_x(lgk_coord),
                                tile_qor.efpga_physical_start_y(lgk_coord));
      if (phy_loc.x() * ratio > image.width() || phy_loc.y() * ratio > image.height() ||
          phy_loc.x() < 0. || phy_loc.y() < 0.) {
        VTR_LOG_ERROR(
          "Bottom left point (x=%g, y=%g) of instance '%s' of tile %s is out of bounding box "
          "top-right point (x=%g, y=%g)!",
          phy_loc.x() * ratio, phy_loc.y() * ratio, inst[iinst].c_str(), tile.c_str(),
          image.width(), image.height());
        return openfpga::CMD_EXEC_SUCCESS;
      }
      vtr::Point lgk_end_coord(lgk_coord.x(), lgk_coord.y());
      if (tile_qor.grid_width(tile) > 1) {
        lgk_end_coord.set_x(lgk_coord.x() + tile_qor.grid_width(tile) - 1);
      }
      if (tile_qor.grid_height(tile) > 1) {
        lgk_end_coord.set_y(lgk_coord.y() + tile_qor.grid_height(tile) - 1);
      }
      float tile_width = tile_qor.efpga_physical_end_x(lgk_end_coord) - phy_loc.x();
      float tile_height = tile_qor.efpga_physical_end_y(lgk_end_coord) - phy_loc.y();
      QRectF curr_rect(phy_loc.x() * ratio, phy_loc.y() * ratio, tile_width * ratio,
                       tile_height * ratio);
      painter.drawRect(curr_rect);
      std::string curr_text_str = inst[iinst] + std::string("\n(") + tile + std::string(")");
      VTR_LOGV(verbose,
               "Adding text '%s' to rect\n bottom-left (x=%g, y=%g) -> top-right (x=%g ,y=%g)\n",
               curr_text_str.c_str(), phy_loc.x() * ratio, phy_loc.y() * ratio,
               phy_loc.x() * ratio + tile_width * ratio, phy_loc.y() * ratio + tile_height * ratio);
      painter.setPen(Qt::black);
      painter.setFont(QFont("Arial", 10 * ratio, QFont::Bold));
      /* Flip the text drawing, the whole image will be flipped in the end. So the text will be
       * shown in a normal way */
      painter.save();
      qreal cx = curr_rect.center().x();
      qreal cy = curr_rect.center().y();
      painter.translate(cx, cy);
      painter.scale(1.0, -1.0);
      painter.translate(-cx, -cy);
      painter.drawText(curr_rect, Qt::AlignCenter, curr_text_str.c_str());
      painter.restore();
      VTR_LOGV(verbose, "Text added\n");
    }
  }

  /* Add signal distribution network buffers if required */
  if (mark_sig_distr_ntwk) {
    status =
      draw_signal_distribution_network_on_floorplan_figure(painter, tile_qor, ratio, verbose);
    if (status != openfpga::CMD_EXEC_SUCCESS) {
      VTR_LOG_ERROR(
        "Error occured when drawing signal distribution network on floorplanning results\n");
      return openfpga::CMD_EXEC_FATAL_ERROR;
    }
  }

  /* Finalize and save */
  painter.end();
  /* Due to the Qpainter system is based on top-left as (0, 0). Save the image by flipping
   * vertically (upside down) */
  QImage flipped_image = image.mirrored(false, true);
  std::filesystem::path dirpath = fpath;
  openfpga::create_directory(dirpath.parent_path().string(), true, verbose);
  flipped_image.save(fpath.c_str());
  return status;
}
#endif

int report_floorplan_shaping_txt(const TileQor& tile_qor, const std::string& file_name,
                                 const size_t& precision) {
  int status = openfpga::CMD_EXEC_FATAL_ERROR;
  status = tile_qor.generate_shaping_txt(file_name, precision);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  return status;
}
/** Write a CSV summary about the floorplanning results on the screen
 * - List the height, width, utilization of tiles
 * - Show the height, width and utlization of the eFPGA (pre-shrink)
 * - Show the height, width and utlization of the eFPGA (post-shrink)
 */
int report_floorplan_to_csv(const std::string& rpt_fpath, const std::string& proj_name,
                            const std::string& family, const std::string& pdk_name,
                            const std::string& run_name, const TileQor& tile_qor,
                            const float& shrink_factor, const size_t& precision,
                            const bool& verbose) {
  /* create directories required by the report file path */
  std::filesystem::path csv_fpath(rpt_fpath);
  csv_fpath = std::filesystem::absolute(csv_fpath);
  if (!csv_fpath.parent_path().string().empty()) {
    openfpga::create_directory(csv_fpath.parent_path().string(), true, verbose);
  }
  rapidcsv::Document doc("", rapidcsv::LabelParams(), rapidcsv::SeparatorParams());
  vtr::ScopedStartFinishTimer timer("Write floorplan summary to file");
  VTR_LOG("Write floorplan summary to CSV file:\n\t%s\n", csv_fpath.string().c_str());

  size_t row_idx = 0;

  /* Add headers */
  if (verbose) {
    std::vector<std::string> curr_row;
    curr_row.push_back("eFPGA project name");
    curr_row.push_back(proj_name);
    doc.InsertRow(row_idx, curr_row);
    row_idx++;

    curr_row.clear();
    curr_row.push_back("architecture family");
    curr_row.push_back(family);
    doc.InsertRow(row_idx, curr_row);
    row_idx++;

    curr_row.clear();
    curr_row.push_back("PDK name");
    curr_row.push_back(pdk_name);
    doc.InsertRow(row_idx, curr_row);
    row_idx++;

    curr_row.clear();
    curr_row.push_back("Netlist run");
    curr_row.push_back(run_name);
    doc.InsertRow(row_idx, curr_row);
    row_idx++;

    curr_row.clear();
    curr_row.push_back("Shrink factor");
    curr_row.push_back(to_string_with_precision(shrink_factor, precision));
    doc.InsertRow(row_idx, curr_row);
    row_idx++;
  }

  doc.InsertRow(row_idx, std::vector<std::string>()); /* Add splitter */
  row_idx++;

  int status = report_csv_floorplan_tile(doc, row_idx, tile_qor, shrink_factor, precision, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  doc.InsertRow(row_idx, std::vector<std::string>()); /* Add splitter */
  row_idx++;

  status = report_csv_floorplan_fpga(doc, row_idx, tile_qor, shrink_factor, precision, verbose);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* write csv */
  std::ofstream csv_f(csv_fpath.c_str());
  if (csv_f.is_open()) {
    doc.Save(csv_f);
    csv_f.close();
  } else {
    VTR_LOG_ERROR("Error opening csv file for writing\n");
  }
  VTR_LOG("Done\n");

  return openfpga::CMD_EXEC_SUCCESS;
}

/* Output buffer-related information of signal distribution networks to an XML file */
int report_floorplan_signal_distribution_network_to_xml(const TileQor& tile_qor,
                                                        const std::string& fname) {
  vtr::ScopedStartFinishTimer timer(
    "Write signal distribution network floorplaning results to file");

  pugi::xml_document out_xml;

  pugi::xml_node root_node = out_xml.append_child(XML_SIG_DISTR_NTWK_ROOT_NODE_NAME);

  /* Output each network */
  int status = tile_qor.append_xml_signal_distribution_network(root_node);
  if (status != openfpga::CMD_EXEC_SUCCESS) {
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  out_xml.save_file(fname.c_str());

  return openfpga::CMD_EXEC_SUCCESS;
}

/* Write io physical constraints to a text file */
int report_floorplan_tile_pin_physical_location_to_text(const TileQor& tile_qor,
                                                        const std::string& rpt_fpath,
                                                        const bool& verbose) {
  /* create directories required by the report file path */
  std::filesystem::path txt_fpath(rpt_fpath);
  txt_fpath = std::filesystem::absolute(txt_fpath);
  if (!txt_fpath.parent_path().string().empty()) {
    openfpga::create_directory(txt_fpath.parent_path().string(), true, verbose);
  }
  std::ofstream ofs(txt_fpath, std::ios::out);  // append mode
  if (!ofs) {
    VTR_LOG_ERROR("Cannot create file: '%s'\n", rpt_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  ofs << "# THIS FILE IS AUTOMATICALLY GENERATED. DO NOT MODIFY MANUALLY\n";
  /* TODO: Use constants here instead of hardcoding */
  for (auto curr_pinloc : tile_qor.tile_pin_loc()) {
    ofs << "if {[sizeof [get_pins -of_objects [get_cells */* -filter ref_name==" << curr_pinloc.tile
        << "] -filter name==" << curr_pinloc.pin2str() << "]] > 0} {" << std::endl;
    ofs << "\tset_individual_pin_constraints -pins [get_pins -of_objects [get_cells */* -filter "
           "ref_name=="
        << curr_pinloc.tile << "] -filter name==" << curr_pinloc.pin2str() << "]"
        << " -sides " << tile_qor.side2int(curr_pinloc.pin, curr_pinloc.tile, curr_pinloc.side)
        << std::endl;
    ofs << "}" << std::endl;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

int report_floorplan_io_physical_location_to_text(const TileQor& tile_qor,
                                                  const std::string& rpt_fpath,
                                                  const bool& verbose) {
  /* create directories required by the report file path */
  std::filesystem::path txt_fpath(rpt_fpath);
  txt_fpath = std::filesystem::absolute(txt_fpath);
  if (!txt_fpath.parent_path().string().empty()) {
    openfpga::create_directory(txt_fpath.parent_path().string(), true, verbose);
  }
  std::ofstream ofs(txt_fpath, std::ios::out);  // append mode
  if (!ofs) {
    VTR_LOG_ERROR("Cannot create file: '%s'\n", rpt_fpath.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  /* TODO: Use constants here instead of hardcoding */
  ofs << "# THIS FILE IS AUTOMATICALLY GENERATED. DO NOT MODIFY MANUALLY\n";
  for (auto curr_io : tile_qor.io_locations()) {
    std::string ofs_str = std::string(" -offset {");
    /* the offset should cover a complete tile gap at the pin location! */
    /* Add the offsets which is dependent on the side to place the pin */
    if (curr_io.side == TOP || curr_io.side == BOTTOM) {
      ofs_str += std::to_string(curr_io.phy_loc.x());
      ofs_str += std::string(" ");
      ofs_str += std::to_string(curr_io.phy_loc.x() + tile_qor.horizontal_tile_gap());
    } else {
      /* Sanity check should already be done in the tile_qor data structure. */
      VTR_ASSERT_SAFE(curr_io.side == LEFT || curr_io.side == RIGHT);
      ofs_str += std::to_string(curr_io.phy_loc.y());
      ofs_str += std::string(" ");
      ofs_str += std::to_string(curr_io.phy_loc.y() + tile_qor.vertical_tile_gap());
    }
    ofs_str += std::string("}");

    ofs << "set_individual_pin_constraints "
        << " -ports [get_ports " << curr_io.name.to_simple_verilog_string() << "]"
        << " -allowed_layers [get_layers " << curr_io.metal_layer << "]"
        << " -sides {" << tile_qor.side2int(curr_io.name, "fpga top module", curr_io.side) << "}"
        << ofs_str << " -width " << curr_io.width << " -length " << curr_io.length << std::endl;
  }

  return openfpga::CMD_EXEC_SUCCESS;
}

}  // End of namespace arkangel
