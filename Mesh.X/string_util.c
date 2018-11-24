#include "string_util.h"
#include <stdlib.h>
#include <string.h>

/**
 * 
 * @param buf
 * @param buf_len
 * @param term
 * @param term_len
 * @return 0 if buf ends with term, 1 otherwise 
 */
int ends_with(char* buf, int buf_len, char* term, int term_len) {
  int i;
  int ti = term_len-1;
  for (i = buf_len-1; i >= 0 && ti >= 0; i--, ti--) {
    if (buf[i] != term[ti]) {
      return 1;
    }
  }
  return 0;
}

/**
 * 
 * @param str
 * @param str_len
 * @param pre
 * @param pre_len
 * @return 0 if str ends with pre, 1 otherwise
 */
int starts_with(char* str, int str_len, char* pre, int pre_len) {
  if (str_len < pre_len) {
    return 1;
  }
  return strncmp(pre, str, pre_len);
}

/**
 * Splits the string pointed to by str by the first instance of delim.
 * Returns the first split and updates str to point to the second split
 * @param str
 * @param delim
 * @return 
 */
char* strp(char** str, char* delim) {
  if (*str == NULL) {
    return NULL;
  }
  char* tmp = strstr(*str, delim);
  if (tmp == NULL) {
    char* ret = *str;
    *str = NULL;
    return ret;
  }
  tmp[0] = '\0';
  tmp += strlen(delim);
  char* ret = *str;
  *str = tmp;
  return ret;
}