// If Graphviz is found, enable the following functionality
// This is a temporay fix due to the graphviz package issue on CentOS 7.9
#ifdef USE_GRAPHVIZ

#include "arkangel_graphviz_utils.h"

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
// #include <cgraph.h>
// #include <gvc.h>
#include <cstdio>
#include <iostream>

#include "command_exit_codes.h"
#include "vtr_log.h"

namespace arkangel {
int convert_graphviz_to_image(const std::string &input_file, const std::string &output_file,
                              const std::string &file_format, int verbose) {
  GVC_t *gvc;
  Agraph_t *graph;
  FILE *fp;

  // Initialize the Graphviz context
  gvc = gvContext();
  if (!gvc) {
    VTR_LOG_ERROR("Failed to initialize Graphviz context.\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  // Open the input file
  fp = fopen(input_file.c_str(), "r");
  if (!fp) {
    VTR_LOG_ERROR("Failed to open input file: %s\n", input_file.c_str());
    gvFreeContext(gvc);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  // Parse the input file into a graph structure
  graph = agread(fp, nullptr);
  fclose(fp);

  if (!graph) {
    VTR_LOG_ERROR("Failed to parse graph from file: %s\n", input_file.c_str());
    gvFreeContext(gvc);
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  // Apply the layout algorithm
  int layout_result = gvLayout(gvc, graph, "dot");
  if (layout_result != 0) {
    VTR_LOG_ERROR("Failed to apply layout algorithm to graph.\n");
    agclose(graph);
    gvFreeContext(gvc);
    return openfpga::CMD_EXEC_FATAL_ERROR;  // Error
  }

  // Render based on the specified format
  int render_result = openfpga::CMD_EXEC_FATAL_ERROR;  // Default to error
  if (!is_graphviz_supported_format(file_format)) {
    VTR_LOG_ERROR("Unsupported file format: %s. Only 'png' and 'svg' are supported.\n",
                  file_format.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  FILE *output_fp = fopen(output_file.c_str(), "w");
  if (!output_fp) {
    VTR_LOG_ERROR("Failed to open output file: %s\n", output_file.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  render_result = gvRender(gvc, graph, file_format.c_str(), output_fp);
  fclose(output_fp);

  // Clean up resources
  gvFreeLayout(gvc, graph);
  agclose(graph);
  gvFreeContext(gvc);

  if (render_result != 0) {
    VTR_LOGV(verbose, "Failed to convert %s to %s in %s format.\n", input_file.c_str(),
             output_file.c_str(), file_format.c_str());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }
  VTR_LOGV(verbose, "Successfully converted %s to %s in %s format.\n", input_file.c_str(),
           output_file.c_str(), file_format.c_str());
  return openfpga::CMD_EXEC_SUCCESS;
}
bool is_graphviz_supported_format(const std::string &file_format) {
  return (file_format == "png" || file_format == "svg");
}

}  // namespace arkangel

#endif
