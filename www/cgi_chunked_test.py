#!/usr/bin/env python3
"""
分块传输测试 - 测试chunked encoding处理
"""

import os
import sys
import time

def output_chunk(data):
    """输出一个数据块"""
    sys.stdout.write(data)
    sys.stdout.flush()

print("Content-Type: text/html")
print("")

print("""<!DOCTYPE html>
<html>
<head>
    <title>Chunked Output Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .chunk { background: #f0f0f0; padding: 10px; margin: 5px 0; border-left: 3px solid #007cba; }
    </style>
</head>
<body>
    <h1>📦 Chunked Output Test</h1>
    <p>This page demonstrates chunked output generation...</p>
""")

# 输出多个块，每个块之间有延迟
chunks = [
    "<div class='chunk'><h3>Chunk 1</h3><p>First chunk of data - timestamp: {}</p></div>",
    "<div class='chunk'><h3>Chunk 2</h3><p>Second chunk of data - timestamp: {}</p></div>",
    "<div class='chunk'><h3>Chunk 3</h3><p>Third chunk of data - timestamp: {}</p></div>",
    "<div class='chunk'><h3>Chunk 4</h3><p>Fourth chunk of data - timestamp: {}</p></div>",
    "<div class='chunk'><h3>Chunk 5</h3><p>Final chunk of data - timestamp: {}</p></div>"
]

for i, chunk_template in enumerate(chunks):
    timestamp = __import__('datetime').datetime.now().strftime('%H:%M:%S.%f')[:-3]
    chunk = chunk_template.format(timestamp)
    output_chunk(chunk)

    # 添加小延迟来模拟真实的分块处理
    if i < len(chunks) - 1:
        time.sleep(0.1)

print("""
    <div class='chunk'>
        <h3>✅ Test Complete</h3>
        <p>All chunks have been successfully transmitted!</p>
        <p>If you can see this message, chunked output is working correctly.</p>
    </div>

    <h2>Technical Details:</h2>
    <ul>
        <li>Total chunks sent: {}</li>
        <li>Content-Type: {}</li>
        <li>Transfer method: Progressive output with flush</li>
    </ul>

</body>
</html>""".format(
    len(chunks) + 1,
    'text/html'
))