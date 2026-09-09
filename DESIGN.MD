# DESIGN

## Current Architecture
The project currently consists of a simple HTTP client implemented in C++:
- **`Socket` Class (`socket.h`):** Wraps standard POSIX sockets and integrates OpenSSL to handle both plain TCP and TLS connections. It handles `connect`, `send`, and `receive` operations. SSL initialization is tied directly to the class instantiation.
- **`httprequest` Struct (`httprequest.h`):** A very basic structure representing an HTTP POST request. It provides a `ToString` method to serialize the request content.
- **`main.cpp`:** Contains a demonstration of creating sockets, connecting to Google (which redirects), parsing the response for the `Location` header, and following the redirect to the HTTPS endpoint. It releases the shared `Socket::ssl_ctx` after the socket objects have been destroyed.

## Class Responsibilities
1. **`Socket`:**
   - Resource management for the underlying file descriptor (`sockfd`) and OpenSSL connection (`ssl`).
   - Resolution of hostnames to IP addresses using `getaddrinfo`.
   - Establishing connections (both standard and SSL-wrapped).
   - Reading and writing data to/from the socket.

2. **`httprequest`:**
   - Storing content type, length, and body.
   - Formatting an HTTP POST request string.

## Future Design Goals
1. **Separation of Concerns:**
   - Extract the SSL logic from the base `Socket` class. Consider creating an interface (e.g., `ISocket`) with `TCPSocket` and `TLSSocket` implementations.
   - Decouple the HTTP protocol logic (request formatting, response parsing) from the main application flow. Create an `HttpClient` class.

2. **Robust HTTP Handling:**
   - Implement a proper HTTP response parser to handle status codes, headers (case-insensitively), and chunked transfer encoding.
   - Expand `httprequest` to handle various HTTP methods (GET, POST, PUT, DELETE, etc.) and arbitrary headers.

3. **Configuration and Error Handling:**
   - Introduce custom exceptions or a robust error-code returning mechanism instead of relying solely on `-1` and printing to `std::cerr`.
   - Add configurable connection timeouts and read/write timeouts.

4. **Resource Management:**
   - `Socket` instances own their file descriptors and `SSL*` connections; `main` owns final cleanup of the shared `Socket::ssl_ctx` after all sockets are out of scope.

## Tested lifecycle recovery (2026-09-07)

Socket moves now transfer SSL ownership; reconnect closes previous resources and failed connection attempts clear the descriptor. Offline request, move/destruction, and loopback tests are registered with CTest. TLS peer verification and use of SSL_read/SSL_write remain pending. See WORKFLOW.md.
