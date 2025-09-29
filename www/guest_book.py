#!/usr/bin/env python3
"""
简单留言本 - 测试CGI的POST功能和表单处理
"""

import os
import sys
import json
import urllib.parse
from datetime import datetime

GUESTBOOK_FILE = "./www/guestbook.json"

def load_guestbook():
    """加载留言数据"""
    try:
        if os.path.exists(GUESTBOOK_FILE):
            with open(GUESTBOOK_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
        else:
            return {"messages": []}
    except:
        return {"messages": []}

def save_guestbook(data):
    """保存留言数据"""
    try:
        with open(GUESTBOOK_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        return True
    except Exception as e:
        return False

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

def process_post_request():
    """处理POST请求 - 添加新留言"""
    post_data = read_post_data()
    if not post_data:
        return False, "Aucune donnée reçue"

    try:
        # 解析表单数据
        parsed_data = urllib.parse.parse_qs(post_data)
        name = parsed_data.get('name', [''])[0].strip()
        message = parsed_data.get('message', [''])[0].strip()

        if not name or not message:
            return False, "Nom et message requis"

        # 加载现有数据
        guestbook = load_guestbook()

        # 创建新留言
        new_message = {
            "name": name[:100],  # 限制长度
            "message": message[:500],  # 限制长度
            "timestamp": datetime.now().isoformat(),
            "ip": os.environ.get('REMOTE_ADDR', '127.0.0.1')
        }

        # 添加到留言列表开头
        guestbook["messages"].insert(0, new_message)

        # 只保留最近20条留言
        if len(guestbook["messages"]) > 20:
            guestbook["messages"] = guestbook["messages"][:20]

        # 保存数据
        if save_guestbook(guestbook):
            return True, "Message ajouté avec succès!"
        else:
            return False, "Erreur lors de la sauvegarde"

    except Exception as e:
        return False, f"Erreur: {str(e)}"

def create_guestbook_html():
    """生成留言本HTML"""
    method = os.environ.get('REQUEST_METHOD', 'GET')
    success_message = ""
    error_message = ""

    # 处理POST请求
    if method == 'POST':
        success, message = process_post_request()
        if success:
            success_message = message
        else:
            error_message = message

    # 加载留言
    guestbook = load_guestbook()

    # 生成留言列表
    messages_html = ""
    for i, msg in enumerate(guestbook.get("messages", [])[:10], 1):
        try:
            timestamp = datetime.fromisoformat(msg["timestamp"]).strftime("%Y-%m-%d %H:%M")
        except:
            timestamp = "Date inconnue"

        messages_html += f"""
        <div class="message-item">
            <div class="message-header">
                <strong>{msg["name"]}</strong>
                <span class="message-time">{timestamp}</span>
            </div>
            <div class="message-content">{msg["message"]}</div>
        </div>
        """

    if not messages_html:
        messages_html = '<div class="no-messages">Aucun message pour le moment. Soyez le premier à laisser un message!</div>'

    # 状态消息
    status_html = ""
    if success_message:
        status_html = f'<div class="success-message">✅ {success_message}</div>'
    elif error_message:
        status_html = f'<div class="error-message">❌ {error_message}</div>'

    html = f"""
    <div class="guestbook" id="guestbook">
        <h4>📝 Livre d'or</h4>

        {status_html}

        <form class="guestbook-form" method="POST" action="/guest_book.py">
            <div class="form-group">
                <label for="name">Votre nom:</label>
                <input type="text" id="name" name="name" maxlength="100" required>
            </div>
            <div class="form-group">
                <label for="message">Votre message:</label>
                <textarea id="message" name="message" rows="3" maxlength="500" required></textarea>
            </div>
            <button type="submit">Laisser un message</button>
        </form>

        <div class="messages-section">
            <h5>Messages récents ({len(guestbook.get("messages", []))}):</h5>
            {messages_html}
        </div>
    </div>

    <style>
        .guestbook {{
            background: #fff;
            border: 2px solid #e9ecef;
            border-radius: 10px;
            padding: 20px;
            margin: 15px 0;
        }}
        .success-message {{
            background: #d4edda;
            color: #155724;
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            border: 1px solid #c3e6cb;
        }}
        .error-message {{
            background: #f8d7da;
            color: #721c24;
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            border: 1px solid #f5c6cb;
        }}
        .guestbook-form {{
            background: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            margin: 15px 0;
        }}
        .form-group {{
            margin: 10px 0;
        }}
        .form-group label {{
            display: block;
            font-weight: bold;
            margin-bottom: 5px;
        }}
        .form-group input,
        .form-group textarea {{
            width: 100%;
            padding: 8px;
            border: 1px solid #ced4da;
            border-radius: 4px;
            font-family: inherit;
            box-sizing: border-box;
        }}
        .guestbook-form button {{
            background: #007bff;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-weight: bold;
        }}
        .guestbook-form button:hover {{
            background: #0056b3;
        }}
        .messages-section {{
            margin-top: 20px;
        }}
        .message-item {{
            background: #f1f3f4;
            padding: 12px;
            margin: 10px 0;
            border-radius: 6px;
            border-left: 4px solid #007bff;
        }}
        .message-header {{
            display: flex;
            justify-content: space-between;
            margin-bottom: 8px;
        }}
        .message-time {{
            color: #6c757d;
            font-size: 0.85em;
        }}
        .message-content {{
            line-height: 1.4;
        }}
        .no-messages {{
            text-align: center;
            color: #6c757d;
            font-style: italic;
            padding: 20px;
        }}
    </style>
    """

    return html

# CGI响应
print("Content-Type: text/html")
print("")
print(create_guestbook_html())