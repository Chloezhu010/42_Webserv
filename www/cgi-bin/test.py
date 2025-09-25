#!/usr/bin/env python3

import os

print("Content-Type: text/html")
print()  # Empty line to end headers

print("""<!DOCTYPE html>
<html>
<head><title>CGI Test</title></head>
<body>
<h1>CGI Environment Test</h1>
<table border="1">""")

# Display all environment variables
for key, value in sorted(os.environ.items()):
    print(f"<tr><td><strong>{key}</strong></td><td>{value}</td></tr>")

print("""</table>
<h2>CGI Variables</h2>
<ul>
<li>REQUEST_METHOD: {}</li>
<li>QUERY_STRING: {}</li>
<li>CONTENT_TYPE: {}</li>
<li>CONTENT_LENGTH: {}</li>
</ul>
</body>
</html>""".format(
    os.environ.get('REQUEST_METHOD', 'Not set'),
    os.environ.get('QUERY_STRING', 'Not set'),
    os.environ.get('CONTENT_TYPE', 'Not set'),
    os.environ.get('CONTENT_LENGTH', 'Not set')
))