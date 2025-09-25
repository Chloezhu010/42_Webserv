#!/usr/bin/env python3

import os
import sys
import json
from urllib.parse import parse_qs

# CGI headers
print("Content-Type: application/json")
print()  # Empty line to end headers

# Get CGI environment variables
env_vars = {
    'REQUEST_METHOD': os.environ.get('REQUEST_METHOD', ''),
    'QUERY_STRING': os.environ.get('QUERY_STRING', ''),
    'CONTENT_TYPE': os.environ.get('CONTENT_TYPE', ''),
    'CONTENT_LENGTH': os.environ.get('CONTENT_LENGTH', '0'),
    'HTTP_HOST': os.environ.get('HTTP_HOST', ''),
    'HTTP_USER_AGENT': os.environ.get('HTTP_USER_AGENT', ''),
    'PATH_INFO': os.environ.get('PATH_INFO', ''),
    'SCRIPT_NAME': os.environ.get('SCRIPT_NAME', ''),
    'SERVER_NAME': os.environ.get('SERVER_NAME', ''),
    'SERVER_PORT': os.environ.get('SERVER_PORT', ''),
}

# Read request body if present
body = ""
content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
if content_length > 0:
    body = sys.stdin.read(content_length)

# Parse query parameters
query_params = parse_qs(os.environ.get('QUERY_STRING', ''))

# Build response similar to httpbin.org
response = {
    'args': query_params,
    'data': body,
    'files': {},
    'form': {},
    'headers': {k.replace('HTTP_', '').replace('_', '-').title(): v
                for k, v in os.environ.items() if k.startswith('HTTP_')},
    'json': None,
    'method': env_vars['REQUEST_METHOD'],
    'origin': os.environ.get('REMOTE_ADDR', ''),
    'url': f"http://{env_vars['HTTP_HOST']}{env_vars['SCRIPT_NAME']}{env_vars['PATH_INFO']}?{env_vars['QUERY_STRING']}"
}

# Try to parse JSON body
if body and env_vars['CONTENT_TYPE'] == 'application/json':
    try:
        response['json'] = json.loads(body)
    except:
        pass

# Parse form data
if body and env_vars['CONTENT_TYPE'] == 'application/x-www-form-urlencoded':
    response['form'] = parse_qs(body)

print(json.dumps(response, indent=2))