#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <filesystem>
#include <string>

namespace arkangel {

/********************************************************************
 * Function declaration
 *******************************************************************/
int universal_setenv(const char* name, const char* value, int overwrite);
// return a string to path of current work dir
std::string get_cwd();

// Check and convert windows-style path to generic ones
std::string format_windows_path(const std::string& path_name);

// set current directory to given path_name
void set_cwd(std::string path_name);

// check if a given path_name is legal direcotry
bool path_is_legal(std::string path_name);

// check if a given path_name is legal file
bool file_is_legal(std::string path_name);

// create a path to a given path_name
bool create_path(std::string path_name);

// return a string of file path to a given file_name
// add a given work path if that file name is relative path
std::string locate_file_path(std::string work_path, std::string file_name);

// return a string of file's full path
std::string absolute_file_path(std::string file_name);

// Create symbolic link support both windows and linuxk
bool universal_create_symlink(const std::filesystem::path& target,
                              const std::filesystem::path& link_path);

}  // namespace arkangel
