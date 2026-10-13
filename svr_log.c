#include "svr_log.h"
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

static uint16_t u16(const unsigned char *p) {return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t u32(const unsigned char *p) {return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
int check_len(const unsigned char *packet) {return u16(packet);}

/* Decode a DNS name manually; bounded pointer following rejects cycles.
 * start/end refer to the DNS message, excluding its two-byte TCP prefix. */
static int name(const unsigned char *p,size_t n,size_t start,size_t *end,char *out) {
    size_t pos=start,used=0,hops=0;int jumped=0;
    while(pos<n && hops++<=n) {
        unsigned c=p[pos++];
        if((c&0xc0)==0xc0) {
            if(pos>=n)return 0;
            size_t target=((size_t)(c&0x3f)<<8)|p[pos++];
            if(!jumped)*end=pos;
            jumped=1;pos=target;continue;
        }
        if(c&0xc0 || c>63 || pos+c>n)return 0;
        if(!c) {
            if(!jumped)*end=pos;
            if(out){if(!used)out[used++]='.';out[used]=0;}
            return 1;
        }
        if(used+(used?1:0)+c>253)return 0;
        if(used && out)out[used]='.';
        if(used)used++;
        for(unsigned i=0;i<c;i++) {
            unsigned byte=p[pos++];
            if(out)out[used]=(byte>32 && byte<127 && byte!='\\')?(char)byte:'_';
            used++;
        }
    }
    return 0;
}
int analyse_pkt(const unsigned char *packet,size_t length,dns_info *info) {
    memset(info,0,sizeof(*info));
    if(length<14 || (size_t)check_len(packet)+2!=length)return 0;
    const unsigned char *p=packet+2;size_t n=length-2,end=0;
    info->flags=u16(p+2);
    if(u16(p+4)!=1 || !name(p,n,12,&end,info->domain) || end+4>n)return 0;
    info->type=u16(p+end);info->class_code=u16(p+end+2);end+=4;
    info->question_end=end+2;
    unsigned answers=u16(p+6),authority=u16(p+8),additional=u16(p+10);
    unsigned count=answers+authority+additional;
    for(unsigned i=0;i<count;i++) {
        if(!name(p,n,end,&end,NULL) || end+10>n)return 0;
        unsigned type=u16(p+end),class_code=u16(p+end+2),rdlen=u16(p+end+8);
        uint32_t ttl=u32(p+end+4);
        if(end+10+rdlen>n || (type==28 && rdlen!=16))return 0;
        /* Cache only a complete single-AAAA response: no partial RR TTL aging. */
        if(i==0 && answers==1 && authority==0 && additional==0 && type==28 && class_code==1 && ttl>0 && (info->flags&0x8000) && !(info->flags&0x020f)) {
            info->cacheable=1;info->ttl=ttl;info->ttl_offset=end+6;
            if(!inet_ntop(AF_INET6,p+end+10,info->address,sizeof(info->address)))return 0;
        }
        end+=10+rdlen;
    }
    return end==n;
}
void current_time(char *buffer) {
    time_t now=time(NULL);struct tm info;
    if(!localtime_r(&now,&info) || !strftime(buffer,80,"%FT%T%z",&info))strcpy(buffer,"unknown-time");
}
void rp_log_print(FILE *fp,const char *domain,const char *address) {
    char stamp[80];current_time(stamp);fprintf(fp,"%s %s is at %s\n",stamp,domain,address);fflush(fp);
}
void res_log_print(FILE *fp,const char *domain) {
    char stamp[80];current_time(stamp);fprintf(fp,"%s requested %s\n",stamp,domain);fflush(fp);
}
void unimplemented_print(FILE *fp) {
    char stamp[80];current_time(stamp);fprintf(fp,"%s unimplemented request\n",stamp);fflush(fp);
}
void cache_log_print(FILE *fp,const char *domain,const char *expire) {
    char stamp[80];current_time(stamp);fprintf(fp,"%s %s expires at %s\n",stamp,domain,expire);fflush(fp);
}
