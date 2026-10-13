#ifndef DOWNSTREAM_SVR_H
#define DOWNSTREAM_SVR_H
int setup_socket(const char *port,const char *bind_address);
unsigned char *not_AAAA_rp(unsigned char *packet);
#endif
