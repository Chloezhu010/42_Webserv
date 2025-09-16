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
    for (size_t i = 0; i < config.listen.size(); ++i) {
        int port = config.listen[i];
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
    for (size_t i = 0; i < socketFds.size(); ++i) {
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

void WebServer::cleanup() {
    // 关闭所有客户端连接
    for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
         it != clientConnections.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    clientConnections.clear();
    
    // 清理服务器实例
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerInstance* server = servers[i];
        delete server;
    }
    servers.clear();
    portToServers.clear();
}

bool ServerInstance::isListeningOnPort(int port) const {
    return portToSocket.find(port) != portToSocket.end();
}

int ServerInstance::getSocketForPort(int port) const {
    std::map<int, int>::const_iterator it = portToSocket.find(port);
    return (it != portToSocket.end()) ? it->second : -1;
}

LocationConfig* ServerInstance::findMatchingLocation(const std::string& path) {
    LocationConfig* bestMatch = NULL;
    size_t maxLength = 0;
    
    for (size_t i = 0; i < config.locations.size(); ++i) {
        LocationConfig& location = config.locations[i];
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
    for (size_t i = 0; i < config.serverName.size(); ++i) {
        const std::string& serverName = config.serverName[i];
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
	std::cout << "\n";
    displayFullConfig(config);
    std::cout << "\n";
    
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
    
    for (size_t i = 0; i < config.getServerCount(); ++i) {
        const ServerConfig& server = config.getServer(i);
        
        // 检查是否有监听端口
        if (server.listen.empty()) {
            std::cerr << "Server " << i << " has no listen ports" << std::endl;
            return false;
        }
        
        // 验证端口范围
        for (size_t j = 0; j < server.listen.size(); ++j) {
            int port = server.listen[j];
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
    for (size_t i = 0; i < config.getServerCount(); ++i) {
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
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerInstance* server = servers[i];
        const std::vector<int>& listenPorts = server->getConfig().listen;
        for (size_t j = 0; j < listenPorts.size(); ++j) {
            int port = listenPorts[j];
            portToServers[port].push_back(server);
        }
    }
    
    // 检查端口冲突（同一端口上的多个服务器需要通过server_name区分）
    for (std::map<int, std::vector<ServerInstance*> >::const_iterator it = portToServers.begin();
         it != portToServers.end(); ++it) {
        int port = it->first;
        const std::vector<ServerInstance*>& serverList = it->second;
        
        if (serverList.size() > 1) {
            std::cout << "Port " << port << " is shared by " << serverList.size() 
                      << " servers (virtual hosting)" << std::endl;
            
            // 检查是否有默认服务器（没有server_name的服务器）
            bool hasDefault = false;
            for (size_t i = 0; i < serverList.size(); ++i) {
                ServerInstance* server = serverList[i];
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
    
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& config = servers[i]->getConfig();
        std::cout << "\nServer " << (i + 1) << ":" << std::endl;
        
        // 监听端口
        std::cout << "  Listen ports: ";
        for (size_t j = 0; j < config.listen.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << config.listen[j];
        }
        std::cout << std::endl;
        
        // 服务器名
        if (!config.serverName.empty()) {
            std::cout << "  Server names: ";
            for (size_t j = 0; j < config.serverName.size(); ++j) {
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
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerInstance* server = servers[i];
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
    
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerInstance* server = servers[i];
        server->cleanup();
    }
    
    running = false;
    std::cout << "WebServer stopped." << std::endl;
}

void ServerInstance::cleanup() {
    // 关闭所有监听的socket文件描述符
    for (size_t i = 0; i < socketFds.size(); ++i) {
        int sockfd = socketFds[i];
        if (sockfd != -1) {
            close(sockfd);
            std::cout << "Closed socket fd: " << sockfd << std::endl;
        }
    }
    
    // 清空容器
    socketFds.clear();
    portToSocket.clear();
    
    std::cout << "ServerInstance cleanup completed" << std::endl;
}

ServerInstance* WebServer::findServerByHost(const std::string& hostHeader, int port) {
    std::map<int, std::vector<ServerInstance*> >::iterator it = portToServers.find(port);
    if (it == portToServers.end()) {
        return NULL;
    }
    
    const std::vector<ServerInstance*>& serverList = it->second;
    
    // 首先尝试精确匹配服务器名
    for (size_t i = 0; i < serverList.size(); ++i) {
        ServerInstance* server = serverList[i];
        if (server->matchesServerName(hostHeader)) {
            return server;
        }
    }
    
    // 如果没有找到匹配的，返回第一个（默认服务器）
    return serverList.empty() ? NULL : serverList[0];
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


void WebServer::run() {
    if (!running) {
        std::cerr << "Server not running" << std::endl;
        return;
    }
    
    std::cout << "Starting main event loop..." << std::endl;
    
    // 初始化maxFd
    updateMaxFd();
    
    while (running) {
        // 清除fd集合
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);
        
        // 添加所有监听socket到读集合
        for (size_t i = 0; i < servers.size(); ++i) {
            const std::vector<int>& socketFds = servers[i]->getSocketFds();
            for (size_t j = 0; j < socketFds.size(); ++j) {
                int fd = socketFds[j];
                FD_SET(fd, &readFds);
            }
        }
        
        // 添加客户端连接到相应集合
        for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
             it != clientConnections.end(); ++it) {
            int fd = it->first;
            ClientConnection* conn = it->second;
            
            if (!conn->request_complete) {
                FD_SET(fd, &readFds);  // 等待读取请求
            }
            if (conn->response_ready && conn->bytes_sent < conn->response_buffer.size()) {
                FD_SET(fd, &writeFds); // 等待发送响应
            }
        }
        
        // 设置超时
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        // 使用select监控文件描述符
        int activity = select(maxFd + 1, &readFds, &writeFds, NULL, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) {
                continue; // 被信号中断，继续循环
            }
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            break;
        }
        
        if (activity == 0) {
            // 超时，继续循环
            continue;
        }
        
        // 检查监听socket是否有新连接
        for (size_t i = 0; i < servers.size(); ++i) {
            const std::vector<int>& socketFds = servers[i]->getSocketFds();
            for (size_t j = 0; j < socketFds.size(); ++j) {
                int serverFd = socketFds[j];
                if (FD_ISSET(serverFd, &readFds)) {
                    handleNewConnection(serverFd);
                }
            }
        }
        
        // 检查客户端连接事件
        std::vector<int> fdsToRemove;
        for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
             it != clientConnections.end(); ++it) {
            int clientFd = it->first;
            
            if (FD_ISSET(clientFd, &readFds)) {
                handleClientRequest(clientFd);
            }
            if (FD_ISSET(clientFd, &writeFds)) {
                handleClientResponse(clientFd);
            }
            
            // 检查是否需要关闭连接
            ClientConnection* conn = it->second;
            if (conn->response_ready && conn->bytes_sent >= conn->response_buffer.size()) {
                // For HTTP/1.1, keep the connection alive by default unless "Connection: close"
                bool keep_alive = false;
                if (conn->http_request && conn->http_request->getIsParsed())
                    keep_alive = conn->http_request->getConnection();
                // std::cout << "🚧 DEBUG: keep_alive=" << (keep_alive ? "true" : "false") << std::endl;
                if (!keep_alive)
                    fdsToRemove.push_back(clientFd);
                else
                    resetConnectionForResue(conn); // reset for next request
            }
        }
        
        // 关闭已完成的连接
        for (size_t i = 0; i < fdsToRemove.size(); ++i) {
            closeClientConnection(fdsToRemove[i]);
        }
    }
    
    std::cout << "Event loop ended." << std::endl;
}

void WebServer::handleNewConnection(int serverFd) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd == -1) {
        std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
        return;
    }
    
    // 设置非阻塞模式
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking mode" << std::endl;
        close(clientFd);
        return;
    }
    
    // 创建客户端连接对象
    ClientConnection* conn = new ClientConnection(clientFd);
    clientConnections[clientFd] = conn;
    
    // 更新maxFd
    if (clientFd > maxFd) {
        maxFd = clientFd;
    }
    
    std::cout << "New connection accepted: fd=" << clientFd << std::endl;
}

// // original simplified handle client request
// void WebServer::handleClientRequest(int clientFd) {
//     ClientConnection* conn = clientConnections[clientFd];
//     if (!conn) return;
    
//     char buffer[4096];
//     ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    
//     if (bytesRead <= 0) {
//         if (bytesRead == 0) {
//             std::cout << "Client disconnected: fd=" << clientFd << std::endl;
//         } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
//             std::cerr << "recv() failed: " << strerror(errno) << std::endl;
//         }
//         closeClientConnection(clientFd);
//         return;
//     }
    
//     buffer[bytesRead] = '\0';
//     conn->request_buffer += buffer;
    
//     // 检查是否收到完整的HTTP请求
//     if (conn->request_buffer.find("\r\n\r\n") != std::string::npos) {
//         conn->request_complete = true;
        
//         // 解析并处理请求
//         if (parseHttpRequest(conn)) {
//             buildHttpResponse(conn);
//             conn->response_ready = true;
//         } else {
//             // 解析失败，发送400错误
//             conn->response_buffer = "HTTP/1.1 400 Bad Request\r\n"
//                                    "Content-Length: 0\r\n"
//                                    "Connection: close\r\n\r\n";
//             conn->response_ready = true;
//         }
//     }
// }


// // orginal simplified parse request
// bool WebServer::parseHttpRequest(ClientConnection* conn) {
//     // 简单的HTTP请求解析
//     std::istringstream iss(conn->request_buffer);
//     std::string line;
    
//     // 解析请求行
//     if (!std::getline(iss, line)) return false;
    
//     std::istringstream requestLine(line);
//     std::string method, path, version;
//     requestLine >> method >> path >> version;
    
//     if (method.empty() || path.empty()) return false;
    
//     // 存储解析结果（你可以扩展ClientConnection结构体来存储这些信息）
//     // 这里简单存储在response_buffer中作为临时方案
//     conn->response_buffer = path; // 临时存储path
    
//     std::cout << "Parsed request: " << method << " " << path << " " << version << std::endl;
//     return true;
// }


/* complete handle client request, integrated with HttpRequest
    - request reception
    - check request completeness
    - if request complete
        - parse request
        - prepare for response
    - if request invalid
        - prepare error response
    - if need more data
        - keep building the buffer
*/
void WebServer::handleClientRequest(int clientFd) {
    /* request reception */
    ClientConnection* conn = clientConnections[clientFd];
    if (!conn) return;
    
    char buffer[4096];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "Client disconnected: fd=" << clientFd << std::endl;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "recv() failed: " << strerror(errno) << std::endl;
        }
        closeClientConnection(clientFd);
        return;
    }
    
    buffer[bytesRead] = '\0';
    conn->request_buffer += buffer;

    /* check for request completeness & parsing & response */
    // create HttpRequest & HttpResponse object if not exists
    if (!conn->http_request)
        conn->http_request = new HttpRequest();
    if (!conn->http_response)
        conn->http_response = new HttpResponse();
    // check request completeness
    RequestStatus status = conn->http_request->isRequestComplete(conn->request_buffer);
    if (status == REQUEST_COMPLETE) // request is complete
    {
        conn->request_complete = true;
        if (parseHttpRequest(conn)) // parse & validate request successfully
        {
            buildHttpResponse(conn);
            conn->response_ready = true;
        }
        else // parse & validate request fails
        {
            conn->response_buffer = conn->http_response->buildErrorResponse(400, "Bad Request");
            conn->response_ready = true;
        }
    }
    // TBU: need to customize the error msg to accomendate request_too_large
    else if (status == REQUEST_TOO_LARGE)
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(400, "Bad Request");
        conn->response_ready = true;
    }
    else if (status == INVALID_REQUEST)
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(400, "Bad Request");
        conn->response_ready = true;
    }
    // if status == NEED_MORE_DATA, keep building the buffer
}


/* complete parse request, integrated with HttpRequest
    @purpose: convert raw http request into a structured, validated request object
    @return: true if request parsed & validated successfully, false otherwise
*/
bool WebServer::parseHttpRequest(ClientConnection* conn) {

    /* parse the request */
    if (!conn->http_request->parseRequest(conn->request_buffer))
    {
        std::cerr << "HTTP request parsing failed\n";

        // std::cout << "=== Debug ===:"
        //         << "\nmethod: " << conn->http_request->getMethodStr()
        //         << "\nuri: " << conn->http_request->getURI()
        //         << "\nhttp version: " << conn->http_request->getHttpVersion()
        //         << "\nquery string: " << conn->http_request->getQueryString()
        //         << "\nhost: " << conn->http_request->getHost()
        //         << "\nbody: " << conn->http_request->getBody()
        //         << "\nis complete: " << conn->http_request->getIsComplete()
        //         << "\nis parsed: " << conn->http_request->getIsParsed()
        //         << "\nis connected: " << conn->http_request->getConnection()
        //         << "\nvalidation status: " << conn->http_request->getValidationStatus()
        //         << std::endl;

        return false;
    }
    /* validate the parsed request */
    ValidationResult val_status = conn->http_request->validateRequest();
    if (val_status != VALID_REQUEST)
    {
        std::cerr << "HTTP request validation failed: " << val_status << std::endl;
        return false;
    }

    std::cout << "Parsed request: "
                << conn->http_request->getMethodStr() << " "
                << conn->http_request->getURI() << " "
                << conn->http_request->getHttpVersion() << " " << std::endl;
    return true;
}

// // original simplified http response building
// void WebServer::buildHttpResponse(ClientConnection* conn) {
//     // 从临时存储中获取path
//     std::string path = conn->response_buffer;
//     conn->response_buffer.clear();
    
//     // 处理根路径
//     if (path == "/") {
//         path = "/index.html";
//     }
    
//     // 构建完整文件路径（使用第一个服务器的root配置）
//     std::string filePath = "./www" + path; // 可以改进为根据Host头选择正确的服务器
    
//     // 尝试发送文件
//     sendStaticFile(conn, filePath);
// }

/* helper function for buildHttpResponse
    - build the response for GET
*/
static void handleGetResponse(ClientConnection* conn, std::string& uri)
{
    // map uri to file path
    std::string file_path = "./www" + uri;
    if (uri == "/")
        file_path = "./www/index.html";
    // use HttpResponse to serve the file
    conn->response_buffer = conn->http_response->buildFileResponse(file_path);
}

/* helper function for buildHttpResponse
    - build the response for POST, should process the data
*/
static void handlePostResponse(ClientConnection* conn, std::string& uri)
{
    // TODO: need to update incorporating config file uri 
    (void)uri;
    // set success state and body for response
    conn->http_response->setStatusCode(200);
    conn->http_response->setBody("<h1>POST successful</h1>\r\n\r\n");
    conn->http_response->setHeader("Content-Type", "text/html");
    // update the response_buffer
    conn->response_buffer = conn->http_response->buildFullResponse(*conn->http_request);
}

/* helper function for buildHttpResponse
    - build the response for DELETE, should delete resources
*/
static void handleDeleteResponse(ClientConnection* conn, std::string& uri)
{
    // TODO: need to update incorporating config file uri 
    // Map URI to file path
    std::string file_path = "./www" + uri;
    if (uri == "/")
        file_path = "./www/index.html";
    // if file doesn't exist
    if (access(file_path.c_str(), F_OK) != 0)
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(404, "Not Found");
    }
    // if file exist, try to delete
    else
    {
        // delete successfully
        if (unlink(file_path.c_str()) == 0)
        {
            conn->http_response->setStatusCode(200);
            conn->response_buffer = conn->http_response->buildFullResponse(*conn->http_request);
        }
        // cannot delete
        else
            conn->response_buffer = conn->http_response->buildErrorResponse(403, "Forbidden");
    }
}

/* complete http response generation
    @purpose: generate correspondent http response based on the validated request
*/
void WebServer::buildHttpResponse(ClientConnection* conn) {
    // create HttpResponse object if needed
    if (!conn->http_response)
        conn->http_response = new HttpResponse();
    // get validation status from the parsed request
    ValidationResult val_status = conn->http_request->getValidationStatus();

    // if not VALID_REQUEST
    if (val_status != VALID_REQUEST)
    {
        conn->response_buffer = conn->http_response->buildFullResponse(*conn->http_request);
    }
    else // if VALID_REQUEST
    {
        // get the method and uri
        std::string method = conn->http_request->getMethodStr();
        std::string uri = conn->http_request->getURI();

        if (method == "GET")
            handleGetResponse(conn, uri);
        else if (method == "POST")
            handlePostResponse(conn, uri);
        else if (method == "DELETE")
            handleDeleteResponse(conn, uri);
    }
}

/* send prepared http response to client over the socket connection */
void WebServer::handleClientResponse(int clientFd) {
    ClientConnection* conn = clientConnections[clientFd];
    if (!conn || !conn->response_ready) return;
    
    size_t remaining = conn->response_buffer.size() - conn->bytes_sent;
    if (remaining == 0) return;
    
    const char* data = conn->response_buffer.c_str() + conn->bytes_sent;
    ssize_t bytesSent = send(clientFd, data, remaining, 0);
    
    if (bytesSent > 0) {
        conn->bytes_sent += bytesSent;
        std::cout << "Sent " << bytesSent << " bytes to fd=" << clientFd << std::endl;
    } else if (bytesSent == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "send() failed: " << strerror(errno) << std::endl;
        closeClientConnection(clientFd);
    }
}

void WebServer::sendStaticFile(ClientConnection* conn, const std::string& filePath) {
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        send404Response(conn);
        return;
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 读取文件内容
    std::string content;
    content.resize(fileSize);
    file.read(&content[0], fileSize);
    file.close();
    
    // 构建HTTP响应
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << fileSize << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;
    
    conn->response_buffer = response.str();
    std::cout << "Serving file: " << filePath << " (" << fileSize << " bytes)" << std::endl;
}

void WebServer::send404Response(ClientConnection* conn) {
    std::string content = "<html><body><h1>404 Not Found</h1></body></html>";
    
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;
    
    conn->response_buffer = response.str();
}

void WebServer::resetConnectionForResue(ClientConnection* conn) {
    // reset connection state for next request
    conn->request_buffer.clear();
    conn->response_buffer.clear();
    conn->bytes_sent = 0;
    conn->request_complete = false;
    conn->response_ready = false;
    if (conn->http_request) {
        delete conn->http_request;
        conn->http_request = NULL;
    }
    if (conn->http_response) {
        delete conn->http_response;
        conn->http_response = NULL;
    }
    std::cout << "Connection reset for reuse: fd=" << conn->fd << std::endl;
}

void WebServer::closeClientConnection(int clientFd) {
    std::map<int, ClientConnection*>::iterator it = clientConnections.find(clientFd);
    if (it != clientConnections.end()) {
        delete it->second;
        clientConnections.erase(it);
    }
    close(clientFd);
    updateMaxFd();
    std::cout << "Connection closed: fd=" << clientFd << std::endl;
}

void WebServer::updateMaxFd() {
    maxFd = -1;
    
    // 检查监听socket
    for (size_t i = 0; i < servers.size(); ++i) {
        const std::vector<int>& socketFds = servers[i]->getSocketFds();
        for (size_t j = 0; j < socketFds.size(); ++j) {
            if (socketFds[j] > maxFd) {
                maxFd = socketFds[j];
            }
        }
    }
    
    // 检查客户端连接
    for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
         it != clientConnections.end(); ++it) {
        if (it->first > maxFd) {
            maxFd = it->first;
        }
    }
}