CC ?= cc
CFLAGS ?= -std=gnu99 -O2 -Wall -Wextra -Wpedantic
SOURCES = dns_svr.c downstream_svr.c upstream_svr.c svr_cache.c svr_log.c
HEADERS = downstream_svr.h upstream_svr.h svr_cache.h svr_log.h

dns_svr: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -pthread -o $@

test: dns_svr .build/test-parser
	./.build/test-parser
	python3 -B -m unittest discover -s tests -v

.build/test-parser: tests/test_parser.c svr_log.c svr_log.h
	mkdir -p .build
	$(CC) $(CFLAGS) tests/test_parser.c svr_log.c -o $@

sanitize:
	mkdir -p .build
	$(CC) -std=gnu99 -O1 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) -pthread -o .build/dns-sanitize
	$(CC) -std=gnu99 -O1 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_parser.c svr_log.c -o .build/parser-sanitize
	./.build/parser-sanitize
	PROGRAM=$(CURDIR)/.build/dns-sanitize python3 -B -m unittest discover -s tests -v

clean:
	rm -f dns_svr *.o *.gch
.PHONY: test sanitize clean
