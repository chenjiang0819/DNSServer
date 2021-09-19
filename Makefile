dns_svr: dns_svr.o downstream_svr.o upstream_svr.o svr_cache.o svr_log.o
	gcc -o dns_svr dns_svr.o downstream_svr.o upstream_svr.o svr_cache.o svr_log.o -Wall -lm -g -lpthread

dns_svr.o: dns_svr.c downstream_svr.h upstream_svr.h downstream_svr.h svr_cache.h svr_log.h
	gcc -c  dns_svr.c downstream_svr.h upstream_svr.h downstream_svr.h svr_log.h -Wall -lm -g -lpthread

downstream_svr.o: downstream_svr.c downstream_svr.h svr_log.h
	gcc -c  downstream_svr.c downstream_svr.h svr_log.h -Wall -lm -g -lpthread

upstream_svr.o: upstream_svr.c upstream_svr.h svr_log.h
	gcc -c  upstream_svr.c upstream_svr.h -Wall svr_log.h -lm -g -lpthread

svr_cache.o: svr_cache.c svr_cache.h svr_log.h
	gcc -c  svr_cache.c svr_cache.h svr_log.h -Wall -lm -g -lpthread

svr_log.o: svr_log.c svr_log.h
	gcc -c  svr_log.c svr_log.h -Wall -lm -g -lpthread

clean:
	rm -f dns_svr *.o *.gch





