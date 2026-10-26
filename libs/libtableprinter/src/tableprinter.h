#pragma once
#include <string>
#include <vector>

namespace arcxpress {
class TablePrinter {
 public:
  TablePrinter(const std::vector<std::string>& headers) : headers(headers) { DELIM_ = "|"; }

  void add_row(const std::vector<std::string>& row);

  void set_delim(const std::string& val);
  void print(const std::string& file_path = "", int max_width = -1);
  void print_transposed(const std::string& file_path = "") const;
  void set_max_width(int max_width) { MAX_WIDTH_ = max_width; }

 private:
  std::vector<std::string> headers;
  std::vector<std::vector<std::string>> rows;
  int MAX_WIDTH_ = 80;
  bool is_csv_file(const std::string& file_path) const;
  std::vector<size_t> get_column_widths() const;
  std::vector<size_t> get_transposed_column_widths() const;

  void print_separator(const std::vector<size_t>& widths, const std::string& file_path = "") const;
  void print_row(const std::vector<std::string>& row, const std::vector<size_t>& widths,
                 const std::string& file_path = "") const;
  std::string DELIM_;
};
}  // namespace arcxpress
