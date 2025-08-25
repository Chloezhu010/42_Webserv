#include "initialize.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <errno.h>

// =================== ServerInstance Implementation ===================

ServerInstance::ServerInstance(const ServerConfig& serverConfig) 
    : config(serverConfig) {
}

ServerInstance::~ServerInstance() {
    cleanup();
}

bool ServerInstance::initialize() {
    // 为每个监听端口创建socket
    for (int port : config.listen) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            std::cerr << "Failed to create socket for port " << port 
                      << ": " << strerror(errno) << std::endl;
            cleanup();
            return false;
        }
        
        // 设置socket选项
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            std::cerr << "Failed to set SO_REUSEADDR for port " << port 
                      << ": " << strerror(errno) << std::endl;
            close(sockfd);
            cleanup();
            return false;
        }
        
        // 设置非阻塞模式
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags == -1 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
            std::cerr << "Failed to set non-blocking mode for port " << port 
                      << ": " << strerror(errno) << std::endl;
            close(sockfd);
            cleanup();
            return false;
        }
        
        // 绑定端口
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            std::cerr << "Failed to bind to port " << port 
                      << ": " << strerror(errno) << std::endl;
            close(sockfd);
            cleanup();
            return false;
        }
        
        socketFds.push_back(sockfd);
        portToSocket[port] = sockfd;
        
        std::cout << "Socket created and bound to port " << port << std::endl;
    }
    
    return true;
}

bool ServerInstance::startListening() {
    for (size_t i = 0; i < socketFds.size(); i++) {
        int sockfd = socketFds[i];
        int port = config.listen[i];
        
        if (listen(sockfd, SOMAXCONN) == -1) {
            std::cerr << "Failed to listen on port " << port 
                      << ": " << strerror(errno) << std::endl;
            return false;
        }
        
        std::cout << "Listening on port " << port << std::endl;
    }
    
    return true;
}

void ServerInstance::cleanup() {
    for (int sockfd : socketFds) {
        if (sockfd != -1) {
            close(sockfd);
        }
    }
    socketFds.clear();
    portToSocket.clear();
}

bool ServerInstance::isListeningOnPort(int port) const {
    return portToSocket.find(port) != portToSocket.end();
}

int ServerInstance::getSocketForPort(int port) const {
    auto it = portToSocket.find(port);
    return (it != portToSocket.end()) ? it->second : -1;
}

LocationConfig* ServerInstance::findMatchingLocation(const std::string& path) {
    LocationConfig* bestMatch = nullptr;
    size_t maxLength = 0;
    
    for (auto& location : config.locations) {
        // 简单的前缀匹配
        if (path.substr(0, location.path.length()) == location.path) {
            if (location.path.length() > maxLength) {
                maxLength = location.path.length();
                bestMatch = &location;
            }
        }
    }
    
    return bestMatch;
}

bool ServerInstance::matchesServerName(const std::string& hostHeader) const {
    if (config.serverName.empty()) {
        return true; // 默认服务器
    }
    
    // 提取主机名（去掉端口）
    std::string host = hostHeader;
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        host = host.substr(0, colonPos);
    }
    
    // 检查是否匹配任何服务器名
    for (const std::string& serverName : config.serverName) {
        if (host == serverName || serverName == "_") {
            return true;
        }
    }
    
    return false;
}

// =================== WebServer Implementation ===================

WebServer::WebServer() : initialized(false), running(false) {
}

WebServer::~WebServer() {
    stop();
    cleanup();
}

bool WebServer::initialize(const std::string& configFile) {
    std::cout << "Initializing WebServer with config file: " << configFile << std::endl;
    
    // 解析配置文件
    ConfigParser parser;
    if (!parser.parseFile(configFile, config)) {
        std::cerr << "Failed to parse config file: " << parser.getLastError() << std::endl;
        return false;
    }
    
    return initializeFromConfig(config);
}

bool WebServer::initializeFromConfig(const Config& cfg) {
    config = cfg;
    
    // 验证配置
    if (!validateConfig()) {
        std::cerr << "Configuration validation failed" << std::endl;
        return false;
    }
    
    // 创建服务器实例
    if (!createServerInstances()) {
        std::cerr << "Failed to create server instances" << std::endl;
        cleanup();
        return false;
    }
    
    // 设置端口映射
    if (!setupPortMapping()) {
        std::cerr << "Failed to setup port mapping" << std::endl;
        cleanup();
        return false;
    }
    
    initialized = true;
    printServerInfo();
    
    return true;
}

bool WebServer::validateConfig() {
    if (config.empty()) {
        std::cerr << "No server configurations found" << std::endl;
        return false;
    }
    
    for (size_t i = 0; i < config.getServerCount(); i++) {
        const ServerConfig& server = config.getServer(i);
        
        // 检查是否有监听端口
        if (server.listen.empty()) {
            std::cerr << "Server " << i << " has no listen ports" << std::endl;
            return false;
        }
        
        // 验证端口范围
        for (int port : server.listen) {
            if (port < 1 || port > 65535) {
                std::cerr << "Invalid port number: " << port << std::endl;
                return false;
            }
            if (port < 1024) {
                std::cout << "Warning: Using privileged port " << port 
                          << " (requires root privileges)" << std::endl;
            }
        }
        
        // 验证根目录
        if (!server.root.empty()) {
            // 这里可以添加目录存在性检查
            std::cout << "Server " << i << " root directory: " << server.root << std::endl;
        }
    }
    
    return true;
}

bool WebServer::createServerInstances() {
    for (size_t i = 0; i < config.getServerCount(); i++) {
        const ServerConfig& serverConfig = config.getServer(i);
        
        ServerInstance* server = new ServerInstance(serverConfig);
        if (!server->initialize()) {
            delete server;
            return false;
        }
        
        servers.push_back(server);
    }
    
    return true;
}

bool WebServer::setupPortMapping() {
    // 为每个端口建立服务器映射
    for (ServerInstance* server : servers) {
        for (int port : server->getConfig().listen) {
            portToServers[port].push_back(server);
        }
    }
    
    // 检查端口冲突（同一端口上的多个服务器需要通过server_name区分）
    for (const auto& pair : portToServers) {
        int port = pair.first;
        const std::vector<ServerInstance*>& serverList = pair.second;
        
        if (serverList.size() > 1) {
            std::cout << "Port " << port << " is shared by " << serverList.size() 
                      << " servers (virtual hosting)" << std::endl;
            
            // 检查是否有默认服务器（没有server_name的服务器）
            bool hasDefault = false;
            for (ServerInstance* server : serverList) {
                if (server->getConfig().serverName.empty()) {
                    hasDefault = true;
                    break;
                }
            }
            
            if (!hasDefault) {
                std::cout << "Warning: No default server for port " << port << std::endl;
            }
        }
    }
    
    return true;
}

void WebServer::printServerInfo() {
    std::cout << "\n========== Server Configuration ==========" << std::endl;
    std::cout << "Total servers configured: " << servers.size() << std::endl;
    
    for (size_t i = 0; i < servers.size(); i++) {
        const ServerConfig& config = servers[i]->getConfig();
        std::cout << "\nServer " << (i + 1) << ":" << std::endl;
        
        // 监听端口
        std::cout << "  Listen ports: ";
        for (size_t j = 0; j < config.listen.size(); j++) {
            if (j > 0) std::cout << ", ";
            std::cout << config.listen[j];
        }
        std::cout << std::endl;
        
        // 服务器名
        if (!config.serverName.empty()) {
            std::cout << "  Server names: ";
            for (size_t j = 0; j < config.serverName.size(); j++) {
                if (j > 0) std::cout << ", ";
                std::cout << config.serverName[j];
            }
            std::cout << std::endl;
        }
        
        // 根目录
        if (!config.root.empty()) {
            std::cout << "  Root directory: " << config.root << std::endl;
        }
        
        // 客户端最大请求体大小
        std::cout << "  Max body size: " << config.clientMaxBodySize << " bytes" << std::endl;
        
        // Location数量
        std::cout << "  Locations configured: " << config.locations.size() << std::endl;
        
        // 错误页面
        if (!config.errorPages.empty()) {
            std::cout << "  Custom error pages: " << config.errorPages.size() << std::endl;
        }
    }
    
    std::cout << "==========================================\n" << std::endl;
}

bool WebServer::start() {
    if (!initialized) {
        std::cerr << "Server not initialized" << std::endl;
        return false;
    }
    
    if (running) {
        std::cout << "Server is already running" << std::endl;
        return true;
    }
    
    // 开始监听所有服务器
    for (ServerInstance* server : servers) {
        if (!server->startListening()) {
            std::cerr << "Failed to start listening on server" << std::endl;
            return false;
        }
    }
    
    running = true;
    std::cout << "WebServer started successfully!" << std::endl;
    
    return true;
}

void WebServer::stop() {
    if (!running) {
        return;
    }
    
    std::cout << "Stopping WebServer..." << std::endl;
    
    for (ServerInstance* server : servers) {
        server->cleanup();
    }
    
    running = false;
    std::cout << "WebServer stopped." << std::endl;
}

void WebServer::cleanup() {
    for (ServerInstance* server : servers) {
        delete server;
    }
    servers.clear();
    portToServers.clear();
}

ServerInstance* WebServer::findServerByHost(const std::string& hostHeader, int port) {
    auto it = portToServers.find(port);
    if (it == portToServers.end()) {
        return nullptr;
    }
    
    const std::vector<ServerInstance*>& serverList = it->second;
    
    // 首先尝试精确匹配服务器名
    for (ServerInstance* server : serverList) {
        if (server->matchesServerName(hostHeader)) {
            return server;
        }
    }
    
    // 如果没有找到匹配的，返回第一个（默认服务器）
    return serverList.empty() ? nullptr : serverList[0];
}

const Config& WebServer::getConfig() const {
    return config;
}

size_t WebServer::getServerCount() const {
    return servers.size();
}

std::string WebServer::getLastError() const {
    return "Check console output for detailed error messages";
}