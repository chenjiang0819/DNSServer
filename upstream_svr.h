#ifndef UPSTREAM_SVR_H
#define UPSTREAM_SVR_H
#include <stddef.h>
int connect_server(const char *server_ip,const char *server_port);
int send_pkt(int sockfd,const unsigned char *buffer,int msg_len);
int read_pkt(int sockfd,unsigned char **buffer,size_t *length);
int socket_timeout(int sockfd);
#endif
