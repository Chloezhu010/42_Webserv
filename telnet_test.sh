#!/bin/bash

echo "=== 详细的Telnet风格HTTP测试 ==="

# 启动服务器
./webserv1 &
SERVER_PID=$!
sleep 2

# 函数：发送HTTP请求并显示响应
send_http_request() {
    local request="$1"
    local test_name="$2"
    
    echo -e "\n📤 发送请求: $test_name"
    echo "   $request"
    echo "📥 服务器响应:"
    echo "----------------------------------------"
    
    # 使用echo和nc模拟telnet输入
    echo -e "$request" | nc -w 3 localhost 8080
    
    echo "----------------------------------------"
}

# 测试不同的HTTP请求
send_http_request "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" "正常首页请求"

send_http_request "GET /index1.html HTTP/1.1\r\nHost: example.com\r\n\r\n" "404错误页面(你遇到的问题)"

send_http_request "GET /about.html HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n" "about页面请求"

send_http_request "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 13\r\n\r\nname=test&age=25" "POST请求测试"

send_http_request "DELETE /test HTTP/1.1\r\nHost: localhost\r\n\r\n" "DELETE请求测试"

send_http_request "INVALID METHOD\r\n\r\n" "无效HTTP请求"

# 停止服务器
kill $SERVER_PID 2>/dev/null

echo -e "\n=== 测试完成 ==="