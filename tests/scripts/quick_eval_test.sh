#!/bin/bash

# ============================================================================
# QUICK EVALUATION TEST
# Fast sanity check before full evaluation
# ============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

SERVER="localhost:8080"
PASS=0
FAIL=0

test() {
    local name=$1
    local cmd=$2
    local expect=$3

    echo -n "Testing: $name... "
    result=$(eval $cmd 2>&1)

    if echo "$result" | grep -q "$expect"; then
        echo -e "${GREEN}✓${NC}"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} (Expected: $expect, Got: $(echo $result | head -n1))"
        ((FAIL++))
    fi
}

echo "========================================="
echo "  WEBSERV QUICK EVALUATION TEST"
echo "========================================="
echo ""

# Basic HTTP Methods
test "GET request" "curl -s -i http://$SERVER/" "HTTP/1.1 200"
test "POST request" "curl -s -i -X POST -d 'test' http://$SERVER/" "HTTP/1.1 200\|201"
test "DELETE request" "curl -s -i -X DELETE http://$SERVER/test.txt" "HTTP/1.1"
test "404 Not Found" "curl -s -i http://$SERVER/nonexistent" "HTTP/1.1 404"

# Method validation
test "405 Method Not Allowed" "curl -s -i -X PATCH http://$SERVER/" "HTTP/1.1 405"
test "411 Length Required" "printf 'POST / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc localhost 8080" "HTTP/1.1 411"

# URI validation
test "414 URI Too Long" "curl -s -i http://$SERVER/\$(python3 -c 'print(\"a\"*3000)')" "HTTP/1.1 414"
test "400 Path Traversal" "curl -s -i http://$SERVER/../../../etc/passwd" "HTTP/1.1 400"

# Header validation
test "400 Missing Host (HTTP/1.1)" "printf 'GET / HTTP/1.1\r\n\r\n' | nc localhost 8080" "HTTP/1.1 400"
test "505 HTTP Version" "printf 'GET / HTTP/2.0\r\nHost: localhost\r\n\r\n' | nc localhost 8080" "HTTP/1.1 505"

# Keep-Alive
test "Keep-Alive connection" "{ printf 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n'; sleep 0.5; printf 'GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n'; } | nc localhost 8080 | grep -c 'HTTP/1.1 200'" "2"

# CGI (if exists)
if [ -f "www/cgi-bin/test.py" ]; then
    chmod +x www/cgi-bin/test.py
    test "CGI Python script" "curl -s -i http://$SERVER/cgi-bin/test.py" "HTTP/1.1 200"
fi

# No crash tests
echo -n "Testing: Malformed request (no crash)... "
printf "GARBAGE\r\n\r\n" | nc localhost 8080 > /dev/null 2>&1
sleep 1
if lsof -Pi :8080 -sTCP:LISTEN -t >/dev/null 2>&1; then
    echo -e "${GREEN}✓${NC}"
    ((PASS++))
else
    echo -e "${RED}✗ (Server crashed)${NC}"
    ((FAIL++))
fi

echo ""
echo "========================================="
echo "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo "========================================="

[ $FAIL -eq 0 ] && exit 0 || exit 1
