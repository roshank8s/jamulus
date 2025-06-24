# P2P Mode

Jamulus normally relies on a central server which mixes all audio streams. Since version 3.10, an experimental peer‑to‑peer (P2P) mode is also available. When enabled, clients exchange their network addresses via the server and then send audio directly to each other.

There are two ways to use this feature:

* **Enable P2P Mode** – activates the client’s ability to send and receive audio packets directly from other connected clients while still communicating with the server.
* **Pure P2P Mode** – disables the mixed stream from the server so that only audio from peers is used.

P2P Mode works best with a small number of clients and may require proper firewall and NAT traversal (STUN/TURN) support. If a direct connection cannot be established, the server mix will continue to be used.

