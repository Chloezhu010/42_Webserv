# Webserv Evaluation Checklist

Based on the 42 Evaluation Sheet - Use this during peer evaluation

## Pre-Evaluation Setup

- [ ] Clone repository to fresh directory
- [ ] Check for malicious aliases or scripts
- [ ] Verify `make` compiles without errors
- [ ] Check for memory leaks: `make && valgrind ./webserv config/default.conf`
- [ ] Start server: `./webserv config/default.conf`

---

## 1. Code Review & Architecture (CRITICAL - CAN FAIL TO 0)

### I/O Multiplexing (MANDATORY)
- [ ] **Uses `poll()` (or select/epoll/kqueue) in main loop**
- [ ] **Checks read AND write fds AT THE SAME TIME** ⚠️ If not → **FAIL (0 points)**
- [ ] **Only ONE read/write per client per poll cycle**
- [ ] **NEVER reads/writes fd outside poll** ⚠️ If yes → **FAIL (0 points)**
- [ ] **NEVER checks `errno` after read/recv/write/send** ⚠️ If yes → **FAIL (0 points)**
- [ ] **Checks return values properly (both -1 AND 0)**

**Code Locations to Check:**
```bash
# Find poll() usage
grep -n "poll(" src/**/*.cpp

# Check read/write error handling
grep -n "read\|recv\|write\|send" src/**/*.cpp

# Verify no errno checks after socket operations
grep -A3 "read\|recv\|write\|send" src/**/*.cpp | grep errno
```

### Error Handling (MANDATORY)
- [ ] **No segfaults during evaluation** ⚠️ If segfault → **FAIL (0 points)**
- [ ] **No crashes on malformed requests**
- [ ] **Removes client on socket errors**
- [ ] **No memory leaks** (checked with valgrind/leaks)

**Test Commands:**
```bash
# Malformed request test
printf "GARBAGE\r\n\r\n" | nc localhost 8080

# Memory leak check
leaks webserv
# or
valgrind --leak-check=full ./webserv config/default.conf
```

---

## 2. Configuration File Tests

### Required Configuration Features
- [ ] **Multiple servers with different ports**
  ```bash
  # Test: Check config has multiple server blocks with different ports
  grep "listen" config/default.conf

  # Test different ports serve different content
  curl http://localhost:8080/
  curl http://localhost:8081/
  ```

- [ ] **Virtual hosts (different hostnames)**
  ```bash
  curl --resolve example.com:80:127.0.0.1 http://example.com/
  curl -H "Host: example.com" http://localhost:8080/
  ```

- [ ] **Custom error pages (404, etc.)**
  ```bash
  curl -i http://localhost:8080/nonexistent
  # Should show custom 404 page if configured
  ```

- [ ] **Client body size limits**
  ```bash
  # Send request larger than limit
  curl -X POST -H "Content-Type: text/plain" \
    --data "$(python3 -c 'print("A"*11000000)')" \
    http://localhost:8080/
  # Should return 413 Payload Too Large
  ```

- [ ] **Routes to different directories**
  ```bash
  curl http://localhost:8080/images/
  curl http://localhost:8080/admin/
  ```

- [ ] **Default index files for directories**
  ```bash
  curl http://localhost:8080/
  # Should serve index.html or configured default
  ```

- [ ] **Method restrictions per route**
  ```bash
  # If /admin only allows GET:
  curl -X POST http://localhost:8080/admin/
  # Should return 405 Method Not Allowed
  ```

---

## 3. Basic HTTP Method Tests

### GET Requests
- [ ] **GET / returns 200 OK**
  ```bash
  curl -i http://localhost:8080/
  ```

- [ ] **GET static file works**
  ```bash
  curl -i http://localhost:8080/index.html
  ```

- [ ] **GET non-existent returns 404**
  ```bash
  curl -i http://localhost:8080/nonexistent.html
  ```

### POST Requests
- [ ] **POST with body works**
  ```bash
  curl -i -X POST -d "test data" http://localhost:8080/
  ```

- [ ] **POST without Content-Length returns 411**
  ```bash
  printf "POST / HTTP/1.1\r\nHost: localhost\r\n\r\ntest" | nc localhost 8080
  # Should return 411 Length Required
  ```

- [ ] **POST exceeding body limit returns 413**
  ```bash
  curl -X POST --data "$(python3 -c 'print("A"*11000000)')" http://localhost:8080/
  # Should return 413 Payload Too Large
  ```

### DELETE Requests
- [ ] **DELETE existing file returns 200**
  ```bash
  echo "test" > www/test_delete.txt
  curl -i -X DELETE http://localhost:8080/test_delete.txt
  ```

- [ ] **DELETE non-existent returns 404**
  ```bash
  curl -i -X DELETE http://localhost:8080/nonexistent.txt
  ```

### Unknown Methods
- [ ] **PATCH returns 405 Method Not Allowed**
  ```bash
  curl -i -X PATCH http://localhost:8080/
  ```

- [ ] **Invalid method doesn't crash**
  ```bash
  printf "INVALID / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
  ```

### Status Code Validation
⚠️ **IMPORTANT**: Look up HTTP status codes - **wrong codes = no points for that test**

Common codes to verify:
- 200 OK
- 201 Created
- 400 Bad Request
- 404 Not Found
- 405 Method Not Allowed
- 411 Length Required
- 413 Payload Too Large
- 414 URI Too Long
- 500 Internal Server Error

---

## 4. File Upload

- [ ] **Upload file via POST**
  ```bash
  echo "test content" > /tmp/upload_test.txt
  curl -i -X POST -F "file=@/tmp/upload_test.txt" http://localhost:8080/upload
  ```

- [ ] **Retrieve uploaded file**
  ```bash
  curl -i http://localhost:8080/uploads/upload_test.txt
  ```

---

## 5. Browser Compatibility

- [ ] **Open browser, navigate to http://localhost:8080**
- [ ] **Check Network tab (request/response headers)**
- [ ] **Serves fully static website**
- [ ] **Wrong URL shows error page**
- [ ] **Directory listing works (if enabled)**
- [ ] **URL redirects work**
- [ ] **Try various things (images, CSS, JS)**

---

## 6. Port Configuration Tests

- [ ] **Multiple ports with different websites**
  ```bash
  # Check config has multiple servers on different ports
  # Verify each serves correct content
  curl http://localhost:8080/
  curl http://localhost:8081/
  ```

- [ ] **Same port multiple times fails**
  ```bash
  # Add duplicate port to config, server should fail to start
  # or show error message
  ```

- [ ] **Multiple servers with common ports**
  ```bash
  # Start server with config having conflicting ports
  # Should handle gracefully (fail or error message)
  ```

---

## 7. Siege & Stress Test

**Install Siege:**
```bash
brew install siege  # macOS
sudo apt install siege  # Linux
```

### Availability Test (MUST BE >99.5%)
```bash
siege -b -t30S http://localhost:8080/
# Check "Availability" in output
# MUST be > 99.5%
```

### Memory Leak Test
```bash
# Monitor memory during siege
top -pid $(pgrep webserv)

# Run siege for extended period
siege -b -t5M http://localhost:8080/

# Memory usage should NOT increase indefinitely
```

### No Hanging Connections
```bash
# Check for hanging connections
netstat -an | grep 8080

# Or
lsof -i :8080
```

### Indefinite Usage
```bash
# Server should run indefinitely
siege -b http://localhost:8080/
# Let it run for several minutes, server should remain stable
```

---

## 8. HTTP/1.1 Protocol Tests

### Host Header (MANDATORY for HTTP/1.1)
- [ ] **HTTP/1.1 without Host returns 400**
  ```bash
  printf "GET / HTTP/1.1\r\n\r\n" | nc localhost 8080
  # Should return 400 Bad Request
  ```

### HTTP Version Support
- [ ] **HTTP/1.0 works**
  ```bash
  printf "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n" | nc localhost 8080
  ```

- [ ] **HTTP/1.1 works**
  ```bash
  printf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
  ```

- [ ] **HTTP/2.0 returns 505**
  ```bash
  printf "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n" | nc localhost 8080
  # Should return 505 HTTP Version Not Supported
  ```

### Keep-Alive
- [ ] **Keep-Alive allows multiple requests**
  ```bash
  {
    printf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
    sleep 0.5
    printf "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
  } | nc localhost 8080
  # Should receive TWO responses
  ```

---

## 9. URI Validation Tests

- [ ] **URI too long (>2048) returns 414**
  ```bash
  curl -i http://localhost:8080/$(python3 -c 'print("a"*3000)')
  # Should return 414 URI Too Long
  ```

- [ ] **Path traversal blocked**
  ```bash
  curl -i http://localhost:8080/../../../etc/passwd
  # Should return 400 Bad Request

  curl -i http://localhost:8080/..%2f..%2fetc/passwd
  # Should return 400 Bad Request
  ```

- [ ] **Percent-encoded URI works**
  ```bash
  curl -i http://localhost:8080/test%20file.html
  ```

---

## 10. Header Validation Tests

- [ ] **Too many headers (>100) returns 431**
  ```bash
  {
    printf "GET / HTTP/1.1\r\nHost: localhost\r\n"
    for i in {1..150}; do
      printf "X-Header-$i: value\r\n"
    done
    printf "\r\n"
  } | nc localhost 8080
  # Should return 431 Request Header Fields Too Large
  ```

- [ ] **Header line too long (>8KB) returns 431**
  ```bash
  curl -H "X-Long: $(python3 -c 'print("A"*9000)')" http://localhost:8080/
  ```

- [ ] **Multiple Content-Length returns 400**
  ```bash
  printf "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\nContent-Length: 20\r\n\r\n" | nc localhost 8080
  ```

---

## 11. CGI Tests

- [ ] **Python CGI script works**
  ```bash
  curl -i http://localhost:8080/cgi-bin/test.py
  # Should return 200 with correct Content-Type
  ```

- [ ] **CGI with POST data**
  ```bash
  curl -i -X POST -d "name=test" http://localhost:8080/cgi-bin/test.py
  ```

- [ ] **CGI with GET parameters**
  ```bash
  curl -i "http://localhost:8080/cgi-bin/test.py?param=value"
  ```

- [ ] **CGI environment variables set correctly**
  ```bash
  # Check CGI script outputs correct REQUEST_METHOD, QUERY_STRING, etc.
  ```

---

## 12. Malformed Request Tests (NO CRASH)

⚠️ **CRITICAL**: Any crash = **FAIL (0 points)**

- [ ] **Incomplete request line**
  ```bash
  printf "GET\r\n\r\n" | nc localhost 8080
  ```

- [ ] **Missing HTTP version**
  ```bash
  printf "GET /\r\n\r\n" | nc localhost 8080
  ```

- [ ] **Binary garbage**
  ```bash
  dd if=/dev/urandom bs=1024 count=1 | nc localhost 8080
  # Server should NOT crash
  ```

- [ ] **Null bytes**
  ```bash
  printf "GET / HTTP/1.1\x00\r\nHost: localhost\r\n\r\n" | nc localhost 8080
  # Server should NOT crash
  ```

- [ ] **Empty request**
  ```bash
  printf "\r\n\r\n" | nc localhost 8080
  ```

---

## 13. Bonus (Optional)

### Cookies and Sessions
- [ ] **Session management works**
- [ ] **Cookies are set and persisted**

### Multiple CGI Systems
- [ ] **Python CGI works**
- [ ] **PHP CGI works**
- [ ] **Shell script CGI works**

---

## Final Checks

### Compilation
- [ ] **`make` compiles without errors**
- [ ] **No re-link issues**
- [ ] **Uses C++98 standard**
- [ ] **Compiles with -Wall -Wextra -Werror**

### Memory
- [ ] **No memory leaks (valgrind/leaks)**
- [ ] **No segfaults**
- [ ] **Stable under load**

### Standards Compliance
- [ ] **HTTP/1.1 compliant**
- [ ] **Correct status codes**
- [ ] **Proper headers in responses**

---

## Grading Notes

**Automatic Fail (0 points) if:**
- ❌ Segfault occurs
- ❌ Server crashes on any input
- ❌ poll() doesn't check read AND write simultaneously
- ❌ Read/write happens outside poll()
- ❌ errno checked after read/recv/write/send
- ❌ Memory leaks present
- ❌ Doesn't compile
- ❌ Uses forbidden functions

**Deduct points if:**
- Wrong HTTP status codes
- Missing configuration features
- Failed stress tests (availability < 99.5%)
- Incomplete functionality

---

## Evaluation Command Reference

```bash
# Quick sanity check
./tests/quick_eval_test.sh

# Full evaluation suite
./tests/evaluation_suite.sh

# Manual testing
telnet localhost 8080
curl -i http://localhost:8080/

# Memory check
valgrind --leak-check=full ./webserv config/default.conf
leaks webserv

# Stress test
siege -b -t30S http://localhost:8080/

# Check listening sockets
lsof -i :8080
netstat -an | grep 8080
```

---

## Tips for Evaluators

1. **Be thorough with I/O multiplexing check** - This is CRITICAL
2. **Verify status codes against RFC** - Wrong codes = no points
3. **Test malformed requests extensively** - Server must not crash
4. **Run stress tests** - Availability MUST be >99.5%
5. **Check memory** - Use valgrind or leaks tool
6. **Read the config file** - Understand what features are configured
7. **Ask questions** - Understand the architecture and design choices

---

## Expected Results Summary

| Test | Expected Result |
|------|-----------------|
| GET / | 200 OK |
| POST without CL | 411 Length Required |
| DELETE non-existent | 404 Not Found |
| PATCH method | 405 Method Not Allowed |
| URI too long | 414 URI Too Long |
| Headers too large | 431 Request Header Fields Too Large |
| Body too large | 413 Payload Too Large |
| Missing Host (HTTP/1.1) | 400 Bad Request |
| HTTP/2.0 | 505 HTTP Version Not Supported |
| Path traversal | 400 Bad Request |
| Malformed request | 400 Bad Request (NO CRASH) |
| Siege availability | >99.5% |
| Memory leaks | None |
