
# Day 19 — Wireshark & iptables

## Wireshark

- Installed on Windows (Pi's apt install was too slow over WiFi)

- Captured live traffic between own tcp_client (WSL2) and tcp_server (Pi) on port 8080

- Filter used: tcp.port == 8080

- Observed full TCP lifecycle on real captured packets:

  - 3-way handshake: SYN -> SYN,ACK -> ACK

  - Saw a real SYN retransmission (server not ready yet) - confirmed TCP's built-in reliability

  - PSH,ACK packets carrying actual application data (Len matched exact message byte counts)

  - 4-way FIN teardown on connection close

- Reviewed header fields: Seq/Ack (byte-level tracking), Win (flow control),

  TSval/TSecr (RTT measurement), MSS (max segment size, negotiated only at handshake),

  SACK_PERM (selective ack), WS (window scaling)

## iptables

- Installed via apt (not present by default on this Debian Trixie-based image;

  iptables is now a compat layer over nftables)

- Tested INPUT chain rules against own WSL2 client IP on port 8080:

  - DROP: client hangs silently ~1-2 min before "Connection timed out". Confirmed via

    rule packet/byte counters incrementing (11 packets, 660 bytes caught)

  - REJECT --reject-with tcp-reset: client fails almost instantly with

    "Connection refused" - active RST sent back

- Key takeaway: "connection refused" (fast) = something actively rejected you;

  "connection timed out" (slow) = packets silently dropped somewhere - real

  debugging signal for diagnosing network issues

- Cleaned up all test rules afterward (iptables -D), confirmed clean INPUT chain

## Relevance

- Same client/server pair from Day 18 sockets, now inspected at the packet level

  and tested against firewall rules - ties directly back to that day's work

