#pragma once

#include <map>
#include <string>
#include <vector>

#include "openfpga_port.h"
#include "physical_types.h"
#include "vtr_geometry.h"

/* namespace arcxpress begins */
namespace arcxpress {

/**
 * The data structure stores data for ArcXpress benchmark suites
 * It is used to provide necessary information to categorize spot benchmark files
 */
class FloorplanStrategy {
 public: /* -- Types -- */
  struct MetalLayer {
    std::string name;
    float length;
    float width;
    float spacing;
    MetalLayer(const std::string& name, const float& length, const float& width,
               const float& spacing)
      : name(name), length(length), width(width), spacing(spacing) {}
  };
  enum class e_distribution_network_type { HTREE = 0, FISHBONE, NUM_TYPES };
  struct SignalDistributionNetwork {
    /* data */
    openfpga::BasicPort port;
    enum e_distribution_network_type type;
    std::string buf_cell;
    size_t depth;
    vtr::Point<size_t> entry_point;
    /* methods */
    bool auto_depth() const { return depth == 0; }
    bool reach_depth(const size_t& input_depth) const {
      return !auto_depth() && input_depth == depth;
    }
  };

 public: /*  -- Ranges -- */
 public: /* -- Constructors -- */
  FloorplanStrategy(const float& cpp = 0., const float& sc_h = 0.);

 public: /* -- Aggregators -- */
  std::array<e_side, NUM_2D_SIDES> sides() const;
  std::vector<std::string> tiles() const;

 public: /* -- Accessors -- */
  float cpp() const;
  float standard_cell_height() const;
  float boundary_cell_width() const;
  float height_width_ratio() const;
  float tile_default_utilization() const;
  /* If the tile is defined with a custom utilization rate, return it. If not, return the default
   * utilization */
  float tile_utilization(const std::string& tile) const;
  float tile_horizental_gap() const;
  float tile_vertical_gap() const;
  float side_gap(const e_side& side) const;
  bool valid_signal_distribution_network() const;
  std::vector<FloorplanStrategy::SignalDistributionNetwork> signal_distribution_networks() const;
  float metal_layer_length(const std::string& name) const;
  float metal_layer_width(const std::string& name) const;
  float metal_layer_spacing(const std::string& name) const;

 public: /* -- Mutators -- */
  void set_cpp(const float& val);
  void set_standard_cell_height(const float& val);
  void set_boundary_cell_width(const std::string& val);
  void set_height_width_ratio(const float& val);
  void set_tile_default_utilization(const float& val);
  void set_tile_utilization(const std::string& tile, const float& val);
  void set_tile_horizental_gap(const std::string& val);
  void set_tile_vertical_gap(const std::string& val);
  void set_side_gap(const e_side& side, const std::string& val);
  void add_metal_layer(const std::string& name, const float& len, const float& width,
                       const float& spacing);
  bool add_signal_distribution_network(const std::string& port, const std::string& type_str,
                                       const std::string& bufcell, const size_t& depth,
                                       const vtr::Point<size_t>& entry_point);
  void clear();

 public: /* -- Validator -- */
  /* Validate if the content is legal, without the validate. Any data access will cause errors */
  bool validate() const;

  e_distribution_network_type str2distribution_network_type(const std::string& str,
                                                            const bool& verbose = false) const;
  std::string distribution_network_type2str(const e_distribution_network_type& type,
                                            const bool& verbose = false) const;
  std::string distribution_network_type_all2str() const;
  void check_valid_distribution_network_type(const e_distribution_network_type& type) const;
  bool valid_distribution_network_type(const e_distribution_network_type& type) const;
  bool valid_metal_layer(const std::string& name) const;

 private: /* Private utility */
  float parse_gap(const std::string& gap_str) const;
  bool valid_horizental_gap_string(const std::string& gap_str) const;
  bool valid_vertical_gap_string(const std::string& gap_str) const;
  void clear_all();

 private: /* -- Private Validator -- */
  /* Validate and throw errors if seen */
  void check_valid() const;
  void check_valid_side(const e_side& side) const;
  bool valid_side(const e_side& side) const;

 private:                           /* -- Internal data -- */
  float cpp_;                       /* Contact poly pitch (unit: m) */
  float sc_height_;                 /* Height of standard cells */
  std::string boundary_cell_width_; /* width of boundary cells */
  float
    height_width_ratio_; /* The ratio between height and width when apply auto-sizing on a tile */
  float tile_default_util_;                /* The default tile-level utilization */
  std::map<std::string, float> tile_util_; /* utilization per tile if defined */
  std::array<std::string, 2>
    tile_gap_; /* String version of tile gap in horizental = 0 and veritcal = 1 */
  std::array<std::string, NUM_2D_SIDES> side_gap_; /* String version of side gap in 4 sides */

  /* Terminal related */
  std::vector<MetalLayer> metal_layers_;

  /* Detailed networks */
  std::vector<SignalDistributionNetwork> signal_distribution_network_;

  /* Constants */
  std::string KW_CPP_;
  std::string KW_SC_HEIGHT_;
  char KW_MULT_;
  std::array<std::string, size_t(e_distribution_network_type::NUM_TYPES)>
    DISTRIBUTION_NETWORK_TYPE_STRING_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arcxpress
