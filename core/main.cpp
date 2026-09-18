/********************************************************************
 * Build the ArkAngel shell interface
 *******************************************************************/
#include "arkangel_shell.h"

/********************************************************************
 * Main function to start ArkAngel shell interface
 *******************************************************************/
int main(int argc, char** argv) {
  ArkAngelShell arkangel_shell;
  return arkangel_shell.start(argc, argv);
}
