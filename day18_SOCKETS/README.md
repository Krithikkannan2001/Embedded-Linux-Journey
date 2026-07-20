# Day 18 — Sockets / TCP-IP

## Concept
- Socket = file descriptor representing a network connection, same read()/write()/close() model as device files
- Server flow: socket() -> setsockopt(SO_REUSEADDR) -> bind() -> listen() -> accept() -> read()/write() -> close()
- Client flow: socket() -> connect() -> write()/read() -> close()
- accept() blocks like wait_for_completion() seen in Day 16 ftrace trace - kernel yields CPU while waiting

## Hands-on
- Wrote TCP server in C on Raspberry Pi (day18_SOCKETS/tcp_server.c), listening on port 8080
- Tested initially with telnet from WSL2 (Windows laptop) -> confirmed real network communication,
  not just loopback
- Wrote TCP client in C on WSL2 (Ethernet/Ethernet.c) to replace telnet, full round trip working
- Hit "Address already in use" (TIME_WAIT) after rapid server restarts during testing
- Fixed with SO_REUSEADDR via setsockopt(), standard practice for real servers

## Architecture note
- Client machine = WSL2 (Windows laptop), connects OUT via connect()
- Server machine = Raspberry Pi, listens via bind()/listen()/accept()
- Server needs a fixed known address (bind()); client just needs to know where to connect,
  kernel auto-assigns local port

## Deferred / not covered today
- Ethernet/NIC driver internals (kernel-level packet handling, netdev, skb) - today's work was
  purely application-layer socket programming, one layer above the actual NIC driver
- UDP (SOCK_DGRAM) - only covered TCP (SOCK_STREAM) today
