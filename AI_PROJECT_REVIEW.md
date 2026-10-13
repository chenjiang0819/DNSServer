# AI Project Review

## Project Summary

A handwritten POSIX C coursework TCP DNS proxy with per-connection workers, manual packet parsing, IPv6 AAAA forwarding, a five-entry cache and request/cache logging.

Chen Jiang’s COMP30023 Computer Systems project from May 2021. Socket setup follows the attributed course practical material; the thread-per-request/manual-parser/five-entry-cache architecture is preserved.

## Original Engineering Strengths

The original project decomposes a network service into downstream listener, upstream connection, packet analysis/logging and cache modules. This makes protocol transport, interpretation and stored state separate responsibilities rather than hiding the service behind a DNS framework.

The five-entry cache explicitly stores packet responses and expiry state. Transaction identity, response reuse and TTL handling are visible low-level decisions, providing a concrete systems discussion about mutable shared state and protocol fields.

## Technical Highlights

Each accepted connection receives a heap-owned worker context; one shared initialized mutex governs the cache and log. Network I/O occurs outside that mutex, and worker draining precedes cache/log destruction.

The DNS parser remains handwritten, now checking TCP frame lengths, label/pointer bounds, question structure and resource-record lengths. Cache hits copy a response, replace its transaction ID and age its full 32-bit TTL from the insertion time.

## 2026 Maintenance Improvements

- Corrected worker ownership, shared synchronization and log/socket lifetime; bounded workers and added graceful shutdown.
- Handled segmented TCP prefixes/bodies, EOF, short writes, connect/frame deadlines and failed upstream isolation.
- Validated DNS bounds and correlated upstream transaction/question fields; cache only complete single-AAAA replies.
- Corrected TTL aging and cache response ownership; preserved five-slot expired-entry/FIFO replacement.
- Defaulted the listener to loopback, removed the student-number source comment, and added strict builds, parser and local-network regressions.

The changes retain the project's original mechanisms and strengthen their execution
boundaries and verification contract. They do not substitute a new project concept.

## Verification

- Strict Clang -O0 and -O2 builds passed six loopback integration methods and a 40,000-case bounded parser harness.
- macOS ASan/UBSan and Linux GCC 14 -Werror ASan/UBSan/LeakSanitizer checks passed, including graceful process exit.
- Linux GCC ThreadSanitizer passed all six integration methods without race diagnostics.
- Cases include fragmented packets, malformed/EOF traffic, absolute timeout, NOTIMP, 32-bit TTL/expiry/eviction, isolated bad upstream replies and 240 requests across 32 concurrent client workers.
- Evidence: dns_before_malformed.log, dns_macos_tests.log, dns_O2_tests.log, dns_linux_tests.log, dns_tsan_tests.log.

Verification is limited to the commands and cases listed above; passing these checks is not a claim of universal correctness.

## Skills Demonstrated

POSIX sockets, TCP framing, manual protocol parsing, thread ownership, shared-state locking, cache TTL semantics, resource lifetime, sanitizer and concurrent integration testing.

## Engineering Assessment

This is a concrete low-level networking project with inspectable protocol and cache mechanics. Maintenance gives the preserved architecture explicit ownership and executable concurrency evidence, while keeping its educational scope clear.

## Maintenance Transparency

The original implementation is preserved in `archive/handwritten-original` at
`0b29e5d7912790bc5b6a62b2810c9693b8be4196`. The reviewed maintained version contains a later AI-assisted engineering
maintenance pass focused on verification, robustness, testing, and documentation
while retaining the original project approach.
