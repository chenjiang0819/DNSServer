/* 
 * This code is written by Chen Jiang (student ID: 1127411)
 * referred the material gaven in the practial class of COMP30023 Computer System 
 * at the University of Melbourne in semester 1 2021 
 * for Project2 of COMP30023 Computer System
 * from 05 May 2021 at the University of MElbourne
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "downstream_svr.h"
#include "upstream_svr.h"
#include "svr_cache.h"
#include "svr_log.h"

#define NONBLOCKING
#define CACHE
#define LEN_BYTE 2
#define BUFFER 256
#define STOP_POINT 1
#define SERVER_PORT "8053"
#define TRUE 1
#define FALSE 0
#define INITIAL 0
#define RP -1
#define REC 1
#define CACHE_NUM 5

/****************************************************************/
// create a struct to deliver the parameters into an operation function through pthread
struct mypara{
    FILE* fp; 
	int newsockfd;
	unsigned char *req_buffer;
	unsigned char *rep_buffer;
	unsigned char *server_cache[CACHE_NUM]; 
	time_t expire_time[CACHE_NUM][LEN_BYTE];
	char** argv_char;  
	char* filename;
};

// check the length of a pkt before read it 
int check_pkt_len(int newsockfd, unsigned char* pkt_len);
// read a pkt from outside 
void read_pkt(int msg_len, unsigned char* pkt_len, unsigned char* buffer, int sockfd);
// create space for buffer
unsigned char* create_space(int length, unsigned char* buffer);
// create a API for using pthread
void* API(void* delvry);
// pack all process in one function to use the thread via API
void process_req(FILE* fp, int newsockfd, unsigned char *req_buffer,unsigned char *rep_buffer, unsigned char *server_cache[CACHE_NUM], 
	time_t expire_time[CACHE_NUM][LEN_BYTE], char** argv, char* filename);

/****************************************************************/

// main function
int main(int argc, char *argv[]){
	// setting the parameters 
	int sockfd,  newsockfd;
	unsigned char *req_buffer = NULL, *rep_buffer = NULL;
	socklen_t client_addr_size;
	struct addrinfo hints_myserver;
	struct addrinfo *req_myserver = NULL;
	struct sockaddr_storage client_addr;
	struct mypara delvr;


	// initialise the parameters
	// initialise the cache
	memset(delvr.server_cache, INITIAL, sizeof(delvr.server_cache));	
	// initialise the expire_time;
	for(int i = 0; i < CACHE_NUM; i++){
		delvr.expire_time[i][0] = -1;
		delvr.expire_time[i][1] = -1;
	}

	// open the log file under writing and reading mode
	char* filename = "dns_svr.log";
	FILE* fp = fopen(filename, "w");
	if (fp == NULL) {
		fprintf(stderr, "The log file is not opened\n");
		exit(EXIT_FAILURE);
	}

	// to detect the incorrect input
	if (argc < 3) {
		fprintf(stderr, "The input is not correct\n");
		exit(EXIT_FAILURE);
	}

	// setup and bind a socket on our server
	setup_socket(hints_myserver, req_myserver, &sockfd);
	
	// to keep the scerver running
	while(1){
		// referred the materials gaven in the practial class
		// Accept a connection - blocks until a connection is ready to be accepted
		// Get back a new file descriptor to communicate on
		client_addr_size = sizeof(client_addr);
		newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
		if (newsockfd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		// pack all parameters in the struct
		delvr.fp = fp; 
		delvr.filename = filename;
		delvr.newsockfd = newsockfd;
		delvr.req_buffer = req_buffer;
		delvr.rep_buffer = rep_buffer;
		delvr.argv_char = argv; 
		// implement the thread
		pthread_t tid;
		pthread_create(&tid, NULL, API, (void*)&delvr);	
	}
	// free malloced places
	for(int i = 0; i < CACHE_NUM; i++){
		free(delvr.server_cache[i]);
		delvr.server_cache[i] = NULL;
	}
	fclose(fp);
	freeaddrinfo(req_myserver);
	close(sockfd);
	return 0;
}
/****************************************************************/
// create a API for using pthread
void* API(void* delvry){
	struct mypara* delvr = (struct mypara*) delvry;
	process_req(delvr -> fp, delvr -> newsockfd, delvr -> req_buffer, delvr -> rep_buffer, delvr -> server_cache, delvr -> expire_time, 
						delvr -> argv_char, delvr -> filename);
	pthread_exit(0);  
}

// check the length of a pkt before read it 
int check_pkt_len(int newsockfd, unsigned char* pkt_len){
	int msg_len = INITIAL;
	int read_check = read(newsockfd,pkt_len,2);
	if(read_check < 0){
		perror("read");
		exit(EXIT_FAILURE);
	}
	msg_len = check_len(pkt_len);
	return msg_len;
}

// read a pkt from outside 
void read_pkt(int msg_len, unsigned char* pkt_len, unsigned char* buffer, int sockfd){
	int i = 2;
	int read_check;
	buffer[0] = pkt_len[0];
	buffer[1] = pkt_len[1];
	while(msg_len){
		read_check = read(sockfd, buffer + i, 1);
		if(read_check < 0){
			perror("read");
			exit(EXIT_FAILURE);
		}
		msg_len --;
		i++;
	}
}

// create space for buffer
unsigned char* create_space(int length, unsigned char* buffer){
	buffer = (unsigned char*) malloc (length);
	assert(buffer);
	return buffer;
}

// pack all process in one function to use the thread via API
void process_req(FILE* fp, int newsockfd, unsigned char *req_buffer,unsigned char *rep_buffer, unsigned char *server_cache[CACHE_NUM], 
	time_t expire_time[CACHE_NUM][LEN_BYTE], char** argv, char* filename){
	// Initialse the varibles	
	int msg_len = INITIAL, pkt_kind = INITIAL, upstream_sockfd = INITIAL;
	int AAAA_check = FALSE, rp_check = FALSE,  cache_check = FALSE, cacheable_check= FALSE;
	unsigned char pkt_len[LEN_BYTE];
	char domain[BUFFER], address[BUFFER]; 
	struct addrinfo  hints_upstream;
	struct addrinfo  *rp = NULL, *servinfo = NULL;

	// define a spin lock
	pthread_spinlock_t spin;
	// Read characters from the client, then process
	msg_len = check_pkt_len(newsockfd, pkt_len);
	// malloc the space for the buffer
	req_buffer = create_space(msg_len + LEN_BYTE + STOP_POINT, req_buffer);
	// read PKT
	read_pkt(msg_len, pkt_len, req_buffer, newsockfd);

	// record the receiving activities
	pkt_kind = REC;
	log_pkt(req_buffer, fp, &AAAA_check, &rp_check, &cacheable_check, pkt_kind, domain, address);
	if(AAAA_check == FALSE){
		rep_buffer = not_AAAA_rp(req_buffer);
		// send pkt back to client
		send_pkt(newsockfd, rep_buffer, msg_len); 
	}else{
		// check_cache and send response
		check_cache(fp,server_cache, req_buffer, &cache_check, newsockfd, expire_time);		
		if(cache_check == FALSE){	
			res_log_print(fp, domain);												
			// if the request is not in the cache
			// connect the upstream server
			connect_server(hints_upstream, &upstream_sockfd, rp, servinfo, argv[1], argv[2]);
			// Send message to upstream server
			send_pkt(upstream_sockfd, req_buffer, msg_len);
			// receive the response from upstream server
			msg_len = check_pkt_len(upstream_sockfd, pkt_len);
			// malloc the space for the buffer
			rep_buffer = create_space(msg_len + LEN_BYTE + STOP_POINT, rep_buffer);
			assert(rep_buffer);
			// read PKT from the upstream
			read_pkt(msg_len, pkt_len, rep_buffer, upstream_sockfd);
			// record the sending activities 
			pkt_kind =RP;					
			log_pkt(rep_buffer, fp, &AAAA_check, &rp_check, &cacheable_check, pkt_kind, domain, address);
			// send pkt back to client			
			msg_len = check_len(rep_buffer);
			send_pkt(newsockfd, rep_buffer, msg_len); 
			// push the last AAAA response into the cache 								
			if(cacheable_check == TRUE){
				pthread_spin_trylock(&spin);			
				push_cache(fp, server_cache, req_buffer, rep_buffer, expire_time);
				pthread_spin_unlock(&spin);	
				if(rp_check == TRUE){
					rp_log_print(fp, domain, address);
				}
			}else{ // if it is not cacheable, free malloced rp places
				free(rep_buffer);
				rep_buffer = NULL;
			}
			// print the log 
			close(upstream_sockfd);
		}
	}
	// free malloced receive places
	free(req_buffer);
	req_buffer = NULL;
	freeaddrinfo(servinfo);
	fclose(fp);
	close(newsockfd);	
	AAAA_check = FALSE; 
	rp_check = FALSE;
	cache_check = FALSE;
	cacheable_check = FALSE;
	msg_len = INITIAL;
	fp = fopen(filename, "a");
	pthread_spin_destroy(&spin);	
}



















