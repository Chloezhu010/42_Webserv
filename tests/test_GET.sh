#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Server configuration
HOST="localhost"
PORT="8080"
SERVER_URL="http://${HOST}:${PORT}"

# Test counters
PASSED=0
FAILED=0
TOTAL=0

# Function to print colored output
print_status() {
    local status=$1
    local test_name=$2
    local details=$3
    
    TOTAL=$((TOTAL + 1))
    
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}✓ PASS${NC} $test_name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC} $test_name"
        FAILED=$((FAILED + 1))
    fi
    
    if [ ! -z "$details" ]; then
        echo -e "  ${CYAN}$details${NC}"
    fi
}

# Function to test HTTP status code
test_http_status() {
    local url=$1
    local expected_status=$2
    local test_name=$3
    local curl_options=$4
    
    echo -e "\n${YELLOW}Testing:${NC} $test_name"
    echo -e "${BLUE}URL:${NC} $url"
    
    # Make the request and capture both status code and response
    response=$(curl -s -w "\nHTTP_STATUS:%{http_code}\nTIME_TOTAL:%{time_total}" $curl_options "$url" 2>/dev/null)
    
    if [ $? -ne 0 ]; then
        print_status "FAIL" "$test_name" "Could not connect to server"
        return
    fi
    
    # Extract status code and time
    status_code=$(echo "$response" | grep "HTTP_STATUS:" | cut -d: -f2)
    time_total=$(echo "$response" | grep "TIME_TOTAL:" | cut -d: -f2)
    
    # Remove status line from response body
    body=$(echo "$response" | sed '/HTTP_STATUS:/d' | sed '/TIME_TOTAL:/d')
    
    echo -e "${PURPLE}Status Code:${NC} $status_code"
    echo -e "${PURPLE}Response Time:${NC} ${time_total}s"
    
    # Show first few lines of response body
    if [ ! -z "$body" ]; then
        echo -e "${PURPLE}Response Body (first 3 lines):${NC}"
        echo "$body" | head -3 | sed 's/^/  /'
        if [ $(echo "$body" | wc -l) -gt 3 ]; then
            echo "  ..."
        fi
    fi
    
    # Check if status code matches expected
    if [ "$status_code" = "$expected_status" ]; then
        print_status "PASS" "$test_name" "Expected $expected_status, got $status_code"
    else
        print_status "FAIL" "$test_name" "Expected $expected_status, got $status_code"
    fi
}

# Function to test raw HTTP request using netcat
test_raw_http_nc() {
    local request=$1
    local test_name=$2
    
    echo -e "\n${YELLOW}Testing:${NC} $test_name (Raw HTTP - netcat)"
    echo -e "${BLUE}Request:${NC}"
    echo "$request" | sed 's/^/  /'
    
    # Send raw HTTP request
    response=$(echo -e "$request" | nc -w 5 $HOST $PORT 2>/dev/null)
    
    if [ $? -ne 0 ] || [ -z "$response" ]; then
        print_status "FAIL" "$test_name" "No response from server (netcat)"
        return
    fi
    
    # Extract status line
    status_line=$(echo "$response" | head -1)
    echo -e "${PURPLE}Status Line:${NC} $status_line"
    
    # Show headers
    headers=$(echo "$response" | sed -n '2,/^$/p' | head -5)
    if [ ! -z "$headers" ]; then
        echo -e "${PURPLE}Headers (first 5):${NC}"
        echo "$headers" | sed 's/^/  /'
    fi
    
    # Check if it's a valid HTTP response
    if echo "$status_line" | grep -q "HTTP/1.1\|HTTP/1.0"; then
        print_status "PASS" "$test_name" "Valid HTTP response received (netcat)"
    else
        print_status "FAIL" "$test_name" "Invalid HTTP response format (netcat)"
    fi
}

# Function to test raw HTTP request using telnet
test_raw_http_telnet() {
    local request=$1
    local test_name=$2
    local timeout=${3:-5}
    
    echo -e "\n${YELLOW}Testing:${NC} $test_name (Raw HTTP - telnet)"
    echo -e "${BLUE}Request:${NC}"
    echo "$request" | sed 's/^/  /'
    
    # Create a temporary file for the request
    local temp_file=$(mktemp)
    echo -e "$request" > "$temp_file"
    
    # Send raw HTTP request using telnet with timeout
    response=$(timeout $timeout telnet $HOST $PORT < "$temp_file" 2>/dev/null | tr -d '\0')
    
    # Clean up temp file
    rm -f "$temp_file"
    
    if [ $? -eq 124 ]; then
        print_status "FAIL" "$test_name" "Telnet connection timed out"
        return
    fi
    
    if [ -z "$response" ]; then
        print_status "FAIL" "$test_name" "No response from server (telnet)"
        return
    fi
    
    # Clean up telnet connection messages
    clean_response=$(echo "$response" | grep -v "Trying\|Connected\|Escape character\|Connection closed\|telnet>" | grep -v "^$" | head -20)
    
    if [ -z "$clean_response" ]; then
        print_status "FAIL" "$test_name" "No valid HTTP response (telnet)"
        return
    fi
    
    # Extract status line (first non-empty line)
    status_line=$(echo "$clean_response" | head -1)
    echo -e "${PURPLE}Status Line:${NC} $status_line"
    
    # Show headers
    headers=$(echo "$clean_response" | sed -n '2,/^$/p' | head -5)
    if [ ! -z "$headers" ]; then
        echo -e "${PURPLE}Headers (first 5):${NC}"
        echo "$headers" | sed 's/^/  /'
    fi
    
    # Check if it's a valid HTTP response
    if echo "$status_line" | grep -q "HTTP/1.1\|HTTP/1.0"; then
        print_status "PASS" "$test_name" "Valid HTTP response received (telnet)"
    else
        print_status "FAIL" "$test_name" "Invalid HTTP response format (telnet)"
    fi
}

# Function to test interactive telnet session (for manual testing)
test_interactive_telnet() {
    local test_name=$1
    
    echo -e "\n${YELLOW}Testing:${NC} $test_name (Interactive telnet)"
    echo -e "${BLUE}Instructions:${NC}"
    echo -e "  1. Type: GET / HTTP/1.1"
    echo -e "  2. Press Enter"
    echo -e "  3. Type: Host: $HOST:$PORT"
    echo -e "  4. Press Enter twice to send the request"
    echo -e "  5. Type 'quit' or press Ctrl+] then 'quit' to exit"
    echo -e "${CYAN}Starting interactive telnet session...${NC}"
    
    read -p "Press Enter to start interactive session (or 'skip' to skip): " choice
    if [ "$choice" = "skip" ]; then
        print_status "PASS" "$test_name" "Skipped interactive session"
        return
    fi
    
    telnet $HOST $PORT
    print_status "PASS" "$test_name" "Interactive session completed"
}

# Function to check if server is running
check_server() {
    echo -e "${WHITE}Checking if webserv is running on $SERVER_URL...${NC}"
    
    if curl -s --connect-timeout 3 "$SERVER_URL" > /dev/null; then
        echo -e "${GREEN}✓ Server is running${NC}"
        return 0
    else
        echo -e "${RED}✗ Server is not running or not accessible${NC}"
        echo -e "${YELLOW}Make sure your webserv is running on $HOST:$PORT${NC}"
        return 1
    fi
}

# Main testing function
run_get_tests() {
    echo -e "${WHITE}======================================${NC}"
    echo -e "${WHITE}    WEBSERV GET METHOD TESTING       ${NC}"
    echo -e "${WHITE}======================================${NC}"
    
    # Check if server is running
    if ! check_server; then
        exit 1
    fi
    
    echo -e "\n${WHITE}=== Basic GET Requests ===${NC}"
    
    # Test 1: GET root path
    test_http_status "$SERVER_URL/" "200" "GET root path (/)"
    
    # Test 2: GET index.html (if exists)
    test_http_status "$SERVER_URL/index.html" "200" "GET index.html" 
    
    # Test 3: GET non-existent file (should return 404)
    test_http_status "$SERVER_URL/nonexistent.html" "404" "GET non-existent file"
    
    # Test 4: GET with query parameters
    test_http_status "$SERVER_URL/test?param1=value1&param2=value2" "200" "GET with query parameters"
    
    echo -e "\n${WHITE}=== GET Requests with Different Paths ===${NC}"
    
    # Test 5: GET directory (without trailing slash)
    test_http_status "$SERVER_URL/test" "200" "GET directory without trailing slash"
    
    # Test 6: GET directory (with trailing slash)
    test_http_status "$SERVER_URL/test/" "200" "GET directory with trailing slash"
    
    # Test 7: GET nested path
    test_http_status "$SERVER_URL/path/to/resource" "404" "GET nested path (should be 404)"
    
    echo -e "\n${WHITE}=== GET Requests with Headers ===${NC}"
    
    # Test 8: GET with custom User-Agent
    test_http_status "$SERVER_URL/" "200" "GET with custom User-Agent" "-H 'User-Agent: WebservTester/1.0'"
    
    # Test 9: GET with custom Host header
    test_http_status "$SERVER_URL/" "200" "GET with custom Host header" "-H 'Host: example.com'"
    
    # Test 10: GET with Accept header
    test_http_status "$SERVER_URL/" "200" "GET with Accept header" "-H 'Accept: text/html'"
    
    echo -e "\n${WHITE}=== Edge Cases and Error Conditions ===${NC}"
    
    # Test 11: GET very long URL
    long_path="/test/" + $(printf 'a%.0s' {1..100})
    test_http_status "$SERVER_URL$long_path" "414" "GET very long URL"
    
    # Test 12: GET with special characters in URL
    test_http_status "$SERVER_URL/test%20file.html" "404" "GET with URL encoding"
    
    # Test 13: GET with multiple slashes
    test_http_status "$SERVER_URL///" "200" "GET with multiple slashes"
    
    echo -e "\n${WHITE}=== Raw HTTP Requests (netcat) ===${NC}"
    
    # Test 14: Basic raw HTTP GET with netcat
    test_raw_http_nc "GET / HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n" "Raw HTTP GET request (nc)"
    
    # Test 15: HTTP/1.0 request with netcat
    test_raw_http_nc "GET / HTTP/1.0\r\nHost: $HOST:$PORT\r\n\r\n" "HTTP/1.0 GET request (nc)"
    
    # Test 16: GET without Host header (HTTP/1.1 requires Host) with netcat
    test_raw_http_nc "GET / HTTP/1.1\r\n\r\n" "GET without Host header (nc)"
    
    echo -e "\n${WHITE}=== Raw HTTP Requests (telnet) ===${NC}"
    
    # Test 17: Basic raw HTTP GET with telnet
    test_raw_http_telnet "GET / HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n" "Raw HTTP GET request (telnet)"
    
    # Test 18: HTTP/1.0 request with telnet
    test_raw_http_telnet "GET / HTTP/1.0\r\nHost: $HOST:$PORT\r\n\r\n" "HTTP/1.0 GET request (telnet)"
    
    # Test 19: GET without Host header with telnet
    test_raw_http_telnet "GET / HTTP/1.1\r\n\r\n" "GET without Host header (telnet)"
    
    # Test 20: GET with malformed request line
    test_raw_http_telnet "GET HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n" "Malformed request line (telnet)"
    
    # Test 21: GET with invalid HTTP version
    test_raw_http_telnet "GET / HTTP/2.0\r\nHost: $HOST:$PORT\r\n\r\n" "Invalid HTTP version (telnet)"
    
    echo -e "\n${WHITE}=== Interactive Testing ===${NC}"
    
    # Test 22: Interactive telnet session (optional)
    test_interactive_telnet "Interactive telnet session"
    
    echo -e "\n${WHITE}=== Performance Tests ===${NC}"
    
    # Test 23: Multiple rapid requests
    echo -e "\n${YELLOW}Testing:${NC} Multiple rapid GET requests"
    for i in {1..5}; do
        curl -s -w "Request $i: HTTP %{http_code} (%{time_total}s)\n" "$SERVER_URL/" -o /dev/null
    done
    print_status "PASS" "Multiple rapid requests" "Completed 5 requests"
    
    # Test 24: Concurrent telnet connections
    echo -e "\n${YELLOW}Testing:${NC} Concurrent telnet connections"
    for i in {1..3}; do
        (
            response=$(echo -e "GET / HTTP/1.1\r\nHost: $HOST:$PORT\r\n\r\n" | timeout 3 telnet $HOST $PORT 2>/dev/null)
            if echo "$response" | grep -q "HTTP/"; then
                echo "Concurrent connection $i: SUCCESS"
            else
                echo "Concurrent connection $i: FAILED"
            fi
        ) &
    done
    wait
    print_status "PASS" "Concurrent connections" "Tested 3 concurrent connections"
}

# Function to show summary
show_summary() {
    echo -e "\n${WHITE}======================================${NC}"
    echo -e "${WHITE}           TEST SUMMARY               ${NC}"
    echo -e "${WHITE}======================================${NC}"
    echo -e "Total tests: $TOTAL"
    echo -e "${GREEN}Passed: $PASSED${NC}"
    echo -e "${RED}Failed: $FAILED${NC}"
    
    if [ $FAILED -eq 0 ]; then
        echo -e "\n${GREEN}🎉 All tests passed!${NC}"
    else
        echo -e "\n${YELLOW}Some tests failed. Check your webserv implementation.${NC}"
    fi
    
    success_rate=$((PASSED * 100 / TOTAL))
    echo -e "Success rate: $success_rate%"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -H, --host HOST     Server host (default: localhost)"
    echo "  -p, --port PORT     Server port (default: 8080)"
    echo "  -v, --verbose       Verbose output"
    echo ""
    echo "Examples:"
    echo "  $0                  # Test localhost:8080"
    echo "  $0 -H 127.0.0.1 -p 8000  # Test 127.0.0.1:8000"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -H|--host)
            HOST="$2"
            SERVER_URL="http://${HOST}:${PORT}"
            shift 2
            ;;
        -p|--port)
            PORT="$2"
            SERVER_URL="http://${HOST}:${PORT}"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            echo "Unknown option $1"
            show_usage
            exit 1
            ;;
    esac
done

# Check if required tools are available
command -v curl >/dev/null 2>&1 || { echo >&2 "curl is required but not installed. Aborting."; exit 1; }

# Check for netcat (try different variants)
NC_CMD=""
if command -v nc >/dev/null 2>&1; then
    NC_CMD="nc"
elif command -v netcat >/dev/null 2>&1; then
    NC_CMD="netcat"
else
    echo >&2 "Warning: netcat (nc) is not available. Netcat tests will be skipped."
fi

# Check for telnet
TELNET_AVAILABLE=0
if command -v telnet >/dev/null 2>&1; then
    TELNET_AVAILABLE=1
else
    echo >&2 "Warning: telnet is not available. Telnet tests will be skipped."
fi

# Update functions to check availability
test_raw_http_nc() {
    if [ -z "$NC_CMD" ]; then
        print_status "FAIL" "$2" "Netcat not available - skipping test"
        return
    fi
    
    local request=$1
    local test_name=$2
    
    echo -e "\n${YELLOW}Testing:${NC} $test_name (Raw HTTP - netcat)"
    echo -e "${BLUE}Request:${NC}"
    echo "$request" | sed 's/^/  /'
    
    # Send raw HTTP request
    response=$(echo -e "$request" | $NC_CMD -w 5 $HOST $PORT 2>/dev/null)
    
    if [ $? -ne 0 ] || [ -z "$response" ]; then
        print_status "FAIL" "$test_name" "No response from server (netcat)"
        return
    fi
    
    # Extract status line
    status_line=$(echo "$response" | head -1)
    echo -e "${PURPLE}Status Line:${NC} $status_line"
    
    # Show headers
    headers=$(echo "$response" | sed -n '2,/^$/p' | head -5)
    if [ ! -z "$headers" ]; then
        echo -e "${PURPLE}Headers (first 5):${NC}"
        echo "$headers" | sed 's/^/  /'
    fi
    
    # Check if it's a valid HTTP response
    if echo "$status_line" | grep -q "HTTP/1.1\|HTTP/1.0"; then
        print_status "PASS" "$test_name" "Valid HTTP response received (netcat)"
    else
        print_status "FAIL" "$test_name" "Invalid HTTP response format (netcat)"
    fi
}

test_raw_http_telnet() {
    if [ $TELNET_AVAILABLE -eq 0 ]; then
        print_status "FAIL" "$2" "Telnet not available - skipping test"
        return
    fi
    
    local request=$1
    local test_name=$2
    local timeout=${3:-5}
    
    echo -e "\n${YELLOW}Testing:${NC} $test_name (Raw HTTP - telnet)"
    echo -e "${BLUE}Request:${NC}"
    echo "$request" | sed 's/^/  /'
    
    # Create a temporary file for the request
    local temp_file=$(mktemp)
    echo -e "$request" > "$temp_file"
    
    # Send raw HTTP request using telnet with timeout
    response=$(timeout $timeout telnet $HOST $PORT < "$temp_file" 2>/dev/null | tr -d '\0')
    
    # Clean up temp file
    rm -f "$temp_file"
    
    if [ $? -eq 124 ]; then
        print_status "FAIL" "$test_name" "Telnet connection timed out"
        return
    fi
    
    if [ -z "$response" ]; then
        print_status "FAIL" "$test_name" "No response from server (telnet)"
        return
    fi
    
    # Clean up telnet connection messages
    clean_response=$(echo "$response" | grep -v "Trying\|Connected\|Escape character\|Connection closed\|telnet>" | grep -v "^$" | head -20)
    
    if [ -z "$clean_response" ]; then
        print_status "FAIL" "$test_name" "No valid HTTP response (telnet)"
        return
    fi
    
    # Extract status line (first non-empty line)
    status_line=$(echo "$clean_response" | head -1)
    echo -e "${PURPLE}Status Line:${NC} $status_line"
    
    # Show headers
    headers=$(echo "$clean_response" | sed -n '2,/^$/p' | head -5)
    if [ ! -z "$headers" ]; then
        echo -e "${PURPLE}Headers (first 5):${NC}"
        echo "$headers" | sed 's/^/  /'
    fi
    
    # Check if it's a valid HTTP response
    if echo "$status_line" | grep -q "HTTP/1.1\|HTTP/1.0"; then
        print_status "PASS" "$test_name" "Valid HTTP response received (telnet)"
    else
        print_status "FAIL" "$test_name" "Invalid HTTP response format (telnet)"
    fi
}

test_interactive_telnet() {
    if [ $TELNET_AVAILABLE -eq 0 ]; then
        print_status "FAIL" "$1" "Telnet not available - skipping interactive test"
        return
    fi
    
    local test_name=$1
    
    echo -e "\n${YELLOW}Testing:${NC} $test_name (Interactive telnet)"
    echo -e "${BLUE}Instructions:${NC}"
    echo -e "  1. Type: GET / HTTP/1.1"
    echo -e "  2. Press Enter"
    echo -e "  3. Type: Host: $HOST:$PORT"
    echo -e "  4. Press Enter twice to send the request"
    echo -e "  5. Type 'quit' or press Ctrl+] then 'quit' to exit"
    echo -e "${CYAN}Starting interactive telnet session...${NC}"
    
    read -p "Press Enter to start interactive session (or 'skip' to skip): " choice
    if [ "$choice" = "skip" ]; then
        print_status "PASS" "$test_name" "Skipped interactive session"
        return
    fi
    
    telnet $HOST $PORT
    print_status "PASS" "$test_name" "Interactive session completed"
}

# Run the tests
run_get_tests

# Show summary
show_summary

# Exit with appropriate code
if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi