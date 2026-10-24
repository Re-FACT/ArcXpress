/********************************************************************
 * This file includes the functions to create the title page
 * which introduces generality of OpenFPGA framework
 *******************************************************************/
#include "arkangel_title.h"

#include "arkangel_version.h"
#include "vtr_log.h"

/********************************************************************
 * Generate a string of arkangel title introduction
 * This is mainly used when launching ArkAngel shell
 *******************************************************************/
std::string create_arkangel_title() {
  std::string title;

  title += std::string("\n");
  title += std::string("                        _                                        _ \n");
  title += std::string("         /\\            | |        /\\                            | |\n");
  title += std::string("        /  \\     _ __  | | __    /  \\     _ __     __ _    ___  | |\n");
  title +=
    std::string("       / /\\ \\   | '__| | |/ /   / /\\ \\   | '_ \\   / _` |  / _ \\ | |\n");
  title += std::string("      / ____ \\  | |    |   <   / ____ \\  | | | | | (_| | |  __/ | |\n");
  title += std::string(
    "     /_/    \\_\\ |_|    |_|\\_\\ /_/    \\_\\ |_| |_|  \\__, |  \\___| |_|     \n");
  title += std::string("                                                   __/ |           \n");
  title += std::string("                                                  |___/            \n");
  title += std::string("\n");
  title += std::string(
    "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  title += std::string("\n");
  title += std::string("                        Arcxpress FPGA Design Tool Suites\n");
  title += std::string("             Copyright (C) 2026 by ReFACT\n");
  title += std::string("                              Under GPL3 license\n");
  title += std::string("\n");
  title += std::string("The open-source software is provided on an \"AS IS\" basis\n");
  title += std::string("\n");
  title +=
    std::string("ANY WARRANTIES, EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED\n");
  title +=
    std::string("OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE HEREBY DISCLAIMED\n");
  title += std::string(
    "AS: RAPID FLEX SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL\n");
  title += std::string(
    "EXEMPLARY, OR CONSEQUENTIAL DAMAGES INCLUDING BUT NOT LIMITED TO PROCUREMENT OF\n");
  title +=
    std::string("SUBSTITUTE GOODS OR SERVICES, LOSS OF USE OR DATA OR PROFITS, OR BUSINESS\n");
  title +=
    std::string("INTERRUPTION, NO MATTER HOW THEY ARE CAUSED IN USING OF THIS SOFRWARE, ON ANY\n");
  title +=
    std::string("THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT ARISING IN\n");
  title +=
    std::string("ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n");
  title += std::string("SUCH DAMAGE.\n");
  title += std::string("\n");
  title += std::string(
    "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  title += std::string("\n");

  return title;
}

/********************************************************************
 * Generate a string of openfpga title introduction
 * This is mainly used when launching ArkAngel shell
 *******************************************************************/
void print_arkangel_version_info() {
  /* Display version */
  VTR_LOG("Version: %s\n", arcxpress::VERSION);
  VTR_LOG("Revision: %s\n", arcxpress::VCS_REVISION);
  VTR_LOG("Compiled: %s\n", arcxpress::BUILD_TIMESTAMP);
  VTR_LOG("Compiler: %s\n", arcxpress::COMPILER);
  VTR_LOG("Build Info: %s\n", arcxpress::BUILD_INFO);
  VTR_LOG("\n");
}
