#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "upstream_svr.h"

#define LEN_BYTE 2
#define INITIAL 0

/****************************************************************/

// referred the materials gaven in the practial class
// connect the upstream server
void connect_server(struct addrinfo hints, int* sockfd, struct addrinfo* rp, struct addrinfo* servinfo, char* server_ip, char* server_port){
	int getaddrinfo_check;
	// connect to upstream server
	memset(&hints, INITIAL, sizeof(hints));
	hints.ai_family = AF_INET;       // Use IPv4
	hints.ai_socktype = SOCK_STREAM; // Use TCP. If we use the UDP, ai_socktype = SOCK_DGRAM. SOCK_RAW
	getaddrinfo_check = getaddrinfo(server_ip, server_port, &hints, &servinfo);
	if (getaddrinfo_check != INITIAL){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_check));
		exit(EXIT_FAILURE);
	}
	for (rp = servinfo; rp != NULL; rp = rp->ai_next){
		*sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (*sockfd == -1){
			continue;
		}
		if (connect(*sockfd, rp->ai_addr, rp->ai_addrlen) != -1){
			break; // success
		}
	}
	if (rp == NULL){
		fprintf(stderr, "client: failed to connect\n");
		exit(EXIT_FAILURE);
	}
}

// referred the materials gaven in the practial class
// send a pkt to outside 
void send_pkt(int sockfd, unsigned char* buffer, int msg_len){
	int send_check = write(sockfd, buffer, msg_len + LEN_BYTE);
	if (send_check < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
}


