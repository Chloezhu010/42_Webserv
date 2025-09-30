#!/usr/bin/env python3
"""
Informations dynamiques sur l'état du serveur - Fournit des données en temps réel
"""

import os
import sys
import time
import platform
import json
from datetime import datetime

def get_system_info():
    """Obtenir les informations système"""
    try:
        return {
            "os": platform.system(),
            "platform": platform.platform(),
            "python_version": platform.python_version(),
            "machine": platform.machine(),
            "processor": platform.processor() or "Inconnu"
        }
    except:
        return {"error": "Impossible d'obtenir les informations système"}

def get_server_stats():
    """Obtenir les statistiques du serveur"""
    current_time = datetime.now()

    # Traduction des jours en français
    days_fr = {
        'Monday': 'Lundi',
        'Tuesday': 'Mardi',
        'Wednesday': 'Mercredi',
        'Thursday': 'Jeudi',
        'Friday': 'Vendredi',
        'Saturday': 'Samedi',
        'Sunday': 'Dimanche'
    }

    day_en = current_time.strftime("%A")
    day_fr = days_fr.get(day_en, day_en)

    return {
        "current_time": current_time.strftime("%Y-%m-%d %H:%M:%S"),
        "timestamp": int(time.time()),
        "day_of_week": day_fr,
        "timezone": time.tzname[0] if time.tzname else "UTC",
        "server_software": "webserv/1.0 (Projet 42)",
        "cgi_version": "CGI/1.1",
        "request_method": os.environ.get('REQUEST_METHOD', 'GET'),
        "remote_addr": os.environ.get('REMOTE_ADDR', '127.0.0.1'),
        "user_agent": os.environ.get('HTTP_USER_AGENT', 'Inconnu'),
    }

def create_html_response():
    """Générer la réponse HTML"""
    system_info = get_system_info()
    server_stats = get_server_stats()

    html = f"""
    <div class="dynamic-content" id="server-status">
        <h4>🖥️ État du serveur en temps réel</h4>
        <div class="status-grid">
            <div class="status-item">
                <strong>Heure actuelle:</strong> {server_stats['current_time']}
            </div>
            <div class="status-item">
                <strong>Jour:</strong> {server_stats['day_of_week']}
            </div>
            <div class="status-item">
                <strong>Système:</strong> {system_info.get('os', 'Inconnu')} ({system_info.get('machine', 'Inconnu')})
            </div>
            <div class="status-item">
                <strong>Version CGI:</strong> {server_stats['cgi_version']}
            </div>
            <div class="status-item">
                <strong>Votre IP:</strong> {server_stats['remote_addr']}
            </div>
            <div class="status-item">
                <strong>Méthode:</strong> {server_stats['request_method']}
            </div>
            <div class="status-item">
                <strong>Navigateur:</strong> {server_stats['user_agent'][:50]}{"..." if len(server_stats['user_agent']) > 50 else ""}
            </div>
        </div>
        <div class="status-footer">
            ✅ CGI fonctionnel - Données générées dynamiquement
        </div>
    </div>

    <style>
        .dynamic-content {{
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 8px;
            padding: 15px;
            margin: 15px 0;
        }}
        .status-grid {{
            display: grid;
            grid-template-columns: 1fr;
            gap: 8px;
            margin: 10px 0;
        }}
        .status-item {{
            padding: 8px;
            background: white;
            border-radius: 4px;
            border-left: 3px solid #007bff;
        }}
        .status-footer {{
            text-align: center;
            color: #28a745;
            font-weight: bold;
            margin-top: 10px;
        }}
    </style>
    """

    return html

# Réponse CGI
print("Content-Type: text/html; charset=UTF-8")
print("")
print(create_html_response())