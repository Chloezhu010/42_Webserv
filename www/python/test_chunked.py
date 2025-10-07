#!/usr/bin/env python3
import sys
import os

# Read from stdin (server passes unchunked body here)
content_length = os.environ.get('CONTENT_LENGTH', '')

if content_length:
    body = sys.stdin.read(int(content_length))
else:
    # No Content-Length means read until EOF
    body = sys.stdin.read()

# Output HTTP response headers
print("Content-Type: text/plain\r")
print("\r")

# Output what we received
print("=== CGI Received ===")
print(f"Body length: {len(body)} bytes")
print(f"Body content: '{body}'")
print(f"CONTENT_LENGTH env: '{content_length}'")
print(f"Transfer-Encoding: (server unchunks before CGI)")
