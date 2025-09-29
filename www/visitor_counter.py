#!/usr/bin/env python3
"""
访问计数器 - 测试CGI文件操作和状态保持
"""

import os
import time
import json
from datetime import datetime

COUNTER_FILE = "./www/visitor_count.json"

def load_counter_data():
    """加载计数器数据"""
    try:
        if os.path.exists(COUNTER_FILE):
            with open(COUNTER_FILE, 'r') as f:
                return json.load(f)
        else:
            return {"count": 0, "first_visit": None, "last_visit": None, "visits": []}
    except:
        return {"count": 0, "first_visit": None, "last_visit": None, "visits": []}

def save_counter_data(data):
    """保存计数器数据"""
    try:
        with open(COUNTER_FILE, 'w') as f:
            json.dump(data, f, indent=2)
        return True
    except:
        return False

def update_counter():
    """更新访问计数"""
    data = load_counter_data()
    current_time = datetime.now().isoformat()

    # 增加计数
    data["count"] += 1

    # 更新时间信息
    if data["first_visit"] is None:
        data["first_visit"] = current_time
    data["last_visit"] = current_time

    # 记录最近10次访问
    if "visits" not in data:
        data["visits"] = []

    visit_info = {
        "timestamp": current_time,
        "ip": os.environ.get('REMOTE_ADDR', '127.0.0.1'),
        "user_agent": os.environ.get('HTTP_USER_AGENT', 'Unknown')[:50]
    }

    data["visits"].append(visit_info)

    # 只保留最近10次访问
    if len(data["visits"]) > 10:
        data["visits"] = data["visits"][-10:]

    # 保存数据
    save_counter_data(data)
    return data

def create_counter_html(data):
    """生成计数器HTML"""
    try:
        first_visit = datetime.fromisoformat(data["first_visit"]).strftime("%Y-%m-%d %H:%M:%S") if data["first_visit"] else "N/A"
        last_visit = datetime.fromisoformat(data["last_visit"]).strftime("%Y-%m-%d %H:%M:%S") if data["last_visit"] else "N/A"
    except:
        first_visit = "N/A"
        last_visit = "N/A"

    # 生成最近访问列表
    recent_visits = ""
    for i, visit in enumerate(reversed(data.get("visits", [])[-5:]), 1):
        try:
            visit_time = datetime.fromisoformat(visit["timestamp"]).strftime("%H:%M:%S")
            recent_visits += f"""
            <div class="visit-item">
                <span class="visit-number">{i}.</span>
                <span class="visit-time">{visit_time}</span>
                <span class="visit-ip">{visit["ip"]}</span>
            </div>
            """
        except:
            continue

    html = f"""
    <div class="visitor-counter" id="visitor-counter">
        <h4>📊 Compteur de visiteurs</h4>
        <div class="counter-stats">
            <div class="counter-main">
                <div class="count-number">{data["count"]}</div>
                <div class="count-label">Visites totales</div>
            </div>
            <div class="counter-details">
                <div class="detail-item">
                    <strong>Première visite:</strong> {first_visit}
                </div>
                <div class="detail-item">
                    <strong>Dernière visite:</strong> {last_visit}
                </div>
            </div>
        </div>

        <div class="recent-visits">
            <h5>Visites récentes:</h5>
            {recent_visits if recent_visits else '<div class="visit-item">Aucune visite récente</div>'}
        </div>
    </div>

    <style>
        .visitor-counter {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border-radius: 10px;
            padding: 20px;
            margin: 15px 0;
            box-shadow: 0 4px 15px rgba(0,0,0,0.2);
        }}
        .counter-stats {{
            display: flex;
            align-items: center;
            gap: 20px;
            margin-bottom: 15px;
        }}
        .counter-main {{
            text-align: center;
        }}
        .count-number {{
            font-size: 2.5em;
            font-weight: bold;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }}
        .count-label {{
            font-size: 0.9em;
            opacity: 0.9;
        }}
        .counter-details {{
            flex: 1;
        }}
        .detail-item {{
            background: rgba(255,255,255,0.1);
            padding: 8px 12px;
            border-radius: 5px;
            margin: 5px 0;
        }}
        .recent-visits {{
            border-top: 1px solid rgba(255,255,255,0.2);
            padding-top: 15px;
        }}
        .recent-visits h5 {{
            margin: 0 0 10px 0;
            opacity: 0.9;
        }}
        .visit-item {{
            display: flex;
            justify-content: space-between;
            background: rgba(255,255,255,0.1);
            padding: 5px 10px;
            border-radius: 3px;
            margin: 3px 0;
            font-size: 0.85em;
        }}
        .visit-number {{
            width: 20px;
        }}
        .visit-time {{
            flex: 1;
            text-align: center;
        }}
        .visit-ip {{
            width: 100px;
            text-align: right;
        }}
    </style>
    """

    return html

# 主执行
print("Content-Type: text/html")
print("")

try:
    counter_data = update_counter()
    html_output = create_counter_html(counter_data)
    print(html_output)
except Exception as e:
    print(f"""
    <div class="error-message">
        ❌ Erreur du compteur: {str(e)}
    </div>
    """)