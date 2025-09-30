#!/bin/bash

# =============================================================================
# 42 WebServ - CGI 全面测试套件
# =============================================================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 服务器配置
SERVER_HOST="localhost"
SERVER_PORT="8080"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"

# 日志文件
TEST_LOG="cgi_test_results.log"
ERROR_LOG="cgi_test_errors.log"

# 清理旧日志
> "$TEST_LOG"
> "$ERROR_LOG"

print_header() {
    echo -e "\n${CYAN}===========================================${NC}"
    echo -e "${CYAN} 42 WebServ - CGI Comprehensive Test Suite${NC}"
    echo -e "${CYAN}===========================================${NC}\n"
}

print_test_header() {
    echo -e "\n${BLUE}📋 Test Category: $1${NC}"
    echo -e "${BLUE}------------------------------------------${NC}"
}

print_test() {
    echo -e "\n${YELLOW}🧪 Test: $1${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

print_pass() {
    echo -e "${GREEN}✅ PASS: $1${NC}"
    PASSED_TESTS=$((PASSED_TESTS + 1))
    echo "$(date): PASS - $1" >> "$TEST_LOG"
}

print_fail() {
    echo -e "${RED}❌ FAIL: $1${NC}"
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo "$(date): FAIL - $1" >> "$TEST_LOG"
    echo "$(date): FAIL - $1 - Details: $2" >> "$ERROR_LOG"
}

print_info() {
    echo -e "${PURPLE}ℹ️  INFO: $1${NC}"
}

check_server() {
    print_test "Server Availability Check"
    response=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/" 2>/dev/null)
    if [ "$response" = "200" ] || [ "$response" = "404" ]; then
        print_pass "Server is responding on $BASE_URL"
        return 0
    else
        print_fail "Server not responding on $BASE_URL" "HTTP code: $response"
        echo -e "\n${RED}❌ Cannot proceed with tests - server not available${NC}"
        echo -e "${YELLOW}💡 Please start your webserv with: ./webserv nginx1.conf${NC}\n"
        exit 1
    fi
}

test_basic_cgi() {
    print_test "Basic CGI Functionality"
    response=$(curl -s "$BASE_URL/cgi_basic.py")
    http_code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_basic.py")

    if [ "$http_code" = "200" ] && echo "$response" | grep -q "Basic CGI Test - SUCCESS"; then
        print_pass "Basic CGI execution works"
    else
        print_fail "Basic CGI test failed" "HTTP: $http_code, Content check failed"
    fi

    # 检查CGI环境变量
    if echo "$response" | grep -q "REQUEST_METHOD"; then
        print_pass "CGI environment variables are set"
    else
        print_fail "CGI environment variables missing"
    fi
}

test_get_request() {
    print_test "GET Request with Query Parameters"
    response=$(curl -s "$BASE_URL/cgi_basic.py?param1=hello&param2=world")
    http_code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_basic.py?param1=hello&param2=world")

    if [ "$http_code" = "200" ] && echo "$response" | grep -q "param1=hello"; then
        print_pass "GET request with query parameters works"
    else
        print_fail "GET request with query parameters failed" "HTTP: $http_code"
    fi
}

test_post_request() {
    print_test "POST Request with Form Data"
    response=$(curl -s -X POST -d "name=TestUser&email=test@example.com&message=Hello+CGI" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        "$BASE_URL/cgi_post_test.py")
    http_code=$(curl -s -X POST -d "name=TestUser&email=test@example.com" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_post_test.py")

    if [ "$http_code" = "200" ] && echo "$response" | grep -q "POST data received successfully"; then
        print_pass "POST request with form data works"
    else
        print_fail "POST request failed" "HTTP: $http_code"
    fi

    # 测试POST数据解析
    if echo "$response" | grep -q "TestUser"; then
        print_pass "POST data parsing works"
    else
        print_fail "POST data parsing failed"
    fi
}

test_large_post() {
    print_test "Large POST Data"
    # 创建大量数据 (约10KB)
    large_data=$(python3 -c "print('data=' + 'A' * 10000)")
    response=$(curl -s -X POST -d "$large_data" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        "$BASE_URL/cgi_post_test.py")
    http_code=$(curl -s -X POST -d "$large_data" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_post_test.py")

    if [ "$http_code" = "200" ]; then
        print_pass "Large POST data handling works"
    else
        print_fail "Large POST data failed" "HTTP: $http_code"
    fi
}

test_file_upload() {
    print_test "File Upload (multipart/form-data)"
    # 创建临时测试文件
    temp_file=$(mktemp)
    echo "This is a test file for CGI upload testing." > "$temp_file"

    response=$(curl -s -F "username=TestUser" \
        -F "upload_file=@$temp_file" \
        -F "description=Test file upload" \
        "$BASE_URL/cgi_upload_test.py")
    http_code=$(curl -s -F "upload_file=@$temp_file" \
        -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_upload_test.py")

    if [ "$http_code" = "200" ] && echo "$response" | grep -q "Multipart Data Received"; then
        print_pass "File upload (multipart) works"
    else
        print_fail "File upload failed" "HTTP: $http_code"
    fi

    rm -f "$temp_file"
}

test_custom_status_codes() {
    print_test "Custom Status Codes"
    http_code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_error_test.py?type=custom_status")

    if [ "$http_code" = "418" ]; then
        print_pass "Custom status code (418) works"
    else
        print_fail "Custom status code failed" "Expected: 418, Got: $http_code"
    fi
}

test_redirect() {
    print_test "CGI Redirect"
    # 使用-i 获取headers，-L不跟随重定向以测试重定向响应
    response=$(curl -s -i "$BASE_URL/cgi_error_test.py?type=redirect")

    if echo "$response" | grep -q "302 Found" && echo "$response" | grep -q "Location:"; then
        print_pass "CGI redirect works"
    else
        print_fail "CGI redirect failed"
    fi
}

test_error_conditions() {
    print_test "CGI Error Exit Code"
    http_code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_error_test.py?type=exit_error")

    # 应该返回502 Bad Gateway当CGI脚本失败时
    if [ "$http_code" = "502" ]; then
        print_pass "CGI error exit code handling works"
    else
        print_fail "CGI error handling failed" "Expected: 502, Got: $http_code"
    fi
}

test_timeout() {
    print_test "CGI Timeout Handling"
    print_info "Testing CGI timeout (this may take up to 30 seconds)..."

    start_time=$(date +%s)
    http_code=$(timeout 35s curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_error_test.py?type=timeout")
    end_time=$(date +%s)
    duration=$((end_time - start_time))

    if [ "$http_code" = "502" ] || [ "$http_code" = "504" ] && [ "$duration" -lt 35 ]; then
        print_pass "CGI timeout handling works (took ${duration}s)"
    else
        print_fail "CGI timeout test failed" "HTTP: $http_code, Duration: ${duration}s"
    fi
}

test_chunked_output() {
    print_test "Chunked Output"
    response=$(curl -s "$BASE_URL/cgi_chunked_test.py")
    http_code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_chunked_test.py")

    if [ "$http_code" = "200" ] && echo "$response" | grep -q "Test Complete"; then
        print_pass "Chunked output works"
    else
        print_fail "Chunked output failed" "HTTP: $http_code"
    fi
}

test_concurrent_requests() {
    print_test "Concurrent CGI Requests"
    print_info "Running 5 concurrent requests..."

    # 启动5个并发请求
    for i in {1..5}; do
        curl -s "$BASE_URL/cgi_basic.py?request=$i" > "/tmp/cgi_concurrent_$i.log" &
    done

    # 等待所有请求完成
    wait

    # 检查结果
    success_count=0
    for i in {1..5}; do
        if grep -q "Basic CGI Test - SUCCESS" "/tmp/cgi_concurrent_$i.log" 2>/dev/null; then
            success_count=$((success_count + 1))
        fi
        rm -f "/tmp/cgi_concurrent_$i.log"
    done

    if [ "$success_count" -eq 5 ]; then
        print_pass "Concurrent CGI requests work ($success_count/5)"
    else
        print_fail "Concurrent requests failed" "Only $success_count/5 succeeded"
    fi
}

test_special_characters() {
    print_test "Special Characters in URLs"
    # URL编码的特殊字符
    encoded_url="$BASE_URL/cgi_basic.py?test=%E2%9C%85%20special%20chars%20%26%20symbols"
    http_code=$(curl -s -o /dev/null -w "%{http_code}" "$encoded_url")

    if [ "$http_code" = "200" ]; then
        print_pass "Special characters in URLs work"
    else
        print_fail "Special characters test failed" "HTTP: $http_code"
    fi
}

test_empty_post() {
    print_test "Empty POST Request"
    response=$(curl -s -X POST -d "" "$BASE_URL/cgi_post_test.py")
    http_code=$(curl -s -X POST -d "" -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_post_test.py")

    if [ "$http_code" = "200" ]; then
        print_pass "Empty POST request works"
    else
        print_fail "Empty POST request failed" "HTTP: $http_code"
    fi
}

run_stress_test() {
    print_test "CGI Stress Test"
    print_info "Running 20 rapid requests..."

    failed_requests=0
    for i in {1..20}; do
        http_code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi_basic.py?stress_test=$i" 2>/dev/null)
        if [ "$http_code" != "200" ]; then
            failed_requests=$((failed_requests + 1))
        fi
    done

    success_rate=$(( (20 - failed_requests) * 100 / 20 ))
    if [ "$failed_requests" -lt 3 ]; then  # 允许少量失败
        print_pass "Stress test passed (${success_rate}% success rate)"
    else
        print_fail "Stress test failed" "${failed_requests}/20 requests failed"
    fi
}

print_summary() {
    echo -e "\n${CYAN}===========================================${NC}"
    echo -e "${CYAN}           TEST SUMMARY${NC}"
    echo -e "${CYAN}===========================================${NC}"
    echo -e "Total Tests: ${TOTAL_TESTS}"
    echo -e "${GREEN}Passed: ${PASSED_TESTS}${NC}"
    echo -e "${RED}Failed: ${FAILED_TESTS}${NC}"

    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}🎉 ALL TESTS PASSED! Your CGI implementation is working perfectly!${NC}"
    else
        echo -e "\n${YELLOW}⚠️  Some tests failed. Check the logs for details:${NC}"
        echo -e "  - Test log: ${TEST_LOG}"
        echo -e "  - Error log: ${ERROR_LOG}"
    fi

    success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    echo -e "\n${BLUE}Success Rate: ${success_rate}%${NC}\n"
}

# =============================================================================
# 主测试执行
# =============================================================================

main() {
    print_header

    # 给脚本执行权限
    chmod +x www/cgi_*.py 2>/dev/null

    # 检查服务器可用性
    check_server

    # 基础功能测试
    print_test_header "Basic Functionality Tests"
    test_basic_cgi
    test_get_request

    # POST测试
    print_test_header "POST Request Tests"
    test_post_request
    test_large_post
    test_empty_post

    # 文件上传测试
    print_test_header "File Upload Tests"
    test_file_upload

    # HTTP功能测试
    print_test_header "HTTP Features Tests"
    test_custom_status_codes
    test_redirect

    # 错误处理测试
    print_test_header "Error Handling Tests"
    test_error_conditions
    test_timeout

    # 高级功能测试
    print_test_header "Advanced Features Tests"
    test_chunked_output
    test_concurrent_requests

    # 边界情况测试
    print_test_header "Edge Cases Tests"
    test_special_characters

    # 性能测试
    print_test_header "Performance Tests"
    run_stress_test

    # 打印测试总结
    print_summary
}

# 执行主函数
main "$@"