#include "task_scheduler.h"

#include <cstdio>
#include <queue>
#include <string>
#include <utility>

#include "arcxpress_process_util.h"
#include "command_exit_codes.h"
#include "vtr_log.h"

/* -----------------------------------------------------------------------
 * Platform includes
 * ----------------------------------------------------------------------- */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>

#include <atomic>
#include <list>
#include <thread>
#include <vector>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace arcxpress {

/* ======================================================================
 * Constructor / setters
 * ====================================================================== */

ParallelTaskScheduler::ParallelTaskScheduler(int max_processors, bool verbose)
  : max_processors_(1), verbose_(verbose) {
  set_max_processors(max_processors);
}

void ParallelTaskScheduler::set_max_processors(int max_processors) {
  max_processors_ = (max_processors < 1) ? 1 : max_processors;
}

void ParallelTaskScheduler::set_verbose(bool verbose) { verbose_ = verbose; }

void ParallelTaskScheduler::add_task(std::function<int()> task) {
  tasks_.push_back(std::move(task));
}

void ParallelTaskScheduler::clear_tasks() { tasks_.clear(); }

size_t ParallelTaskScheduler::num_tasks() const { return tasks_.size(); }

/* ======================================================================
 * run_tasks() – platform implementations
 * ====================================================================== */

#ifdef _WIN32
/* ----------------------------------------------------------------------
 * Windows implementation
 *
 * Strategy
 * --------
 * Windows has no fork().  Each task lambda is run inside a std::thread
 * that captures stdout/stderr through an anonymous pipe.  The thread:
 *   1. Redirects its own stdout/stderr to the write end of the pipe.
 *   2. Executes the task lambda.
 *   3. Closes the write end so the read end reaches EOF.
 *
 * The scheduler maintains a vector of "slots", each holding:
 *   - the thread object
 *   - an atomic exit-code cell the thread writes before exiting
 *   - the read end of the pipe (drained non-blockingly in the main loop)
 *   - the accumulated output string
 *
 * "Fail-fast" is implemented by setting a shared cancellation flag;
 * running threads check it (via the lambda's own logic) only if the
 * lambda cooperates.  For hard termination we rely on process exit.
 *
 * Note: because we use threads rather than processes the "process group"
 * concept does not apply; terminate_task_process_group() is still called
 * for completeness when a child spawns sub-processes via system_with_pid.
 * ---------------------------------------------------------------------- */

struct WinTaskSlot {
  std::thread thread;
  std::atomic<int> exit_code{-1}; /* -1 = still running */
  HANDLE read_pipe = INVALID_HANDLE_VALUE;
  HANDLE write_pipe = INVALID_HANDLE_VALUE;
  std::string output;
  NativePid child_pid = NATIVE_INVALID_PID; /* set if task spawns a sub-process */
};

/* Non-blocking drain of a Windows anonymous pipe into slot.output */
static void win_drain_slot(WinTaskSlot& slot) {
  if (slot.read_pipe == INVALID_HANDLE_VALUE) return;
  char buf[4096];
  while (true) {
    DWORD avail = 0;
    if (!PeekNamedPipe(slot.read_pipe, nullptr, 0, nullptr, &avail, nullptr) || avail == 0) break;
    DWORD nread = 0;
    if (!ReadFile(slot.read_pipe, buf, sizeof(buf) - 1, &nread, nullptr) || nread == 0) break;
    buf[nread] = '\0';
    slot.output.append(buf, nread);
  }
}

int ParallelTaskScheduler::run_tasks() const {
  VTR_LOGV(verbose_, "Running %zu parallel tasks with up to %d processors\n", tasks_.size(),
           max_processors_);

  std::queue<std::function<int()>> task_queue;
  for (const auto& t : tasks_) task_queue.push(t);

  std::list<WinTaskSlot> slots;

  bool all_ok = true;

  auto cleanup_all_slots = [&]() {
    for (auto& s : slots) {
      /* Signal any sub-process the task may have spawned */
      if (s.child_pid != NATIVE_INVALID_PID) terminate_task_process_group(s.child_pid);
      if (s.thread.joinable()) s.thread.join();
      if (s.read_pipe != INVALID_HANDLE_VALUE) CloseHandle(s.read_pipe);
      if (s.write_pipe != INVALID_HANDLE_VALUE) CloseHandle(s.write_pipe);
    }
    slots.clear();
  };

  while (!task_queue.empty() || !slots.empty()) {
    /* ---- launch new tasks ---- */
    while (!task_queue.empty() && static_cast<int>(slots.size()) < max_processors_) {
      auto task = task_queue.front();
      task_queue.pop();

      SECURITY_ATTRIBUTES sa{};
      sa.nLength = sizeof(sa);
      sa.bInheritHandle = FALSE; /* threads share the same process; no inherit needed */

      WinTaskSlot slot;
      if (!CreatePipe(&slot.read_pipe, &slot.write_pipe, &sa, 0)) {
        VTR_LOG_ERROR("[Arcxpress ERROR] Failed to create output pipe\n");
        cleanup_all_slots();
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }

      /* Launch task in a thread; capture slot's write_pipe */
      HANDLE wp = slot.write_pipe;
      std::atomic<int>* ec = &slot.exit_code;

      slot.thread = std::thread([task, wp, ec]() {
        /* Redirect this thread's stdout/stderr to the pipe.
           _open_osfhandle + _fdopen let us use the CRT on top of a HANDLE. */
        int crt_fd = _open_osfhandle(reinterpret_cast<intptr_t>(wp), 0 /*_O_RDONLY not set*/);
        FILE* pipe_file = (crt_fd >= 0) ? _fdopen(crt_fd, "w") : nullptr;

        FILE* old_stdout = nullptr;
        FILE* old_stderr = nullptr;
        if (pipe_file) {
          old_stdout = stdout; /* save */
          old_stderr = stderr;
          /* Redirect at CRT level */
          freopen_s(&old_stdout, "CONOUT$", "w", stdout); /* dummy; overwritten below */
          *stdout = *pipe_file;
          *stderr = *pipe_file;
        }

        int result = task();
        fflush(stdout);
        fflush(stderr);

        if (pipe_file) fclose(pipe_file); /* closes wp as well */

        ec->store(result == openfpga::CMD_EXEC_SUCCESS ? 0 : 1);
      });

      // After:
      slots.emplace_back();
      auto& new_slot = slots.back();
      new_slot.read_pipe = slot.read_pipe;
      new_slot.write_pipe = slot.write_pipe;
      new_slot.thread = std::move(slot.thread);
      new_slot.exit_code.store(-1);
      new_slot.output = std::move(slot.output);
      new_slot.child_pid = slot.child_pid;
    }

    /* ---- drain all running slots ---- */
    for (auto& s : slots) win_drain_slot(s);

    /* ---- check for finished slots ---- */
    for (auto it = slots.begin(); it != slots.end();) {
      int ec = it->exit_code.load();
      if (ec == -1) { /* still running */
        ++it;
        continue;
      }

      /* Thread finished – join and collect remaining output */
      if (it->thread.joinable()) it->thread.join();
      win_drain_slot(*it);
      CloseHandle(it->read_pipe);
      it->read_pipe = INVALID_HANDLE_VALUE;

      if (ec != 0) {
        /* Print buffered output and fail-fast */
        std::string sanitized = sanitize_child_error_prefixes(it->output);
        VTR_LOG("%s", sanitized.c_str());
        if (!sanitized.empty() && sanitized.back() != '\n') VTR_LOG("\n");
        VTR_LOG("Parallel task failed\n");
        all_ok = false;

        it = slots.erase(it);
        cleanup_all_slots();
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }

      if (verbose_) replay_child_output({NATIVE_INVALID_PID, NATIVE_INVALID_FD, it->output});
      it = slots.erase(it);
    }

    Sleep(100); /* avoid busy-loop: ~100 ms */
  }

  return all_ok ? openfpga::CMD_EXEC_SUCCESS : openfpga::CMD_EXEC_FATAL_ERROR;
}

#else /* ================================================================   \
       * POSIX (Linux / macOS) implementation – original logic, unchanged \
       * ================================================================ */

int ParallelTaskScheduler::run_tasks() const {
  VTR_LOGV(verbose_, "Running %zu parallel tasks with up to %d processors\n", tasks_.size(),
           max_processors_);

  std::queue<std::function<int()>> task_queue;
  for (const auto& t : tasks_) task_queue.push(t);

  std::vector<RunningTaskProcess> running_children;
  bool all_ok = true;

  while (!task_queue.empty() || !running_children.empty()) {
    /* ---- fork new children while slots are available ---- */
    while (!task_queue.empty() && running_children.size() < static_cast<size_t>(max_processors_)) {
      auto task = task_queue.front();
      task_queue.pop();

      int output_pipe[2];
      if (pipe(output_pipe) != 0) {
        VTR_LOG_ERROR("[Arcxpress ERROR] Failed to create child output pipe\n");
        for (auto& child : running_children) {
          terminate_task_process_group(child.pid);
          waitpid(child.pid, nullptr, 0);
          close_output_fd(child.output_fd);
        }
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }

      fflush(stdout);
      fflush(stderr);

      pid_t pid = fork();
      if (pid < 0) {
        close(output_pipe[0]);
        close(output_pipe[1]);
        VTR_LOG_ERROR("[Arcxpress ERROR] Failed to fork child process\n");
        for (auto& child : running_children) {
          terminate_task_process_group(child.pid);
          waitpid(child.pid, nullptr, 0);
          close_output_fd(child.output_fd);
        }
        return openfpga::CMD_EXEC_FATAL_ERROR;
      }

      if (pid == 0) {
        /* Child: isolate descendants, redirect output, run task */
        setsid();
        close(output_pipe[0]);
        dup2(output_pipe[1], STDOUT_FILENO);
        dup2(output_pipe[1], STDERR_FILENO);
        close(output_pipe[1]);
        int task_status = task();
        fflush(stdout);
        fflush(stderr);
        _exit(task_status == openfpga::CMD_EXEC_SUCCESS ? 0 : 1);
      } else {
        /* Parent: keep the read end; make it non-blocking */
        close(output_pipe[1]);
        fcntl(output_pipe[0], F_SETFL, fcntl(output_pipe[0], F_GETFL) | O_NONBLOCK);
        running_children.push_back({pid, output_pipe[0], std::string()});
      }
    }

    drain_all_child_outputs(running_children);

    /* ---- reap finished children ---- */
    for (auto it = running_children.begin(); it != running_children.end();) {
      int status = 0;
      pid_t ret = waitpid(it->pid, &status, WNOHANG);

      if (ret > 0) {
        drain_child_output(*it);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
          replay_child_output(*it);
          VTR_LOG("Parallel child process %d failed\n", it->pid);
          all_ok = false;

          /* Fail-fast: kill all sibling process groups */
          for (auto& child : running_children) {
            if (child.pid != it->pid) {
              terminate_task_process_group(child.pid);
              waitpid(child.pid, nullptr, 0);
              close_output_fd(child.output_fd);
            }
          }
          close_output_fd(it->output_fd);
          running_children.clear();
          return openfpga::CMD_EXEC_FATAL_ERROR;
        }

        if (verbose_) replay_child_output(*it);
        close_output_fd(it->output_fd);
        it = running_children.erase(it);

      } else if (ret < 0) {
        VTR_LOG_ERROR("[Arcxpress ERROR] waitpid failed for child %d\n", it->pid);
        close_output_fd(it->output_fd);
        it = running_children.erase(it);
        all_ok = false;
      } else {
        ++it;
      }
    }

    usleep(100000); /* avoid busy-loop: 100 ms */
  }

  return all_ok ? openfpga::CMD_EXEC_SUCCESS : openfpga::CMD_EXEC_FATAL_ERROR;
}

#endif /* _WIN32 */

}  // namespace arcxpress
