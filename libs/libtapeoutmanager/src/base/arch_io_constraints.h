#pragma once

#include <map>
#include <string>
#include <vector>

#include "physical_types.h"

/* namespace arkangel begins */
namespace arkangel {

/**
 * The data structure stores data for ArkAngel eFPGA architecture I/O constraints
 * It is used to provide necessary information to categorize spot benchmark files
 */
class ArchIoConstraints {
 public: /* -- Types -- */
  enum class e_io_direction { INPUT = 0, OUTPUT, NUM_TYPES };

 public: /* -- Constructors -- */
  ArchIoConstraints();

 public: /* -- Aggregators -- */
 public: /* -- Accessors -- */
  /* TODO: Calculate the number of io per type:
   * - If I/O number is set to 0, only need to modify the io_type string to empty
   * - If I/O number is set to a integer, consider the max number per type and then spread evenly on
   * the rest. Handle the corner case where the minimum number should be respected
   */
  int find_and_assign_arch_io_type_vars(const e_side& io_side,
                                        std::map<std::string, std::string>& io_vars,
                                        const bool& verbose) const;
  int find_and_assign_arch_io_vars(const e_side& io_side, const e_io_direction& io_type,
                                   const size_t& io_num, const int& height, const int& width,
                                   std::map<std::string, std::string>& io_vars,
                                   const bool& verbose) const;

 public: /* -- Mutators -- */
  bool add_num_io_per_tile_setting(const std::string& side_str, const std::string& func,
                                   const std::string& var_name, const std::string& msb_var_name,
                                   const size_t& max);
  bool add_io_type_setting(const std::string& side_str, const std::string& var_name);

 public: /* -- Validator -- */
  bool validate(const bool& verbose) const;

 public: /* Public utility */
  e_side str2side(const std::string& str, const bool& verbose) const;
  std::string side2str(const e_side& type, const bool& verbose = false) const;
  std::string side_all2str() const;

  e_io_direction str2io_direction(const std::string& str, const bool& verbose) const;
  std::string io_direction2str(const e_io_direction& type, const bool& verbose = false) const;
  std::string io_direction_all2str() const;
  std::vector<e_io_direction> io_directions() const;

  bool valid_io_direction(const e_io_direction& f_type) const;
  bool valid_side(const e_side& f_type) const;

 public: /* Private utility */
  /* Get the minimum number of I/Os per side on a selected io direction */
  size_t min_io_num(const e_side& io_side, const e_io_direction& io_dir, const int& height,
                    const int& width) const;
  /* Return all the variables on a given side and an I/O direction */
  std::vector<std::string> var_names(const e_side& io_side, const e_io_direction& io_dir) const;
  /* Return only the constrained/unconstrained variables on a given side and an I/O direction */
  std::vector<std::string> var_names_constrained(const e_side& io_side,
                                                 const e_io_direction& io_dir) const;
  std::vector<std::string> var_names_unconstrained(const e_side& io_side,
                                                   const e_io_direction& io_dir) const;
  size_t max_num(const std::string& var_name) const;
  /* Return the variable number which represents the I/O type existence */
  std::string io_type_var_name(const e_side& io_side) const;
  size_t io_num_by_side(const e_side& io_side, const size_t& io_num_per_tile, const int& height,
                        const int& width) const;
  size_t io_num_per_tile_by_side(const e_side& io_side, const size_t& io_num, const int& height,
                                 const int& width) const;

 private: /* -- Private Validator -- */
  void check_valid() const;
  void check_valid_side(const e_side& side) const;
  void check_valid_io_direction(const e_io_direction& type) const;

 private: /* -- Internal data -- */
  /* Variable name in architecture variable list, which represents the number of I/O */
  std::map<std::string, size_t> name2ids_;
  /* [side][direction][var_names]*/
  std::array<std::array<std::vector<std::string>, size_t(e_io_direction::NUM_TYPES)>,
             size_t(NUM_2D_SIDES)>
    var_lookup_;

  /* The Variable name which represents the MSB whose value will always be -1 on the var_name*/
  std::vector<std::string> msb_var_names_;
  /* Upper bound which adjust the I/O number. 0 means no upper bound */
  std::vector<size_t> max_per_tile_;
  /* Minimum number is always 1 */
  std::vector<size_t> min_per_tile_;
  std::array<std::string, NUM_2D_SIDES> arch_io_types_; /* [side][var_name] = msb_var_name */

  /* Constants */
  std::array<std::string, size_t(e_io_direction::NUM_TYPES)> IO_DIRECTION_STRING_;

  /* Flag */
  mutable bool is_valid_;
};

}  // namespace arkangel
