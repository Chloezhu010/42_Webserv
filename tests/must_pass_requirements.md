Critical Requirements (Must Pass)

  1. I/O Multiplexing Architecture

  - Must use poll() (or select/epoll/kqueue) in main loop
  - Must check read AND write fds AT THE SAME TIME (fail = 0)
  - Only ONE read/write per client per poll cycle
  - NEVER read/write file descriptors outside poll (strict FORBIDDEN)
  - NEVER check errno after read/recv/write/send (fail = 0)
  - Must check return values properly (both -1 AND 0, not just one)

  2. Error Handling

  - No segfaults during entire evaluation (fail = 0)
  - No crashes on UNKNOWN/malformed requests
  - Remove client on socket read/write errors
  - No memory leaks (check with valgrind/leaks)

  3. Configuration File Requirements

  - ✅ Multiple servers with different ports
  - ✅ Multiple servers with different hostnames (virtual hosts)
  - ✅ Default error pages (customizable 404, etc.)
  - ✅ Client body size limits
  - ✅ Routes to different directories
  - ✅ Default index files for directories
  - ✅ Method restrictions per route (GET/POST/DELETE permissions)

  4. HTTP Method Support

  - ✅ GET requests - must work
  - ✅ POST requests - must work
  - ✅ DELETE requests - must work
  - ✅ File upload and retrieval
  - ✅ Correct HTTP status codes for ALL responses

  5. Browser Compatibility

  - ✅ Serve fully static websites
  - ✅ Handle wrong URLs
  - ✅ Directory listing
  - ✅ URL redirects

  6. Port Management

  - ✅ Multiple ports with different websites
  - ✅ Prevent duplicate port configurations (should fail)
  - ✅ Handle multiple servers with common ports correctly

  7. Stress Testing (Siege)

  - ✅ >99.5% availability on simple GET requests (siege -b)
  - ✅ No memory leaks during extended stress tests
  - ✅ No hanging connections
  - ✅ Server must run indefinitely without restart

  Bonus Points

  - Cookies and session management
  - Multiple CGI systems (not just one)

  Current Implementation Gaps

  Based on your CLAUDE.md, you still need:

  ❌ Missing Critical Features:

  1. File Upload Storage - Parsing done, need file save logic
  2. Custom Error Pages from Config - Need to integrate with response building
  3. Client Body Size Enforcement - Config exists, validation incomplete
  4. Siege Stress Testing Validation - Need to verify >99.5% availability

  ✅ Implemented Features:

  - ✅ poll() I/O multiplexing
  - ✅ Non-blocking sockets
  - ✅ HTTP request parsing (GET/POST/DELETE)
  - ✅ Config file parsing (multiple servers, locations, methods)
  - ✅ Virtual host routing
  - ✅ Location-based routing
  - ✅ Method restrictions per location
  - ✅ HTTP redirects
  - ✅ Keep-alive connections
  - ✅ Static file serving
  - ✅ Multipart form data parsing

