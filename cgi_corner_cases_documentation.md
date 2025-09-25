# CGI Corner Cases 测试文档

## 🎯 目标
全面测试 CGI 实现中的各种边界情况和异常场景，确保服务器的鲁棒性。

## 📋 Corner Cases 分类

### 1. 输入验证 Corner Cases

#### 1.1 URL 路径边界情况
- **空路径**: `/`
- **超长路径**: `/cgi_basic.py` + 8KB 查询参数
- **路径遍历攻击**: `/../../etc/passwd`
- **特殊字符路径**: `/cgi%2Ebasic%2Epy`
- **Unicode 字符**: `/测试.py`
- **NULL 字节注入**: `/cgi_basic.py%00.txt`

#### 1.2 查询字符串边界情况
```bash
# 空查询字符串
GET /cgi_basic.py? HTTP/1.1

# 重复参数
GET /cgi_basic.py?param=value1&param=value2 HTTP/1.1

# 无值参数
GET /cgi_basic.py?flag&other=value HTTP/1.1

# 特殊字符未编码
GET /cgi_basic.py?test=hello world&符号=测试 HTTP/1.1

# 超长单个参数值 (8KB)
GET /cgi_basic.py?data=AAAA... HTTP/1.1

# 大量参数 (1000个)
GET /cgi_basic.py?param1=1&param2=2&...&param1000=1000 HTTP/1.1
```

### 2. HTTP Headers Corner Cases

#### 2.1 Content-Length 边界情况
```bash
# Content-Length 为 0 但有数据
POST /cgi_post_test.py HTTP/1.1
Content-Type: application/x-www-form-urlencoded
Content-Length: 0

data=should_not_be_read

# Content-Length 大于实际数据
POST /cgi_post_test.py HTTP/1.1
Content-Length: 1000

small_data

# Content-Length 小于实际数据
POST /cgi_post_test.py HTTP/1.1
Content-Length: 5

this_is_much_longer_data

# 负数 Content-Length
POST /cgi_post_test.py HTTP/1.1
Content-Length: -1

# 非数字 Content-Length
POST /cgi_post_test.py HTTP/1.1
Content-Length: invalid
```

#### 2.2 Content-Type 边界情况
```bash
# 缺少 Content-Type
POST /cgi_post_test.py HTTP/1.1
Content-Length: 13

name=TestUser

# 无效的 Content-Type
POST /cgi_post_test.py HTTP/1.1
Content-Type: invalid/type
Content-Length: 13

name=TestUser

# 复杂的 multipart boundary
POST /cgi_upload_test.py HTTP/1.1
Content-Type: multipart/form-data; boundary="----=_Part_0_123456789.987654321"

# Charset 参数
POST /cgi_post_test.py HTTP/1.1
Content-Type: application/x-www-form-urlencoded; charset=UTF-8
```

### 3. POST 数据 Corner Cases

#### 3.1 数据大小边界
- **0 字节数据**
- **1 字节数据**
- **正好达到服务器限制的数据**
- **超过服务器限制的数据** (如 1MB)
- **分段到达的数据**

#### 3.2 数据格式边界
```python
# 创建特殊测试脚本
#!/usr/bin/env python3
# www/cgi_corner_test.py

import os
import sys

print("Content-Type: text/plain")
print("")

# 测试二进制数据处理
content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
if content_length > 0:
    data = sys.stdin.buffer.read(content_length)
    print(f"Received {len(data)} bytes")
    print(f"First 10 bytes (hex): {data[:10].hex()}")
    print(f"Contains null bytes: {b'\\x00' in data}")
    print(f"Is valid UTF-8: ", end="")
    try:
        data.decode('utf-8')
        print("Yes")
    except:
        print("No")
```

### 4. CGI 执行环境 Corner Cases

#### 4.1 环境变量边界情况
- **超长环境变量值**
- **包含特殊字符的环境变量**
- **缺少必需环境变量**
- **环境变量注入攻击**

#### 4.2 工作目录和权限
```bash
# 测试不同权限的 CGI 脚本
chmod 000 www/no_permission.py  # 无权限
chmod 444 www/read_only.py      # 只读
chmod 755 www/normal.py         # 正常
chmod 777 www/all_permission.py # 全权限
```

### 5. 进程管理 Corner Cases

#### 5.1 CGI 脚本行为异常
```python
#!/usr/bin/env python3
# www/cgi_abnormal_test.py

import os
import sys
import signal
import time

test_type = os.environ.get('QUERY_STRING', '').split('=')[-1]

print("Content-Type: text/html")
print("")

if test_type == 'no_output':
    # CGI 脚本不产生任何输出就退出
    sys.exit(0)

elif test_type == 'partial_headers':
    # 只输出部分头部就退出
    print("Content-Type: text/html")
    sys.exit(0)

elif test_type == 'invalid_headers':
    # 输出格式错误的头部
    print("Invalid Header Format")
    print("Another-Bad: Header")
    print("")
    print("This should cause problems")

elif test_type == 'signal_exit':
    # 通过信号异常退出
    os.kill(os.getpid(), signal.SIGTERM)

elif test_type == 'infinite_output':
    # 无限输出（测试缓冲区处理）
    while True:
        print("A" * 1000)
        sys.stdout.flush()
        time.sleep(0.001)

elif test_type == 'memory_bomb':
    # 内存炸弹（谨慎使用！）
    data = "A" * (1024 * 1024)  # 1MB
    while True:
        data += data
        print(len(data))
```

### 6. 并发和竞态条件

#### 6.1 并发访问测试脚本
```bash
#!/bin/bash
# concurrent_cgi_test.sh

echo "Testing concurrent CGI requests..."

# 同时启动100个CGI请求
for i in {1..100}; do
    curl -s "http://localhost:8080/cgi_basic.py?id=$i" > /tmp/cgi_result_$i.txt &
done

wait

# 检查结果
success=0
for i in {1..100}; do
    if grep -q "SUCCESS" /tmp/cgi_result_$i.txt 2>/dev/null; then
        success=$((success + 1))
    fi
    rm -f /tmp/cgi_result_$i.txt
done

echo "Successful requests: $success/100"
```

### 7. 资源耗尽 Corner Cases

#### 7.1 文件描述符耗尽
```python
#!/usr/bin/env python3
# www/cgi_fd_test.py

import os
import sys

print("Content-Type: text/plain")
print("")

# 尝试打开大量文件描述符
fds = []
try:
    for i in range(1000):
        fd = os.open('/dev/null', os.O_RDONLY)
        fds.append(fd)
    print(f"Opened {len(fds)} file descriptors")
finally:
    for fd in fds:
        os.close(fd)
```

#### 7.2 内存使用测试
```python
#!/usr/bin/env python3
# www/cgi_memory_test.py

import os
import sys
import gc

print("Content-Type: text/plain")
print("")

# 分配大量内存
test_type = os.environ.get('QUERY_STRING', '').split('=')[-1]

if test_type == 'large_memory':
    # 分配 100MB 内存
    data = bytearray(100 * 1024 * 1024)
    print(f"Allocated {len(data)} bytes")
    del data
    gc.collect()

elif test_type == 'memory_leak':
    # 模拟内存泄漏
    leaked_data = []
    for i in range(10000):
        leaked_data.append("A" * 1000)
    print(f"Created {len(leaked_data)} objects")
    # 故意不清理 leaked_data
```

### 8. 网络相关 Corner Cases

#### 8.1 连接异常测试
```bash
# 客户端突然断开连接
(echo -e "POST /cgi_post_test.py HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 1000\r\n\r\n"; sleep 0.1; echo "partial") | nc localhost 8080

# 超慢的数据传输
echo "POST /cgi_post_test.py HTTP/1.1
Host: localhost:8080
Content-Length: 100

" | (while read line; do echo "$line"; sleep 1; done) | nc localhost 8080
```

### 9. 协议违规 Corner Cases

#### 9.1 HTTP 协议边界情况
```bash
# 缺少 Host 头部
telnet localhost 8080
GET /cgi_basic.py HTTP/1.1


# 使用 HTTP/0.9
telnet localhost 8080
GET /cgi_basic.py

# 无效的 HTTP 方法
telnet localhost 8080
INVALID /cgi_basic.py HTTP/1.1
Host: localhost:8080

# 超长的 HTTP 头部
telnet localhost 8080
GET /cgi_basic.py HTTP/1.1
Host: localhost:8080
Very-Long-Header: AAAAA...(8KB)

```

### 10. 安全相关 Corner Cases

#### 10.1 注入攻击测试
```bash
# 命令注入尝试
GET /cgi_basic.py?cmd=;ls;echo HTTP/1.1

# 环境变量污染
GET /cgi_basic.py HTTP/1.1
Host: localhost:8080
X-Custom-Header: $(whoami)

# 路径遍历
GET /../../../etc/passwd HTTP/1.1

# 脚本注入
POST /cgi_post_test.py HTTP/1.1
Content-Type: application/x-www-form-urlencoded
Content-Length: 50

name=<script>alert('xss')</script>&data=malicious
```

## 🧪 测试执行策略

### 1. 逐步测试
1. 先测试基础功能正常
2. 逐个引入边界条件
3. 观察服务器行为变化
4. 记录异常情况

### 2. 自动化测试集成
将这些 corner cases 集成到现有的测试套件中：

```bash
# 添加到 cgi_comprehensive_test.sh
test_corner_cases() {
    print_test_header "Corner Cases Tests"

    # 测试空查询字符串
    print_test "Empty Query String"
    # ... 实现

    # 测试超长URL
    print_test "Long URL"
    # ... 实现

    # 更多 corner cases...
}
```

### 3. 监控和日志
在测试过程中监控：
- 内存使用情况
- 文件描述符数量
- 进程创建/销毁
- 错误日志输出
- 网络连接状态

这些 corner cases 将帮助发现 CGI 实现中的潜在问题，提高服务器的稳定性和安全性。