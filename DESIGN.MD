# DESIGN

## Current Architecture
The project currently consists of a simple HTTP client implemented in C++:
- **`Socket` Class (`socket.h`):** Wraps standard POSIX sockets and integrates OpenSSL to handle both plain TCP and TLS connections. It handles `connect`, `send`, and `receive` operations. Callers can set a socket send/receive timeout and opt-in TCP connection and TLS handshake timeouts; DNS lookup remains unbounded. SSL initialization is tied directly to the class instantiation.
- **`httprequest` Struct (`httprequest.h`):** Stores a method, body, and caller-supplied request headers. It provides a `ToString` method to serialize an HTTP/1.1 request. The existing two-argument constructor still creates a POST request with content type and length.
- **`main.cpp`:** Contains a demonstration of creating sockets, connecting to Google (which redirects), extracting an absolute `Location` response header, and following the redirect to the HTTPS endpoint. It releases the shared `Socket::ssl_ctx` after the socket objects have been destroyed.
- **`http_response.h`:** Provides the currently narrow, offline-tested `Location` header extractor. It searches only a complete CRLF-delimited header section, recognizes the field name case-insensitively, and permits spaces or tabs after the colon; it is not a complete HTTP response parser.

## Class Responsibilities
1. **`Socket`:**
   - Resource management for the underlying file descriptor (`sockfd`) and OpenSSL connection (`ssl`).
   - Resolution of hostnames to IP addresses using `getaddrinfo`.
   - Establishing connections with an explicit caller-selected TLS mode, rather than inferring TLS from a port string.
   - Callers may opt into TLS peer verification with an expected DNS hostname or IP address and either a PEM CA file or OpenSSL's default CA paths. The per-connection trust store and DNS or IP certificate identity are checked during the handshake; SNI is sent for DNS names. The legacy default remains unverified for API compatibility.
   - Reading and writing data to/from the socket; callers can set the maximum receive length, and a zero length returns an empty result without reading. Connected TLS sockets route application data through OpenSSL. TLS receives support ordinary reads and `MSG_PEEK`; other TLS send/receive flags are rejected because OpenSSL does not provide equivalent semantics.
   - Applying a millisecond I/O timeout to the underlying socket before or after `Connect`; this governs plaintext `Send` and `Receive`. Optional connection and TLS handshake timeouts use nonblocking I/O and poll; they do not bound DNS lookup. `Receive` currently returns an empty string for both timeout and EOF.

2. **`httprequest`:**
   - Storing the request method, arbitrary headers, and body.
   - Formatting an HTTP/1.1 request string, with a default `Connection: close` unless the caller supplies that header in any case spelling. A nonempty body gets a computed `Content-Length` unless the caller supplies length or transfer framing. The serializer owns the single `Host` header and rejects caller-supplied duplicates, empty hosts, and host whitespace. Request methods and header names must be tokens; control characters in header values, host, and request target are rejected to prevent line injection. Caller-supplied framing values are not checked against the body.

## Future Design Goals
1. **Separation of Concerns:**
   - Extract the SSL logic from the base `Socket` class. Consider creating an interface (e.g., `ISocket`) with `TCPSocket` and `TLSSocket` implementations.
   - Decouple the HTTP protocol logic (request formatting, response parsing) from the main application flow. Create an `HttpClient` class.

2. **Robust HTTP Handling:**
   - Implement a proper HTTP response parser to handle status codes, headers (case-insensitively), and chunked transfer encoding.
   - Validate request methods and headers, and derive or check `Content-Length` when the body changes.

3. **Configuration and Error Handling:**
   - Introduce custom exceptions or a robust error-code returning mechanism instead of relying solely on `-1` and printing to `std::cerr`.
   - Add bounded DNS resolution; make timeout errors distinguishable from EOF in `Receive`.

4. **Resource Management:**
   - `Socket` instances own their file descriptors and `SSL*` connections; `main` owns final cleanup of the shared `Socket::ssl_ctx` after all sockets are out of scope.

## Tested lifecycle recovery (2026-09-07)

`Connect` now checks TLS setup and handshake results and closes the attempted descriptor on failure. Offline tests exercise a plaintext peer, opt-in certificate and hostname verification, and encrypted data I/O. These tests do not establish broader production TLS acceptance.

Socket moves now transfer SSL ownership; reconnect closes previous resources and failed connection attempts clear the descriptor. Offline request, move/destruction, loopback, and TLS tests are registered with CTest. See WORKFLOW.md.
