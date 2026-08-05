# TODO

## Bugs
1. **Resource Leak on GetAddrInfo failure:**
   - If `getaddrinfo` fails, an error is printed, but if it somehow left allocated data in `res` (rare if it fails, but good practice), it's not freed. More importantly, memory allocated by `SSL_new` is correctly handled, but `ssl_ctx` is never cleaned up upon program exit (though it's a static pointer, so it's a one-time leak).

## Architectural / Design Issues
1. **Hardcoded logic:**
   - Hardcoded protocol string comparison (`if (service == "443")`) instead of a robust mechanism to determine whether to use SSL or not based on configuration or URL schema.
   - Buffer size for `Receive` is fixed at 4096.

2. **HTTP parsing:**
   - Finding headers like `Location: ` uses raw string parsing (`std::string::find`) which is error-prone (e.g. if case differs, or whitespace differs).

3. **Single responsibility principle:**
   - The `Socket` class is doing too much: it's handling both low-level socket operations AND SSL connection logic.
   - It would be better to abstract the SSL layer or create a specialized `SSLSocket` class.

4. **Missing features:**
   - No way to specify a timeout for `Connect`, `Send`, or `Receive`.
   - The `httprequest` struct is minimal and doesn't handle headers robustly.
