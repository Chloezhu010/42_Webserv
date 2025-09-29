#!/usr/bin/env python3
"""
错误处理测试 - 测试各种错误情况
"""

import os
import sys

# 获取测试类型参数
query = os.environ.get('QUERY_STRING', '')
test_type = 'normal'

if 'type=' in query:
    for param in query.split('&'):
        if param.startswith('type='):
            test_type = param.split('=')[1]
            break

print("Content-Type: text/html")
print("")

if test_type == 'exit_error':
    print("<h1>Testing Exit Code Error</h1>")
    print("<p>This script will exit with error code 1</p>")
    sys.exit(1)

elif test_type == 'timeout':
    print("<h1>Testing Timeout</h1>")
    print("<p>This script will hang for a long time...</p>")
    import time
    time.sleep(60)  # 等待60秒，应该会超时

elif test_type == 'large_output':
    print("<h1>Testing Large Output</h1>")
    print("<p>Generating large output...</p>")
    for i in range(10000):
        print(f"<p>Line {i}: This is a test of large output generation</p>")

elif test_type == 'invalid_headers':
    print("Invalid-Header-Format")  # 故意写错误的header格式
    print("")
    print("<h1>Invalid Headers Test</h1>")

elif test_type == 'no_content_type':
    # 不输出Content-Type header
    print("")
    print("<h1>No Content-Type Header Test</h1>")
    print("<p>This response has no Content-Type header</p>")

elif test_type == 'custom_status':
    print("Status: 418 I'm a teapot")
    print("Content-Type: text/html")
    print("")
    print("""
    <h1>🫖 Custom Status Code Test</h1>
    <p>This response has a custom status code: 418 I'm a teapot</p>
    """)

elif test_type == 'redirect':
    print("Status: 302 Found")
    print("Location: /")
    print("Content-Type: text/html")
    print("")
    print("<h1>Redirect Test</h1><p>You should be redirected to /</p>")

elif test_type == 'binary_confusion':
    print("Content-Type: application/octet-stream")
    print("")
    # 输出一些二进制数据（但实际上是文本）
    sys.stdout.buffer.write(b'\x00\x01\x02\x03HTML after binary\x04\x05')

else:
    # 正常测试页面
    print(f"""
    <h1>🧪 CGI Error & Edge Case Tests</h1>

    <h2>Available Tests:</h2>
    <ul>
        <li><a href="?type=normal">Normal Response</a> (current)</li>
        <li><a href="?type=exit_error">Exit Code Error Test</a></li>
        <li><a href="?type=timeout">Timeout Test</a> (⚠️ Will hang)</li>
        <li><a href="?type=large_output">Large Output Test</a></li>
        <li><a href="?type=invalid_headers">Invalid Headers Test</a></li>
        <li><a href="?type=no_content_type">No Content-Type Test</a></li>
        <li><a href="?type=custom_status">Custom Status Code Test</a></li>
        <li><a href="?type=redirect">Redirect Test</a></li>
        <li><a href="?type=binary_confusion">Binary Data Test</a></li>
    </ul>

    <h2>Current Test: Normal Response</h2>
    <p>✅ This is a normal, successful CGI response.</p>
    <p>Query string: {query if query else '(empty)'}</p>
    <p>Script executed successfully with no errors.</p>
    """)