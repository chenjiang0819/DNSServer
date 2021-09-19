#include <stdio.h>
#include <netdb.h>

#ifndef SVR_CACHE_H
#define SVR_CACHE_H

/****************************************************************/

// update and push new tuples into the cache
void push_cache(FILE* fp, unsigned char** server_cache,unsigned char* res_buffer, unsigned char* rp_buffer ,time_t expire_time[5][2]);

// check the existed tuples into the cache
void check_cache(FILE *fp, unsigned char** server_cache, unsigned char* res_buffer, int* cache_check, int newsockfd, time_t expire_time[5][2]);

#endif 