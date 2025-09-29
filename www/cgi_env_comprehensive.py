#!/usr/bin/env python3
"""
全面的CGI环境变量测试 - 验证CGI/1.1标准合规性
"""

import os
import sys
import cgi

def get_env_var(name, default="(未设置)"):
    """安全获取环境变量"""
    return os.environ.get(name, default)

def print_header():
    """输出HTTP头"""
    print("Content-Type: text/html")
    print("")

def print_html_start():
    """输出HTML开始"""
    print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI环境变量全面测试</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .success { color: green; }
        .error { color: red; }
        .warning { color: orange; }
        .info { color: blue; }
        table { border-collapse: collapse; width: 100%; margin-top: 10px; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .category { background-color: #e8f4fd; font-weight: bold; }
        pre { background: #f5f5f5; padding: 10px; border: 1px solid #ddd; }
    </style>
</head>
<body>""")

def print_html_end():
    """输出HTML结束"""
    print("""</body>
</html>""")

def test_required_vars():
    """测试CGI/1.1标准要求的环境变量"""
    print("<h1>🔬 CGI/1.1标准环境变量测试</h1>")

    # 必需的CGI变量
    required_vars = [
        ("REQUEST_METHOD", "请求方法"),
        ("SERVER_NAME", "服务器名称"),
        ("SERVER_PORT", "服务器端口"),
        ("SERVER_PROTOCOL", "服务器协议"),
        ("SERVER_SOFTWARE", "服务器软件"),
        ("GATEWAY_INTERFACE", "网关接口"),
        ("SCRIPT_NAME", "脚本名称"),
        ("PATH_INFO", "路径信息"),
        ("QUERY_STRING", "查询字符串"),
        ("CONTENT_TYPE", "内容类型"),
        ("CONTENT_LENGTH", "内容长度"),
        ("REMOTE_ADDR", "远程地址"),
        ("REMOTE_HOST", "远程主机")
    ]

    print("<h2>必需的CGI标准变量</h2>")
    print("<table>")
    print("<tr><th>变量名</th><th>描述</th><th>值</th><th>状态</th></tr>")

    missing_count = 0
    for var_name, description in required_vars:
        value = get_env_var(var_name, "")
        status = "✅ 设置" if value else "❌ 缺失"
        status_class = "success" if value else "error"
        if not value:
            missing_count += 1

        print(f"<tr><td><code>{var_name}</code></td><td>{description}</td><td>{value if value else '(空)'}</td><td class='{status_class}'>{status}</td></tr>")

    print("</table>")

    if missing_count == 0:
        print(f"<p class='success'>✅ 所有必需变量都已正确设置！</p>")
    else:
        print(f"<p class='error'>❌ 有 {missing_count} 个必需变量缺失或为空</p>")

def test_http_headers():
    """测试HTTP头环境变量"""
    print("<h2>HTTP头环境变量</h2>")

    # 常见的HTTP头变量
    http_headers = [
        ("HTTP_HOST", "主机头"),
        ("HTTP_USER_AGENT", "用户代理"),
        ("HTTP_ACCEPT", "接受类型"),
        ("HTTP_ACCEPT_LANGUAGE", "接受语言"),
        ("HTTP_ACCEPT_ENCODING", "接受编码"),
        ("HTTP_CONNECTION", "连接类型"),
        ("HTTP_CACHE_CONTROL", "缓存控制"),
        ("HTTP_COOKIE", "Cookie"),
        ("HTTP_REFERER", "来源页面"),
        ("HTTP_AUTHORIZATION", "授权信息")
    ]

    print("<table>")
    print("<tr><th>HTTP头变量</th><th>描述</th><th>值</th></tr>")

    for var_name, description in http_headers:
        value = get_env_var(var_name, "(未设置)")
        print(f"<tr><td><code>{var_name}</code></td><td>{description}</td><td>{value}</td></tr>")

    print("</table>")

def test_all_env_vars():
    """显示所有环境变量"""
    print("<h2>所有环境变量</h2>")
    print("<table>")
    print("<tr><th>变量名</th><th>值</th></tr>")

    # 按字母顺序排序环境变量
    env_vars = sorted(os.environ.items())
    for key, value in env_vars:
        # 截断过长的值
        display_value = value if len(value) <= 100 else value[:100] + "..."
        print(f"<tr><td><code>{key}</code></td><td>{display_value}</td></tr>")

    print("</table>")

def test_request_data():
    """测试请求数据处理"""
    method = get_env_var("REQUEST_METHOD", "GET")
    content_length = get_env_var("CONTENT_LENGTH", "0")

    print("<h2>请求数据分析</h2>")
    print(f"<p><strong>请求方法:</strong> {method}</p>")
    print(f"<p><strong>内容长度:</strong> {content_length}</p>")

    if method == "POST":
        try:
            length = int(content_length)
            if length > 0:
                post_data = sys.stdin.read(length)
                print(f"<p><strong>POST数据长度:</strong> {len(post_data)} bytes</p>")
                print("<h3>POST数据内容:</h3>")
                print(f"<pre>{post_data if len(post_data) <= 500 else post_data[:500] + '...'}</pre>")
            else:
                print("<p>没有POST数据</p>")
        except (ValueError, IOError) as e:
            print(f"<p class='error'>读取POST数据失败: {str(e)}</p>")

def run_comprehensive_test():
    """运行全面测试"""
    print_header()
    print_html_start()

    test_required_vars()
    test_http_headers()
    test_request_data()
    test_all_env_vars()

    print("<hr>")
    print(f"<p><small>测试完成 - 生成于 cgi_env_comprehensive.py</small></p>")

    print_html_end()

if __name__ == "__main__":
    run_comprehensive_test()