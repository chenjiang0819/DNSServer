#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>

#include "svr_log.h"

#define LEN_BYTE 2
#define TIME_SIZE 80
#define BUFFER 256
#define STOP_POINT 1
#define TRUE 1
#define FALSE 0
#define INITIAL 0
#define RP -1
#define REC 1
#define DOMAIN_START 14
#define AAAA 28

/****************************************************************/

// check the length of a received pkt 
int check_len(unsigned char* PKT){
	int len = INITIAL;
	len = PKT[0] << 8;
	len = len + PKT[1];
	return len;
}

// check current time
void current_time(char* time_buffer){
	time_t rawtime;
	struct tm *info;
	time(&rawtime);
	info = localtime(&rawtime);
	strftime(time_buffer, TIME_SIZE, "%FT%T%z", info);
}

// log the reply from upstream server
void rp_log_print(FILE* fp, char *domain, char* address){
	//stamp the time at the begining of a record
	char time_buffer[TIME_SIZE];
	current_time(time_buffer);
	//write the records into the log
	fprintf(fp,"%s %s is at %s\n",time_buffer, domain, address);
	fflush(fp);
}

// log a normal request
void res_log_print(FILE* fp,  char *domain){
	//stamp the time at the begining of a record
	char time_buffer[TIME_SIZE];
	current_time(time_buffer);
	//write the records into the log
	fprintf(fp,"%s requested %s\n", time_buffer, domain);
	fflush(fp);
}

// log a unimplemented request
void unimplemented_print(FILE* fp){
	//stamp the time at the begining of a record
	char time_buffer[TIME_SIZE];
	current_time(time_buffer);
	fprintf(fp,"%s unimplemented request\n",time_buffer);
	fflush(fp);
}

// log a cache operation
void cache_log_print(FILE* fp, char *domain, char *expire_time){
	//stamp the time at the begining of a record
	char time_buffer[TIME_SIZE];
	current_time(time_buffer);
	//log the record
	fprintf(fp,"%s %s expires at %s\n",time_buffer, domain, expire_time);
	fflush(fp);
}

// writing a log
void log_pkt(unsigned char* PKT, FILE* fp, int* AAAA_check, int* rp_check,  int* cacheable_check, int pkt_kind, char* domain, char* address){
	int TTL = INITIAL;
	int take_space;
	*cacheable_check = FALSE;
	// print out the records into the log
	if(pkt_kind == REC){
		analyse_pkt(PKT, AAAA_check, rp_check, cacheable_check, pkt_kind, &TTL, domain, address, INITIAL);
		if(*AAAA_check == FALSE){
			res_log_print(fp, domain);
			unimplemented_print(fp);
		}
	}else{
		analyse_pkt(PKT, &take_space, rp_check, cacheable_check, pkt_kind, &TTL, domain, address, INITIAL);		
	}
	
}

// analyse a pkt to get parameters 
void analyse_pkt(unsigned char* PKT, int* AAAA_check, int* rp_check, int* cacheable_check, int pkt_kind, int* TTL, char* domain, char* address, int time_pass){
	int start_point = DOMAIN_START;
	int section_len = PKT[DOMAIN_START];
	int current = INITIAL;
	int address_len = INITIAL;
	int i = INITIAL;
	// translate the PKT to readable char
	while(section_len != 0){
		section_len = PKT[start_point];
		for(current = 0; current < section_len; current++){
			domain[i++] = PKT[start_point + current + STOP_POINT];
		}
		domain[i++] = '.';
		start_point = start_point + section_len + STOP_POINT;
	}
	domain[i - LEN_BYTE] = '\0';
	// check if the PKT is the AAAA
	if(PKT[start_point] == 0 && PKT[start_point + STOP_POINT] == AAAA ){
		*AAAA_check = TRUE;
	}
	if(pkt_kind == RP){
		// check if  response is AAAA
		start_point = start_point + 6;	
		if(PKT[start_point] == 0 && PKT[start_point + STOP_POINT] == AAAA){
			*rp_check = TRUE;		
		}	
		start_point = start_point + 4;
		// reduce the TTL
		PKT[start_point + 3] = PKT[start_point + 3] - time_pass;
		// get the TTL
		*TTL = INITIAL;
		*TTL = PKT[start_point] << 24;
		*TTL = *TTL + (PKT[start_point + 1] << 16);
		*TTL = *TTL + (PKT[start_point + 2] << 8);
		*TTL = *TTL + PKT[start_point + 3];
		// check if  response is AAAA and non NULL		
		start_point = start_point + 4;			
		address_len = PKT[start_point] << 8;
		address_len = address_len + PKT[++start_point];
		if(address_len > 0){
			*cacheable_check = TRUE;
			start_point ++;
			// record Ipv6 address	
			inet_ntop(AF_INET6, PKT + start_point, address, BUFFER);
		}
	}
}
