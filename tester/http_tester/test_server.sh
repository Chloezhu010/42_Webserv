#!/bin/bash

echo "=== 启动服务器测试 ==="

# 确保服务器在后台运行
./webserv &
SERVER_PID=$!

# 等待服务器启动
sleep 2

echo "服务器PID: $SERVER_PID"

# 测试1：基本GET请求
echo "测试1：基本GET请求"
curl -i http://localhost:8080/

echo -e "\n测试2：404错误页面"
curl -i http://localhost:8080/nonexistent.html

echo -e "\n测试3：POST请求（如果支持）"
curl -i -X POST http://localhost:8080/ -d "test=data"

echo -e "\n测试4：不同文件类型"
curl -i http://localhost:8080/about.html

echo -e "\n测试5：无效HTTP请求"
echo -e "INVALID REQUEST\r\n\r\n" | nc localhost 8080

echo -e "\n测试6：HTTP/1.1持久连接"
curl -i -H "Connection: keep-alive" http://localhost:8080/

# 停止服务器
kill $SERVER_PID
echo -e "\n=== 测试完成 ==="