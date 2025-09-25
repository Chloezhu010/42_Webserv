#!/usr/bin/env python3
"""
文件上传测试 - 测试multipart/form-data处理
"""

import os
import sys
import cgi
import tempfile

def read_multipart():
    """读取multipart数据"""
    content_type = os.environ.get('CONTENT_TYPE', '')
    if 'multipart/form-data' not in content_type:
        return None

    # 创建FieldStorage来解析multipart数据
    form = cgi.FieldStorage()
    return form

print("Content-Type: text/html")
print("")

method = os.environ.get('REQUEST_METHOD', '')
content_type = os.environ.get('CONTENT_TYPE', '')

print("""<!DOCTYPE html>
<html>
<head>
    <title>File Upload Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .upload-form { background: #f9f9f9; padding: 20px; border: 1px solid #ddd; }
        .result { background: #e8f5e8; padding: 15px; border: 1px solid #4caf50; margin: 10px 0; }
        .error { background: #ffe8e8; padding: 15px; border: 1px solid #f44336; margin: 10px 0; }
    </style>
</head>
<body>
    <h1>📁 File Upload Test</h1>
""")

print(f"""
    <h2>Request Information:</h2>
    <ul>
        <li><strong>Method:</strong> {method}</li>
        <li><strong>Content-Type:</strong> {content_type}</li>
        <li><strong>Content-Length:</strong> {os.environ.get('CONTENT_LENGTH', '0')}</li>
    </ul>
""")

if method == 'POST' and 'multipart/form-data' in content_type:
    try:
        form = read_multipart()

        if form:
            print("<div class='result'>")
            print("<h3>✅ Multipart Data Received Successfully!</h3>")

            # 列出所有表单字段
            print("<h4>Form Fields:</h4><ul>")
            for key in form.keys():
                field = form[key]
                if hasattr(field, 'filename') and field.filename:
                    # 这是一个文件字段
                    file_size = len(field.file.read()) if hasattr(field, 'file') else 0
                    field.file.seek(0)  # 重置文件指针
                    print(f"<li><strong>{key}:</strong> File '{field.filename}' ({file_size} bytes)</li>")
                else:
                    # 这是一个普通字段
                    value = field.value if hasattr(field, 'value') else str(field)
                    print(f"<li><strong>{key}:</strong> {value}</li>")
            print("</ul>")
            print("</div>")
        else:
            print("<div class='error'>❌ Failed to parse multipart data</div>")

    except Exception as e:
        print(f"<div class='error'>❌ Error processing multipart data: {str(e)}</div>")

elif method == 'POST':
    print("<div class='error'>❌ POST request received but Content-Type is not multipart/form-data</div>")

# 显示上传表单
print("""
    <div class='upload-form'>
        <h2>🔄 Test File Upload</h2>
        <form method="POST" action="/cgi_upload_test.py" enctype="multipart/form-data">
            <p>
                <label>Your Name: <input type="text" name="username" value="Test User"></label><br><br>
                <label>Choose File: <input type="file" name="upload_file"></label><br><br>
                <label>Description: <textarea name="description" rows="3">Test file upload</textarea></label><br><br>
                <input type="submit" value="Upload File">
            </p>
        </form>
        <p><small>Note: This tests multipart/form-data parsing capability</small></p>
    </div>

    <h2>📝 Raw POST Data Debug:</h2>
""")

# 显示原始POST数据（用于调试）
if method == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
    if content_length > 0 and content_length < 10000:  # 只显示小文件的内容
        try:
            raw_data = sys.stdin.read(content_length)
            print(f"<pre>{raw_data[:1000]}{'...' if len(raw_data) > 1000 else ''}</pre>")
        except Exception as e:
            print(f"<p>Error reading raw data: {e}</p>")
    else:
        print("<p>Content too large to display or no content</p>")
else:
    print("<p>No POST data (not a POST request)</p>")

print("</body></html>")