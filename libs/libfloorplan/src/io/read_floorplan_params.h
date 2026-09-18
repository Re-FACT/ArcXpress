#pragma once
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "floorplan_params.h"
#include "tile_qor.h"
#include "tile_ref.h"
namespace arkangel {

// util functions used for parsing config yaml file
std::vector<std::vector<int>> parseToVector(const std::vector<std::string>& input);

int parse_yaml_to_map(std::map<std::string, std::string>& desitination, const YAML::Node& source);

int parse_yaml_to_map(std::map<std::string, float>& desitination, const YAML::Node& source);

// read tile_qor.csv file
int read_qor_file(const char* tile_ref_file, const char* tile_qor_csv_file,
                  arkangel::TileQor& tile_qor);

// read floorplan_task.yaml file
int init_floorplan_params_with_yaml_file(const char* config_file,
                                         FloorplanParams* floorplan_params);

}  // namespace arkangel