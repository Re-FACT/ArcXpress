/********************************************************************
 * This file includes the functions to do file/directory operation
 *******************************************************************/
#include "arcxpress_file_os.h"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <system_error>

#include "openfpga_digest.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace arcxpress {

int universal_setenv(const char* name, const char* value, int overwrite) {
#ifdef _WIN32
  // Windows: use _putenv_s (safe version of putenv)
  // If overwrite is 0, check if it already exists before setting
  if (overwrite == 0 && getenv(name) != nullptr) {
    return 0;
  }
  return _putenv_s(name, value);
#else
  // POSIX: Linux, macOS, etc.
  return setenv(name, value, overwrite);
#endif
}

std::string format_windows_path(const std::string& path_name) {
  std::string ret = path_name;
  size_t pos = 0;
  while ((pos = ret.find('\\', pos)) != std::string::npos) {
    ret.replace(pos, 1, "/");
    pos += 2;
  }
  return ret;
}

std::string get_cwd() {
  std::filesystem::path cwd = std::filesystem::current_path();
  return cwd.generic_string();
}

void set_cwd(std::string path_name) {
  // this function now sets both CWD and PWD
  if (path_name == "") {
    return;
  }
  std::filesystem::path path = path_name;
  std::filesystem::current_path(path);
  VTR_ASSERT(universal_setenv("PWD", path_name.c_str(), 1) == 0);
}

bool path_is_legal(std::string path_name) {
  std::filesystem::path path = path_name;
  return (std::filesystem::is_directory(path));
}

bool file_is_legal(std::string path_name) { return (std::filesystem::exists(path_name)); }

bool create_path(std::string path_name) {
  if (arcxpress::path_is_legal(path_name)) {
    return false;
  } else {
    std::filesystem::path path = path_name;
    return std::filesystem::create_directories(path);
  }
}

// in some case like strategy file parsing, have to compute full path on-fly
std::string locate_file_path(std::string work_path, std::string file_name) {
  // file can be found at current work dir
  if (arcxpress::file_is_legal(file_name)) {
    return file_name;
  } else {
    // try to look for through work_path
    std::string path_name = openfpga::format_dir_path(work_path) + file_name;
    if (arcxpress::file_is_legal(path_name)) {
      return path_name;
    }
  }
  return std::string("");
}

// get full path
std::string absolute_file_path(std::string file_name) {
  std::filesystem::path path = file_name;
  return std::filesystem::absolute(path).string();
}

// An universal way to create symbolic links on both windows and linux
bool universal_create_symlink(const std::filesystem::path& target,
                              const std::filesystem::path& link_path) {
  // Basic sanity check: Ensure target exists before linking
  if (!std::filesystem::exists(target)) {
    VTR_LOG_ERROR("Target path does not exist:\n%s\n",
                  std::filesystem::path(target).make_preferred().string().c_str());
    return false;
  }

#ifdef _WIN32
  // --- Windows Implementation ---
  // Convert paths to use native backslashes (\)
  std::string target_str = std::filesystem::path(target).make_preferred().string();
  std::string link_str = std::filesystem::path(link_path).make_preferred().string();

  // Construct and run the Windows cmd mklink command
  std::string command = "cmd /c mklink \"" + link_str + "\" \"" + target_str + "\"";
  return std::system(command.c_str()) == 0;

#else
  // --- Linux / macOS Implementation ---
  std::error_code ec;
  std::filesystem::create_symlink(target, link_path, ec);

  if (ec) {
    VTR_LOG_ERROR("Failed to create symbolic link (error code: %d) for source file:\n%s\n",
                  ec.message().c_str(),
                  std::filesystem::path(target).make_preferred().string().c_str());
    return false;
  }
  return true;
#endif
}

}  // namespace arcxpress
