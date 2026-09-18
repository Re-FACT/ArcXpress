#ifndef ARKANGEL_PROCESS_UTIL_H
#define ARKANGEL_PROCESS_UTIL_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <set>
#include <string>
#include <vector>

#define BUF_SIZE 256

/* ----------------------------------------------------------------
 * Platform-specific process/handle types
 * ---------------------------------------------------------------- */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* On Windows a "process id" is a DWORD and I/O is done through HANDLEs.
   We expose a thin compatibility layer so the rest of the code can use
   the same RunningTaskProcess struct on both platforms. */
using NativePid = DWORD;
using NativeFd = HANDLE;
#define NATIVE_INVALID_FD INVALID_HANDLE_VALUE
#define NATIVE_INVALID_PID static_cast<NativePid>(0)

#else /* POSIX */
#include <sys/types.h>

using NativePid = pid_t;
using NativeFd = int;
#define NATIVE_INVALID_FD (-1)
#define NATIVE_INVALID_PID static_cast<NativePid>(-1)
#endif /* _WIN32 */

/********************************************************************
 * Public API
 *******************************************************************/

/* begin namespace arkangel */
namespace arkangel {

struct RunningTaskProcess {
  NativePid pid = NATIVE_INVALID_PID;
  NativeFd output_fd = NATIVE_INVALID_FD;
  std::string output;

#ifdef _WIN32
  /* Windows needs the process HANDLE (distinct from the PID) to wait/kill. */
  HANDLE process_handle = INVALID_HANDLE_VALUE;
#endif
};

/**
 * @brief Spawn @p command as a child process, returning its PID.
 *
 * If the corresponding fp pointer is non-null the matching pipe end is
 * returned through it; otherwise that end is closed.
 *
 * Returns NATIVE_INVALID_PID on failure.
 */
NativePid system_with_pid(const char* command, NativeFd* infp, NativeFd* outfp, NativeFd* errfp,
                          bool verbose = false);

/**
 * @brief Run @p cmd_line synchronously; returns CMD_EXEC_SUCCESS or
 *        CMD_EXEC_FATAL_ERROR (openfpga codes).
 */
int system_wrapper(std::string cmd_line, bool verbose = false);

/** Close an output file descriptor / handle (no-op if already invalid). */
void close_output_fd(NativeFd fd);

/** Rewrite child "Error N:" prefixes to "ERROR:" for uniform logging. */
std::string sanitize_child_error_prefixes(const std::string& msg);

/** Non-blocking drain of a single child's output pipe into child.output. */
void drain_child_output(RunningTaskProcess& child);

/** Drain every child in @p children. */
void drain_all_child_outputs(std::vector<RunningTaskProcess>& children);

/** Print (and optionally sanitize) the buffered output of @p child. */
void replay_child_output(const RunningTaskProcess& child);

/**
 * @brief Send SIGTERM (Linux) or TerminateProcess (Windows) to the
 *        entire process group / job object rooted at @p pid.
 */
void terminate_task_process_group(NativePid pid);

} /* end namespace arkangel */

#endif /* ARKANGEL_PROCESS_UTIL_H */
