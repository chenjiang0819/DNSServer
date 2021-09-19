#include <stdio.h>

#ifndef SVR_LOG_H
#define SVR_LOG_H

/****************************************************************/

// check the length of a received pkt 
int check_len(unsigned char* PKT);

// check current time
void current_time(char* time_buffer);

// log the reply from upstream server
void rp_log_print(FILE* fp, char *domain, char* address);

// log a normal request
void res_log_print(FILE* fp,  char *domain);

// log a unimplemented request
void unimplemented_print(FILE* fp);

// log a cache operation
void cache_log_print(FILE* fp, char *domain, char *expire_time);

// writing a log
void log_pkt(unsigned char* PKT, FILE* fp, int* AAAA_check, int* rp_check,  int* cacheable_check, int pkt_kind, char* domain, char* address );

// analyse a pkt to get parameters 
void analyse_pkt(unsigned char* PKT, int* AAAA_check, int* rp_check, int* cacheable_check, int pkt_kind, int* TTL, char* domain, char* address, int time_pass);

#endif 