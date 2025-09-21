#!/bin/bash

echo "=== HTTP 错误码全面测试 ==="
echo "测试服务器的所有ValidationResult错误处理能力"
echo

# 启动服务器
./server &
SERVER_PID=$!
sleep 2

# 计数器
TOTAL_TESTS=0
PASSED_TESTS=0

# 测试函数
test_http_request() {
    local test_name="$1"
    local request="$2"
    local expected_status="$3"
    local description="$4"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo "=========================================="
    echo "测试 $TOTAL_TESTS: $test_name"
    echo "描述: $description"
    echo "预期状态码: $expected_status"
    echo "请求内容:"
    echo "$request"
    echo "------------------------------------------"
    
    # 发送请求并捕获响应
    response=$(printf "%b" "$request" | nc -w 5 localhost 8080 2>/dev/null)
    
    if [ -n "$response" ]; then
        # 提取状态码
        status_line=$(echo "$response" | head -1)
        actual_status=$(echo "$status_line" | grep -o '[0-9]\{3\}')
        
        echo "实际响应状态行: $status_line"
        echo "实际状态码: $actual_status"
        
        if [ "$actual_status" = "$expected_status" ]; then
            echo "✅ 测试通过"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo "❌ 测试失败 - 期望 $expected_status，得到 $actual_status"
        fi
    else
        echo "❌ 测试失败 - 无响应"
    fi
    
    echo
    sleep 0.5  # 避免连接过快
}

# 1. 测试有效请求 (200)
test_http_request "有效GET请求" \
    "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "200" \
    "正常的GET请求，应返回200 OK"

# 2. 测试404错误
test_http_request "文件不存在" \
    "GET /nonexistent.html HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "404" \
    "请求不存在的文件，应返回404 Not Found"

# 3. 测试400系列错误 - 无效请求行
test_http_request "无效HTTP请求行" \
    "INVALID_REQUEST_LINE\r\n\r\n" \
    "400" \
    "完全无效的请求行格式"

test_http_request "缺少HTTP版本" \
    "GET /\r\nHost: localhost\r\n\r\n" \
    "400" \
    "请求行缺少HTTP版本"

test_http_request "请求行参数过少" \
    "GET\r\nHost: localhost\r\n\r\n" \
    "400" \
    "请求行只有方法，缺少URI和版本"

# 4. 测试无效HTTP版本
test_http_request "无效HTTP版本" \
    "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n" \
    "400" \
    "使用HTTP/1.0而非HTTP/1.1"

test_http_request "错误HTTP版本格式" \
    "GET / INVALID_VERSION\r\nHost: localhost\r\n\r\n" \
    "400" \
    "完全错误的HTTP版本格式"

# 5. 测试无效URI
test_http_request "URI路径遍历攻击" \
    "GET /../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "400" \
    "包含路径遍历的危险URI"

test_http_request "URI包含空字节" \
    "GET /test\x00/file HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "400" \
    "URI包含控制字符"

# 6. 测试过长URI
long_uri=$(printf "%0*d" 9000 0 | sed 's/0/a/g')
test_http_request "URI过长" \
    "GET /$long_uri HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "414" \
    "URI超过最大长度限制(2048字符)"

# 7. 测试缺少Host头部
test_http_request "缺少Host头部" \
    "GET / HTTP/1.1\r\n\r\n" \
    "400" \
    "HTTP/1.1要求必须有Host头部"

# 8. 测试无效方法
test_http_request "不支持的HTTP方法" \
    "PATCH / HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "405" \
    "服务器不支持PATCH方法"

test_http_request "自定义无效方法" \
    "INVALID_METHOD / HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "405" \
    "完全无效的HTTP方法"

# 9. 测试GET方法带body (方法与body不匹配)
test_http_request "GET请求带body" \
    "GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\nhello body" \
    "400" \
    "GET请求不应该有请求体"

# 10. 测试POST无Content-Length
test_http_request "POST缺少Content-Length" \
    "POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\nhello" \
    "411" \
    "POST请求必须有Content-Length头部"

# 11. 测试Content-Length不匹配
test_http_request "Content-Length不匹配" \
    "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 20\r\n\r\nhello" \
    "400" \
    "Content-Length值与实际body长度不符"

# 12. 测试无效Content-Length
test_http_request "无效Content-Length格式" \
    "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: abc\r\n\r\n" \
    "400" \
    "Content-Length不是有效数字"

test_http_request "负数Content-Length" \
    "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: -10\r\n\r\n" \
    "400" \
    "Content-Length不能为负数"

# 13. 测试冲突的头部
test_http_request "Transfer-Encoding与Content-Length冲突" \
    "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\nhello" \
    "400" \
    "不能同时使用Transfer-Encoding和Content-Length"

# 14. 测试过大payload
large_body=$(printf "%0*d" 11000000 0)  # 大于10MB
test_http_request "Payload过大" \
    "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11000000\r\n\r\n$large_body" \
    "413" \
    "请求体超过最大允许大小"

# 15. 测试过大头部
large_header_value=$(printf "%0*d" 9000 0)
test_http_request "头部过大" \
    "GET / HTTP/1.1\r\nHost: localhost\r\nCustom-Header: $large_header_value\r\n\r\n" \
    "431" \
    "单个头部行超过8KB限制"

# 16. 测试过多头部
many_headers=""
for i in $(seq 1 150); do
    many_headers="${many_headers}Header$i: value$i\r\n"
done
test_http_request "头部数量过多" \
    "GET / HTTP/1.1\r\nHost: localhost\r\n${many_headers}\r\n" \
    "431" \
    "头部数量超过100个限制"

# 17. 测试无效头部格式
test_http_request "头部缺少冒号" \
    "GET / HTTP/1.1\r\nHost localhost\r\nInvalid-Header-No-Colon\r\n\r\n" \
    "400" \
    "头部行格式错误，缺少冒号"

test_http_request "头部名称为空" \
    "GET / HTTP/1.1\r\nHost: localhost\r\n: empty-name\r\n\r\n" \
    "400" \
    "头部名称不能为空"

# 18. 测试chunked编码（如果支持）
test_http_request "无效chunked编码" \
    "POST /submit HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\nZ\r\nworld\r\n0\r\n\r\n" \
    "400" \
    "chunked编码格式错误"

# 19. 测试请求总大小过大
test_http_request "请求总大小过大" \
    "GET /$long_uri HTTP/1.1\r\nHost: localhost\r\nCustom-Header: $large_header_value\r\n\r\n" \
    "414" \
    "整个请求超过8MB限制"

# 20. 测试特殊字符处理
test_http_request "URI包含中文" \
    "GET /中文.html HTTP/1.1\r\nHost: localhost\r\n\r\n" \
    "400" \
    "URI包含非ASCII字符"

# 停止服务器
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

# 统计结果
echo "=========================================="
echo "测试完成！"
echo "总测试数: $TOTAL_TESTS"
echo "通过测试: $PASSED_TESTS"
echo "失败测试: $((TOTAL_TESTS - PASSED_TESTS))"
if [ $TOTAL_TESTS -gt 0 ]; then
    success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    echo "成功率: ${success_rate}%"
fi

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo "🎉 所有测试通过！"
    exit 0
else
    echo "⚠️ 有测试失败，请检查实现"
    exit 1
fi