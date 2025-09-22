#ifndef INITIALIZE_HPP
#define INITIALIZE_HPP

#include "config.hpp"
#include "configparser.hpp"
#include "../client/client_connection.hpp"
#include "../http/http_request.hpp" // handle http request
#include "../http/http_response.hpp" // handle http response
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
    Config config; // main storage for config
    std::vector<ServerInstance*> servers; // each holds ServerConfig copy
    std::map<int, std::vector<ServerInstance*> > portToServers; // port mapping
    bool initialized;
    bool running;

    std::map<int, ClientConnection*> clientConnections;  // fd -> 客户端连接
    fd_set readFds, writeFds;                           // select用的fd集合
    int maxFd;           
	
	void handleNewConnection(int serverFd);
    void handleClientRequest(int clientFd);
    void handleClientResponse(int clientFd);
    void closeClientConnection(int clientFd);
    void resetConnectionForResue(ClientConnection* conn);
    bool parseHttpRequest(ClientConnection* conn);
    void buildHttpResponse(ClientConnection* conn);
    // void sendStaticFile(ClientConnection* conn, const std::string& filePath); // replaced by HttpResponse class
    // void send404Response(ClientConnection* conn); // replaced by HttpResponse class
    void updateMaxFd();// 最大fd值
    
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
    
    /* initialization */
    bool initialize(const std::string& configFile); // parse config file andd setup server instance
    bool initializeFromConfig(const Config& cfg); // create ServerInstance objects from parsed config
    
    /* server lifecycle */
    bool start(); // start listening on all configured ports
    void stop(); // gracefully shut down all servers
    bool isRunning() const { return running; }
    
    // 获取服务器信息 - 只保留声明，定义移到 .cpp 文件
    const Config& getConfig() const;
    size_t getServerCount() const;
    ServerInstance* findServerByHost(const std::string& hostHeader, int port); // virtual host routing, by match http host header to server config
    int getPortFromClientSocket(int clientFd); // extract port from client socket
    
	void run();  // main event loop using select() for I/O multiplexing

    // 错误处理
    std::string getLastError() const;
};

#endif // INITIALIZE_HPP