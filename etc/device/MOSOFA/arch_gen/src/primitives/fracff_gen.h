#pragma once

#include <map>
#include <string>
#include <vector>

#include "vpr_arch_primitive.h"

/* namespace MOSOFA begins */
namespace MOSOFA {

/**
 * ff primitive generator
 */
class FracffGen {
 public: /* -- Constructor -- */
  FracffGen(const std::string& name);

 public: /* -- TYPES -- */
  enum class e_clk_type { RISING = 0, FALLING, NUM_TYPES };
  enum class e_clr_func { NONE = 0, RESET, SET, NUM_TYPES };
  enum class e_clr_type { SYNC = 0, ASYNC, NUM_TYPES };
  enum class e_clr_polarity { ACTIVE_LOW = 0, ACTIVE_HIGH, NUM_TYPES };

 public: /* -- Accessors -- */
  std::vector<arcxpress::VprArchPrimitive> vpr_arch_primitives(
    const bool& include_physical) const;
  std::vector<std::string> vpr_model_names(const bool& include_physical) const;
  arcxpress::VprArchPrimitive vpr_arch_primitive(const e_clk_type& ck_type,
                                                const e_clr_func& clr_func,
                                                const e_clr_type& clr_type,
                                                const e_clr_polarity& clr_pol) const;
  std::string vpr_model_name(const e_clk_type& ck_type, const e_clr_func& clr_func,
                             const e_clr_type& clr_type, const e_clr_polarity& clr_pol) const;
  std::string vpr_physical_model_name() const;
  std::string circuit_model_name() const;
  arcxpress::VprArchPrimitive vpr_arch_physical_primitive() const;
  std::string default_mode_bits() const;
  std::string mode_bits(const e_clk_type& ck_type, const e_clr_func& clr_func,
                        const e_clr_type& clr_type, const e_clr_polarity& clr_pol) const;

 public: /* Public utility */
  e_clk_type str2clk_type(const std::string& str, const bool& verbose = false) const;
  std::string clk_type2str(const e_clk_type& type, const bool& verbose = false) const;
  std::string clk_type2mode_bit(const e_clk_type& type, const bool& verbose = false) const;
  std::string clk_type_all2str() const;
  bool valid_clk_type(const e_clk_type& type) const;
  std::vector<FracffGen::e_clk_type> clk_types() const;

  e_clr_type str2clr_type(const std::string& str, const bool& verbose = false) const;
  std::string clr_type2str(const e_clr_type& type, const bool& verbose = false) const;
  std::string clr_type2mode_bit(const e_clr_type& type, const bool& verbose = false) const;
  std::string clr_type_all2str() const;
  bool valid_clr_type(const e_clr_type& type) const;
  std::vector<FracffGen::e_clr_type> clr_types() const;

  e_clr_func str2clr_func(const std::string& str, const bool& verbose = false) const;
  std::string clr_func2str(const e_clr_func& type, const bool& verbose = false) const;
  std::string clr_func2mode_bit(const e_clr_func& type, const bool& verbose = false) const;
  std::string clr_func_all2str() const;
  bool valid_clr_func(const e_clr_func& type) const;
  std::vector<FracffGen::e_clr_func> clr_funcs() const;

  e_clr_polarity str2clr_polarity(const std::string& str, const bool& verbose = false) const;
  std::string clr_polarity2str(const e_clr_polarity& type, const bool& verbose = false) const;
  std::string clr_polarity2mode_bit(const e_clr_polarity& type,
                                       const bool& verbose = false) const;
  std::string clr_polarity_all2str() const;
  bool valid_clr_polarity(const e_clr_polarity& type) const;
  std::vector<FracffGen::e_clr_polarity> clr_polarities() const;

 private: /* -- Internal data -- */
  /* Constants */
  std::array<std::string, size_t(e_clk_type::NUM_TYPES)> CLK_TYPE_STRING_;
  std::array<std::string, size_t(e_clr_func::NUM_TYPES)> CLR_FUNC_STRING_;
  std::array<std::string, size_t(e_clr_type::NUM_TYPES)> CLR_TYPE_STRING_;
  std::array<std::string, size_t(e_clr_polarity::NUM_TYPES)> CLR_POLARITY_STRING_;

  std::array<std::string, size_t(e_clk_type::NUM_TYPES)> CLK_TYPE_MODE_BITS_;
  std::array<std::string, size_t(e_clr_func::NUM_TYPES)> CLR_FUNC_MODE_BITS_;
  std::array<std::string, size_t(e_clr_type::NUM_TYPES)> CLR_TYPE_MODE_BITS_;
  std::array<std::string, size_t(e_clr_polarity::NUM_TYPES)> CLR_POLARITY_MODE_BITS_;
  std::string VPR_MODEL_PREFIX_;
  std::string CIRCUIT_MODEL_NAME_;
  std::string vpr_physical_model_name_;
};

}  // namespace MOSOFA
