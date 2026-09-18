#pragma once

/* -----------------------------------------------------------------------
 * Platform portability: on POSIX a "pid" is pid_t; on Windows it is a
 * DWORD.  Both are covered by NativePid from arkangel_process_util.h.
 * ----------------------------------------------------------------------- */
#include <cstddef>
#include <functional>
#include <vector>

#include "arkangel_process_util.h"

/* begin namespace arkangel */
namespace arkangel {

class ParallelTaskScheduler {
 public:
  ParallelTaskScheduler(int max_processors = 1, bool verbose = false);

  void set_max_processors(int max_processors);
  void set_verbose(bool verbose);
  void add_task(std::function<int()> task);
  void clear_tasks();
  size_t num_tasks() const;
  int run_tasks() const;

 private:
  int max_processors_;
  bool verbose_;
  std::vector<std::function<int()>> tasks_;
};

} /* end namespace arkangel */
