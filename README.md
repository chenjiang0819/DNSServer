# TCP AAAA DNS Proxy with Cache

## Overview

A handwritten POSIX C coursework TCP DNS proxy with per-connection workers, manual packet parsing, IPv6 AAAA forwarding, a five-entry cache and request/cache logging.

## Original Context

Chen Jiang’s COMP30023 Computer Systems project from May 2021. Socket setup follows the attributed course practical material; the thread-per-request/manual-parser/five-entry-cache architecture is preserved. The preserved pre-maintenance snapshot is `0b29e5d7912790bc5b6a62b2810c9693b8be4196`;
its most recent original commit is dated 2022-01-14. This date describes Git evidence,
not necessarily the complete implementation period.

## What I Implemented

Length-prefixed TCP request/response handling, DNS question/answer decoding, upstream forwarding, non-AAAA responses, cache eviction/expiry and timestamped logs.

## Technical Approach

Each accepted connection receives a heap-owned worker context; one shared initialized mutex governs the cache and log. Network I/O occurs outside that mutex, and worker draining precedes cache/log destruction.

The DNS parser remains handwritten, now checking TCP frame lengths, label/pointer bounds, question structure and resource-record lengths. Cache hits copy a response, replace its transaction ID and age its full 32-bit TTL from the insertion time.

## Build / Run

Run `make`, then `./dns_svr 127.0.0.1 UPSTREAM_PORT [LISTEN_PORT]` with a local test DNS peer. Default listen port is 8053 and bind address is 127.0.0.1; `DNS_BIND_ADDRESS` explicitly overrides the bind address. Run `make test` and `make sanitize`; tests use only loopback peers and Python standard library. Linux is the original intended runtime; macOS builds were also verified.

## Testing

- Strict Clang -O0 and -O2 builds passed six loopback integration methods and a 40,000-case bounded parser harness.
- macOS ASan/UBSan and Linux GCC 14 -Werror ASan/UBSan/LeakSanitizer checks passed, including graceful process exit.
- Linux GCC ThreadSanitizer passed all six integration methods without race diagnostics.
- Cases include fragmented packets, malformed/EOF traffic, absolute timeout, NOTIMP, 32-bit TTL/expiry/eviction, isolated bad upstream replies and 240 requests across 32 concurrent client workers.
- Evidence: dns_before_malformed.log, dns_macos_tests.log, dns_O2_tests.log, dns_linux_tests.log, dns_tsan_tests.log.

## 2026 Maintenance

- Corrected worker ownership, shared synchronization and log/socket lifetime; bounded workers and added graceful shutdown.
- Handled segmented TCP prefixes/bodies, EOF, short writes, connect/frame deadlines and failed upstream isolation.
- Validated DNS bounds and correlated upstream transaction/question fields; cache only complete single-AAAA replies.
- Corrected TTL aging and cache response ownership; preserved five-slot expired-entry/FIFO replacement.
- Defaulted the listener to loopback, removed the student-number source comment, and added strict builds, parser and local-network regressions.

The later work is AI-assisted maintenance, separate from the original implementation.
The exact original snapshot remains on `archive/handwritten-original`.
See [AI_PROJECT_REVIEW.md](AI_PROJECT_REVIEW.md) for a technical review.

## Known Limitations

- Educational TCP-only service: one question/connection, standard QUERY for IN AAAA; other types/classes receive NOTIMP. No UDP, DNSSEC validation, public-resolver authentication or general DNS-service certification.
- Only complete single-AAAA replies are cached; other valid responses are forwarded without cache insertion.
- Malformed/unmatched/failed upstream exchanges close the client connection rather than synthesize every standard DNS error response.
- At most 128 active workers; connect and frame I/O have two-second deadlines. Configured hostname resolution, if used instead of numeric IPv4, is not covered by the frame deadline.
- No external DNS provider or public network deployment was tested.

## Original Assignment Notes

# DNSServer

This program (makefile produces an executable named ’dns_svr’.) accepts DNS “AAAA” query over TCP on port 8053. Forward it to a server whose IPv4 address is the first command-line argument and whose port is the second command-line argument. Send the response back to the client who sent the request, over the same TCP connection. There will be a separate TCP connection for each query/response with the client. 
  
Input example:  
Server: ./dns_svr (uper server address) (upper server port)  
Client: dig +tcp a(dns server address) -p(dns server port) AAAA google.com  
  
Log will keep a log at ./dns_svr.log (i.e., in current directory) with messages of the following format:  
- (timestamp) requested (domain_name) – when dns server receive a request  
- (timestamp) unimplemented request – if the request type is not AAAA  
- (timestamp) (domain_name) expires at (timestamp) – for each request the dns server receive that is in the cache  
- (timestamp) replacing (domain_name) by (domain_name) – for each cache eviction  
- (timestamp) (domain_name) is at (IP address)  
  
Log example:  
2021-05-19T22:24:06+1000 requested www.example.com  
2021-05-19T22:24:06+1000 www.example.com is at 2606:2800:220:1:248:1893:25c8:1946  
2021-05-19T22:24:06+1000 requested www.google.com  
2021-05-19T22:24:06+1000 replacing www.example.com by www.google.com  
2021-05-19T22:24:06+1000 www.google.com is at 2404:6800:4015:800::2004  
  
The server will not be shut down by itself. SIGINT (like CTRL-C) needs to be used to terminate the server.
