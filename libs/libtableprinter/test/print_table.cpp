#include "tableprinter.h"

int main() {
  arkangel::TablePrinter tp({"ID", "Name", "Score"});
  tp.add_row({"1", "Alice", "95"});
  tp.add_row({"2", "Bob", "88"});
  tp.add_row({"3", "Charlie", "100"});

  tp.print();
  return 0;
}