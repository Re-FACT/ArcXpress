#include "tableprinter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "openfpga_digest.h"
#include "rapidcsv.h"
#include "vtr_log.h"

namespace arcxpress {
bool TablePrinter::is_csv_file(const std::string& file_path) const {
  return std::filesystem::path(file_path).extension() == ".csv";
}

void TablePrinter::add_row(const std::vector<std::string>& row) {
  if (row.size() != headers.size()) {
    throw std::runtime_error("Row size must match header size.");
  }
  rows.push_back(row);
}

void TablePrinter::print(const std::string& file_path, int max_width) {
  std::vector<size_t> colWidths = get_column_widths();
  if (max_width != -1) {
    set_max_width(max_width);
  }
  size_t total_width = 0;
  for (auto w : colWidths) total_width += w + 3;  // +3 for padding and border
  total_width += 1;                               // final '+'

  if (std::filesystem::exists(file_path)) {
    VTR_LOG_WARN("Removing an existing report file: %s", file_path.c_str());
    std::filesystem::remove(file_path);
  }

  if (!file_path.empty() && is_csv_file(file_path)) {
    /* Create directory before creating the file */
    std::filesystem::path fdir = std::filesystem::path(file_path).parent_path();
    openfpga::create_directory(fdir.string(), true, false);
    std::ofstream create_file(file_path);
    create_file.close();
  }

  /* If the table is displayed on the screen and exceeds the screen width, we
   * transpose it. For file output, the table is always kept in its original
   * orientation.
   */
  if (total_width > MAX_WIDTH_ && file_path.empty()) {
    VTR_LOG("Table too wide (%zu > %zu) — printing in transposed mode.\n", total_width, MAX_WIDTH_);
    print_transposed(file_path);
    return;
  }

  // Print header
  print_separator(colWidths, file_path);
  print_row(headers, colWidths, file_path);
  print_separator(colWidths, file_path);

  // Print rows
  for (const auto& row : rows) {
    print_row(row, colWidths, file_path);
  }
  print_separator(colWidths, file_path);
}

std::vector<size_t> TablePrinter::get_column_widths() const {
  std::vector<size_t> widths(headers.size());
  for (size_t i = 0; i < headers.size(); i++) {
    widths[i] = headers[i].size();
  }

  for (const auto& row : rows) {
    for (size_t i = 0; i < row.size(); i++) {
      widths[i] = std::max(widths[i], row[i].size());
    }
  }
  return widths;
}

std::vector<size_t> TablePrinter::get_transposed_column_widths() const {
  size_t header_width = 0;
  size_t value_width = 0;

  for (const auto& h : headers) {
    header_width = std::max(header_width, h.size());
  }

  for (size_t i = 0; i < headers.size(); ++i) {
    for (const auto& row : rows) {
      if (i < row.size()) {
        value_width = std::max(value_width, row[i].size());
      }
    }
  }

  return {header_width, value_width};
}

void TablePrinter::print_transposed(const std::string& file_path) const {
  std::vector<size_t> widths = get_transposed_column_widths();

  for (size_t r = 0; r < rows.size(); ++r) {
    print_separator(widths, file_path);
    for (size_t i = 0; i < headers.size(); ++i) {
      std::vector<std::string> kv = {headers[i], rows[r][i]};
      print_row(kv, widths, file_path);
    }
  }
  print_separator(widths, file_path);
}

void TablePrinter::print_separator(const std::vector<size_t>& widths,
                                   const std::string& file_path) const {
  if (!file_path.empty() && is_csv_file(file_path)) {
    return;
  }

  if (file_path.empty()) {
    // print on screen
    std::ostringstream oss;
    for (size_t w : widths) {
      oss << "+" << std::string(w + 2, '-');
    }
    oss << "+" << std::endl;
    std::string output_string = oss.str();
    VTR_LOG("%s", output_string.c_str());
  } else {
    // print to file
    std::ofstream ofs(file_path, std::ios::app);  // append mode
    if (!ofs) {
      throw std::runtime_error("Cannot open file: " + file_path);
    }
    for (size_t w : widths) {
      ofs << "+" << std::string(w + 2, '-');
    }
    ofs << "+" << std::endl;
  }
}

void TablePrinter::set_delim(const std::string& val) { DELIM_ = val; }

void TablePrinter::print_row(const std::vector<std::string>& row, const std::vector<size_t>& widths,
                             const std::string& file_path) const {
  bool is_csv = !file_path.empty() && is_csv_file(file_path);
  if (is_csv) {
    try {
      rapidcsv::Document doc(file_path, rapidcsv::LabelParams(-1, -1),
                             rapidcsv::SeparatorParams(',', false, false));
      doc.InsertRow<std::string>(doc.GetRowCount(), row);
      doc.Save(file_path);
    } catch (const std::exception& e) {
      throw std::runtime_error(std::string("rapidcsv error: ") + e.what());
    }
    return;
  }

  if (file_path.empty()) {
    // print on screen
    std::ostringstream oss;
    for (size_t i = 0; i < row.size(); i++) {
      oss << DELIM_ << " " << std::left << std::setw(widths[i]) << row[i] << " ";
    }
    oss << DELIM_ << std::endl;
    std::string output_string = oss.str();
    VTR_LOG("%s", output_string.c_str());
  } else {
    // print to file
    std::ofstream ofs(file_path, std::ios::app);  // append mode
    if (!ofs) {
      throw std::runtime_error("Cannot open file: " + file_path);
    }
    for (size_t i = 0; i < row.size(); i++) {
      ofs << DELIM_ << " " << std::left << std::setw(widths[i]) << row[i] << " ";
    }
    ofs << DELIM_ << std::endl;
  }
}
}  // namespace arcxpress
