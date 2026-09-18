#pragma once

// If Graphviz is found, enable the following functionality
// This is a temporay fix due to the graphviz package issue on CentOS 7.9
#ifdef USE_GRAPHVIZ

#include <string>

namespace arkangel {

/**
 * @brief Converts a Graphviz DOT file to image format (PNG or SVG)
 *
 * @param input_file Path to the input DOT file
 * @param output_file Path to the output image file
 * @param file_format Output format ("png" or "svg")
 * @param verbosity Verbosity level for output
 * @return int Status code (0 for success, non-zero for error)
 */
int convert_graphviz_to_image(const std::string& input_file, const std::string& output_file,
                              const std::string& file_format, int verbosity);

/**
 * @brief Validates if the given file format is supported
 *
 * @param file_format Format to validate ("png", "svg", etc.)
 * @return true if supported, false otherwise
 */
bool is_graphviz_supported_format(const std::string& file_format);

}  // namespace arkangel

#endif
