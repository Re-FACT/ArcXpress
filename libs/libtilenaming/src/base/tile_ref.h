#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "vtr_geometry.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArkAngel benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class TileRef {
 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  TileRef();

 public: /* -- Aggregators -- */
  /* List all the available tile names*/
  std::vector<std::string> tiles() const;

 public: /* -- Accessors -- */
  size_t num_tiles() const;
  /* Get the instance count for a given tile name */
  size_t num_instances(const std::string& tile) const;
  /* Get the list of coordinates of instances */
  int instance_coordinates(const std::string& tile, std::vector<vtr::Point<size_t>>& coords,
                           const bool& verbose = false) const;
  /*Get the pair of tile instance and its corresponding coords. useful for generating shaping.txt*/
  int tile_instance_and_coord_pairs(const std::string& tile,
                                    std::vector<std::string>& instances_name,
                                    std::vector<vtr::Point<size_t>>& coords,
                                    const bool& verbose = 1) const;

  /* Get the type of functional block of a tile */
  std::string type(const std::string& tile) const;
  /* Generate a custom name for a tile: <name>_Type<type>_Count<count>_Id<idx_in_current_type>*/
  std::string custom_tile_name(const std::string& tile) const;
  /* Check if two tile reference are equivalent. Tile names match and their types match */
  bool check_equivalent(const TileRef& ref, const bool& verbose) const;
  bool tile_exist(const std::string& tile) const;
  /* Get the range of offsets from the grid-level heights of a given tile */
  size_t grid_height(const std::string& tile) const;
  size_t grid_width(const std::string& tile) const;
  std::vector<size_t> grid_height_offsets(const std::string& tile) const;
  std::vector<size_t> grid_width_offsets(const std::string& tile) const;

 public: /* -- Mutators -- */
  bool add_tile(const std::string& tile, const int& height, const int& width);
  bool set_type(const std::string& tile, const std::string& type_str);
  bool add_instance(const std::string& tile, const std::string& inst);
  void clear();

 public: /* -- Validator -- */
  void check_valid() const;
  bool valid_internal_index(const size_t& idx) const;
  size_t find_internal_index(const std::string& tile) const;

 private: /* -- Utility -- */
  int parse_tile_coord(const std::string& inst_name, vtr::Point<size_t>& coord,
                       const bool& verbose = false) const;
  void init();

 private: /* -- Internal data -- */
  /* Name -> internal index */
  std::map<std::string, size_t> name2ids_;
  /* Use internal index to walk through */
  std::vector<size_t> grid_type_ids_;
  std::vector<size_t> grid_height_;
  std::vector<size_t> grid_width_;
  std::vector<std::vector<std::string>> instances_;
  /* Use grid type ids to create */
  std::vector<std::string> grid_types_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress
