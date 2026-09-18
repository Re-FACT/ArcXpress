#include "arkangel_process_util.h"

/* ====================================================================
 * Platform includes
 * ==================================================================== */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>      /* _pipe, _read, _close, _write */
#include <process.h> /* _getpid */
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <queue>
#include <set>
#include <thread>

#include "signal_handler_manager.h"
#include "vtr_log.h"

namespace arkangel {

/* ====================================================================
 * Helpers – thin wrappers so the logic above stays readable
 * ==================================================================== */

/** Close a single native fd/handle; ignores invalid values. */
static void close_fd(NativeFd fd) {
#ifdef _WIN32
  if (fd != INVALID_HANDLE_VALUE) CloseHandle(fd);
#else
  if (fd >= 0) close(fd);
#endif
}

/* ====================================================================
 * system_with_pid – spawn a child with stdin/stdout/stderr pipes
 * ==================================================================== */

#ifdef _WIN32
/* ------------------------------------------------------------------ */
/* Windows implementation                                              */
/* ------------------------------------------------------------------ */
NativePid system_with_pid(const char* command, NativeFd* infp, NativeFd* outfp, NativeFd* errfp,
                          bool verbose) {
  /* Security attributes that allow handles to be inherited */
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;

  /* --- stdin pipe --- */
  HANDLE stdin_rd = INVALID_HANDLE_VALUE, stdin_wr = INVALID_HANDLE_VALUE;
  if (!CreatePipe(&stdin_rd, &stdin_wr, &sa, 0)) return NATIVE_INVALID_PID;
  /* Parent's write end must NOT be inherited by the child */
  SetHandleInformation(stdin_wr, HANDLE_FLAG_INHERIT, 0);

  /* --- stdout pipe --- */
  HANDLE stdout_rd = INVALID_HANDLE_VALUE, stdout_wr = INVALID_HANDLE_VALUE;
  if (!CreatePipe(&stdout_rd, &stdout_wr, &sa, 0)) {
    CloseHandle(stdin_rd);
    CloseHandle(stdin_wr);
    return NATIVE_INVALID_PID;
  }
  SetHandleInformation(stdout_rd, HANDLE_FLAG_INHERIT, 0);

  /* --- stderr pipe --- */
  HANDLE stderr_rd = INVALID_HANDLE_VALUE, stderr_wr = INVALID_HANDLE_VALUE;
  if (!CreatePipe(&stderr_rd, &stderr_wr, &sa, 0)) {
    CloseHandle(stdin_rd);
    CloseHandle(stdin_wr);
    CloseHandle(stdout_rd);
    CloseHandle(stdout_wr);
    return NATIVE_INVALID_PID;
  }
  SetHandleInformation(stderr_rd, HANDLE_FLAG_INHERIT, 0);

  /* Build command string for CreateProcess */
  /* cmd.exe /C <command> mirrors the Linux execl("/bin/bash", "sh", "-c", ...) */
  std::string cmd_str = std::string("cmd.exe /C ") + command;

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = (infp != nullptr) ? stdin_rd : GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = (outfp != nullptr) ? stdout_wr : GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdError = (errfp != nullptr) ? stderr_wr : GetStdHandle(STD_ERROR_HANDLE);

  PROCESS_INFORMATION pi{};
  /* CREATE_NEW_PROCESS_GROUP lets us send Ctrl+Break to the whole group later */
  BOOL ok = CreateProcessA(nullptr, cmd_str.data(), /* CreateProcessA may modify this buffer */
                           nullptr,                 /* process security */
                           nullptr,                 /* thread security */
                           TRUE,                    /* inherit handles */
                           CREATE_NEW_PROCESS_GROUP, nullptr, /* environment */
                           nullptr,                           /* current directory */
                           &si, &pi);

  /* Close child-side handles in parent regardless */
  CloseHandle(stdin_rd);
  CloseHandle(stdout_wr);
  CloseHandle(stderr_wr);

  if (!ok) {
    CloseHandle(stdin_wr);
    CloseHandle(stdout_rd);
    CloseHandle(stderr_rd);
    VTR_LOG_ERROR("[ARKANGEL ERROR] CreateProcess failed (error %lu)\n", GetLastError());
    return NATIVE_INVALID_PID;
  }

  /* We don't need the primary thread handle */
  CloseHandle(pi.hThread);

  NativePid child_pid = pi.dwProcessId;
  VTR_LOGV(verbose, "[ARKANGEL INFO] Parent pid: %lu, child pid: %lu\n",
           static_cast<DWORD>(GetCurrentProcessId()), child_pid);

  /* Return or discard parent-side pipe ends */
  if (infp == nullptr) {
    CloseHandle(stdin_wr);
  } else {
    *infp = stdin_wr;
  }
  if (outfp == nullptr) {
    CloseHandle(stdout_rd);
  } else {
    *outfp = stdout_rd;
  }
  if (errfp == nullptr) {
    CloseHandle(stderr_rd);
  } else {
    *errfp = stderr_rd;
  }

  /* Store the process HANDLE somewhere accessible.
     Callers that need it (system_wrapper) retrieve it via OpenProcess.
     We close pi.hProcess here; callers can re-open if needed. */
  CloseHandle(pi.hProcess);

  return child_pid;
}

#else  /* ------------------------------------------------------------------ */
/* POSIX (Linux / macOS) implementation – unchanged from original            */
/* ------------------------------------------------------------------ */
NativePid system_with_pid(const char* command, NativeFd* infp, NativeFd* outfp, NativeFd* errfp,
                          bool verbose) {
  int p_stdin[2], status_stdin = pipe(p_stdin);
  int p_stdout[2], status_stdout = pipe(p_stdout);
  int p_stderr[2], status_stderr = pipe(p_stderr);

  /* Synchronisation pipe: child writes a token, parent waits for it */
  char msg[] = "child process has run";
  int pipefd[2], status_pipefd = pipe(pipefd);

  if ((-1 == status_stdin) || (-1 == status_stdout) || (-1 == status_stderr) ||
      (-1 == status_pipefd)) {
    if (0 == status_stdin) {
      close(p_stdin[0]);
      close(p_stdin[1]);
    }
    if (0 == status_stdout) {
      close(p_stdout[0]);
      close(p_stdout[1]);
    }
    if (0 == status_stderr) {
      close(p_stderr[0]);
      close(p_stderr[1]);
    }
    if (0 == status_pipefd) {
      close(pipefd[0]);
      close(pipefd[1]);
    }
    return -1;
  }

  pid_t parent_pid = getpid();
  pid_t pid = fork();

  if (pid < 0) {
    close(p_stdin[0]);
    close(p_stdin[1]);
    close(p_stdout[0]);
    close(p_stdout[1]);
    close(p_stderr[0]);
    close(p_stderr[1]);
    close(pipefd[0]);
    close(pipefd[1]);
    return -1;

  } else if (pid == 0) {
    /* ---- child ---- */
    if (infp != nullptr) dup2(p_stdin[0], 0);
    if (outfp != nullptr) dup2(p_stdout[1], 1);
    if (errfp != nullptr) dup2(p_stderr[1], 2);

    setsid();
    pid_t child_pid = getpid();
    setpgid(child_pid, child_pid);

    VTR_LOGV(verbose, "[ARKANGEL INFO] Parent pid: %d, child pid: %d, child has run\n", parent_pid,
             child_pid);
    fflush(stdout);

    close(p_stdin[1]);
    close(p_stdout[0]);
    close(p_stderr[0]);
    close(pipefd[0]);

    /* Notify parent */
    write(pipefd[1], msg, strlen(msg) + 1);

    /* Close all fds except stdin/stdout/stderr */
    int fd_limit = static_cast<int>(sysconf(_SC_OPEN_MAX));
    for (int fd = STDERR_FILENO + 1; fd < fd_limit; ++fd) {
      if ((fcntl(fd, F_GETFD) == -1) || (errno == EBADF)) continue;
      close(fd);
    }

    execl("/bin/bash", "sh", "-c", command, nullptr);
    VTR_LOG_ERROR("[ARKANGEL ERROR] execl failed for: %s\n", command);
    return -1;
  }

  /* ---- parent ---- */
  pid_t child_pid = pid;
  VTR_LOGV(verbose, "[ARKANGEL INFO] Parent pid: %d, child pid: %d, parent running\n", parent_pid,
           child_pid);
  fflush(stdout);

  char buf[sizeof(msg)] = {0};
  while (true) {
    int ret = read(pipefd[0], buf, sizeof(buf));
    if (ret > 0) {
      VTR_LOGV(verbose, "[ARKANGEL INFO] Parent received: %s\n", buf);
      if (0 == strncmp(buf, msg, strlen(msg))) break;
    } else {
      VTR_LOGV(verbose, "[ARKANGEL INFO] Waiting for child process to start\n");
      usleep(500000);
    }
  }

  close(p_stdin[0]);
  close(p_stdout[1]);
  close(p_stderr[1]);
  close(pipefd[0]);
  close(pipefd[1]);

  if (infp == nullptr) {
    close(p_stdin[1]);
  } else {
    *infp = p_stdin[1];
  }
  if (outfp == nullptr) {
    close(p_stdout[0]);
  } else {
    *outfp = p_stdout[0];
  }
  if (errfp == nullptr) {
    close(p_stderr[0]);
  } else {
    *errfp = p_stderr[0];
  }

  return child_pid;
}
#endif /* _WIN32 */

/* ====================================================================
 * system_wrapper – run a command synchronously; return exec status
 * ==================================================================== */
int system_wrapper(std::string cmd_line, bool verbose) {
  if (cmd_line.empty()) return openfpga::CMD_EXEC_SUCCESS;

  NativeFd err_fd = NATIVE_INVALID_FD;
  NativePid child_pid = system_with_pid(cmd_line.c_str(), nullptr, nullptr, &err_fd, verbose);

  if (child_pid == NATIVE_INVALID_PID) {
    VTR_LOG_ERROR("[ARKANGEL ERROR] Failed to create child process\n");
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  /* Signal-handler setup (Linux only; no-op shim expected on Windows) */
  arkangel::SignalHandlerManager::set_sig_handler(false);
  arkangel::SignalHandlerManager::set_running_cmd_flag(true);
  std::set<NativePid> family_pid_collect = {child_pid};
  arkangel::SignalHandlerManager::set_monitored_process(family_pid_collect);

  VTR_LOGV(verbose, "[ARKANGEL INFO] Waiting for process pid: %lu\n",
           static_cast<unsigned long>(child_pid));
  fflush(stdout);

/* ---- Wait for child to finish ---- */
#ifdef _WIN32
  HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, child_pid);
  DWORD exit_code = 1;
  if (hProcess != INVALID_HANDLE_VALUE) {
    WaitForSingleObject(hProcess, INFINITE);
    GetExitCodeProcess(hProcess, &exit_code);
    CloseHandle(hProcess);
  }
  int status = static_cast<int>(exit_code);
#else
  int status = 0;
  waitpid(child_pid, &status, 0);
  status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif

  VTR_LOGV(verbose, "[ARKANGEL INFO] Process %lu finished with status %d\n",
           static_cast<unsigned long>(child_pid), status);
  fflush(stdout);

  /* ---- Drain error pipe (non-blocking) ---- */
  char buf[BUF_SIZE] = {0};
  std::string err_msg;

#ifdef _WIN32
  /* Set pipe to non-blocking via PeekNamedPipe before each read */
  while (true) {
    DWORD avail = 0;
    if (!PeekNamedPipe(err_fd, nullptr, 0, nullptr, &avail, nullptr) || avail == 0) break;
    DWORD nread = 0;
    if (!ReadFile(err_fd, buf, BUF_SIZE - 1, &nread, nullptr) || nread == 0) break;
    buf[nread] = '\0';
    err_msg += buf;
  }
#else
  fcntl(err_fd, F_SETFL, fcntl(err_fd, F_GETFL) | O_NONBLOCK);
  while (true) {
    int ret = read(err_fd, buf, BUF_SIZE - 1);
    if (ret <= 0) break;
    if (ret < static_cast<int>(sizeof(buf))) buf[ret] = '\0';
    err_msg += buf;
  }
#endif

  if (!err_msg.empty()) {
    VTR_LOG(err_msg.c_str());
    VTR_LOG_ERROR("[ARKANGEL ERROR] Errors running: %s\n", cmd_line.c_str());
  }

  fflush(stdout);
  close_output_fd(err_fd);

  arkangel::SignalHandlerManager::set_sig_handler(true);

  return ((status == 0) && err_msg.empty()) ? openfpga::CMD_EXEC_SUCCESS
                                            : openfpga::CMD_EXEC_FATAL_ERROR;
}

/* ====================================================================
 * Utility helpers
 * ==================================================================== */

void close_output_fd(NativeFd fd) {
#ifdef _WIN32
  if (fd != INVALID_HANDLE_VALUE) CloseHandle(fd);
#else
  if (fd >= 0) close(fd);
#endif
}

std::string sanitize_child_error_prefixes(const std::string& msg) {
  std::string sanitized;
  sanitized.reserve(msg.size());

  for (size_t i = 0; i < msg.size();) {
    constexpr const char* kErrorPrefix = "Error ";
    constexpr size_t kErrorPrefixLen = 6;
    if (msg.compare(i, kErrorPrefixLen, kErrorPrefix) == 0) {
      size_t digit_pos = i + kErrorPrefixLen;
      size_t scan_pos = digit_pos;
      while (scan_pos < msg.size() && std::isdigit(static_cast<unsigned char>(msg[scan_pos]))) {
        ++scan_pos;
      }
      if (scan_pos > digit_pos && scan_pos < msg.size() && msg[scan_pos] == ':') {
        sanitized += "ERROR:";
        i = scan_pos + 1;
        continue;
      }
    }
    sanitized += msg[i++];
  }
  return sanitized;
}

void drain_child_output(RunningTaskProcess& child) {
  if (child.output_fd == NATIVE_INVALID_FD) return;

  char buffer[4096];

#ifdef _WIN32
  while (true) {
    DWORD avail = 0;
    if (!PeekNamedPipe(child.output_fd, nullptr, 0, nullptr, &avail, nullptr) || avail == 0) break;
    DWORD nread = 0;
    if (!ReadFile(child.output_fd, buffer, sizeof(buffer), &nread, nullptr) || nread == 0) break;
    child.output.append(buffer, nread);
  }
#else
  while (true) {
    ssize_t nread = read(child.output_fd, buffer, sizeof(buffer));
    if (nread <= 0) break;
    child.output.append(buffer, static_cast<size_t>(nread));
  }
#endif
}

void drain_all_child_outputs(std::vector<RunningTaskProcess>& children) {
  for (auto& child : children) drain_child_output(child);
}

void replay_child_output(const RunningTaskProcess& child) {
  if (child.output.empty()) return;
  std::string sanitized = sanitize_child_error_prefixes(child.output);
  VTR_LOG("%s", sanitized.c_str());
  if (!sanitized.empty() && sanitized.back() != '\n') VTR_LOG("\n");
}

void terminate_task_process_group(NativePid pid) {
  if (pid == NATIVE_INVALID_PID) return;

#ifdef _WIN32
  /* On Windows we open the process and terminate it; there is no direct
     "process group" concept equivalent to POSIX, but CREATE_NEW_PROCESS_GROUP
     in system_with_pid means GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid)
     reaches the whole group before we hard-kill. */
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if (hProcess != INVALID_HANDLE_VALUE) {
    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
    Sleep(100);
    TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
  }
#else
  kill(-pid, SIGTERM);
  usleep(100000);
  kill(-pid, SIGKILL);
#endif
}

}  // namespace arkangel
