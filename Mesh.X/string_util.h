/* 
 * File:   string_util.h
 * Author: dantech
 *
 * Created on November 15, 2018, 6:18 PM
 */

#ifndef STRING_UTIL_H
#define	STRING_UTIL_H

int ends_with(char* buf, int buf_len, char* term, int term_len);

int starts_with(char* str, int str_len, char* pre, int pre_len);

char* strp(char** str, char* delim);

#endif	/* STRING_UTIL_H */

