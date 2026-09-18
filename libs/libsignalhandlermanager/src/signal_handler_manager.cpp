#include "signal_handler_manager.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>

#include "vtr_log.h"

// ============================================================
// Platform-specific includes
// ============================================================
#if defined(ARKANGEL_PLATFORM_WINDOWS)
// Native Windows – no sigjmp_buf.
// Fatal-fault recovery is implemented with SEH (__try / __except).
// Ctrl+C recovery is implemented with SetConsoleCtrlHandler + an Event.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// An Event that sig_handler_ctrl_c signals so set_recover_handler() can
// unblock and return CMD_EXEC_SIGINT_HANDLED.
static HANDLE s_sigint_event = NULL;

#else
// POSIX (Linux / MSYS2)
#include <setjmp.h>
#include <sys/types.h>
#include <unistd.h>

// Translation-unit-local jmp_buf objects.
// They are static so they are never visible outside this file.
static jmp_buf s_env_fatal;    // used by set_fatal_handler / sig_handler_fatal
static jmp_buf s_env_recover;  // used by set_recover_handler / sig_handler_ctrl_c
#endif

namespace arkangel {

// ============================================================
// Static member definitions
// ============================================================
bool SignalHandlerManager::running_cmd_flag_ = false;
int SignalHandlerManager::fatal_sig_num_ = 0;
bool SignalHandlerManager::print_sig_num_ = false;
std::string SignalHandlerManager::str_cur_handler = "";
std::set<platform_pid_t> SignalHandlerManager::monitored_process_;

// Signal/handler registration table.
//
// Notes:
//  • SIGKILL and SIGSTOP cannot be caught or ignored on POSIX; they are omitted.
//  • SIGPIPE, SIGTSTP, SIGHUP, SIGCHLD, SIGCONT, SIGTTIN, SIGTTOU are
//    POSIX-only and are guarded accordingly.
//  • On native Windows only SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM
//    are defined by the CRT.
std::vector<std::pair<int, sig_func_t>> SignalHandlerManager::sig_handler_collect = {
  {SIGABRT, SignalHandlerManager::sig_handler_exit},
  {SIGFPE, SignalHandlerManager::sig_handler_exit},
  {SIGILL, SignalHandlerManager::sig_handler_exit},
  {SIGTERM, SignalHandlerManager::sig_handler_exit},

#if defined(ARKANGEL_PLATFORM_POSIX)
  // POSIX-only signals
  {SIGPIPE, SignalHandlerManager::sig_handler_exit},
  {SIGTSTP, SignalHandlerManager::sig_handler_exit},
  {SIGALRM, SignalHandlerManager::sig_handler_default},
  {SIGHUP, SignalHandlerManager::sig_handler_default},
  {SIGCHLD, SignalHandlerManager::sig_handler_default},
  {SIGCONT, SignalHandlerManager::sig_handler_default},
  {SIGTTIN, SignalHandlerManager::sig_handler_default},
  {SIGTTOU, SignalHandlerManager::sig_handler_default},
#endif
};

// ============================================================
// Constructor
// ============================================================
SignalHandlerManager::SignalHandlerManager(bool print_sig_num) {
  print_sig_num_ = print_sig_num;
  set_sig_handler();
}

// ============================================================
// kill_monitored_processes
// ============================================================
void SignalHandlerManager::kill_monitored_processes() {
#if defined(ARKANGEL_PLATFORM_WINDOWS)
  for (auto pid : monitored_process_) {
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProc != NULL) {
      TerminateProcess(hProc, 1);
      CloseHandle(hProc);
      VTR_LOG("[ARKANGEL INFO] Stopped process with pid %lu\n", static_cast<unsigned long>(pid));
    }
  }
#else
  for (auto pid : monitored_process_) {
    kill(pid, SIGKILL);
    VTR_LOG("[ARKANGEL INFO] Stopped process with pid %d\n", static_cast<int>(pid));
  }
#endif
  monitored_process_.clear();
}

// ============================================================
// set_sig_handler
// ============================================================
void SignalHandlerManager::set_sig_handler(bool gui_mode) {
  // Register all entries from the table using plain function pointers –
  // no std::function / target<> cast needed.
  for (const auto& entry : sig_handler_collect) {
    signal(entry.first, entry.second);
  }

#if defined(ARKANGEL_PLATFORM_WINDOWS)
  // On native Windows, SIGINT via signal() is unreliable in console apps.
  // SetConsoleCtrlHandler gives us CTRL_C_EVENT and CTRL_BREAK_EVENT.
  SetConsoleCtrlHandler(win_ctrl_handler, TRUE);
  (void)gui_mode;  // interactive prompt not supported on Windows console
#else
  if (gui_mode) {
    signal(SIGINT, sig_handler_ctrl_c_interactive);
  } else {
    signal(SIGINT, sig_handler_ctrl_c);
  }
#endif
}

// ============================================================
// sig_handler_ctrl_c  (non-interactive)
// ============================================================
void SignalHandlerManager::sig_handler_ctrl_c(int /*signum*/) {
  VTR_LOG("\n[ARKANGEL INFO] Ctrl+C pressed. Terminating the running program.\n");
  kill_monitored_processes();

#if defined(ARKANGEL_PLATFORM_POSIX)
  if (running_cmd_flag_) {
    siglongjmp(s_env_recover, CMD_EXEC_SIGINT_HANDLED);
  } else {
    exit(0);
  }
#else
  // Native Windows path: signal the event so set_recover_handler() unblocks.
  if (running_cmd_flag_ && s_sigint_event != NULL) {
    SetEvent(s_sigint_event);
  } else {
    exit(0);
  }
#endif
}

// ============================================================
// sig_handler_ctrl_c_interactive  (prompts the user)
// Only reachable on POSIX; Windows uses win_ctrl_handler instead.
// ============================================================
void SignalHandlerManager::sig_handler_ctrl_c_interactive(int /*signum*/) {
#if defined(ARKANGEL_PLATFORM_POSIX)
  VTR_LOG(
    "\n[ARKANGEL INFO] Ctrl+C pressed. "
    "Are you sure you want to terminate the running program? [y/n]\n");
  fflush(stdout);

  // Re-arm so a second Ctrl+C during the prompt is handled too.
  signal(SIGINT, sig_handler_ctrl_c_interactive);

  int ch = getchar();
  if (ch == 'y' || ch == 'Y') {
    kill_monitored_processes();
    if (running_cmd_flag_) {
      siglongjmp(s_env_recover, CMD_EXEC_SIGINT_HANDLED);
    } else {
      exit(0);
    }
  } else {
    VTR_LOG("\n[ARKANGEL INFO] Continuing (character '%c' pressed).\n",
            (ch >= 32 && ch < 127) ? static_cast<char>(ch) : '?');
  }
#endif
}

// ============================================================
// recover_running_cmd
// ============================================================
void SignalHandlerManager::recover_running_cmd(int status) {
#if defined(ARKANGEL_PLATFORM_POSIX)
  if (running_cmd_flag_) {
    siglongjmp(s_env_recover, status);
  }
#else
  // Native Windows: signal the event with the requested status embedded.
  // For simplicity we treat any non-zero status as "interrupted".
  if (running_cmd_flag_ && s_sigint_event != NULL) {
    SetEvent(s_sigint_event);
  }
  (void)status;
#endif
}

// ============================================================
// sig_handler_exit
// ============================================================
void SignalHandlerManager::sig_handler_exit(int signum) {
  VTR_LOG("\n[ARKANGEL INFO] Signal %d received. Exiting the program.\n", signum);
  exit(1);
}

// ============================================================
// sig_handler_default
// ============================================================
void SignalHandlerManager::sig_handler_default(int signum) {
  if (print_sig_num_) {
    VTR_LOG("\n[ARKANGEL INFO] Signal %d received.\n", signum);
  }
}

// ============================================================
// sig_handler_fatal  (SIGSEGV on POSIX)
// ============================================================
void SignalHandlerManager::sig_handler_fatal(int sig) {
#if defined(ARKANGEL_PLATFORM_POSIX)
  if (sig == SIGSEGV) {
    siglongjmp(s_env_fatal, CMD_EXEC_FATAL_ERROR_HANDLED);
  }
#else
  // On native Windows SIGSEGV maps to an Access Violation; recovery is done
  // via SEH inside set_fatal_handler(). This handler is a no-op fallback.
  (void)sig;
#endif
}

// ============================================================
// set_fatal_handler
// ============================================================
int SignalHandlerManager::set_fatal_handler(const char* handler_name) {
  str_cur_handler = (handler_name != nullptr) ? handler_name : "";

#if defined(ARKANGEL_PLATFORM_POSIX)
  int ret = sigsetjmp(s_env_fatal, 1);
  if (ret == 0) {
    signal(SIGSEGV, sig_handler_fatal);
  } else {
    inc_fatal_sig_num();
    std::string msg = "[ARKANGEL FATAL] Segment fault caught and handled";
    if (!str_cur_handler.empty()) msg += " by " + str_cur_handler;
    msg += "\n";
    VTR_LOG("%s", msg.c_str());
    fflush(stdout);
  }
  return ret;

#else
  // Native Windows: the caller wraps the dangerous code in __try/__except.
  // set_fatal_handler() just resets book-keeping and always returns 0 here;
  // the SEH block in the caller returns CMD_EXEC_FATAL_ERROR_HANDLED itself.
  //
  // Recommended usage in the caller:
  //
  //   SignalHandlerManager::set_fatal_handler("MyHandler");
  //   int rc = CMD_EXEC_SUCCESS;
  //   __try {
  //     dangerous_call();
  //   } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
  //                   ? EXCEPTION_EXECUTE_HANDLER
  //                   : EXCEPTION_CONTINUE_SEARCH) {
  //     SignalHandlerManager::inc_fatal_sig_num();
  //     VTR_LOG("[ARKANGEL FATAL] Access violation caught by %s\n",
  //             handler_name ? handler_name : "");
  //     rc = CMD_EXEC_FATAL_ERROR_HANDLED;
  //   }
  //   return rc;
  return 0;
#endif
}

// ============================================================
// set_recover_handler
// ============================================================
int SignalHandlerManager::set_recover_handler(bool gui_mode) {
#if defined(ARKANGEL_PLATFORM_POSIX)
  int ret = sigsetjmp(s_env_recover, 1);
  if (ret == 0) {
    // First entry: arm the Ctrl+C handler.
    if (gui_mode) {
      signal(SIGINT, sig_handler_ctrl_c_interactive);
    } else {
      signal(SIGINT, sig_handler_ctrl_c);
    }
    return ret;
  } else {
    // Resumed after Ctrl+C.
    running_cmd_flag_ = false;
    return ret;
  }

#else
  // Native Windows: create (or reset) an Event the Ctrl+C handler will signal.
  (void)gui_mode;
  if (s_sigint_event == NULL) {
    s_sigint_event = CreateEvent(NULL, FALSE, FALSE, NULL);
  } else {
    ResetEvent(s_sigint_event);
  }

  // Block until Ctrl+C fires or a timeout (INFINITE means "wait forever").
  // Return 0 immediately if no command is running so callers can proceed.
  if (!running_cmd_flag_) return 0;

  DWORD wait_result = WaitForSingleObject(s_sigint_event, INFINITE);
  if (wait_result == WAIT_OBJECT_0) {
    running_cmd_flag_ = false;
    return CMD_EXEC_SIGINT_HANDLED;
  }
  return 0;
#endif
}

// ============================================================
// win_ctrl_handler  (native Windows only)
// ============================================================
#if defined(ARKANGEL_PLATFORM_WINDOWS)
BOOL WINAPI SignalHandlerManager::win_ctrl_handler(DWORD ctrl_type) {
  if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
    VTR_LOG("\n[ARKANGEL INFO] Ctrl+C pressed. Terminating the running program.\n");
    kill_monitored_processes();

    if (running_cmd_flag_ && s_sigint_event != NULL) {
      SetEvent(s_sigint_event);
      return TRUE;  // signal handled; do not call default handler
    }
    exit(0);
  }
  return FALSE;  // let the default handler process other events
}
#endif

}  // namespace arkangel
