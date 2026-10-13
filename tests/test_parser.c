#include "../svr_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    dns_info info;unsigned seed=2026;
    for(size_t n=0;n<400;n++) {
        unsigned char *packet=malloc(n?n:1);if(!packet)return 2;
        for(int trial=0;trial<100;trial++) {
            for(size_t i=0;i<n;i++){seed=seed*1664525u+1013904223u;packet[i]=(unsigned char)(seed>>24);}
            if(n>=2){packet[0]=(unsigned char)((n-2)>>8);packet[1]=(unsigned char)(n-2);}
            if(n>=8) {packet[6]=0;packet[7]=1;}
            (void)analyse_pkt(packet,n,&info);
        }
        free(packet);
    }
    unsigned char root[]={0,17,0,1,1,0,0,1,0,0,0,0,0,0,0,0,28,0,1};
    if(!analyse_pkt(root,sizeof(root),&info) || strcmp(info.domain,"."))return 1;
    root[14]=0xc0;root[15]=12;if(analyse_pkt(root,sizeof(root),&info))return 1;
    puts("40,000 bounded parser cases, root name and compression-cycle rejection passed");return 0;
}
