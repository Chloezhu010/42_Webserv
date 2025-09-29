# Telnet CGI 手动测试指南

## 🎯 测试目标
使用 telnet 手动测试 WebServ 的 CGI 功能，验证所有关键场景和边界情况。

## 📋 准备工作
1. 启动你的 WebServ 服务器：`./webserv nginx1.conf`
2. 确保所有 CGI 脚本有执行权限：`chmod +x www/cgi_*.py`
3. 开启新的终端窗口进行 telnet 测试

## 🔧 基础 Telnet 命令格式

### 连接服务器
```bash
telnet localhost 8080
```

### HTTP 请求格式模板
```
METHOD /path HTTP/1.1
Host: localhost:8080
[其他头部]

[请求体数据]
```

## 📚 测试用例

### 1. 基础 CGI GET 请求
```bash
telnet localhost 8080

# 输入以下内容：
GET /cgi_basic.py HTTP/1.1
Host: localhost:8080

# 按两次回车发送请求
```

**期望结果：**
- HTTP/1.1 200 OK
- Content-Type: text/html
- 包含 "Basic CGI Test - SUCCESS"

### 2. CGI GET 请求带查询参数
```bash
telnet localhost 8080

GET /cgi_basic.py?name=test&value=123 HTTP/1.1
Host: localhost:8080

```

**期望结果：**
- 200 状态码
- 显示查询参数：name=test, value=123

### 3. CGI POST 请求（表单数据）
```bash
telnet localhost 8080

POST /cgi_post_test.py HTTP/1.1
Host: localhost:8080
Content-Type: application/x-www-form-urlencoded
Content-Length: 42

name=TestUser&email=test@example.com
```

**期望结果：**
- 200 状态码
- 显示 "POST data received successfully"
- 解析的表单数据

### 4. CGI POST 请求（大数据）
```bash
telnet localhost 8080

POST /cgi_post_test.py HTTP/1.1
Host: localhost:8080
Content-Type: application/x-www-form-urlencoded
Content-Length: 10010

data=AAAAAAAA...(重复10000个A)
```

**测试方法：**
```bash
# 先生成大数据
echo -n "data=" > large_post.txt
python3 -c "print('A' * 10000)" >> large_post.txt

# 计算长度
wc -c large_post.txt

# 手动输入或使用脚本
telnet localhost 8080
```

### 5. CGI 文件上传（Multipart）
```bash
telnet localhost 8080

POST /cgi_upload_test.py HTTP/1.1
Host: localhost:8080
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary
Content-Length: 245

------WebKitFormBoundary
Content-Disposition: form-data; name="username"

TestUser
------WebKitFormBoundary
Content-Disposition: form-data; name="upload_file"; filename="test.txt"
Content-Type: text/plain

Hello CGI Upload!
------WebKitFormBoundary--
```

### 6. CGI 自定义状态码测试
```bash
telnet localhost 8080

GET /cgi_error_test.py?type=custom_status HTTP/1.1
Host: localhost:8080

```

**期望结果：**
- HTTP/1.1 418 I'm a teapot

### 7. CGI 重定向测试
```bash
telnet localhost 8080

GET /cgi_error_test.py?type=redirect HTTP/1.1
Host: localhost:8080

```

**期望结果：**
- HTTP/1.1 302 Found
- Location: /

### 8. CGI 错误退出测试
```bash
telnet localhost 8080

GET /cgi_error_test.py?type=exit_error HTTP/1.1
Host: localhost:8080

```

**期望结果：**
- HTTP/1.1 502 Bad Gateway

### 9. CGI 超时测试
```bash
telnet localhost 8080

GET /cgi_error_test.py?type=timeout HTTP/1.1
Host: localhost:8080

```

**期望结果：**
- 服务器应在超时时间内返回 502 或 504

### 10. CGI 分块输出测试
```bash
telnet localhost 8080

GET /cgi_chunked_test.py HTTP/1.1
Host: localhost:8080

```

**期望结果：**
- 渐进式输出多个数据块
- 最终显示 "Test Complete"

## 🧪 边界情况测试

### 11. 空 POST 数据
```bash
telnet localhost 8080

POST /cgi_post_test.py HTTP/1.1
Host: localhost:8080
Content-Type: application/x-www-form-urlencoded
Content-Length: 0

```

### 12. 无 Content-Type 的 POST
```bash
telnet localhost 8080

POST /cgi_post_test.py HTTP/1.1
Host: localhost:8080
Content-Length: 13

test=noheader
```

### 13. 特殊字符 URL 编码
```bash
telnet localhost 8080

GET /cgi_basic.py?test=%E2%9C%85%20special%20chars%20%26%20symbols HTTP/1.1
Host: localhost:8080

```

### 14. 长 URL 测试
```bash
telnet localhost 8080

GET /cgi_basic.py?very_long_parameter=AAAA...(重复1000个A) HTTP/1.1
Host: localhost:8080

```

### 15. 无效 CGI 脚本路径
```bash
telnet localhost 8080

GET /nonexistent_cgi.py HTTP/1.1
Host: localhost:8080

```

**期望结果：** 404 Not Found

### 16. 非可执行文件的 CGI 请求
```bash
# 先创建不可执行的文件
echo "#!/usr/bin/env python3\nprint('test')" > www/no_exec.py

telnet localhost 8080

GET /no_exec.py HTTP/1.1
Host: localhost:8080

```

## 📊 测试检查清单

### HTTP 协议合规性
- [ ] 正确的状态行格式
- [ ] 必需的头部字段
- [ ] 正确的 Content-Length
- [ ] 适当的 Connection 处理

### CGI 环境变量
验证以下变量是否正确传递：
- [ ] REQUEST_METHOD
- [ ] QUERY_STRING
- [ ] CONTENT_LENGTH
- [ ] CONTENT_TYPE
- [ ] SCRIPT_NAME
- [ ] PATH_INFO
- [ ] SERVER_NAME
- [ ] SERVER_PORT

### 错误处理
- [ ] CGI 脚本不存在时返回 404
- [ ] CGI 脚本执行失败时返回 502
- [ ] CGI 超时时正确处理
- [ ] 大数据请求的内存管理

### 性能和稳定性
- [ ] 多个连续请求不崩溃
- [ ] 内存泄漏检查
- [ ] 并发请求处理
- [ ] 长时间运行稳定性

## 🔍 调试提示

### 常见问题排查
1. **连接被拒绝**：检查服务器是否运行在正确端口
2. **502 错误**：检查 CGI 脚本权限和路径
3. **超时**：检查服务器的超时设置
4. **数据截断**：验证 Content-Length 正确性

### 日志分析
监控服务器日志输出，关注：
- CGI 进程创建/销毁
- 错误消息
- 性能指标

### 工具辅助
```bash
# 监控进程
ps aux | grep python

# 监控网络
netstat -an | grep :8080

# 监控文件描述符
lsof -p <webserv_pid>
```

## 📝 测试报告模板

为每个测试用例记录：
- **测试用例编号**：
- **输入**：完整的 telnet 命令
- **期望输出**：预期的 HTTP 响应
- **实际输出**：服务器实际响应
- **结果**：通过/失败
- **备注**：任何异常或观察

这样可以系统地验证你的 CGI 实现的正确性和鲁棒性。