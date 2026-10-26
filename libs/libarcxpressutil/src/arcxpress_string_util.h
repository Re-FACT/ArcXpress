#ifndef ARCXPRESS_STRING_UTIL_H
#define ARCXPRESS_STRING_UTIL_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <string>
#include <vector>

/********************************************************************
 * Function declaration
 *******************************************************************/
char** string_list_to_char_array(const std::vector<std::string>& strings);
void free_char_array(const size_t& ary_len, char** ary);

#endif
