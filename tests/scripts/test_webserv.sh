#!/bin/bash

# HTTP Server Testing Script for ./webserv nginx1.conf
# Tests HTTP/1.1 functionality including keep-alive, methods, and error handling

SERVER_HOST="127.0.0.1"
SERVER_PORT="8080"
SERVER_URL="http://${SERVER_HOST}:${SERVER_PORT}"
TIMEOUT=5

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0

print_header() {
    echo -e "\n${BLUE}===== $1 =====${NC}"
}

print_test() {
    echo -e "${YELLOW}Testing: $1${NC}"
}

print_pass() {
    echo -e "${GREEN}✅ PASS: $1${NC}"
    ((PASSED_TESTS++))
}

print_fail() {
    echo -e "${RED}❌ FAIL: $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

check_server() {
    print_header "Server Connectivity Check"

    # Check if server is responding
    if curl -s --connect-timeout $TIMEOUT "$SERVER_URL" > /dev/null; then
        print_pass "Server is responding on $SERVER_URL"
        return 0
    else
        print_fail "Server not responding on $SERVER_URL"
        echo "Please ensure ./webserv nginx1.conf is running"
        exit 1
    fi
}

test_basic_get() {
    print_header "Basic GET Request Tests"

    # Test 1: GET / (index.html)
    print_test "GET / (root path)"
    ((TOTAL_TESTS++))
    response=$(curl -s -w "HTTPCODE:%{http_code}\nCONTENT_TYPE:%{content_type}\nCONNECTION:%{header_connection}\n" "$SERVER_URL/")

    if echo "$response" | grep -q "HTTPCODE:200"; then
        print_pass "GET / returns 200 OK"
    else
        print_fail "GET / did not return 200 OK"
        echo "Response: $response"
    fi

    # Test 2: GET /about.html
    print_test "GET /about.html"
    ((TOTAL_TESTS++))
    response=$(curl -s -w "HTTPCODE:%{http_code}\n" "$SERVER_URL/about.html")

    if echo "$response" | grep -q "HTTPCODE:200"; then
        print_pass "GET /about.html returns 200 OK"
    else
        http_code=$(echo "$response" | grep "HTTPCODE:" | cut -d: -f2)
        if [ "$http_code" = "404" ]; then
            print_info "GET /about.html returns 404 (file doesn't exist - expected)"
        else
            print_fail "GET /about.html returned unexpected code: $http_code"
        fi
    fi

    # Test 3: GET non-existent file
    print_test "GET /nonexistent.html (404 test)"
    ((TOTAL_TESTS++))
    response=$(curl -s -w "HTTPCODE:%{http_code}\n" "$SERVER_URL/nonexistent.html")

    if echo "$response" | grep -q "HTTPCODE:404"; then
        print_pass "GET /nonexistent.html returns 404 Not Found"
    else
        print_fail "GET /nonexistent.html did not return 404"
    fi
}

test_post_requests() {
    print_header "POST Request Tests"

    # Test 1: Basic POST
    print_test "POST with simple data"
    ((TOTAL_TESTS++))
    response=$(curl -s -X POST -w "HTTPCODE:%{http_code}\n" \
        -d "name=test&value=data" \
        "$SERVER_URL/")

    if echo "$response" | grep -q "HTTPCODE:200"; then
        print_pass "POST request returns 200 OK"
    else
        print_fail "POST request failed"
        echo "Response: $response"
    fi

    # Test 2: POST with JSON data
    print_test "POST with JSON content"
    ((TOTAL_TESTS++))
    response=$(curl -s -X POST -w "HTTPCODE:%{http_code}\n" \
        -H "Content-Type: application/json" \
        -d '{"test":"data","number":123}' \
        "$SERVER_URL/")

    if echo "$response" | grep -q "HTTPCODE:200"; then
        print_pass "POST with JSON returns 200 OK"
    else
        print_fail "POST with JSON failed"
    fi
}

test_delete_requests() {
    print_header "DELETE Request Tests"

    # Create a test file first (if possible)
    echo "<h1>Test file for deletion</h1>" > www/test_delete.html 2>/dev/null || true

    # Test DELETE existing file
    print_test "DELETE existing file"
    ((TOTAL_TESTS++))
    response=$(curl -s -X DELETE -w "HTTPCODE:%{http_code}\n" "$SERVER_URL/test_delete.html")

    http_code=$(echo "$response" | grep "HTTPCODE:" | cut -d: -f2)
    if [ "$http_code" = "200" ]; then
        print_pass "DELETE existing file returns 200 OK"
    elif [ "$http_code" = "404" ]; then
        print_info "DELETE returns 404 (file doesn't exist)"
    else
        print_fail "DELETE returned unexpected code: $http_code"
    fi

    # Test DELETE non-existent file
    print_test "DELETE non-existent file"
    ((TOTAL_TESTS++))
    response=$(curl -s -X DELETE -w "HTTPCODE:%{http_code}\n" "$SERVER_URL/nonexistent.html")

    if echo "$response" | grep -q "HTTPCODE:404"; then
        print_pass "DELETE non-existent file returns 404"
    else
        print_fail "DELETE non-existent file did not return 404"
    fi
}

test_http_headers() {
    print_header "HTTP Headers and Protocol Tests"

    # Test 1: Check required headers
    print_test "HTTP/1.1 headers validation"
    ((TOTAL_TESTS++))
    headers=$(curl -s -I "$SERVER_URL/")

    if echo "$headers" | grep -q "HTTP/1.1 200"; then
        print_pass "Server responds with HTTP/1.1"
    else
        print_fail "Server not responding with HTTP/1.1"
    fi

    # Test 2: Check Server header
    if echo "$headers" | grep -q "Server:"; then
        server_header=$(echo "$headers" | grep "Server:" | head -1)
        print_pass "Server header present: $server_header"
    else
        print_info "No Server header found"
    fi

    # Test 3: Check Content-Type
    if echo "$headers" | grep -q "Content-Type:"; then
        content_type=$(echo "$headers" | grep "Content-Type:" | head -1)
        print_pass "Content-Type header present: $content_type"
    else
        print_fail "No Content-Type header found"
    fi
}

test_keep_alive() {
    print_header "HTTP/1.1 Keep-Alive Tests"

    # Test 1: Default keep-alive behavior
    print_test "Default HTTP/1.1 keep-alive"
    ((TOTAL_TESTS++))

    # Use telnet-like approach with nc (netcat)
    if command -v nc >/dev/null; then
        response=$(echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc -w 3 $SERVER_HOST $SERVER_PORT)

        if echo "$response" | grep -q "Connection: keep-alive"; then
            print_pass "Default keep-alive working"
        elif echo "$response" | grep -q "Connection: close"; then
            print_info "Server closes connection by default"
        else
            print_fail "No connection header found"
        fi
    else
        print_info "netcat not available, skipping keep-alive test"
    fi

    # Test 2: Explicit Connection: close
    print_test "Explicit Connection: close"
    ((TOTAL_TESTS++))
    response=$(curl -s -H "Connection: close" -I "$SERVER_URL/")

    if echo "$response" | grep -q "Connection: close"; then
        print_pass "Connection: close honored"
    else
        print_fail "Connection: close not honored"
    fi
}

test_error_handling() {
    print_header "Error Handling Tests"

    # Test 1: Invalid HTTP method
    print_test "Invalid HTTP method"
    ((TOTAL_TESTS++))
    response=$(curl -s -X INVALID -w "HTTPCODE:%{http_code}\n" "$SERVER_URL/")

    http_code=$(echo "$response" | grep "HTTPCODE:" | cut -d: -f2)
    if [ "$http_code" = "400" ] || [ "$http_code" = "405" ] || [ "$http_code" = "501" ]; then
        print_pass "Invalid method returns error code: $http_code"
    else
        print_fail "Invalid method returned unexpected code: $http_code"
    fi

    # Test 2: Malformed request
    print_test "Malformed HTTP request"
    ((TOTAL_TESTS++))
    if command -v nc >/dev/null; then
        response=$(echo -e "INVALID REQUEST\r\n\r\n" | nc -w 3 $SERVER_HOST $SERVER_PORT)

        if echo "$response" | grep -q "400"; then
            print_pass "Malformed request returns 400 Bad Request"
        else
            print_fail "Malformed request handling failed"
        fi
    else
        print_info "netcat not available, skipping malformed request test"
    fi

    # Test 3: Request without Host header (HTTP/1.1 requirement)
    print_test "Missing Host header"
    ((TOTAL_TESTS++))
    if command -v nc >/dev/null; then
        response=$(echo -e "GET / HTTP/1.1\r\n\r\n" | nc -w 3 $SERVER_HOST $SERVER_PORT)

        if echo "$response" | grep -q "400"; then
            print_pass "Missing Host header returns 400"
        else
            print_info "Server accepts request without Host header"
        fi
    else
        print_info "netcat not available, skipping Host header test"
    fi
}

test_concurrent_connections() {
    print_header "Concurrent Connection Tests"

    print_test "Multiple simultaneous connections"
    ((TOTAL_TESTS++))

    # Launch 5 concurrent requests
    pids=()
    for i in {1..5}; do
        curl -s "$SERVER_URL/" > /tmp/concurrent_$i.out &
        pids+=($!)
    done

    # Wait for all to complete
    success_count=0
    for pid in "${pids[@]}"; do
        if wait $pid; then
            ((success_count++))
        fi
    done

    if [ $success_count -eq 5 ]; then
        print_pass "All 5 concurrent requests successful"
    else
        print_fail "Only $success_count/5 concurrent requests successful"
    fi

    # Cleanup
    rm -f /tmp/concurrent_*.out
}

test_large_requests() {
    print_header "Large Request Tests"

    # Test large POST body
    print_test "Large POST request body"
    ((TOTAL_TESTS++))

    # Create 10KB of data
    large_data=$(head -c 10240 < /dev/zero | tr '\0' 'A')

    response=$(curl -s -X POST -w "HTTPCODE:%{http_code}\n" \
        -d "$large_data" \
        "$SERVER_URL/")

    http_code=$(echo "$response" | grep "HTTPCODE:" | cut -d: -f2)
    if [ "$http_code" = "200" ]; then
        print_pass "Large POST request handled successfully"
    elif [ "$http_code" = "413" ]; then
        print_info "Large POST request rejected (413 - expected if body size limit set)"
    else
        print_fail "Large POST request returned unexpected code: $http_code"
    fi
}

run_stress_test() {
    print_header "Stress Test (Optional)"

    print_test "Rapid sequential requests"
    ((TOTAL_TESTS++))

    success_count=0
    for i in {1..20}; do
        if curl -s --max-time 2 "$SERVER_URL/" > /dev/null; then
            ((success_count++))
        fi
    done

    if [ $success_count -ge 18 ]; then
        print_pass "Stress test: $success_count/20 requests successful"
    else
        print_fail "Stress test: Only $success_count/20 requests successful"
    fi
}

print_summary() {
    print_header "Test Summary"

    echo "Total Tests: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $((TOTAL_TESTS - PASSED_TESTS))"

    if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
        echo -e "${GREEN}🎉 All tests passed!${NC}"
        exit 0
    else
        success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
        echo -e "${YELLOW}Success Rate: ${success_rate}%${NC}"

        if [ $success_rate -ge 80 ]; then
            echo -e "${GREEN}✅ Good - Most functionality working${NC}"
            exit 0
        else
            echo -e "${RED}❌ Needs improvement - Multiple failures detected${NC}"
            exit 1
        fi
    fi
}

# Main execution
main() {
    echo -e "${BLUE}🚀 HTTP Server Test Suite${NC}"
    echo "Testing server at: $SERVER_URL"
    echo "Make sure ./webserv nginx1.conf is running before starting tests"
    echo ""

    # Check if server is running
    check_server

    # Run all tests
    test_basic_get
    test_post_requests
    test_delete_requests
    test_http_headers
    test_keep_alive
    test_error_handling
    test_concurrent_connections
    test_large_requests

    # Optional stress test
    if [ "$1" = "--stress" ]; then
        run_stress_test
    fi

    # Print summary
    print_summary
}

# Help function
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "HTTP Server Test Suite"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --help, -h     Show this help message"
    echo "  --stress       Include stress testing"
    echo ""
    echo "Before running:"
    echo "  1. Start your server: ./webserv nginx1.conf"
    echo "  2. Ensure server is listening on $SERVER_URL"
    echo "  3. Run this script: $0"
    echo ""
    exit 0
fi

# Run main function
main "$@"