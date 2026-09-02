# TODO

## Bugs
1. ~~**Uninitialized `ssl` pointer in `Socket` class:**~~
   - ~~The `ssl` member variable in `Socket` is not initialized to `nullptr` in the default constructor, leading to a conditional jump depending on uninitialized values in `~Socket()`.~~
   - ~~**Fix:** Initialize `ssl` to `nullptr` in the constructor initializer list or in the class definition.~~

2. **Move constructor does not transfer `ssl`:**
   - The move constructor transfers `sockfd` but neglects `ssl`. This leaves the moved-from object with a potentially dangling `ssl` pointer (if it were initialized) and the moved-to object with a default (likely uninitialized) `ssl`.
   - **Fix:** Transfer the `ssl` pointer in the move constructor and move assignment operator, and set the moved-from object's `ssl` to `nullptr`.

3. **Move assignment operator logic issues:**
   - `sockfd` is correctly transferred and closed if already open, but `ssl` is ignored. If `this->ssl` is not null, it needs to be freed before taking ownership of the `other.ssl`.
   - **Fix:** Add `SSL_free(ssl)` if it exists, take `other.ssl`, and set `other.ssl = nullptr`.

4. **Missing field initializers warning:**
   - The `addrinfo hints` struct initialization in `Connect` does not initialize all fields, which can trigger compiler warnings (`-Wmissing-field-initializers`).
   - **Fix:** Add `, 0` or similar for remaining fields or zero out the structure first using `memset` or `{}` initialization correctly.

5. **Resource Leak on GetAddrInfo failure:**
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
