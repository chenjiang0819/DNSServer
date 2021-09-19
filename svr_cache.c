#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "svr_cache.h"
#include "upstream_svr.h"
#include "svr_log.h"

#define LEN_BYTE 2
#define TIME_SIZE 80
#define BUFFER 256
#define TRUE 1
#define FALSE 0
#define INITIAL 0
#define RP -1
#define REC 1
#define CACHE_NUM 5
#define PKT_ID_1 2
#define PKT_ID_2 3

/****************************************************************/

// update and push new tuples into the cache
void push_cache(FILE* fp, unsigned char** server_cache,unsigned char* res_buffer, unsigned char* rp_buffer ,time_t expire_time[CACHE_NUM][LEN_BYTE]){
	int TTL_new = INITIAL, TTL_eviction = INITIAL;
	int AAAA_check = FALSE, rp_check = FALSE, cacheable_check = FALSE, expired_exist = FALSE;
	int pkt_kind = RP;
	char address[BUFFER];
	char new_domain[BUFFER];
	char eviction_domain[BUFFER];
	// get the current time
	time_t current;
	char time_buffer[TIME_SIZE];
	current_time(time_buffer);
	time(&current);
	// analyse the response
	analyse_pkt(rp_buffer, &AAAA_check, &rp_check, &cacheable_check, pkt_kind, &TTL_new, new_domain, address, INITIAL);
	for(int i = 0; i < CACHE_NUM; i++){
		if(current >= expire_time[i][1] && expire_time[i][1] >= INITIAL){		
			analyse_pkt(server_cache[i], &AAAA_check, &rp_check, &cacheable_check, pkt_kind, &TTL_eviction, eviction_domain, address, INITIAL);                                                                          
			// record the new tuple
			free(server_cache[i]);
			server_cache[i] = NULL;
			server_cache[i] = rp_buffer;
			expire_time[i][0] = current;
			expire_time[i][1] = current + TTL_new;
			expired_exist = TRUE;
			// write the records into the log
			fprintf(fp,"%s replacing %s by %s\n",time_buffer, eviction_domain, new_domain);
			fflush(fp);
			break;
		}
	}
	if(expired_exist == FALSE){		
		if(server_cache[INITIAL] != NULL){
			// anaylse the eviction record
			analyse_pkt(server_cache[INITIAL], &AAAA_check, &rp_check, &cacheable_check, pkt_kind, &TTL_eviction, eviction_domain, address, INITIAL);                                                                          
			// write the records into the log
			fprintf(fp,"%s replacing %s by %s\n",time_buffer, eviction_domain, new_domain);
			fflush(fp);
			free(server_cache[INITIAL]);
			server_cache[INITIAL] = NULL;
		}
		// insert the response
		server_cache[0] = server_cache[1];
		server_cache[1] = server_cache[2];
		server_cache[2] = server_cache[3];
		server_cache[3] = server_cache[4];
		server_cache[4] = rp_buffer;
		// insert the response time
		expire_time[0][0] = expire_time[1][0];
		expire_time[0][1] = expire_time[1][1];
		expire_time[1][0] = expire_time[2][0];
		expire_time[1][1] = expire_time[2][1];
		expire_time[2][0] = expire_time[3][0];
		expire_time[2][1] = expire_time[3][1];
		expire_time[3][0] = expire_time[4][0];
		expire_time[3][1] = expire_time[4][1];
		expire_time[4][0] = INITIAL;
		expire_time[4][0] = current;
		expire_time[4][1] = INITIAL;
		expire_time[4][1] = current + TTL_new; 
	}
}

// check the existed tuples into the cache
void check_cache(FILE *fp, unsigned char** server_cache, unsigned char* res_buffer, int* cache_check, int newsockfd, time_t expire_time[CACHE_NUM][LEN_BYTE]){
	int TTL = INITIAL, time_diff = INITIAL;
	int pkt_kind;
	int AAAA_check = FALSE, rp_check = FALSE, cacheable_check = FALSE;
	time_t current;
	char expire_time_in_char [TIME_SIZE];
	char cache_domain[BUFFER];
	char address[BUFFER];
	char new_domain[BUFFER];
	int msg_len = INITIAL;
	// gather the information from lient
	pkt_kind = REC;
	analyse_pkt(res_buffer, &AAAA_check, &rp_check, &cacheable_check, pkt_kind, &TTL, new_domain, address, INITIAL);
	// check the cache
	pkt_kind = RP;
	for(int i = 0; i < CACHE_NUM; i++){
		if(server_cache[i] != NULL){
			rp_check = FALSE;
			*cache_check = FALSE;
			time(&current);
			time_diff = current - expire_time[i][0];			
			analyse_pkt(server_cache[i], &AAAA_check, &rp_check, &cacheable_check, pkt_kind, &TTL, cache_domain, address, time_diff);	
			if(strcmp(new_domain, cache_domain) == 0  && TTL > INITIAL && (expire_time[i][1] - current) >= INITIAL){				
				*cache_check = TRUE;
				res_log_print(fp, new_domain);
				// covert the expire time to required format
				struct tm *info = localtime(&expire_time[i][1]); 
				strftime(expire_time_in_char, TIME_SIZE, "%FT%T%z", info);
				cache_log_print(fp, new_domain, expire_time_in_char); 
				// get the length of the response
				msg_len = server_cache[i][0] << 8;
				msg_len = msg_len + server_cache[i][1];
				// change the ID of the record 
				server_cache[i][PKT_ID_1] = res_buffer[PKT_ID_1];
				server_cache[i][PKT_ID_2] = res_buffer[PKT_ID_2];
				// send response back to the client
				msg_len = check_len(server_cache[i]);
				send_pkt(newsockfd, server_cache[i], msg_len); 
				// record the activity
				if(rp_check == TRUE){
					rp_log_print(fp, new_domain, address);
				}
				break;
			}
		}
	}
}