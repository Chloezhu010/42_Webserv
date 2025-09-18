#!/bin/bash

# Manual Telnet Testing Script for HTTP Methods
# Tests GET, POST, DELETE using telnet/netcat

HOST="127.0.0.1"
PORT="8080"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "\n${BLUE}========== $1 ==========${NC}"
}

print_test() {
    echo -e "${YELLOW}Testing: $1${NC}"
}

send_request() {
    local request="$1"
    local test_name="$2"

    echo -e "${BLUE}Request:${NC}"
    echo "$request"
    echo ""

    echo -e "${BLUE}Response:${NC}"
    if command -v nc >/dev/null; then
        echo -e "$request" | nc -w 3 $HOST $PORT
    else
        echo "netcat (nc) not available. Use manual telnet:"
        echo "telnet $HOST $PORT"
        echo "Then type the request manually."
    fi
    echo ""
    echo "----------------------------------------"
}

print_header "HTTP Methods Testing with Telnet"
echo "Target: $HOST:$PORT"
echo "Make sure ./webserv nginx1.conf is running"
echo ""

# Test 1: Basic GET Request
print_test "GET / (Root Path)"
request="GET / HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n"
send_request "$request" "GET Root"

# Test 2: GET Specific File
print_test "GET /about.html"
request="GET /about.html HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n"
send_request "$request" "GET File"

# Test 3: GET Non-existent File (404 Test)
print_test "GET /nonexistent.html (404 Test)"
request="GET /nonexistent.html HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n"
send_request "$request" "GET 404"

# Test 4: Basic POST Request
print_test "POST with Form Data"
post_data="name=test&value=123&message=hello"
content_length=${#post_data}
request="POST / HTTP/1.1\r\nHost: $HOST:$PORT\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: $content_length\r\n\r\n$post_data"
send_request "$request" "POST Form"

# Test 5: POST with JSON Data
print_test "POST with JSON Data"
json_data='{"name":"test","value":123,"array":[1,2,3]}'
content_length=${#json_data}
request="POST / HTTP/1.1\r\nHost: $HOST:$PORT\r\nContent-Type: application/json\r\nContent-Length: $content_length\r\n\r\n$json_data"
send_request "$request" "POST JSON"

# Test 6: DELETE Existing File
print_test "DELETE /test_file.html"
request="DELETE /test_file.html HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n"
send_request "$request" "DELETE File"

# Test 7: DELETE Non-existent File
print_test "DELETE /nonexistent.html (404 Test)"
request="DELETE /nonexistent.html HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n"
send_request "$request" "DELETE 404"

# Test 8: Invalid Method
print_test "INVALID Method Test"
request="INVALID / HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n"
send_request "$request" "Invalid Method"

# Test 9: Missing Host Header (HTTP/1.1 Requirement)
print_test "GET without Host Header"
request="GET / HTTP/1.1\r\n\r\n"
send_request "$request" "No Host Header"

# Test 10: Keep-Alive Test
print_test "Keep-Alive Connection Test"
request="GET / HTTP/1.1\r\nHost: $HOST:$PORT\r\nConnection: keep-alive\r\n\r\n"
send_request "$request" "Keep-Alive"

# Test 11: Connection Close Test
print_test "Connection Close Test"
request="GET / HTTP/1.1\r\nHost: $HOST:$PORT\r\nConnection: close\r\n\r\n"
send_request "$request" "Connection Close"

echo -e "${GREEN}Manual testing completed!${NC}"
echo ""
echo -e "${YELLOW}Notes:${NC}"
echo "- Look for HTTP status codes: 200, 400, 404, 405, 500"
echo "- Check Connection headers: keep-alive vs close"
echo "- Verify Content-Type and Content-Length headers"
echo "- POST should process body data"
echo "- DELETE should remove files or return 404"
echo ""
echo -e "${BLUE}For interactive testing, use:${NC}"
echo "telnet $HOST $PORT"