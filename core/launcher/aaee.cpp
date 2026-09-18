/**
 * aaee - Cross-platform launcher for the arkangel executable.
 *
 * Replicates the logic of the original Bash script:
 *   1. Locate the directory that contains this binary  (AAEE_HOME).
 *   2. Optionally prepend <AAEE_HOME>/../lib to the library search path
 *      if the directory exists.
 *   3. Optionally prepend <AAEE_HOME>/../env/eda/lib to the library search
 *      path (highest priority) ONLY on CentOS / RHEL, and only if the
 *      directory exists.  Detection is done by reading /etc/os-release at
 *      runtime; no compile-time flag is required.
 *   4. Exec  <AAEE_HOME>/arkangel  with fixed args + any args passed by
 *      the caller.
 *
 * Platform notes
 * --------------
 *   Linux / macOS : uses LD_LIBRARY_PATH  (or DYLD_LIBRARY_PATH on macOS)
 *   Windows       : uses PATH (DLL resolution falls back to PATH on Windows;
 *                   there is no LD_LIBRARY_PATH equivalent)
 */

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "command_exit_codes.h"
#include "vtr_log.h"

#ifdef _WIN32
#include <shlwapi.h>  // PathRemoveFileSpecW  (link with shlwapi.lib)
#include <windows.h>  // GetModuleFileNameW, CreateProcessW
#pragma comment(lib, "shlwapi.lib")
#else
#include <unistd.h>  // execv
#endif

// ---------------------------------------------------------------------------
// Helper: return the directory that contains the running executable
// ---------------------------------------------------------------------------
static std::filesystem::path self_dir() {
#ifdef _WIN32
  wchar_t buf[MAX_PATH + 1] = {};
  if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0) {
    VTR_LOG_ERROR("aaee: GetModuleFileNameW failed\n");
    std::exit(openfpga::CMD_EXEC_FATAL_ERROR);
  }
  return std::filesystem::path(buf).parent_path();
#else
  // /proc/self/exe is Linux-specific; fall back to argv[0] on other POSIX
  std::error_code ec;
  std::filesystem::path p = std::filesystem::read_symlink("/proc/self/exe", ec);
  if (!ec) {
    return p.parent_path();
  }
  // macOS / BSD: _NSGetExecutablePath or use argv[0] resolution
  // For portability we rely on the caller to set argv[0] correctly.
  // (argv[0] is available in main – see selfDirFromArgv below)
  VTR_LOG_ERROR("aaee: cannot determine executable path\n");
  std::exit(openfpga::CMD_EXEC_FATAL_ERROR);
#endif
}

// ---------------------------------------------------------------------------
// Helper: prepend a path segment to an environment variable
// ---------------------------------------------------------------------------
static void prepend2env_path(const std::string& varName, const std::filesystem::path& newDir) {
  const char* current = std::getenv(varName.c_str());
#ifdef _WIN32
  const char SEP = ';';
#else
  const char SEP = ':';
#endif
  std::string updated = newDir.string();
  if (current && current[0] != '\0') {
    updated += SEP;
    updated += current;
  }
#ifdef _WIN32
  _putenv_s(varName.c_str(), updated.c_str());
#else
  setenv(varName.c_str(), updated.c_str(), /*overwrite=*/1);
#endif
}

// ---------------------------------------------------------------------------
// The name of the library-path variable for this platform
// ---------------------------------------------------------------------------
#ifdef _WIN32
static constexpr const char* LIB_PATH_VAR = "PATH";
#elif defined(__APPLE__)
static constexpr const char* LIB_PATH_VAR = "DYLD_LIBRARY_PATH";
#else
static constexpr const char* LIB_PATH_VAR = "LD_LIBRARY_PATH";
#endif

// ---------------------------------------------------------------------------
// Helper: detect CentOS or Red Hat Enterprise Linux at runtime.
//
// Reads /etc/os-release (the standard freedesktop.org file present on all
// modern RPM-based distros) and checks whether the ID or ID_LIKE field
// contains "centos" or "rhel".  Falls back to the legacy /etc/redhat-release
// if os-release is absent (RHEL 5 / CentOS 5).
// Always returns false on Windows.
// ---------------------------------------------------------------------------
static bool isCentOSOrRHEL() {
#ifdef _WIN32
  return false;
#else
  // --- primary: /etc/os-release -------------------------------------------
  auto containsRHEL = [](const std::string& val) -> bool {
    // val may be a space-separated list (ID_LIKE="rhel fedora")
    std::string lower = val;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find("rhel") != std::string::npos || lower.find("centos") != std::string::npos;
  };

  std::ifstream osRelease("/etc/os-release");
  if (osRelease.is_open()) {
    std::string line;
    while (std::getline(osRelease, line)) {
      // Lines look like:  KEY="value"  or  KEY=value
      auto eq = line.find('=');
      if (eq == std::string::npos) continue;

      std::string key = line.substr(0, eq);
      if (key != "ID" && key != "ID_LIKE") continue;

      // Strip surrounding quotes from value
      std::string val = line.substr(eq + 1);
      if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
        val = val.substr(1, val.size() - 2);

      if (containsRHEL(val)) return true;
    }
  }

  // --- fallback: /etc/redhat-release (RHEL/CentOS 5 and older) -----------
  std::ifstream rhrRelease("/etc/redhat-release");
  if (rhrRelease.is_open()) {
    std::string content((std::istreambuf_iterator<char>(rhrRelease)),
                        std::istreambuf_iterator<char>());
    if (containsRHEL(content)) return true;
    // /etc/redhat-release exists on any RHEL-family distro
    // even if the name string doesn't contain "rhel"/"centos"
    return true;
  }

  return false;
#endif
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  // --- 1. Locate AAEE_HOME ------------------------------------------------
  const std::filesystem::path aaee_home = self_dir();

  // --- 2. Default lib path: <AAEE_HOME>/../lib ----------------------------
  const std::filesystem::path root_path = std::filesystem::canonical(aaee_home / "..");
  const std::filesystem::path defaultLibPath = root_path / "lib";
  if (std::filesystem::is_directory(defaultLibPath)) {
    VTR_LOG("Include the default lib path\n");
    prepend2env_path(LIB_PATH_VAR, std::filesystem::canonical(defaultLibPath));
  }

  // --- 3. EDA lib path: <AAEE_HOME>/../env/eda/lib (highest priority) -----
  //        Only applied on CentOS / RHEL.
  const std::filesystem::path edaLibPath = root_path / "env" / "eda" / "lib";
  if (isCentOSOrRHEL()) {
    if (std::filesystem::is_directory(edaLibPath)) {
      std::cout << "Include the EDA lib path as the top priority\n";
      prepend2env_path(LIB_PATH_VAR, std::filesystem::canonical(edaLibPath));
    }
  }

  // --- 4. Build the arkangel command line ---------------------------------
  //   Fixed: --root_path <AAEE_HOME>/../  --boot_config <AAEE_HOME>/../etc/boot_config/install.xml
  //   Variadic: everything the user passed to aaee
  // TODO: use constants for arkangel executable

#ifdef _WIN32
  const std::filesystem::path arkangelExe = aaee_home / "arkangel.exe";
#else
  const std::filesystem::path arkangelExe = aaee_home / "arkangel";
#endif

#ifdef _WIN32
  const std::filesystem::path boot_config_fpath =
    root_path / "etc" / "boot_config" / "install_win_msys2_mingw64.xml";
#else
  const std::filesystem::path boot_config_fpath = root_path / "etc" / "boot_config" / "install.xml";
#endif

#ifdef _WIN32
  // Windows: build a quoted command string and use CreateProcess
  // (exec*() is not available on Windows without POSIX compat layer)

  auto quoteArg = [](const std::string& s) -> std::string {
    // Simple quoting: wrap in double-quotes and escape inner quotes
    std::string out = "\"";
    for (char c : s) {
      if (c == '"')
        out += "\\\"";
      else
        out += c;
    }
    out += '"';
    return out;
  };

  // TODO: use constants for arkangel options
  std::string cmdLine = quoteArg(arkangelExe.string());
  cmdLine += " --root_path " + quoteArg(root_path.string());
  cmdLine += " --boot_config " + quoteArg(boot_config_fpath.string());

  for (int i = 1; i < argc; ++i) {
    cmdLine += ' ';
    cmdLine += quoteArg(argv[i]);
  }

  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};

  // Convert to wide string for the Win32 API
  int wlen = MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, nullptr, 0);
  std::wstring wCmdLine(wlen, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, wCmdLine.data(), wlen);

  if (!CreateProcessW(nullptr, wCmdLine.data(), nullptr, nullptr, /*inheritHandles=*/TRUE, 0,
                      nullptr, nullptr, &si, &pi)) {
    VTR_LOG_ERROR("aaee: failed to launch arkangel (error %ld)\n", GetLastError());
    return openfpga::CMD_EXEC_FATAL_ERROR;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exitCode = openfpga::CMD_EXEC_SUCCESS;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return static_cast<int>(exitCode);

#else
  // POSIX: build argv array and execv (replaces the current process image)
  std::vector<std::string> args;
  args.push_back(arkangelExe.string());
  args.push_back("--root_path");
  args.push_back(root_path.string());
  args.push_back("--boot_config");
  args.push_back(boot_config_fpath.string());

  for (int i = 1; i < argc; ++i) args.push_back(argv[i]);

  std::vector<char*> cArgs;
  cArgs.reserve(args.size() + 1);
  for (auto& s : args) cArgs.push_back(const_cast<char*>(s.c_str()));
  cArgs.push_back(nullptr);

  execv(arkangelExe.c_str(), cArgs.data());

  // execv only returns on error
  VTR_LOG_ERROR("aaee: execv failed: %s\n", std::strerror(errno));
  return openfpga::CMD_EXEC_FATAL_ERROR;
#endif
}
