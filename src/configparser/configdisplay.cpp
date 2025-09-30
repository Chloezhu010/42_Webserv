#include "config.hpp"
#include <iostream>
#include <iomanip>
#include <cstdio>  
// 辅助函数：打印缩进
void printIndent(size_t level) {
    for (size_t i = 0; i < level * 2; ++i) {
        std::cout << " ";
    }
}

// 辅助函数：打印分隔线
void printSeparator(const std::string& title, char separator = '=') {
    std::cout << std::string(50, separator) << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << std::string(50, separator) << std::endl;
}

std::string intToString(int value) {
    if (value == 0) {
        return "0";
    }
    
    std::string result;
    bool negative = false;
    
    if (value < 0) {
        negative = true;
        value = -value;
    }
    
    while (value > 0) {
        result = static_cast<char>('0' + (value % 10)) + result;
        value /= 10;
    }
    
    if (negative) {
        result = "-" + result;
    }
    
    return result;
}

// 显示LocationConfig的所有参数
void displayLocationConfig(const LocationConfig& location, size_t indent) {
    printIndent(indent);
    std::cout << "📍 LOCATION CONFIGURATION:" << std::endl;
    
    printIndent(indent);
    std::cout << "├── Path: \"" << location.path << "\"" << std::endl;
    
    printIndent(indent);
    std::cout << "├── Root: \"" << location.root << "\"" << std::endl;
    
    // Index文件列表
    printIndent(indent);
    std::cout << "├── Index files (" << location.index.size() << "): ";
    if (location.index.empty()) {
        std::cout << "(none)" << std::endl;
    } else {
        std::cout << std::endl;
        for (size_t i = 0; i < location.index.size(); ++i) {
            printIndent(indent);
            std::cout << "│   [" << i << "] \"" << location.index[i] << "\"" << std::endl;
        }
    }
    
    // 允许的HTTP方法
    printIndent(indent);
    std::cout << "├── Allowed methods (" << location.allowMethods.size() << "): ";
    if (location.allowMethods.empty()) {
        std::cout << "(none - all allowed)" << std::endl;
    } else {
        for (size_t i = 0; i < location.allowMethods.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << location.allowMethods[i];
        }
        std::cout << std::endl;
    }
    
    // Autoindex设置
    printIndent(indent);
    std::cout << "├── Autoindex: " << (location.autoindex ? "ON" : "OFF") << std::endl;
    
    // CGI配置
    printIndent(indent);
    std::cout << "├── CGI Extension: \"" << location.cgiExtension << "\"" << std::endl;
    
    printIndent(indent);
    std::cout << "├── CGI Path: \"" << location.cgiPath << "\"" << std::endl;
    
    // 重定向设置
    printIndent(indent);
    std::cout << "└── Redirect: \"" << location.redirect << "\"" << std::endl;
}

// 显示ServerConfig的所有参数
void displayServerConfig(const ServerConfig& server, size_t serverIndex) {
    // 使用手动实现的转换函数
    std::string title = "SERVER #" + intToString(static_cast<int>(serverIndex + 1)) + " CONFIGURATION";
    printSeparator(title, '=');
    
    // 监听端口
    std::cout << "Listen Ports (" << server.listen.size() << "): ";
    if (server.listen.empty()) {
        std::cout << "(none)" << std::endl;
    } else {
        for (size_t i = 0; i < server.listen.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << server.listen[i];
        }
        std::cout << std::endl;
    }
    
    // 服务器名称
    std::cout << "Server Names (" << server.serverName.size() << "): ";
    if (server.serverName.empty()) {
        std::cout << "(default server)" << std::endl;
    } else {
        std::cout << std::endl;
        for (size_t i = 0; i < server.serverName.size(); ++i) {
            std::cout << "    [" << i << "] \"" << server.serverName[i] << "\"" << std::endl;
        }
    }
    
    // 客户端最大请求体大小
    std::cout << "Client Max Body Size: " << server.clientMaxBodySize << " bytes";
    if (server.clientMaxBodySize >= 1024 * 1024) {
        std::cout << " (" << (server.clientMaxBodySize / (1024 * 1024)) << " MB)";
    } else if (server.clientMaxBodySize >= 1024) {
        std::cout << " (" << (server.clientMaxBodySize / 1024) << " KB)";
    }
    std::cout << std::endl;
    
    // 根目录
    std::cout << "Root Directory: \"" << server.root << "\"" << std::endl;
    
    // 默认索引文件
    std::cout << "Default Index Files (" << server.index.size() << "): ";
    if (server.index.empty()) {
        std::cout << "(none)" << std::endl;
    } else {
        std::cout << std::endl;
        for (size_t i = 0; i < server.index.size(); ++i) {
            std::cout << "    [" << i << "] \"" << server.index[i] << "\"" << std::endl;
        }
    }
    
    // 错误页面配置
    std::cout << "Custom Error Pages (" << server.errorPages.size() << "): ";
    if (server.errorPages.empty()) {
        std::cout << "(none - using default)" << std::endl;
    } else {
        std::cout << std::endl;
        for (std::map<int, std::string>::const_iterator it = server.errorPages.begin();
             it != server.errorPages.end(); ++it) {
            std::cout << "    " << it->first << " -> \"" << it->second << "\"" << std::endl;
        }
    }
    
    // Location配置
    std::cout << std::endl;
    std::string locationTitle = "LOCATIONS (" + intToString(static_cast<int>(server.locations.size())) + ")";
    printSeparator(locationTitle, '-');
    
    if (server.locations.empty()) {
        std::cout << "  (No locations configured)" << std::endl;
    } else {
        for (size_t i = 0; i < server.locations.size(); ++i) {
            std::cout << std::endl;
            std::cout << "Location #" << (i + 1) << ":" << std::endl;
            displayLocationConfig(server.locations[i], 1);
            
            if (i < server.locations.size() - 1) {
                std::cout << std::string(40, '-') << std::endl;
            }
        }
    }
}

// 显示完整配置
void displayFullConfig(const Config& config) {
    printSeparator("WEBSERV CONFIGURATION DISPLAY", '=');
    std::cout << "Total servers configured: " << config.getServerCount() << std::endl;
    std::cout << std::endl;
    
    if (config.empty()) {
        std::cout << "⚠️  WARNING: No server configurations found!" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < config.getServerCount(); ++i) {
        if (i > 0) {
            std::cout << std::endl << std::endl;
        }
        displayServerConfig(config.getServer(i), i);
    }
    
    printSeparator("END OF CONFIGURATION", '=');
}

// 辅助调试函数：显示配置摘要
void displayConfigSummary(const Config& config) {
    std::cout << "📊 CONFIGURATION SUMMARY" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << "Servers: " << config.getServerCount() << std::endl;
    
    size_t totalPorts = 0;
    size_t totalLocations = 0;
    
    for (size_t i = 0; i < config.getServerCount(); ++i) {
        const ServerConfig& server = config.getServer(i);
        totalPorts += server.listen.size();
        totalLocations += server.locations.size();
    }
    
    std::cout << "Total listening ports: " << totalPorts << std::endl;
    std::cout << "Total locations: " << totalLocations << std::endl;
    std::cout << "========================" << std::endl;
}