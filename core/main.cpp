/********************************************************************
 * Build the ArcXpress shell interface
 *******************************************************************/
#include "arcxpress_shell.h"

/********************************************************************
 * Main function to start ArcXpress shell interface
 *******************************************************************/
int main(int argc, char** argv) {
  ArcXpressShell arcxpress_shell;
  return arcxpress_shell.start(argc, argv);
}
