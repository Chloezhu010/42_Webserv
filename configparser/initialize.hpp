#ifndef INITIALIZE_HPP
#define INITIALIZE_HPP

#include "config.hpp"
#include "configparser.hpp"
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

// 服务器实例类
class ServerInstance {
private:
    ServerConfig config;
    std::vector<int> socketFds;     // 监听的socket文件描述符
    std::map<int, int> portToSocket; // 端口到socket的映射
    
public:
    ServerInstance(const ServerConfig& serverConfig);
    ~ServerInstance();
    
    bool initialize();
    bool startListening();
    void cleanup();
    
    // Getter方法
    const ServerConfig& getConfig() const { return config; }
    const std::vector<int>& getSocketFds() const { return socketFds; }
    bool isListeningOnPort(int port) const;
    int getSocketForPort(int port) const;
    
    // 辅助方法
    LocationConfig* findMatchingLocation(const std::string& path);
    bool matchesServerName(const std::string& hostHeader) const;
};

// Web服务器主类
class WebServer {
private:
    Config config;
    std::vector<ServerInstance*> servers;
    std::map<int, std::vector<ServerInstance*> > portToServers; // 端口到服务器的映射
    bool initialized;
    bool running;
    
    // 初始化辅助方法
    bool validateConfig();
    bool createServerInstances();
    bool setupPortMapping();
    void printServerInfo();
    
    // 清理方法
    void cleanup();

public:
    WebServer();
    ~WebServer();
    
    // 主要初始化方法
    bool initialize(const std::string& configFile);
    bool initializeFromConfig(const Config& cfg);
    
    // 服务器控制
    bool start();
    void stop();
    bool isRunning() const { return running; }
    
    // 获取服务器信息 - 只保留声明，定义移到 .cpp 文件
    const Config& getConfig() const;
    size_t getServerCount() const;
    ServerInstance* findServerByHost(const std::string& hostHeader, int port);
    
    // 错误处理
    std::string getLastError() const;
};

#endif // INITIALIZE_HPP