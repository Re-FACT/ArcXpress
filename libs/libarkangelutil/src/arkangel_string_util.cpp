/********************************************************************
 * This file includes the functions to manipulate strings for Arkangel's usage
 *******************************************************************/
#include "arkangel_string_util.h"

#include "vtr_assert.h"

/********************************************************************
 * Convert a list of string to an array of c-type string (char-based)
 *******************************************************************/
char** string_list_to_char_array(const std::vector<std::string>& strings) {
  char** ret = nullptr;
  /* TODO: make this safe in memory? */
  ret = new char*[strings.size()];
  size_t opt_idx = 0;
  for (const std::string& string : strings) {
    ret[opt_idx] = new char[string.size()];
    /* Avoid to copy the end of string '\0' */
    for (int ichar = 0; ichar < string.length() + 1; ++ichar) {
      ret[opt_idx][ichar] = string[ichar];
    }
    opt_idx++;
  }
  /* Sanity checks */
  VTR_ASSERT(opt_idx == strings.size());
  return ret;
}

/********************************************************************
 * Free an array of char. Use to ensure memory cleaness after using some functions
 * - string_list_to_char_array()
 *******************************************************************/
void free_char_array(const size_t& ary_len, char** ary) {
  for (size_t i = 0; i < ary_len; i++) {
    if (ary[i]) {
      free(ary[i]);
    }
  }
  if (ary) {
    free(ary);
  }
}
