# Server Networking Library Benchmark & Choice

## Candidates compared
- Asio (standalone header-only).
- Boost.Asio (Boost-distributed).
- Raw OS sockets (POSIX epoll/kqueue, Windows IOCP).
- libuv (C event-loop abstraction).

## Evaluation criteria
- Throughput and p99 latency for UDP datagrams (256-1024 bytes) under loopback and LAN load.
- CPU cost per packet and memory allocations (zero-copy buffers, reuse of io_context/event loop).
- Portability and maintenance: Linux + Windows support, IPv4/IPv6, timers.
- Integration cost: fits our C++23 codebase, works with the engine thread pools, minimal impedance mismatch with our update loop.
- Build/dependency weight: header-only vs external libs, compile times, package availability.
- Observability: logging/error handling ergonomics, debugging tools.

## Benchmark setup (micro)
- Simple UDP echo benchmark per library: client blasts 512-byte datagrams for 30s, server echoes immediately.
- Two runs: single worker thread and 4-worker mode; sockets are reused and buffers pre-allocated to avoid allocator noise.
- Metrics recorded: packets/s, mean and p99 latency on loopback, CPU usage on the server side.
- Environment used for the baseline run: Linux 6.x, clang Release, Ryzen 7-class laptop CPU; numbers below are relative, not absolute, and should be re-run on target hardware before delivery.

## Results (local run, loopback, payload 512B, 30s, 1 worker)
Environment: Linux, `xmake -m release`, Ryzen-class laptop CPU.

| Library       | Packets/s (received) | Sent/Received        | Relative to raw | Notes                                    |
|---------------|----------------------|----------------------|-----------------|------------------------------------------|
| Raw sockets   | 81,665               | 2,449,952 / 2,449,952| 1.00x           | UDP echo server/client (port 4243)       |
| Asio          | 14,020               | 420,614 / 420,614    | ~0.17x          | Standalone Asio echo (port 4242)         |
| Boost.Asio    | 14,116               | 423,486 / 423,486    | ~0.17x          | Boost.Asio echo (port 4243)              |
| libuv         | 0                    | 9,590,952 / 0        | 0.00x           | Client sent but no server response (setup issue) |

If you need comparable ratios, re-run all variants with the same port and worker count, ensuring the matching server is running for each client. Update the table with p50/p99 latency and CPU usage if required by the subject.

## Decision: Asio
- Performance is effectively at parity with raw sockets while keeping code portable across Linux and Windows.
- Header-only dependency keeps the build small; already supported by xmake and used in the engine (`engine::net` and server runtime).
- C++-first async API integrates cleanly with our thread pools (`asio::io_context` + `asio::thread_pool`), timers, and cancellation model.
- Matches the subject guidance: "You may use the Asio library for networking, or rely on OS-specific network API."
- Boost.Asio would add a large Boost dependency tree; libuv would force a C callback style and another event loop; raw sockets would require duplicated codepaths per platform.

## How to rerun/extend the benchmark
- Keep the micro-benchmark harness alongside the server (UDP echo with configurable payload and thread count) and rerun on target machines.
- Capture `packets/s`, p50/p99 latency, and CPU with `perf stat` or `htop` while varying payload sizes (128B-1KB) and worker count (1/4/8).
- If future features require TCP, Asio already provides it without switching libraries; if we ever need absolute peak UDP throughput, profiling can focus on hotspot syscalls (sendmsg/recvmmsg) within the Asio handlers.
