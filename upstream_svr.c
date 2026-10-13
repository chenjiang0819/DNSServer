/* POSIX socket setup follows the original COMP30023 practical material. */
#include "upstream_svr.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>

int socket_timeout(int fd) {
    struct timeval timeout={2,0};
    return setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0 && setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))==0;
}
int connect_server(const char *ip,const char *port) {
    struct addrinfo hints={0},*addresses=NULL;int fd=-1;
    hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;
    if(getaddrinfo(ip,port,&hints,&addresses))return -1;
    for(struct addrinfo *rp=addresses;rp;rp=rp->ai_next) {
        fd=socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);if(fd<0)continue;
        int flags=fcntl(fd,F_GETFL,0),error=0;socklen_t length=sizeof(error);
        if(flags>=0 && fcntl(fd,F_SETFL,flags|O_NONBLOCK)==0) {
            int result=connect(fd,rp->ai_addr,rp->ai_addrlen);
            if(result==0 || (errno==EINPROGRESS && poll(&(struct pollfd){fd,POLLOUT,0},1,2000)>0 && getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&length)==0 && !error)) {
                if(fcntl(fd,F_SETFL,flags)==0 && socket_timeout(fd))break;
            }
        }
        close(fd);fd=-1;
    }
    freeaddrinfo(addresses);return fd;
}
/* Absolute per-frame deadline; repeated short reads cannot extend it. */
static int await_io(int fd,short events,const struct timespec *start) {
    for(;;) {
        struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
        long elapsed=(now.tv_sec-start->tv_sec)*1000+(now.tv_nsec-start->tv_nsec)/1000000;
        if(elapsed>=2000)return 0;
        struct pollfd ready={fd,events,0};int result=poll(&ready,1,(int)(2000-elapsed));
        if(result<0 && errno==EINTR)continue;
        return result>0;
    }
}
static int exact_read(int fd,unsigned char *buffer,size_t length,const struct timespec *start) {
    size_t done=0;
    while(done<length) {
        if(!await_io(fd,POLLIN,start))return 0;
        ssize_t got=read(fd,buffer+done,length-done);
        if(got<0 && errno==EINTR)continue;
        if(got<=0)return 0;
        done+=(size_t)got;
    }
    return 1;
}
int read_pkt(int fd,unsigned char **buffer,size_t *length) {
    struct timespec start;clock_gettime(CLOCK_MONOTONIC,&start);
    unsigned char prefix[2];*buffer=NULL;*length=0;
    if(!exact_read(fd,prefix,2,&start))return 0;
    size_t n=((size_t)prefix[0]<<8)|prefix[1];
    if(n<12)return 0;
    unsigned char *p=malloc(n+2);if(!p)return 0;
    p[0]=prefix[0];p[1]=prefix[1];
    if(!exact_read(fd,p+2,n,&start)){free(p);return 0;}
    *buffer=p;*length=n+2;return 1;
}
int send_pkt(int fd,const unsigned char *buffer,int msg_len) {
    struct timespec start;clock_gettime(CLOCK_MONOTONIC,&start);
    size_t done=0,length=(size_t)msg_len+2;
    while(done<length) {
        if(!await_io(fd,POLLOUT,&start))return 0;
        ssize_t written=write(fd,buffer+done,length-done);
        if(written<0 && errno==EINTR)continue;
        if(written<=0)return 0;
        done+=(size_t)written;
    }
    return 1;
}
