#include "svr_cache.h"
#include <stdlib.h>
#include <string.h>
static void clear(cache_entry *entry) {free(entry->request);free(entry->response);memset(entry,0,sizeof(*entry));}
void free_cache(cache_entry cache[CACHE_NUM]) {for(int i=0;i<CACHE_NUM;i++)clear(&cache[i]);}
static int matches(const cache_entry *entry,const unsigned char *request,size_t length) {
    /* Full request equality apart from transaction ID. Preserves flags and EDNS semantics. */
    return entry->request && entry->request_length==length && !memcmp(entry->request,request,2) && !memcmp(entry->request+4,request+4,length-4);
}
unsigned char *check_cache(FILE *fp,cache_entry cache[CACHE_NUM],const unsigned char *request,size_t length,size_t *response_length) {
    time_t now=time(NULL);
    for(int i=0;i<CACHE_NUM;i++) {
        cache_entry *entry=&cache[i];
        if(!entry->response)continue;
        double elapsed=difftime(now,entry->inserted);
        if(elapsed<0)elapsed=0;
        if(elapsed>=entry->ttl){clear(entry);continue;}
        if(!matches(entry,request,length))continue;
        unsigned char *reply=malloc(entry->response_length);if(!reply)return NULL;
        memcpy(reply,entry->response,entry->response_length);reply[2]=request[2];reply[3]=request[3];
        uint32_t remaining=entry->ttl-(uint32_t)elapsed;size_t pos=entry->ttl_offset;
        reply[pos]=(unsigned char)(remaining>>24);reply[pos+1]=(unsigned char)(remaining>>16);reply[pos+2]=(unsigned char)(remaining>>8);reply[pos+3]=(unsigned char)remaining;
        *response_length=entry->response_length;
        dns_info info;if(analyse_pkt(reply,*response_length,&info)) {
            char expires[80];struct tm tm;time_t when=now+(time_t)remaining;
            if(localtime_r(&when,&tm) && strftime(expires,sizeof(expires),"%FT%T%z",&tm))cache_log_print(fp,info.domain,expires);
            rp_log_print(fp,info.domain,info.address);
        }
        return reply;
    }
    return NULL;
}
int push_cache(FILE *fp,cache_entry cache[CACHE_NUM],const unsigned char *request,size_t request_length,const unsigned char *reply,size_t reply_length,const dns_info *info) {
    cache_entry fresh={0};fresh.request=malloc(request_length);fresh.response=malloc(reply_length);
    if(!fresh.request || !fresh.response){clear(&fresh);return 0;}
    memcpy(fresh.request,request,request_length);memcpy(fresh.response,reply,reply_length);
    fresh.request_length=request_length;fresh.response_length=reply_length;fresh.ttl_offset=info->ttl_offset;fresh.ttl=info->ttl;fresh.inserted=time(NULL);
    for(int i=0;i<CACHE_NUM;i++) {
        if(matches(&cache[i],request,request_length) || !cache[i].response || difftime(fresh.inserted,cache[i].inserted)>=cache[i].ttl) {clear(&cache[i]);cache[i]=fresh;return 1;}
    }
    dns_info old;if(analyse_pkt(cache[0].response,cache[0].response_length,&old)) {
        char stamp[80];current_time(stamp);fprintf(fp,"%s replacing %s by %s\n",stamp,old.domain,info->domain);fflush(fp);
    }
    clear(&cache[0]);memmove(cache,cache+1,(CACHE_NUM-1)*sizeof(*cache));cache[CACHE_NUM-1]=fresh;return 1;
}
