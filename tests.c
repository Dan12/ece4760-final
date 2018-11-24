#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int starts_with(char* str, int str_len, char* pre, int pre_len) {
  if (str_len < pre_len) {
    return 1;
  }
  return strncmp(pre, str, pre_len);
}

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

int parse_mac(char* mac) {
  int result = 0;
  char* tmp = strp(&mac, ":");
  while(tmp != NULL) {
    result <<= 8;
    result |= (int) strtol(tmp, NULL, 16);
    tmp = strp(&mac, ":");
  }
  return result;
}

int main(void) {

  char str[] = "+CWLAP:(3,\"ESP8266-Mesh-2\",-6,\"86:f3:eb:59:c2:ca\",1,25,-120,5,3,3,0)\r\n+IPD,0,62:M#F,86:f3:eb:59:c2:ca,2,0,1a:fe:34:a0:75:e1,86:f3:eb:59:c2:ca\n\r\n+CWLAP:(1,\"ESP8266-Mesh-4\",-6,\"86:f3:eb:59:c2:ca\",1,25,-120,5,3,3,0)\r\n";
  char* str_i = str;
  // printf("Hello: %s\n", str);
  char* tmp = strp(&str_i, "CWLAP");
  printf("T: %s\n", tmp);
  tmp = strp(&str_i, "\"");
  char* ssid = strp(&str_i, "\"");
  printf("T: %s\n", ssid);
  printf("SSID: %s\n", starts_with(ssid, strlen(ssid), "ESP8266", 7) == 0 ? "true" : "false");
  tmp = strp(&str_i, ",");
  char* rssi = strp(&str_i, ",");
  printf("T: %d\n", -atoi(rssi));
  tmp = strp(&str_i, "\"");
  char* mac = strp(&str_i, "\"");
  printf("T: %d\n", parse_mac(mac));
  tmp = strp(&str_i, "CWLAP");
  tmp = strp(&str_i, "\"");
  printf("T: %s\n", tmp);
  ssid = strp(&str_i, "\"");
  printf("T: %s\n", ssid);
  printf("SSID: %s\n", starts_with(ssid, strlen(ssid), "ESP8266", 7) == 0 ? "true" : "false");
  tmp = strp(&str_i, ",");
  rssi = strp(&str_i, ",");
  printf("T: %d\n", -atoi(rssi));
  tmp = strp(&str_i, "\"");
  mac = strp(&str_i, "\"");
  printf("T: %d\n", parse_mac(mac));
  tmp = strp(&str_i, "CWLAP");
  tmp = strp(&str_i, "\"");
  printf("T: %s\n", tmp);

  char str2[] = "+IPD,0,62:M#F,86:f3:eb:59:c2:ca,2,0,1a:fe:34:a0:75:e1,86:f3:eb:59:c2:ca\n'";
  char* str2_i = str2;
  tmp = strp(&str2_i, "IPD");
  printf("T: %s\n", tmp);
  tmp = strp(&str2_i, ",");
  char* link_id = strp(&str2_i, ",");
  printf("T: %d\n", atoi(link_id));
  char* len = strp(&str2_i, ":");
  printf("T: %d\n", atoi(len));
  char* msg = strp(&str2_i, "\n");
  printf("T: %s\n", msg);

  char str3[] = "+CIPAPMAC_CUR:\"1a:fe:34:a0:75:e1\"\r\n";
  char* str3_i = str3;
  tmp = strp(&str3_i, "CIPAPMAC_CUR");
  printf("T: %s\n", tmp);
  tmp = strp(&str3_i, "\"");
  printf("T: %s\n", tmp);
  tmp = strp(&str3_i, "\"");
  printf("T: %s\n", tmp);

  printf("Result: %s\n", (!0) ? "true" : "false");

  char line_buffer[1024];
  char str4[] = "+CWLAP:(3,\"ESP8266-Mesh-2\",-6,\"86:f3:eb:59:c2:ca\",1,25,-120,5,3,3,0)\r\n+IPD,0,62:M#F,86:f3:eb:59:c2:ca,2,0,1a:fe:34:a0:75:e1,86:f3:eb:59:c2:ca\n\r\n+CWLAP:(1,\"ESP8266-Mesh-4\",-6,\"86:f3:eb:59:c2:ca\",1,25,-120,5,3,3,0)\r\n";
  str_i = str4;
  tmp = strstr(str_i, "\r\n")+2;
  printf("%p, %p\n", tmp, str_i);
  strncpy(line_buffer, str_i, (int) (tmp - str_i));
  line_buffer[(int) (tmp-str_i)] = '\0';
  printf("Line: %s\n", line_buffer);
  str_i = tmp;
  tmp = strstr(tmp, "\r\n")+2;
  strncpy(line_buffer, str_i, (int) (tmp - str_i));
  line_buffer[(int) (tmp-str_i)] = '\0';
  printf("Line: %s\n", line_buffer);
  str_i = tmp;
  tmp = strstr(tmp, "\r\n")+2;
  strncpy(line_buffer, str_i, (int) (tmp - str_i));
  line_buffer[(int) (tmp-str_i)] = '\0';
  printf("Line: %s\n", line_buffer);

  // char str4[] = "CS,4,111";
  // // printf("Hello: %s\n", );
  // char* next = strchr(str4, ',');
  // next[0] = '\0';
  // next++;
  // // tmp = strtok(str3, "CIPAPMAC_CUR");
  // printf("T: %s\n", str4);
  // // tmp = strtok(NULL, "\"");
  // printf("T: %s\n", next);

  return 0;
}