/********************************************************************
 * Build the ArkAngel shell interface
 *******************************************************************/
#include "arcxpress_shell.h"

/********************************************************************
 * Main function to start ArkAngel shell interface
 *******************************************************************/
int main(int argc, char** argv) {
  ArkAngelShell arcxpress_shell;
  return arcxpress_shell.start(argc, argv);
}
