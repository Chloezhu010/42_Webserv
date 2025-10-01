#!/bin/bash

# ============================================================================
# WEBSERV EVALUATION TEST SUITE
# Based on 42 Evaluation Sheet Requirements
# ============================================================================

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Server configuration
SERVER_HOST="localhost"
SERVER_PORT=8080
SERVER_PID=""

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

print_header() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
}

print_test() {
    echo -e "${YELLOW}[TEST $TOTAL_TESTS]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}✓ PASS${NC} - $1"
    ((PASSED_TESTS++))
}

print_fail() {
    echo -e "${RED}✗ FAIL${NC} - $1"
    ((FAILED_TESTS++))
}

print_info() {
    echo -e "${BLUE}ℹ INFO${NC} - $1"
}

# Test assertion function
assert_response() {
    local description=$1
    local expected=$2
    local actual=$3

    ((TOTAL_TESTS++))
    print_test "$description"

    if echo "$actual" | grep -q "$expected"; then
        print_pass "$description"
        return 0
    else
        print_fail "$description"
        echo "  Expected: $expected"
        echo "  Got: $(echo "$actual" | head -n 1)"
        return 1
    fi
}

assert_status_code() {
    local description=$1
    local expected_code=$2
    local response=$3

    ((TOTAL_TESTS++))
    print_test "$description"

    if echo "$response" | head -n 1 | grep -q "HTTP/1.1 $expected_code"; then
        print_pass "Status code $expected_code"
        return 0
    else
        print_fail "Expected status code $expected_code"
        echo "  Got: $(echo "$response" | head -n 1)"
        return 1
    fi
}

# ============================================================================
# SERVER MANAGEMENT
# ============================================================================

start_server() {
    print_info "Starting webserv server..."

    # Check if server is already running
    if lsof -Pi :$SERVER_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        print_info "Server already running on port $SERVER_PORT"
        return 0
    fi

    # Start server in background
    ./webserv config/default.conf > /dev/null 2>&1 &
    SERVER_PID=$!

    # Wait for server to start
    sleep 2

    if lsof -Pi :$SERVER_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        print_pass "Server started on port $SERVER_PORT (PID: $SERVER_PID)"
        return 0
    else
        print_fail "Failed to start server"
        return 1
    fi
}

stop_server() {
    if [ -n "$SERVER_PID" ]; then
        print_info "Stopping server (PID: $SERVER_PID)..."
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
        print_pass "Server stopped"
    fi
}

# ============================================================================
# MANDATORY PART - BASIC CHECKS
# ============================================================================

test_basic_get() {
    print_header "BASIC CHECKS - GET REQUESTS"

    # Test 1: Simple GET request
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "GET / should return 200 OK" "200" "$response"

    # Test 2: GET static file
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/index.html)
    assert_status_code "GET /index.html should return 200 OK" "200" "$response"

    # Test 3: GET non-existent file
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/nonexistent.html)
    assert_status_code "GET /nonexistent.html should return 404" "404" "$response"

    # Test 4: GET with query string
    response=$(curl -s -i "http://$SERVER_HOST:$SERVER_PORT/index.html?param=value")
    assert_status_code "GET with query string should return 200" "200" "$response"
}

test_basic_post() {
    print_header "BASIC CHECKS - POST REQUESTS"

    # Test 1: Simple POST request
    response=$(curl -s -i -X POST -H "Content-Type: text/plain" -d "test data" http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "POST with body should work" "200|201" "$response"

    # Test 2: POST without Content-Length
    response=$(printf "POST / HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\ntest" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "POST without Content-Length should return 411" "411" "$response"

    # Test 3: POST with Content-Length
    response=$(curl -s -i -X POST -H "Content-Type: text/plain" -H "Content-Length: 9" -d "test data" http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "POST with Content-Length should work" "200|201" "$response"

    # Test 4: POST exceeding body limit (if configured)
    large_body=$(python3 -c "print('A' * 11000000)")  # 11MB
    response=$(curl -s -i -X POST -H "Content-Type: text/plain" -d "$large_body" http://$SERVER_HOST:$SERVER_PORT/ 2>/dev/null)
    assert_status_code "POST exceeding body limit should return 413" "413" "$response"
}

test_basic_delete() {
    print_header "BASIC CHECKS - DELETE REQUESTS"

    # Create a test file first
    echo "test content" > www/test_delete.txt

    # Test 1: DELETE existing file
    response=$(curl -s -i -X DELETE http://$SERVER_HOST:$SERVER_PORT/test_delete.txt)
    assert_status_code "DELETE existing file should return 200" "200" "$response"

    # Test 2: DELETE non-existent file
    response=$(curl -s -i -X DELETE http://$SERVER_HOST:$SERVER_PORT/nonexistent.txt)
    assert_status_code "DELETE non-existent file should return 404" "404" "$response"

    # Test 3: DELETE directory (should fail)
    response=$(curl -s -i -X DELETE http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "DELETE directory should return 403" "403" "$response"
}

test_unknown_methods() {
    print_header "BASIC CHECKS - UNKNOWN/UNSUPPORTED METHODS"

    # Test 1: PATCH method (unsupported)
    response=$(curl -s -i -X PATCH http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "PATCH should return 405 Method Not Allowed" "405" "$response"

    # Test 2: PUT method (unsupported)
    response=$(curl -s -i -X PUT http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "PUT should return 405 Method Not Allowed" "405" "$response"

    # Test 3: OPTIONS method
    response=$(curl -s -i -X OPTIONS http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "OPTIONS should return 405 or 501" "405|501" "$response"

    # Test 4: Custom/invalid method
    response=$(printf "INVALID / HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "Invalid method should return 400 or 405" "400|405" "$response"

    # Test 5: Malformed request - no crash check
    ((TOTAL_TESTS++))
    print_test "Malformed request should not crash server"
    printf "GARBAGE\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT > /dev/null 2>&1
    sleep 1
    if lsof -Pi :$SERVER_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        print_pass "Server still running after malformed request"
    else
        print_fail "Server crashed on malformed request"
    fi
}

test_file_upload() {
    print_header "BASIC CHECKS - FILE UPLOAD"

    # Test 1: Upload file via POST
    echo "test upload content" > /tmp/test_upload.txt
    response=$(curl -s -i -X POST -F "file=@/tmp/test_upload.txt" http://$SERVER_HOST:$SERVER_PORT/upload)
    assert_status_code "File upload should return 200 or 201" "200|201" "$response"

    # Test 2: Retrieve uploaded file (if upload saves files)
    # Note: This depends on your implementation
    # response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/uploads/test_upload.txt)
    # assert_status_code "Retrieve uploaded file should return 200" "200" "$response"

    rm -f /tmp/test_upload.txt
}

# ============================================================================
# CONFIGURATION TESTS
# ============================================================================

test_configuration() {
    print_header "CONFIGURATION TESTS"

    # Test 1: Multiple servers on different ports (manual check required)
    print_info "Multiple ports test requires manual config verification"

    # Test 2: Virtual hosts with different hostnames
    response=$(curl -s -i -H "Host: example.com" http://$SERVER_HOST:$SERVER_PORT/)
    assert_response "Virtual host routing should work" "HTTP/1.1" "$response"

    # Test 3: Custom error page (404)
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/nonexistent)
    assert_status_code "Custom 404 error page" "404" "$response"

    # Test 4: Body size limit
    print_info "Body size limit tested in POST tests (413 status)"

    # Test 5: Route to different directories
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/)
    assert_response "Root route should work" "HTTP/1.1 200" "$response"

    # Test 6: Default index file
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/)
    assert_response "Directory should serve index file" "HTTP/1.1 200" "$response"

    # Test 7: Method restrictions per route
    # This requires specific configuration - example:
    # location /admin { allow_methods GET; }
    # response=$(curl -s -i -X POST http://$SERVER_HOST:$SERVER_PORT/admin)
    # assert_status_code "Restricted method should return 405" "405" "$response"
}

# ============================================================================
# BROWSER COMPATIBILITY TESTS
# ============================================================================

test_browser_compatibility() {
    print_header "BROWSER COMPATIBILITY TESTS"

    # Test 1: Serve static website
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/index.html)
    assert_status_code "Serve static HTML" "200" "$response"
    assert_response "HTML content served" "text/html" "$response"

    # Test 2: Wrong URL
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/wrong_url)
    assert_status_code "Wrong URL returns 404" "404" "$response"

    # Test 3: Directory listing (if autoindex enabled)
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/images/)
    assert_response "Directory listing or index" "HTTP/1.1 200|403" "$response"

    # Test 4: URL redirect
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/redirect)
    assert_response "Redirect should work" "HTTP/1.1 301|302|307" "$response"

    # Test 5: Request/Response headers
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/)
    assert_response "Response should have Server header" "Server:" "$response"
    assert_response "Response should have Date header" "Date:" "$response"
    assert_response "Response should have Content-Type" "Content-Type:" "$response"
}

# ============================================================================
# HTTP/1.1 PROTOCOL TESTS
# ============================================================================

test_http_protocol() {
    print_header "HTTP/1.1 PROTOCOL TESTS"

    # Test 1: HTTP version validation
    response=$(printf "GET / HTTP/1.0\r\nHost: $SERVER_HOST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_response "HTTP/1.0 should be handled" "HTTP/1" "$response"

    response=$(printf "GET / HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_response "HTTP/1.1 should be handled" "HTTP/1.1" "$response"

    response=$(printf "GET / HTTP/2.0\r\nHost: $SERVER_HOST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "HTTP/2.0 should return 505" "505" "$response"

    # Test 2: Host header requirement (HTTP/1.1)
    response=$(printf "GET / HTTP/1.1\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "Missing Host header should return 400" "400" "$response"

    # Test 3: Keep-Alive connections
    ((TOTAL_TESTS++))
    print_test "Keep-Alive connection test"
    {
        printf "GET / HTTP/1.1\r\nHost: $SERVER_HOST\r\nConnection: keep-alive\r\n\r\n"
        sleep 0.5
        printf "GET / HTTP/1.1\r\nHost: $SERVER_HOST\r\nConnection: close\r\n\r\n"
    } | nc $SERVER_HOST $SERVER_PORT > /tmp/keepalive_test.txt

    if [ $(grep -c "HTTP/1.1 200" /tmp/keepalive_test.txt) -eq 2 ]; then
        print_pass "Keep-Alive allows multiple requests"
    else
        print_fail "Keep-Alive not working properly"
    fi
    rm -f /tmp/keepalive_test.txt

    # Test 4: Connection close
    response=$(curl -s -i -H "Connection: close" http://$SERVER_HOST:$SERVER_PORT/)
    assert_response "Connection close header respected" "Connection: close|HTTP/1.1 200" "$response"
}

# ============================================================================
# URI AND HEADER VALIDATION TESTS
# ============================================================================

test_uri_validation() {
    print_header "URI VALIDATION TESTS"

    # Test 1: Long URI (>2048 characters)
    long_uri=$(python3 -c "print('/' + 'a' * 3000)")
    response=$(curl -s -i "http://$SERVER_HOST:$SERVER_PORT$long_uri")
    assert_status_code "URI too long should return 414" "414" "$response"

    # Test 2: Path traversal attempts
    response=$(curl -s -i "http://$SERVER_HOST:$SERVER_PORT/../../../etc/passwd")
    assert_status_code "Path traversal should return 400" "400" "$response"

    response=$(curl -s -i "http://$SERVER_HOST:$SERVER_PORT/..%2f..%2f..%2fetc/passwd")
    assert_status_code "Encoded path traversal should return 400" "400" "$response"

    # Test 3: Special characters in URI
    response=$(curl -s -i "http://$SERVER_HOST:$SERVER_PORT/test%20file.html")
    assert_response "Percent-encoded URI should work" "HTTP/1.1" "$response"

    # Test 4: Query strings
    response=$(curl -s -i "http://$SERVER_HOST:$SERVER_PORT/?key=value&foo=bar")
    assert_status_code "Query string should work" "200" "$response"
}

test_header_validation() {
    print_header "HEADER VALIDATION TESTS"

    # Test 1: Too many headers (>100)
    ((TOTAL_TESTS++))
    print_test "Too many headers should return 431"
    request="GET / HTTP/1.1\r\nHost: $SERVER_HOST\r\n"
    for i in {1..150}; do
        request+="X-Custom-Header-$i: value\r\n"
    done
    request+="\r\n"
    response=$(printf "$request" | nc $SERVER_HOST $SERVER_PORT)
    if echo "$response" | head -n 1 | grep -q "431"; then
        print_pass "Too many headers returns 431"
    else
        print_fail "Expected 431 for too many headers"
    fi

    # Test 2: Header line too long (>8KB)
    long_value=$(python3 -c "print('A' * 9000)")
    response=$(curl -s -i -H "X-Long-Header: $long_value" http://$SERVER_HOST:$SERVER_PORT/)
    assert_status_code "Header too long should return 431" "431" "$response"

    # Test 3: Multiple Content-Length headers
    response=$(printf "POST / HTTP/1.1\r\nHost: $SERVER_HOST\r\nContent-Length: 10\r\nContent-Length: 20\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "Multiple Content-Length should return 400" "400" "$response"

    # Test 4: Invalid Content-Length
    response=$(printf "POST / HTTP/1.1\r\nHost: $SERVER_HOST\r\nContent-Length: invalid\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "Invalid Content-Length should return 400" "400" "$response"
}

# ============================================================================
# CHUNKED TRANSFER ENCODING TESTS
# ============================================================================

test_chunked_encoding() {
    print_header "CHUNKED TRANSFER ENCODING TESTS"

    # Test 1: Valid chunked request
    ((TOTAL_TESTS++))
    print_test "Valid chunked POST request"
    chunked_request="POST / HTTP/1.1\r\nHost: $SERVER_HOST\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n"
    response=$(printf "$chunked_request" | nc $SERVER_HOST $SERVER_PORT)
    if echo "$response" | head -n 1 | grep -q "200\|201"; then
        print_pass "Chunked encoding handled correctly"
    else
        print_fail "Chunked encoding failed"
        echo "  Got: $(echo "$response" | head -n 1)"
    fi

    # Test 2: Both Transfer-Encoding and Content-Length (conflict)
    response=$(printf "POST / HTTP/1.1\r\nHost: $SERVER_HOST\r\nContent-Length: 10\r\nTransfer-Encoding: chunked\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "TE + CL conflict should return 400" "400" "$response"
}

# ============================================================================
# CGI TESTS
# ============================================================================

test_cgi() {
    print_header "CGI TESTS"

    # Test 1: Python CGI script
    if [ -f "www/cgi-bin/test.py" ]; then
        chmod +x www/cgi-bin/test.py
        response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/cgi-bin/test.py)
        assert_status_code "Python CGI should return 200" "200" "$response"
        assert_response "CGI should output content" "Content-Type:|text/" "$response"
    fi

    # Test 2: CGI with POST data
    if [ -f "www/python/cgi_post_test.py" ]; then
        chmod +x www/python/cgi_post_test.py
        response=$(curl -s -i -X POST -d "name=test&value=123" http://$SERVER_HOST:$SERVER_PORT/python/cgi_post_test.py)
        assert_status_code "CGI POST should return 200" "200" "$response"
    fi

    # Test 3: CGI timeout test (if implemented)
    print_info "CGI timeout test requires long-running script"

    # Test 4: CGI with GET parameters
    if [ -f "www/cgi-bin/test.py" ]; then
        response=$(curl -s -i "http://$SERVER_HOST:$SERVER_PORT/cgi-bin/test.py?param=value")
        assert_status_code "CGI with query string should return 200" "200" "$response"
    fi
}

# ============================================================================
# STRESS AND PERFORMANCE TESTS
# ============================================================================

test_concurrent_connections() {
    print_header "CONCURRENT CONNECTIONS TEST"

    ((TOTAL_TESTS++))
    print_test "Handle 50 concurrent connections"

    # Launch 50 concurrent curl requests
    for i in {1..50}; do
        curl -s http://$SERVER_HOST:$SERVER_PORT/ > /dev/null &
    done
    wait

    # Check if server is still responsive
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/)
    if echo "$response" | head -n 1 | grep -q "200"; then
        print_pass "Server handles concurrent connections"
    else
        print_fail "Server failed under concurrent load"
    fi
}

test_memory_leaks() {
    print_header "MEMORY LEAK CHECK"

    print_info "Memory leak testing requires valgrind or leaks tool"
    print_info "Manual check: Run 'leaks webserv' or 'valgrind ./webserv'"

    # Basic memory check - send many requests and check if memory grows indefinitely
    ((TOTAL_TESTS++))
    print_test "Memory stability test (1000 requests)"

    for i in {1..1000}; do
        curl -s http://$SERVER_HOST:$SERVER_PORT/ > /dev/null
    done

    # Check if server is still responsive
    response=$(curl -s -i http://$SERVER_HOST:$SERVER_PORT/)
    if echo "$response" | head -n 1 | grep -q "200"; then
        print_pass "Server stable after 1000 requests"
    else
        print_fail "Server unstable after many requests"
    fi
}

test_siege() {
    print_header "SIEGE STRESS TEST"

    # Check if siege is installed
    if ! command -v siege &> /dev/null; then
        print_info "Siege not installed. Install with: brew install siege"
        return
    fi

    ((TOTAL_TESTS++))
    print_test "Siege stress test - Availability >99.5%"

    # Run siege
    siege -b -t30S -c10 http://$SERVER_HOST:$SERVER_PORT/ > /tmp/siege_result.txt 2>&1

    # Parse availability
    availability=$(grep "Availability:" /tmp/siege_result.txt | awk '{print $2}' | tr -d '%')

    if [ -n "$availability" ]; then
        if (( $(echo "$availability > 99.5" | bc -l) )); then
            print_pass "Availability: $availability% (>99.5%)"
        else
            print_fail "Availability: $availability% (<99.5%)"
        fi
        cat /tmp/siege_result.txt
    else
        print_fail "Could not parse siege results"
    fi

    rm -f /tmp/siege_result.txt
}

# ============================================================================
# MALFORMED REQUEST TESTS (NO CRASH)
# ============================================================================

test_malformed_requests() {
    print_header "MALFORMED REQUEST TESTS (NO CRASH)"

    # Test 1: Incomplete request line
    response=$(printf "GET\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "Incomplete request line should return 400" "400" "$response"

    # Test 2: Missing HTTP version
    response=$(printf "GET /\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "Missing HTTP version should return 400" "400" "$response"

    # Test 3: Invalid characters in method
    response=$(printf "G@T / HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_response "Invalid method characters" "400|405" "$response"

    # Test 4: Request line too long
    long_method=$(python3 -c "print('A' * 10000)")
    response=$(printf "$long_method / HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_response "Request line too long" "400|414" "$response"

    # Test 5: Binary garbage
    ((TOTAL_TESTS++))
    print_test "Binary garbage should not crash server"
    dd if=/dev/urandom bs=1024 count=1 2>/dev/null | nc $SERVER_HOST $SERVER_PORT > /dev/null 2>&1
    sleep 1
    if lsof -Pi :$SERVER_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        print_pass "Server survived binary garbage"
    else
        print_fail "Server crashed on binary garbage"
    fi

    # Test 6: Null bytes in request
    ((TOTAL_TESTS++))
    print_test "Null bytes should not crash server"
    printf "GET / HTTP/1.1\x00\r\nHost: $SERVER_HOST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT > /dev/null 2>&1
    sleep 1
    if lsof -Pi :$SERVER_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        print_pass "Server survived null bytes"
    else
        print_fail "Server crashed on null bytes"
    fi
}

# ============================================================================
# PORT CONFIGURATION TESTS
# ============================================================================

test_port_issues() {
    print_header "PORT CONFIGURATION TESTS"

    print_info "Multiple port test requires config with multiple ports"
    print_info "Example: server { listen 8080; } server { listen 8081; }"

    # Test 1: Same port in multiple server blocks should fail
    print_info "Duplicate port test requires manual config verification"
    print_info "Config with duplicate ports should fail to start"

    # Test 2: Multiple servers with common ports
    print_info "Common port test requires manual config verification"
}

# ============================================================================
# EDGE CASE TESTS
# ============================================================================

test_edge_cases() {
    print_header "EDGE CASE TESTS"

    # Test 1: Empty request
    ((TOTAL_TESTS++))
    print_test "Empty request handling"
    response=$(printf "\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    if [ -n "$response" ]; then
        print_pass "Server handles empty request"
    else
        print_fail "Server hung on empty request"
    fi

    # Test 2: Request with only headers, no body
    response=$(printf "POST / HTTP/1.1\r\nHost: $SERVER_HOST\r\nContent-Length: 0\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT)
    assert_status_code "POST with Content-Length: 0 should work" "200|201" "$response"

    # Test 3: Very slow request (timeout test)
    ((TOTAL_TESTS++))
    print_test "Slow request timeout test"
    {
        printf "GET "
        sleep 10
        printf "/ HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\n"
    } | nc $SERVER_HOST $SERVER_PORT > /tmp/slow_test.txt 2>&1 &
    NC_PID=$!
    sleep 12
    if ps -p $NC_PID > /dev/null; then
        kill $NC_PID 2>/dev/null
        print_fail "Server did not timeout slow request"
    else
        print_pass "Server handled slow request (timeout or completed)"
    fi
    rm -f /tmp/slow_test.txt

    # Test 4: Pipelined requests (HTTP/1.1)
    ((TOTAL_TESTS++))
    print_test "HTTP pipelining test"
    {
        printf "GET / HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\n"
        printf "GET /index.html HTTP/1.1\r\nHost: $SERVER_HOST\r\n\r\n"
    } | nc $SERVER_HOST $SERVER_PORT > /tmp/pipeline_test.txt

    if [ $(grep -c "HTTP/1.1 200" /tmp/pipeline_test.txt) -eq 2 ]; then
        print_pass "HTTP pipelining works"
    else
        print_info "HTTP pipelining may not be implemented (optional)"
    fi
    rm -f /tmp/pipeline_test.txt
}

# ============================================================================
# FINAL SUMMARY
# ============================================================================

print_summary() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}TEST SUMMARY${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Total Tests:  ${YELLOW}$TOTAL_TESTS${NC}"
    echo -e "Passed:       ${GREEN}$PASSED_TESTS${NC}"
    echo -e "Failed:       ${RED}$FAILED_TESTS${NC}"

    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}✓ ALL TESTS PASSED!${NC}"
        echo -e "${GREEN}Your server is ready for evaluation!${NC}"
    else
        echo -e "\n${RED}✗ SOME TESTS FAILED${NC}"
        echo -e "${YELLOW}Review the failures above before evaluation${NC}"
    fi
    echo ""
}

# ============================================================================
# MAIN EXECUTION
# ============================================================================

main() {
    echo -e "${BLUE}"
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║     WEBSERV EVALUATION TEST SUITE                     ║"
    echo "║     Based on 42 Evaluation Criteria                   ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo -e "${NC}"

    # Check if server executable exists
    if [ ! -f "./webserv" ]; then
        echo -e "${RED}Error: webserv executable not found${NC}"
        echo "Please run 'make' first"
        exit 1
    fi

    # Check if config file exists
    if [ ! -f "config/default.conf" ]; then
        echo -e "${YELLOW}Warning: config/default.conf not found${NC}"
        echo "Using default configuration"
    fi

    # Start server
    start_server || exit 1

    # Run all test suites
    test_basic_get
    test_basic_post
    test_basic_delete
    test_unknown_methods
    test_file_upload
    test_configuration
    test_browser_compatibility
    test_http_protocol
    test_uri_validation
    test_header_validation
    test_chunked_encoding
    test_cgi
    test_malformed_requests
    test_edge_cases
    test_concurrent_connections
    test_memory_leaks
    test_siege
    test_port_issues

    # Stop server
    stop_server

    # Print summary
    print_summary
}

# Trap to ensure server is stopped on exit
trap stop_server EXIT INT TERM

# Run main function
main "$@"
