#pragma once
#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>
namespace arcxpress {

// yaml tags. when we read in task.yaml, we will use these tags to parse the yaml file
namespace e_floorplan_yaml_tag {
constexpr const char* CLK_NUM_TAG = "clk_num";
constexpr const char* RST_NUM_TAG = "rst_num";
constexpr const char* STACK_BY_COL_TAG = "stack_by_col";
constexpr const char* USE_CORNER_IO_TAG = "use_corner_io";
constexpr const char* SC_HEIGHT_TAG = "sc_height";
constexpr const char* CPP_TAG = "cpp";
constexpr const char* CKBUF_WIDTH_TAG = "ckbuf_width";
constexpr const char* CKBUF_HEIGHT_TAG = "ckbuf_height";
constexpr const char* CLK_ENTRY_TAG = "clk_entry";
constexpr const char* TILE_LIST_TAG = "tile_list";
constexpr const char* TILE_SIZE_TAG = "tile_size";
constexpr const char* TILE_SIZE_HEIGHT_TAG = "height";
constexpr const char* TILE_SIZE_WIDTH_TAG = "width";
constexpr const char* TILE_SIZE_CELLAREA_TAG = "cell_area";
constexpr const char* TILE_OVWRITE_SIZE_COL_TAG = "tile_ovwrite_size_col";
constexpr const char* TILE_OVWRITE_SIZE_ROW_TAG = "tile_ovwrite_size_row";
constexpr const char* TILE_OVWRITE_SIZE_SPECIFIC_TAG = "tile_ovwrite_size_specific";
constexpr const char* TILE_GAP_TAG = "tile_gap";
constexpr const char* SIDE_GAP_TAG = "side_gap";
constexpr const char* SIDE_GAP_LEFT_TAG = "left";
constexpr const char* SIDE_GAP_TOP_TAG = "top";
constexpr const char* SIDE_GAP_RIGHT_TAG = "right";
constexpr const char* SIDE_GAP_BOTTOM_TAG = "bottom";
constexpr const char* EXTRA_GAP_TAG = "extra_gap";
constexpr const char* FABRIC_CORE_TAG = "fabric_core";
constexpr const char* FABRIC_SIDE_TAG = "fabric_side";
constexpr const char* FABRIC_SIDE_LEFT_TAG = "left";
constexpr const char* FABRIC_SIDE_TOP_TAG = "top";
constexpr const char* FABRIC_SIDE_RIGHT_TAG = "right";
constexpr const char* FABRIC_SIDE_BOTTOM_TAG = "bottom";
constexpr const char* TILE_LOGIC_REM_TAG = "tile_logic_rem";
constexpr const char* CLK_HTREE_LVL_TAG = "clk_htree_lvl";
constexpr const char* CLK_LOGIC_LOC_TAG = "clk_logic_loc";
constexpr const char* CLK_LOGIC_LOC_LEVEL_TAG = "level";
constexpr const char* CLK_LOGIC_LOC_COORD_TAG = "coord";
constexpr const char* CLK_LOGIC_LOC_PARENT_TAG = "parent";
constexpr const char* CLK_LOGIC_LOC_DIR_TAG = "direction";
constexpr const char* CLK_LOGIC_REM_TAG = "clk_logic_rem";
constexpr const char* CLK_LOGIC_ADD_TAG = "clk_logic_add";
constexpr const char* CLK_BUF_INTERVAL_TAG = "clk_buf_interval";
constexpr const char* CLK_LFTBT_IN_TAG = "clk_lftbt_in";
constexpr const char* CLK0_LOGIC_LOC_TAG = "clk0_logic_loc";
constexpr const char* CLK0_LOGIC_LOC_COORD_TAG = "coord";
constexpr const char* CLK0_LOGIC_LOC_PARENT_TAG = "parent";
constexpr const char* CLK0_LOGIC_LOC_BUF_TAG = "buf";
constexpr const char* SCALE_TAG = "scale";
constexpr const char* TILE_SIZE_GENERATION_TAG = "tile_size_generation";
constexpr const char* BOUNDARY_CELL_WIDTH_TAG = "boundary_cell_width";
constexpr const char* BRAM_MACRO_AREA_TAG = "bram_macro_area";
constexpr const char* TILE_WIDTH_HEIGHT_RATIO_TAG = "tile_width_height_ratio";
constexpr const char* DIR_H_TAG = "horizental";
constexpr const char* DIR_V_TAG = "vertical";
constexpr const char* UTILIZATION_TAG = "utilization";
}  // namespace e_floorplan_yaml_tag

/*============= The following codes are all redundant ================ */
// redundant code
struct ClkLogicLoc {
  int level;
  int coord_x;
  int coord_y;
  int parent;
  std::string direction;

  ClkLogicLoc(int level, int x, int y, int p, const std::string& d) {
    level = level;
    coord_x = x;
    coord_y = y;
    parent = p;
    direction = d;
  }
  ClkLogicLoc() = default;
};

struct Clk0LogicLoc {
  int coord_x;
  int coord_y;
  int parent;
  int buf;
};

// redundant code
struct ClockInfo {
  int clk_num;
  std::string clk_entry;
  std::vector<std::string> ckbuf_width_buffer;
  std::vector<std::string> ckbuf_height_buffer;
  int clk_htree_lvl;
  std::vector<ClkLogicLoc> clk_logic_loc;
  std::vector<std::string> clk_logic_rem;
  std::vector<std::vector<int>> clk_logic_add;

  int clk_buf_interval;
  int clk_lftbt_in;
  std::vector<Clk0LogicLoc> clk0_logic_loc;
  std::vector<float> ckbuf_width = {0, 0, 0, 0, 0};
  std::vector<float> ckbuf_height = {0, 0, 0, 0, 0};
};

// redundant code
struct TileProperty {
  std::string height;
  std::string width;
  std::string cell_area;
};

// redundant code
struct TileLocInstance {
  std::string tile_type;
  int coord_x;
  int coord_y;
};

// redundant code
class TileLogicLoc {
 public:
  TileLogicLoc() = default;
  TileLogicLoc(int id, int row, int col, TileLocInstance t) : id_(id), row_(row), col_(col) {
    tile_logic_loc_map_[std::make_pair(row, col)] = t;
  }

  void add_tile_logic_loc(int row, int col, const TileLocInstance& t) {
    if (id_.empty()) {
      id_.push_back(0);
    } else {
      id_.push_back(id_.back() + 1);
    }
    row_.push_back(row);
    col_.push_back(col);
    tile_logic_loc_map_[std::make_pair(row, col)] = t;
  }

  TileLocInstance get_tile_logic_loc(int row, int col) {
    auto it = tile_logic_loc_map_.find(std::make_pair(row, col));
    if (it != tile_logic_loc_map_.end()) {
      return it->second;
    }
    return TileLocInstance();
  }

  std::vector<TileLocInstance> get_tile_logic_loc(int row) {
    std::vector<TileLocInstance> specific_row;
    for (auto row_index : row_) {
      if (row_index == row) {
        specific_row.push_back(tile_logic_loc_map_[std::make_pair(row, col_[row_index])]);
      }
    }
    return specific_row;
  }

  std::vector<int> get_col_index(int row) {
    std::vector<int> specific_col;
    for (auto row_index : row_) {
      if (row_index == row) {
        specific_col.push_back(col_[row_index]);
      }
    }
    return specific_col;
  }

  int get_last_row() {
    if (row_.empty()) return 0;
    return *std::max_element(row_.begin(), row_.end());
  }

  int get_first_col(int row) {
    std::vector<int> specific_col;
    for (auto row_index : row_) {
      if (row_index == row) {
        specific_col.push_back(col_[row_index]);
      }
    }
    if (specific_col.empty()) return 0;
    return *std::min_element(specific_col.begin(), specific_col.end());
  }

  int get_last_col(int row) {
    std::vector<int> specific_col;
    for (auto row_index : row_) {
      if (row_index == row) {
        specific_col.push_back(col_[row_index]);
      }
    }
    if (specific_col.empty()) return 0;
    return *std::max_element(specific_col.begin(), specific_col.end());
  }

  int size() { return id_.size(); }

 private:
  std::vector<int> id_;
  std::vector<int> row_;
  std::vector<int> col_;
  std::map<std::pair<int, int>, TileLocInstance> tile_logic_loc_map_;
};

// redundant code
class TileList {
 public:
  int construct_tile_list_from_yaml(const YAML::Node& raw_tile_list) {
    int id = 0;
    for (size_t core_id = 0; core_id < raw_tile_list.size(); ++core_id) {
      std::string line = raw_tile_list[core_id].as<std::string>();
      std::stringstream ss(line);
      std::string tile_info;
      int tile_sn = 0;
      while (std::getline(ss, tile_info, ',')) {
        auto pos = tile_info.find('*');
        std::string type = tile_info.substr(0, pos);
        int count = std::stoi(tile_info.substr(pos + 1));
        for (int i = 0; i < count; ++i) {
          id_.push_back(id++);
          num_core_.push_back(core_id);
          tile_sn_.push_back(tile_sn++);
          type_.push_back(type);
        }
      }
    }
    return 0;
  }

  std::string get_type(int num_core, int tile_sn) {
    for (size_t i = 0; i < id_.size(); ++i) {
      if (num_core_[i] == num_core && tile_sn_[i] == tile_sn) {
        return type_[i];
      }
    }
  }

 private:
  std::vector<int> id_;
  std::vector<std::string> type_;
  std::vector<int> num_core_;
  std::vector<int> tile_sn_;
};

// TileParams is used, but only its internal attribute: tile_gap is used
struct TileParams {
  // redundant code
  TileList tile_list;
  std::map<std::string, TileProperty> tile_size;
  std::string tile_size_generation;
  std::map<std::string, std::string> tile_overwrite_size_col;
  std::map<std::string, std::string> tile_overwrite_size_row;
  std::map<std::string, std::string> tile_overwrite_size_specific;
  std::vector<std::string> tile_gap_buffer;
  std::map<std::string, float> side_gap;
  std::map<std::string, float> extra_gap;
  std::vector<std::vector<int>> fabric_core;
  std::vector<std::vector<int>> fabric_side;
  std::vector<std::string> fabric_side_left;
  std::map<std::string, std::string> tile_logic_rem;
  float tile_width_height_ratio;
  TileLogicLoc tile_logic_loc;
  // the only used attribute is tile_gap
  std::vector<int> tile_gap = {};
  float top_side_gap;
  float bottom_side_gap;
  float right_side_gap;
  float left_side_gap;
  float horizontal_tile_gap;
  float vertical_tile_gap;
};

struct TileInfo {
  float height = -1.;
  float width = -1.;
  TileInfo(const float& height, const float& width) : height(height), width(width) {}
  TileInfo() = default;
};

// params for floorplan, obtained from yaml file
struct FloorplanParams {
  int num_err = 0;
  int rst_num;
  int stack_by_col;
  int use_coner_io;
  float sc_height;
  float cpp;
  float boundary_cell_width;
  float scale;
  float bram_macro_area;
  float default_utilization;
  std::map<std::string, float> utilization_map;
  ClockInfo clock_info;  // redundant data structure
  TileParams tile_params;
};
}  // namespace arcxpress
