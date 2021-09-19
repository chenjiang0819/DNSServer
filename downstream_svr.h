#include <netdb.h>

#ifndef DOWNSTREAM_SVR_H
#define DOWNSTREAM_SVR_H

/****************************************************************/
// referred the materials gaven in the practial class
// setup and bind a socket on our server
void setup_socket(struct addrinfo hints_myserver, struct addrinfo* res_myserver, int* sockfd);

// change the RA and QR for the non_AAAA request
unsigned char* not_AAAA_rp(unsigned char* PKT);

#endif 