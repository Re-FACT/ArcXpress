#ifndef ARKANGEL_SHELL_H
#define ARKANGEL_SHELL_H

#include <string>

#include "arkangel_context.h"
#include "shell.h"

/********************************************************************
 * This is a shell class built on top of the general-purpose shell
 * It is dedicated for the usage of ArkAngel engines
 * - It includes the data storage required
 * - It includes all the commands available in ArkAngel
 *******************************************************************/
class ArkAngelShell {
 public: /* Contructors */
  ArkAngelShell();

 public: /* Mutators */
  /* Execute a specific command with options in a line, which is available in
   * the shell Note that running a command will be based on the current status
   * of data storage. The data storage is impacted by previous commands that
   * have been run. If you want a clear start, please call API reset() before
   * running a command. Running a command may cause modification to data
   * storage. */
  int run_command(const char* cmd_line);

  /* Start the shell with options. For detailed usage, please refer to online
   * documentation about openfpgashell Depending on the options, a shell may run
   * in
   * - an interactive mode, where users can type in commands and get results
   * or
   * - an batch mode, where users provide a script contains a set of commands to
   * execute
   * TODO: the shell always has a clean start (refreshed data storage).
   */
  int start(int argc, char** argv);

  /* Reset the data storage and shell status, to ensure a clean start */
  void reset();

  /* allow arkangel context to be querried */
  const ArkangelContext* arkangel_context() { return &arkangel_ctx_; }

  void run_interactive_mode();
  void run_script_mode(const char* script_file_name, const bool& batch_mode);

 private: /* Internal data */
  openfpga::Shell<ArkangelContext> shell_;
  ArkangelContext arkangel_ctx_;
};

#endif
