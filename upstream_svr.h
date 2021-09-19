#include <netdb.h>

#ifndef UPSTREAM_SVR_H
#define UPSTREAM_SVR_H

/****************************************************************/

// referred the materials gaven in the practial class
// connect the upstream server
void connect_server(struct addrinfo hints, int* sockfd, struct addrinfo* rp, struct addrinfo* servinfo, char* server_ip, char* server_port);

// referred the materials gaven in the practial class
// send a pkt to outside 
void send_pkt(int sockfd, unsigned char* buffer, int msg_len);

#endif 