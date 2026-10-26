# High-Performance Multi-Threaded TCP Port Scanner

A fast, concurrent network reconnaissance tool engineered in **C++17** utilizing low-level **POSIX sockets**, non-blocking asynchronous I/O (`fcntl`, `select`), and a dynamic worker thread-pool architecture.

---

## Technical Highlights & Architecture

* **Concurrency & Resource Management:** Implemented an asynchronous work-queue distribution model utilizing `std::thread`, `std::mutex`, and `std::atomic` to dynamically feed worker threads without idle spinlock cycles or race conditions.
* **Non-Blocking Network Sockets:** Circumvents blocking kernel socket limits by setting file descriptor flags to `O_NONBLOCK` via POSIX `fcntl()`.
* **Multiplexed Connection Polling:** Employs `select()` with microsecond-level precision (`timeval`) to poll TCP handshake readiness within strict customizable timeout windows, eliminating long OS-level SYN retransmission timeouts.
* **Network Stack Interaction:** Directly queries socket error states using `getsockopt(..., SOL_SOCKET, SO_ERROR, ...)` to confirm 3-way handshake completion (`SYN` -> `SYN-ACK` -> `ACK`) against filtered and closed states (`RST`).
* **Clean DNS & Target Resolution:** Utilizes POSIX `getaddrinfo` and `inet_ntop` for IPv4 hostname-to-address translation.

---

## Project Structure

```text
port_scanner/
├── include/
│   └── PortScanner.hpp   # Class declaration, thread primitives, and state
├── src/
│   ├── PortScanner.cpp   # POSIX socket logic, I/O multiplexing, worker loop
│   └── main.cpp          # CLI argument parsing, boundary validation, execution
├── Makefile              # Modular build system with C++17 & pthread flags
└── README.md
