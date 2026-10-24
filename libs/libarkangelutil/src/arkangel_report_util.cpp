#include "arkangel_report_util.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
namespace arcxpress {
std::string to_string_with_precision(const float& value, const int& precision = 2) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(precision) << value;
  return out.str();
}
}  // namespace arcxpress
