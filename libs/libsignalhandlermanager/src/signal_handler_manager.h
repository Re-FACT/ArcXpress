#ifndef SIGNAL_HANDLER_MANAGER_H
#define SIGNAL_HANDLER_MANAGER_H

// ============================================================
// Platform detection
//   ARKANGEL_PLATFORM_WINDOWS  – native Win32 (MSVC / clang-cl)
//   ARKANGEL_PLATFORM_MSYS2    – MinGW / MSYS2 toolchain on Windows
//   ARKANGEL_PLATFORM_POSIX    – Linux (and MSYS2, which has full POSIX)
// ============================================================
#if defined(_WIN32) || defined(_WIN64)
//#if defined(__MINGW32__) || defined(__MINGW64__) || defined(__MSYS__)
//#define ARKANGEL_PLATFORM_MSYS2 1
//#define ARKANGEL_PLATFORM_POSIX 1
//#else
#define ARKANGEL_PLATFORM_WINDOWS 1
//#endif
#else
#define ARKANGEL_PLATFORM_LINUX 1
#define ARKANGEL_PLATFORM_POSIX 1
#endif

// ============================================================
// Platform-specific includes and process-ID type alias
// ============================================================
#if defined(ARKANGEL_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <csignal>
/// Windows process identifier
using platform_pid_t = DWORD;
#else
#include <sys/types.h>
#include <unistd.h>

#include <csetjmp>
#include <csignal>
/// POSIX process identifier
using platform_pid_t = pid_t;
#endif

#include <cstdio>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "command_exit_codes.h"

namespace arkangel {

/* Keep consistent with openfpga::CMD_EXEC_SUCCESS */
constexpr int CMD_EXEC_SUCCESS = 0;
constexpr int CMD_EXEC_FATAL_ERROR_HANDLED = 3;
constexpr int CMD_EXEC_SIGINT_HANDLED = 4;

typedef void (*sig_func_t)(int);

/**
 * @brief Cross-platform signal / fault handler manager.
 *
 * Supported platforms:
 *   - Linux         : full POSIX signals + sigsetjmp/siglongjmp recovery
 *   - MSYS2/MinGW   : POSIX layer on Windows; behaves like Linux
 *   - Native Windows: SetConsoleCtrlHandler for Ctrl+C;
 *                     SEH (__try/__except) for fatal fault recovery;
 *                     no sigjmp_buf available
 *
 * Usage pattern (same on all platforms):
 * @code
 *   // Arm fatal-fault recovery (SIGSEGV / Access Violation)
 *   if (int rc = SignalHandlerManager::set_fatal_handler("MyHandler")) {
 *     // rc == CMD_EXEC_FATAL_ERROR_HANDLED — fault was caught, handle it
 *   }
 *
 *   // Arm Ctrl+C recovery
 *   if (int rc = SignalHandlerManager::set_recover_handler(gui_mode)) {
 *     // rc == CMD_EXEC_SIGINT_HANDLED — user pressed Ctrl+C
 *   }
 * @endcode
 */
class SignalHandlerManager {
 public:
  explicit SignalHandlerManager(bool print_sig_num = false);

 public: /* Utilities */
  static void set_running_cmd_flag(bool value) { running_cmd_flag_ = value; }

  /** If a command is running, jump back to the recovery point immediately. */
  static void recover_running_cmd(int status);

  /**
   * Install all signal handlers.
   * @param gui_mode  true  → Ctrl+C asks "are you sure?" before exiting
   *                  false → Ctrl+C terminates immediately
   */
  static void set_sig_handler(bool gui_mode = true);

  /**
   * Arm the SIGSEGV / SEH fault recovery point.
   * @return 0 on first call; CMD_EXEC_FATAL_ERROR_HANDLED after a fault.
   */
  static int set_fatal_handler(const char* handler_name);

  /**
   * Arm the SIGINT / Ctrl+C recovery point.
   * @return 0 on first call; CMD_EXEC_SIGINT_HANDLED after Ctrl+C.
   */
  static int set_recover_handler(bool gui_mode);

  static int get_fatal_sig_num() { return fatal_sig_num_; }
  static void inc_fatal_sig_num() { ++fatal_sig_num_; }

  static void set_monitored_process(std::set<platform_pid_t>& pid_collect) {
    monitored_process_ = pid_collect;
  }
  static void add_monitored_process(platform_pid_t pid) { monitored_process_.insert(pid); }

 private: /* Signal handler callbacks */
  static void sig_handler_ctrl_c(int signum);
  static void sig_handler_ctrl_c_interactive(int signum);
  static void sig_handler_exit(int signum);
  static void sig_handler_default(int signum);
  static void sig_handler_fatal(int signum);

#if defined(ARKANGEL_PLATFORM_WINDOWS)
  /** Native Windows console-control handler (replaces SIGINT on Win32). */
  static BOOL WINAPI win_ctrl_handler(DWORD ctrl_type);
#endif

  /** Terminate all monitored child processes and clear the set. */
  static void kill_monitored_processes();

 private: /* State */
  static std::string str_cur_handler;
  static int fatal_sig_num_;
  static bool print_sig_num_;
  static std::vector<std::pair<int, sig_func_t>> sig_handler_collect;
  static bool running_cmd_flag_;
  static std::set<platform_pid_t> monitored_process_;
};

}  // namespace arkangel

#endif  // SIGNAL_HANDLER_MANAGER_H
