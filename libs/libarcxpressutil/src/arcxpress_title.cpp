/********************************************************************
 * This file includes the functions to create the title page
 * which introduces generality of OpenFPGA framework
 *******************************************************************/
#include "arcxpress_title.h"

#include "arcxpress_version.h"
#include "vtr_log.h"

/********************************************************************
 * Generate a string of arkangel title introduction
 * This is mainly used when launching ArcXpress shell
 *******************************************************************/
std::string create_arcxpress_title() {
  std::string title;

  title += std::string("\n");
  title += std::string("    ___              _  __                       \n");  
  title += std::string("   /   |  ________  | |/ /____  ________  __________\n");
  title += std::string("  / /| | / ___/ __ \\|   // __ \\/ ___/ _ \\/ ___/ ___/\n");
  title += std::string(" / ___ |/ /  / /_/ /   |/ /_/ / /  /  __(__  |__  ) \n");
  title += std::string("/_/  |_/_/   \\___//_/|_/ .___/_/   \\___/____/____/  \n");
  title += std::string("                      /_/\n");
  title += std::string("\n");
  title += std::string("┌──────────────────────────────────────────────────────────┐  \n");
  title += std::string("│  ArcXpress eFPGA Design Tool Suites                      │  \n");
  title += std:;string("├──────────────────────────────────────────────────────────┤  \n");
  title += std:;string("│  Copyright (C) 2026 ReFACT                               │  \n");
  title += std:;string("│  Licensed under the GNU General Public License v3.0      │  \n");
  title += std:;string("└──────────────────────────────────────────────────────────┘  \n");
  title += std::string("\n");

  return title;
}

/********************************************************************
 * Generate a string of openfpga title introduction
 * This is mainly used when launching ArcXpress shell
 *******************************************************************/
void print_arcxpress_version_info() {
  /* Display version */
  VTR_LOG("Version: %s\n", arcxpress::VERSION);
  VTR_LOG("Revision: %s\n", arcxpress::VCS_REVISION);
  VTR_LOG("Compiled: %s\n", arcxpress::BUILD_TIMESTAMP);
  VTR_LOG("Compiler: %s\n", arcxpress::COMPILER);
  VTR_LOG("Build Info: %s\n", arcxpress::BUILD_INFO);
  VTR_LOG("\n");
}
