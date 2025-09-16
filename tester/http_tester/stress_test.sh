#!/bin/bash

echo "=== HTTP响应性能测试 ==="

# 启动服务器
./webserv &
SERVER_PID=$!
sleep 2

# 使用ab进行压力测试
echo "并发测试 - 100个请求，10个并发"
ab -n 100 -c 10 http://localhost:8080/

echo -e "\n错误页面测试"
ab -n 50 -c 5 http://localhost:8080/404test

# 停止服务器
kill $SERVER_PID