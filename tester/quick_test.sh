#!/bin/bash

# Quick HTTP Server Test Script
# Fast basic functionality test for ./webserv nginx1.conf

SERVER_URL="http://127.0.0.1:8080"

echo "🔥 Quick HTTP Server Test"
echo "Testing: $SERVER_URL"
echo "=========================="

# Test 1: Basic connectivity
echo "1. Testing basic connectivity..."
if curl -s --connect-timeout 3 "$SERVER_URL" > /dev/null; then
    echo "✅ Server is responding"
else
    echo "❌ Server not responding - is ./webserv nginx1.conf running?"
    exit 1
fi

# Test 2: GET request
echo "2. Testing GET request..."
response=$(curl -s -w "STATUS:%{http_code}" "$SERVER_URL/")
if echo "$response" | grep -q "STATUS:200"; then
    echo "✅ GET / returns 200 OK"
else
    echo "❌ GET / failed"
    echo "Response: $response"
fi

# Test 3: POST request
echo "3. Testing POST request..."
response=$(curl -s -X POST -w "STATUS:%{http_code}" -d "test=data" "$SERVER_URL/")
if echo "$response" | grep -q "STATUS:200"; then
    echo "✅ POST request successful"
else
    echo "❌ POST request failed"
    echo "Response: $response"
fi

# Test 4: 404 handling
echo "4. Testing 404 error handling..."
response=$(curl -s -w "STATUS:%{http_code}" "$SERVER_URL/nonexistent")
if echo "$response" | grep -q "STATUS:404"; then
    echo "✅ 404 handling works"
else
    echo "❌ 404 handling failed"
fi

# Test 5: HTTP headers
echo "5. Testing HTTP headers..."
headers=$(curl -s -I "$SERVER_URL/")
if echo "$headers" | grep -q "HTTP/1.1 200"; then
    echo "✅ HTTP/1.1 protocol working"
else
    echo "❌ HTTP/1.1 headers issue"
fi

# Test 6: Multiple requests (keep-alive test)
echo "6. Testing multiple requests..."
success=0
for i in {1..3}; do
    if curl -s --max-time 2 "$SERVER_URL/" > /dev/null; then
        ((success++))
    fi
done

if [ $success -eq 3 ]; then
    echo "✅ Multiple requests successful ($success/3)"
else
    echo "⚠️  Some requests failed ($success/3)"
fi

echo ""
echo "🎯 Quick test completed!"
echo "For comprehensive testing, run: ./test_webserv.sh"