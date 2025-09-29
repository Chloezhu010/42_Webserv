#!/usr/bin/env python3
"""
POST数据测试 - 验证POST数据接收
"""

import os
import sys
import urllib.parse

def read_post_data():
    """读取POST数据"""
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    try:
        length = int(content_length)
        if length > 0:
            return sys.stdin.read(length)
    except (ValueError, IOError):
        pass
    return ""

print("Content-Type: text/html")
print("")

post_data = read_post_data()
method = os.environ.get('REQUEST_METHOD', 'Unknown')
query_string = os.environ.get('QUERY_STRING', '')

print(f"""<!DOCTYPE html>
<html>
<head>
    <title>POST Data Test</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        .success {{ color: green; }}
        .error {{ color: red; }}
        .info {{ color: blue; }}
        pre {{ background: #f5f5f5; padding: 10px; border: 1px solid #ddd; }}
    </style>
</head>
<body>
    <h1>🔍 POST Data Test Results</h1>

    <h2>Request Details:</h2>
    <ul>
        <li><strong>Method:</strong> {method}</li>
        <li><strong>Query String:</strong> {query_string if query_string else '(empty)'}</li>
        <li><strong>Content Length:</strong> {os.environ.get('CONTENT_LENGTH', '0')}</li>
        <li><strong>Content Type:</strong> {os.environ.get('CONTENT_TYPE', '(not set)')}</li>
    </ul>

    <h2>POST Data:</h2>
    <p><strong>Raw Data Length:</strong> {len(post_data)} bytes</p>
    <pre>{post_data if post_data else '(no POST data received)'}</pre>
""")

if post_data:
    print("<h2>Parsed POST Data:</h2>")
    try:
        if 'application/x-www-form-urlencoded' in os.environ.get('CONTENT_TYPE', ''):
            parsed = urllib.parse.parse_qs(post_data)
            print("<ul>")
            for key, values in parsed.items():
                for value in values:
                    print(f"<li><strong>{key}:</strong> {value}</li>")
            print("</ul>")
        else:
            print("<p class='info'>Content-Type is not form-encoded, showing raw data above.</p>")
    except Exception as e:
        print(f"<p class='error'>❌ Error parsing POST data: {str(e)}</p>")

if method == 'POST' and post_data:
    print("<p class='success'>✅ POST data received successfully!</p>")
elif method == 'POST' and not post_data:
    print("<p class='error'>❌ POST request but no data received</p>")
elif method != 'POST':
    print(f"<p class='info'>ℹ️  This is a {method} request, not POST</p>")

print("""
    <h2>Test Form:</h2>
    <form method="POST" action="/cgi_post_test.py">
        <p>
            <label>Name: <input type="text" name="name" value="Test User"></label><br>
            <label>Email: <input type="email" name="email" value="test@example.com"></label><br>
            <label>Message: <textarea name="message" rows="3">Hello from CGI!</textarea></label><br>
            <input type="submit" value="Submit POST Test">
        </p>
    </form>

    <h2>Query String Test:</h2>
    <p><a href="/cgi_post_test.py?param1=value1&param2=value2">Test with query parameters</a></p>

</body>
</html>""")