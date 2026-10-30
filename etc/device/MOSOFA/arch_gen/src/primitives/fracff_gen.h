#pragma once

#include <map>
#include <string>
#include <vector>

#include "vpr_arch_primitive.h"

namespace MOSOFA { /* namespace MOSOFA begins */

/**
 * Fracturable ff primitive generator
 */
class FracffGen {
 public: /* -- Constructor -- */
  FracffGen(const std::string& name);

 public: /* -- Accessors -- */
  /* The scan capable indicates that the physical model includes the scan mode ports */
  std::vector<arcxpress::VprArchPrimitive> vpr_arch_primitives(
    const bool& include_physical) const;
  std::vector<std::string> vpr_model_names(const bool& include_physical) const;
  arcxpress::VprArchPrimitive vpr_arch_primitive(const bool& scan_capable) const;
  std::string vpr_model_name(const bool& scan_capable) const;
  std::string vpr_physical_model_name() const;
  std::string circuit_model_name() const;
  /* The scan capable indicates that the physical model includes the scan mode ports */
  arcxpress::VprArchPrimitive vpr_arch_physical_primitive() const;

 private: /* -- Internal data -- */
  /* Constants */
  std::string CIRCUIT_MODEL_NAME_;
  std::string vpr_physical_model_name_;
};

}  // namespace MOSOFA
