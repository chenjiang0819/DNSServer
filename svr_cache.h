#ifndef SVR_CACHE_H
#define SVR_CACHE_H
#include "svr_log.h"
#include <time.h>
#define CACHE_NUM 5
typedef struct {
    unsigned char *request,*response;
    size_t request_length,response_length,ttl_offset;
    time_t inserted;
    uint32_t ttl;
} cache_entry;
/* Caller holds the shared server mutex for these cache operations. */
unsigned char *check_cache(FILE *fp,cache_entry cache[CACHE_NUM],const unsigned char *request,size_t request_length,size_t *response_length);
int push_cache(FILE *fp,cache_entry cache[CACHE_NUM],const unsigned char *request,size_t request_length,const unsigned char *response,size_t response_length,const dns_info *info);
void free_cache(cache_entry cache[CACHE_NUM]);
#endif
