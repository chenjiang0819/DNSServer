/*
 * Written by Chen Jiang for COMP30023 Computer Systems, University of Melbourne,
 * semester 1, May 2021; socket setup refers to the course practical material.
 * 2026 maintenance retains per-connection threads, manual DNS parsing and a
 * five-entry cache, with explicit shared ownership and bounded transport I/O.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include "downstream_svr.h"
#include "upstream_svr.h"
#include "svr_cache.h"
#include "svr_log.h"

typedef struct {
    FILE *fp;
    cache_entry cache[CACHE_NUM];
    pthread_mutex_t mutex;
    pthread_cond_t drained;
    unsigned active;
    const char *upstream_ip,*upstream_port;
} server_state;
struct mypara {int newsockfd;server_state *server;};
static volatile sig_atomic_t stopping=0;
static void stop_server(int signal_number) {
    (void)signal_number;stopping=1;

}
static int valid_port(const char *text) {
    if(!text || !*text)return 0;
    unsigned value=0;
    for(const char *p=text;*p;p++){if(*p<'0'||*p>'9')return 0;value=value*10+(unsigned)(*p-'0');if(value>65535)return 0;}
    return value>0;
}
static int same_question(const unsigned char *q,size_t qn,const dns_info *qi,const unsigned char *r,size_t rn,const dns_info *ri) {
    (void)qn;(void)rn;
    return q[2]==r[2] && q[3]==r[3] && (ri->flags&0x8000) && (qi->flags&0x7800)==(ri->flags&0x7800) && qi->question_end==ri->question_end && !memcmp(q+14,r+14,qi->question_end-14);
}
static void process_req(struct mypara *job) {
    server_state *state=job->server;int fd=job->newsockfd,upstream=-1;
    unsigned char *request=NULL,*reply=NULL;size_t request_length=0,reply_length=0;dns_info query,response;
    if(!socket_timeout(fd) || !read_pkt(fd,&request,&request_length) || !analyse_pkt(request,request_length,&query) || (query.flags&0xf800))goto done;
    pthread_mutex_lock(&state->mutex);res_log_print(state->fp,query.domain);pthread_mutex_unlock(&state->mutex);
    if(query.type!=28 || query.class_code!=1) {
        pthread_mutex_lock(&state->mutex);unimplemented_print(state->fp);pthread_mutex_unlock(&state->mutex);
        send_pkt(fd,not_AAAA_rp(request),check_len(request));goto done;
    }
    pthread_mutex_lock(&state->mutex);reply=check_cache(state->fp,state->cache,request,request_length,&reply_length);pthread_mutex_unlock(&state->mutex);
    if(reply){send_pkt(fd,reply,check_len(reply));goto done;}
    upstream=connect_server(state->upstream_ip,state->upstream_port);
    if(upstream<0 || !send_pkt(upstream,request,check_len(request)) || !read_pkt(upstream,&reply,&reply_length) || !analyse_pkt(reply,reply_length,&response) || !same_question(request,request_length,&query,reply,reply_length,&response))goto done;
    pthread_mutex_lock(&state->mutex);
    if(response.cacheable){push_cache(state->fp,state->cache,request,request_length,reply,reply_length,&response);rp_log_print(state->fp,response.domain,response.address);}
    pthread_mutex_unlock(&state->mutex);
    send_pkt(fd,reply,check_len(reply));
done:
    if(upstream>=0)close(upstream);
    free(request);free(reply);close(fd);
}
static void *API(void *delivery) {
    struct mypara *job=delivery;server_state *state=job->server;process_req(job);free(job);
    pthread_mutex_lock(&state->mutex);state->active--;pthread_cond_signal(&state->drained);pthread_mutex_unlock(&state->mutex);
    return NULL;
}
int main(int argc,char **argv) {
    if((argc!=3 && argc!=4) || !valid_port(argv[2]) || (argc==4 && !valid_port(argv[3]))) {fprintf(stderr,"usage: %s upstream-ip upstream-port [listen-port]\n",argv[0]);return 1;}
    const char *bind_address=getenv("DNS_BIND_ADDRESS");if(!bind_address)bind_address="127.0.0.1";
    server_state state={0};state.upstream_ip=argv[1];state.upstream_port=argv[2];
    if(pthread_mutex_init(&state.mutex,NULL)!=0)return 1;
    if(pthread_cond_init(&state.drained,NULL)!=0){pthread_mutex_destroy(&state.mutex);return 1;}
    state.fp=fopen("dns_svr.log","a");if(!state.fp){perror("log");pthread_cond_destroy(&state.drained);pthread_mutex_destroy(&state.mutex);return 1;}
    int fd=setup_socket(argc==4?argv[3]:"8053",bind_address);
    if(fd<0){perror("listen");fclose(state.fp);pthread_cond_destroy(&state.drained);pthread_mutex_destroy(&state.mutex);return 1;}
    struct sigaction action={0};action.sa_handler=stop_server;sigemptyset(&action.sa_mask);
    sigaction(SIGTERM,&action,NULL);sigaction(SIGINT,&action,NULL);signal(SIGPIPE,SIG_IGN);
    int result=0;
    while(!stopping) {
        struct pollfd ready={fd,POLLIN,0};
        int polled=poll(&ready,1,100);
        if(polled<0){if(errno==EINTR)continue;result=1;break;}
        if(!polled || stopping)continue;
        int client=accept(fd,NULL,NULL);
        if(client<0){if(stopping)break;if(errno==EINTR)continue;perror("accept");result=1;break;}
        struct mypara *job=malloc(sizeof(*job));
        if(!job){close(client);continue;}
        pthread_mutex_lock(&state.mutex);
        if(state.active>=128){pthread_mutex_unlock(&state.mutex);close(client);free(job);continue;}
        state.active++;pthread_mutex_unlock(&state.mutex);job->newsockfd=client;job->server=&state;
        pthread_t tid;
        if(pthread_create(&tid,NULL,API,job)!=0){close(client);free(job);pthread_mutex_lock(&state.mutex);state.active--;pthread_mutex_unlock(&state.mutex);}
        else pthread_detach(tid);
    }
    close(fd);
    pthread_mutex_lock(&state.mutex);
    while(state.active)pthread_cond_wait(&state.drained,&state.mutex);
    free_cache(state.cache);pthread_mutex_unlock(&state.mutex);
    fclose(state.fp);pthread_cond_destroy(&state.drained);pthread_mutex_destroy(&state.mutex);return result;
}
