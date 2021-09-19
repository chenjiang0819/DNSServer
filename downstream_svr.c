#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "downstream_svr.h"

#define SERVER_PORT "8053"
#define INITIAL 0
#define QR 4
#define QR_CHANGE 128
#define RCODE 5
#define RCODE_CHANGE 132

/****************************************************************/
// referred the materials gaven in the practial class
// setup and bind a socket on our server
void setup_socket(struct addrinfo hints_myserver, struct addrinfo* res_myserver, int* sockfd){
	int myserver_getaddrinfo_check;
	int enable = 1;
	// Create address we're going to listen on (with given port number)
	memset(&hints_myserver, INITIAL, sizeof(hints_myserver));
	hints_myserver.ai_family = AF_INET;       // Use IPv4
	hints_myserver.ai_socktype = SOCK_STREAM; // Use TCP. If we use the UDP, ai_socktype = SOCK_DGRAM. SOCK_RAW
	hints_myserver.ai_flags = AI_PASSIVE;     // Socket address is intended for `bind'
	myserver_getaddrinfo_check = getaddrinfo(NULL, SERVER_PORT, &hints_myserver, &res_myserver);
	// if the return value of getaddrinfo is not 0, there is error
	if (myserver_getaddrinfo_check != INITIAL){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(myserver_getaddrinfo_check));
		exit(EXIT_FAILURE);
	}
	// Create socket for hearing
	*sockfd = socket(res_myserver->ai_family// protocol  AF_INET AF:adress family 
					, res_myserver->ai_socktype,  // the type of the socket SOCK_STREAM: TCP
					res_myserver->ai_protocol// 0 means the machine will use the AF_INET and socktype to select a correct protocol automatically 
					);
	if (*sockfd < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	// The requirement of Propject 2 2.1-Standard Option
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < INITIAL){
		perror("setsockopt");
		exit(1);
	}
	// Bind address to the socket
	if (bind(*sockfd, //The socket id
		res_myserver->ai_addr, //Ip address
		res_myserver->ai_addrlen //Ip length 
		) < 0){
		perror("bind");
		exit(EXIT_FAILURE);
	}
	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued. The maxium connection is 125
	if (listen(*sockfd, 125) < INITIAL){
		perror("listen");
		exit(EXIT_FAILURE);
	}
}

// change the RA and QR for the non_AAAA request
unsigned char* not_AAAA_rp(unsigned char* PKT){
	PKT[QR] = (PKT[QR] | QR_CHANGE);
	PKT[RCODE] = (PKT[RCODE] | RCODE_CHANGE);
	return PKT;
}

