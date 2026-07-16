# TODO

## Bugs

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
