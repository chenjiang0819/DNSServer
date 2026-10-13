/* Socket setup retains the original COMP30023 practical-based TCP listener. */
#include "downstream_svr.h"
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
int setup_socket(const char *port,const char *address) {
    struct addrinfo hints={0},*addresses=NULL;int fd=-1,enable=1;
    hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;hints.ai_flags=AI_NUMERICHOST;
    if(getaddrinfo(address,port,&hints,&addresses))return -1;
    for(struct addrinfo *rp=addresses;rp;rp=rp->ai_next) {
        fd=socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);if(fd<0)continue;
        if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable))==0 && bind(fd,rp->ai_addr,rp->ai_addrlen)==0 && listen(fd,128)==0)break;
        close(fd);fd=-1;
    }
    freeaddrinfo(addresses);return fd;
}
unsigned char *not_AAAA_rp(unsigned char *p) {
    p[4]|=0x80;p[5]=(unsigned char)((p[5]&0xf0)|0x84);
    return p;
}
