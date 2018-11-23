#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_mac(char* mac) {
  int result = 0;
  char* tmp = strtok(mac, ":");
  while(tmp != NULL) {
    result <<= 8;
    result |= (int) strtol(tmp, NULL, 16);
    tmp = strtok(NULL, ":");
  }
  return result;
}

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

int main(void) {

  char str[] = "+CWLAP:(3,\"ESP8266-Mesh-2\",-6,\"86:f3:eb:59:c2:ca\",1,25,-120,5,3,3,0)";
  printf("Hello: %s\n", str);
  char* tmp = strtok(str, "CWLAP");
  printf("T: %s\n", tmp);
  tmp = strtok(NULL, ",");
  printf("T: %s\n", tmp);
  char* ssid = strtok(NULL, "\"");
  printf("T: %s\n", ssid);
  printf("SSID: %s\n", starts_with(ssid, strlen(ssid), "ESP8266", 7) == 0 ? "true" : "false");
  char* rssi = strtok(NULL, ",");
  printf("T: %d\n", -atoi(rssi));
  char* mac = strtok(NULL, "\"");
  printf("T: %d\n", parse_mac(mac));
  tmp = strtok(NULL, "CWLAP");
  printf("T: %s\n", tmp);

  char str2[] = "+IPD,0,62:M#F,86:f3:eb:59:c2:ca,2,0,1a:fe:34:a0:75:e1,86:f3:eb:59:c2:ca\n'";
  printf("Hello: %s\n", str2);
  tmp = strtok(str2, "IPD");
  printf("T: %s\n", tmp);
  tmp = strtok(NULL, ",");
  printf("T: %s\n", tmp);
  char* link_id = strtok(NULL, ",");
  printf("T: %d\n", atoi(link_id));
  char* len = strtok(NULL, ":");
  printf("T: %d\n", atoi(len));
  char* msg = strtok(NULL, "\n");
  printf("T: %s\n", msg);

  char str3[] = "+CIPAPMAC_CUR:\"1a:fe:34:a0:75:e1\"\r\n";
  printf("Hello: %s\n", str3);
  tmp = strtok(str3, "CIPAPMAC_CUR");
  printf("T: %s\n", tmp);
  tmp = strtok(NULL, "\"");
  printf("T: %s\n", tmp);
  tmp = strtok(NULL, "\"");
  printf("T: %s\n", tmp);

  printf("Result: %s\n", (!0) ? "true" : "false");

  char str4[] = "CS,4,111";
  // printf("Hello: %s\n", );
  char* next = strchr(str4, ',');
  next[0] = '\0';
  next++;
  // tmp = strtok(str3, "CIPAPMAC_CUR");
  printf("T: %s\n", str4);
  // tmp = strtok(NULL, "\"");
  printf("T: %s\n", next);

  return 0;
}