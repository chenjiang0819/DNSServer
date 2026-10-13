#ifndef SVR_LOG_H
#define SVR_LOG_H
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
typedef struct {
    char domain[256], address[256];
    uint16_t type, class_code, flags;
    size_t question_end, ttl_offset;
    uint32_t ttl;
    int cacheable;
} dns_info;
int check_len(const unsigned char *packet);
int analyse_pkt(const unsigned char *packet, size_t length, dns_info *info);
void current_time(char *time_buffer);
void rp_log_print(FILE *fp, const char *domain, const char *address);
void res_log_print(FILE *fp, const char *domain);
void unimplemented_print(FILE *fp);
void cache_log_print(FILE *fp, const char *domain, const char *expire_time);
#endif
