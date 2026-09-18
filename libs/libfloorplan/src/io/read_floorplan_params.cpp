#include "read_floorplan_params.h"

#include <fstream>

#include "floorplan_argparse.h"
#include "floorplan_flow_manager.h"
#include "rapidcsv.h"
#include "read_yaml_tile_ref.h"
#include "tile_qor.h"
#include "tile_ref.h"
namespace arkangel {
std::vector<std::vector<int>> parseToVector(const std::vector<std::string>& input) {
  std::vector<std::vector<int>> result;
  for (const std::string& s_raw : input) {
    std::string s = s_raw.substr(1, s_raw.size() - 2);  // Remove '(' and ')'
    std::vector<int> result_i;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
      result_i.push_back(std::stoi(item));
    }
    result.push_back(result_i);
  }
  return result;
}

int parse_yaml_to_map(std::map<std::string, std::string>& desitination, const YAML::Node& source) {
  for (const auto& node : source) {
    if (node.second.IsScalar()) {
      desitination[node.first.as<std::string>()] = node.second.as<std::string>();
    } else {
      std::cout << "Too much hierachy in the yaml file, please check it!" << std::endl;
      return 1;
    }
  }
  return 0;
}

int parse_yaml_to_map(std::map<std::string, float>& desitination, const YAML::Node& source) {
  for (const auto& node : source) {
    if (node.second.IsScalar()) {
      desitination[node.first.as<std::string>()] = node.second.as<float>();
    } else {
      std::cout << "Too much hierachy in the yaml file, please check it!" << std::endl;
      return 1;
    }
  }
  return 0;
}

int init_floorplan_params_with_yaml_file(const char* config_file,
                                         FloorplanParams* floorplan_params) {
  /* Read in user options */
  YAML::Node config = YAML::LoadFile(config_file);

  // parse tileinfo
  std::cout << "Start parsing tile info" << std::endl;
  floorplan_params->tile_params.tile_width_height_ratio =
    config[e_floorplan_yaml_tag::TILE_WIDTH_HEIGHT_RATIO_TAG].as<float>();
  floorplan_params->tile_params.tile_size_generation =
    config[e_floorplan_yaml_tag::TILE_SIZE_GENERATION_TAG].as<std::string>();

  // parse tile_gap
  if (config[e_floorplan_yaml_tag::TILE_GAP_TAG]) {
    for (const auto& node : config["tile_gap"]) {
      std::string value = node.as<std::string>();

      std::regex pattern(R"(\*(\d+))");  // cpp*7 sc_height*1, we just need to get (7,1)
      std::smatch match;
      if (std::regex_search(value, match, pattern)) {
        int number = std::stoi(match[1]);
        floorplan_params->tile_params.tile_gap.push_back(number);
      }
    }
  }

  // parse utilization
  if (config[e_floorplan_yaml_tag::UTILIZATION_TAG]) {
    parse_yaml_to_map(floorplan_params->utilization_map,
                      config[e_floorplan_yaml_tag::UTILIZATION_TAG]);
  }

  // parse general info
  floorplan_params->num_err = 0;
  floorplan_params->rst_num = config[e_floorplan_yaml_tag::RST_NUM_TAG].as<int>();
  floorplan_params->stack_by_col = config[e_floorplan_yaml_tag::STACK_BY_COL_TAG].as<int>();
  floorplan_params->use_coner_io = config[e_floorplan_yaml_tag::USE_CORNER_IO_TAG].as<int>();
  floorplan_params->sc_height = config[e_floorplan_yaml_tag::SC_HEIGHT_TAG].as<float>();
  floorplan_params->cpp = config[e_floorplan_yaml_tag::CPP_TAG].as<float>();
  floorplan_params->scale = config[e_floorplan_yaml_tag::SCALE_TAG].as<float>();
  floorplan_params->boundary_cell_width =
    config[e_floorplan_yaml_tag::BOUNDARY_CELL_WIDTH_TAG].as<float>();
  floorplan_params->bram_macro_area = config[e_floorplan_yaml_tag::BRAM_MACRO_AREA_TAG].as<float>();

  // The following codes are redundant, may be cleaned up later
  //  parse tile size
  for (auto it : config[e_floorplan_yaml_tag::TILE_SIZE_TAG]) {
    std::string tile_type = it.first.as<std::string>();
    YAML::Node props = it.second;
    TileProperty t;
    t.height = props["height"].as<std::string>();
    t.width = props["width"].as<std::string>();
    t.cell_area = props["cell_area"].as<std::string>();
    floorplan_params->tile_params.tile_size[tile_type] = t;
  }
  // parse tile_list
  const YAML::Node& raw_tile_list = config[e_floorplan_yaml_tag::TILE_LIST_TAG];
  TileList tile_cores;
  tile_cores.construct_tile_list_from_yaml(raw_tile_list);
  floorplan_params->tile_params.tile_list = tile_cores;
  parse_yaml_to_map(floorplan_params->tile_params.tile_overwrite_size_col,
                    config[e_floorplan_yaml_tag::TILE_OVWRITE_SIZE_COL_TAG]);
  parse_yaml_to_map(floorplan_params->tile_params.tile_overwrite_size_row,
                    config[e_floorplan_yaml_tag::TILE_OVWRITE_SIZE_ROW_TAG]);
  parse_yaml_to_map(floorplan_params->tile_params.tile_overwrite_size_specific,
                    config[e_floorplan_yaml_tag::TILE_OVWRITE_SIZE_SPECIFIC_TAG]);

  parse_yaml_to_map(floorplan_params->tile_params.side_gap,
                    config[e_floorplan_yaml_tag::SIDE_GAP_TAG]);
  parse_yaml_to_map(floorplan_params->tile_params.extra_gap,
                    config[e_floorplan_yaml_tag::EXTRA_GAP_TAG]);
  parse_yaml_to_map(floorplan_params->tile_params.tile_logic_rem,
                    config[e_floorplan_yaml_tag::TILE_LOGIC_REM_TAG]);
  // parse fabric core into vector of vector of int
  floorplan_params->tile_params.fabric_core =
    parseToVector(config[e_floorplan_yaml_tag::FABRIC_CORE_TAG].as<std::vector<std::string>>());
  floorplan_params->tile_params.fabric_side =
    parseToVector(config[e_floorplan_yaml_tag::FABRIC_SIDE_TAG].as<std::vector<std::string>>());

  // parse clk info
  floorplan_params->clock_info.clk_num = config[e_floorplan_yaml_tag::CLK_NUM_TAG].as<int>();
  floorplan_params->clock_info.ckbuf_width_buffer =
    config[e_floorplan_yaml_tag::CKBUF_WIDTH_TAG].as<std::vector<std::string>>();
  floorplan_params->clock_info.ckbuf_height_buffer =
    config[e_floorplan_yaml_tag::CKBUF_HEIGHT_TAG].as<std::vector<std::string>>();
  floorplan_params->clock_info.clk_entry =
    config[e_floorplan_yaml_tag::CLK_ENTRY_TAG].as<std::string>();
  floorplan_params->clock_info.clk_htree_lvl =
    config[e_floorplan_yaml_tag::CLK_HTREE_LVL_TAG].as<int>();
  if (config[e_floorplan_yaml_tag::CLK_LOGIC_REM_TAG]) {
    floorplan_params->clock_info.clk_logic_rem =
      config[e_floorplan_yaml_tag::CLK_LOGIC_REM_TAG].as<std::vector<std::string>>();
  }
  if (config[e_floorplan_yaml_tag::CLK_LOGIC_ADD_TAG]) {
    std::vector<std::string> input =
      config[e_floorplan_yaml_tag::CLK_LOGIC_ADD_TAG].as<std::vector<std::string>>();
    floorplan_params->clock_info.clk_logic_add = parseToVector(input);
  }
  floorplan_params->clock_info.clk_buf_interval =
    config[e_floorplan_yaml_tag::CLK_BUF_INTERVAL_TAG].as<int>();
  floorplan_params->clock_info.clk_lftbt_in =
    config[e_floorplan_yaml_tag::CLK_LFTBT_IN_TAG].as<int>();

  // parse clk0_logic_loc
  for (auto it : config[e_floorplan_yaml_tag::CLK0_LOGIC_LOC_TAG]) {
    std::string tile_type = it.first.as<std::string>();
    YAML::Node props = it.second;
    Clk0LogicLoc t;

    std::string coord = props["coord"].as<std::string>();
    int x = 0, y = 0;
    std::stringstream ss(coord);
    std::string item;
    if (std::getline(ss, item, ',')) {
      x = std::stoi(item);
    }
    if (std::getline(ss, item, ',')) {
      y = std::stoi(item);
    }

    t.coord_x = x;
    t.coord_y = y;
    t.parent = props["parent"].as<int>();
    t.buf = props["uf"].as<int>();
    floorplan_params->clock_info.clk0_logic_loc.push_back(t);
  }

  // parse clk_logic_loc
  for (const auto& node : config[e_floorplan_yaml_tag::CLK_LOGIC_LOC_TAG]) {
    ClkLogicLoc t;
    t.level = node["level"].as<int>();
    std::string coord = node["coord"].as<std::string>();
    int x = 0, y = 0;
    std::stringstream ss(coord);
    std::string item;
    if (std::getline(ss, item, ',')) {
      x = std::stoi(item);
    }
    if (std::getline(ss, item, ',')) {
      y = std::stoi(item);
    }

    t.coord_x = x;
    t.coord_y = y;
    t.parent = node["parent"].as<int>();
    t.direction = node["direction"].as<std::string>();
    floorplan_params->clock_info.clk_logic_loc.push_back(t);
  }
  return 0;
}

// read qor file and store the information into tile_ref and tile_qor
int read_qor_file(const char* tile_ref_file, const char* tile_qor_csv_file,
                  arkangel::TileQor& tile_qor) {
  arkangel::TileRef tile_ref;
  int status = arkangel::read_yaml_tile_ref(tile_ref_file, tile_ref, true);
  if (status) {
    VTR_LOG_ERROR("Failed to read tile reference file!");
    VTR_ASSERT(status == 0);
  }
  tile_qor = arkangel::TileQor(tile_ref);

  try {
    rapidcsv::Document csvFile(tile_qor_csv_file, rapidcsv::LabelParams(-1, -1));  // no header

    size_t rowCount = csvFile.GetRowCount();
    for (size_t i = 0; i < rowCount; ++i) {
      std::vector<std::string> row = csvFile.GetRow<std::string>(i);
      if (row.size() < 2) continue;

      const std::string& key = row[0];
      const std::string& value_str = row[1];
      try {
        double value = std::stod(value_str);
        tile_qor.set_cell_area(key, value);
        tile_qor.set_utilization(key, tile_qor.default_utilization());
      } catch (...) {
        VTR_LOG("Warning: Invalid float in row: %d", i);
      }
    }
  } catch (const std::exception& e) {
    VTR_LOG_ERROR("Error loading TileQor CSV!");
  }
  // finshed reading qor file
  return 0;
}
}  // namespace arkangel