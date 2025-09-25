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

    // location iteration
    for (size_t i = 0; i < config.locations.size(); ++i) {
        LocationConfig& location = config.locations[i];
        bool match = false;

        // 1. exact match
        if (location.path == path)
            match = true;
        // 2. directory match: eg. /github/ should match /github
        else if (location.path.length() > 1 && location.path[location.path.length() - 1] == '/'
                 && path + "/" == location.path)
            match = true;
        // 3. prefix match with boundary checkes
        else if (location.path.length() <= path.length()
                && path.substr(0, location.path.length()) == location.path)
                {
                    // boundary check
                    if (location.path[location.path.length() - 1] == '/' // location ends with '/'
                        || path.length() == location.path.length() // exact length match
                        || path[location.path.length()] == '/') // next char is '/
                        match = true;
                }
        // update maxLength & bestMatch
        if (match && location.path.length() > maxLength) {
            maxLength = location.path.length();
            bestMatch = &location;
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
    // port lookup: find all servers listening on this port
    std::map<int, std::vector<ServerInstance*> >::iterator it = portToServers.find(port);
    if (it == portToServers.end()) {
        return NULL; // no server listening on this port
    }
    
    const std::vector<ServerInstance*>& serverList = it->second;
    
    // host header lookup: find the server that matches the host header
    for (size_t i = 0; i < serverList.size(); ++i) {
        ServerInstance* server = serverList[i];
        if (server->matchesServerName(hostHeader)) {
            return server;
        }
    }
    
    // default server fallback: if no exact match, return the 1st server as default
    return serverList.empty() ? NULL : serverList[0];
}

int WebServer::getPortFromClientSocket(int clientFd) {
    // init
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    // get socket name
    if (getsockname(clientFd, (struct sockaddr*)&addr, &addrlen) == 0)
        return ntohs(addr.sin_port);
    // error case
    return -1;
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
    // ensure server is running before entering event loop
    if (!running) {
        std::cerr << "Server not running" << std::endl;
        return;
    }
    
    std::cout << "Starting main event loop..." << std::endl;
    // init maxFd to find the highest fd for select() call
    updateMaxFd();
    
    while (running) {
        // clear previous iteration's fd sets for select()
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);
        
        // add all server listening sockets to read set - monitor for new connections
        for (size_t i = 0; i < servers.size(); ++i) {
            const std::vector<int>& socketFds = servers[i]->getSocketFds();
            for (size_t j = 0; j < socketFds.size(); ++j) {
                int fd = socketFds[j];
                FD_SET(fd, &readFds);
            }
        }
        
        /* client connection monitoring */ 
        for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
             it != clientConnections.end(); ++it) {
            int fd = it->first;
            ClientConnection* conn = it->second;
            // add clients that wait for request data to read set
            if (!conn->request_complete) {
                FD_SET(fd, &readFds);  // 等待读取请求
            }
            // add clients that have response ready to write set
            if (conn->response_ready && conn->bytes_sent < conn->response_buffer.size()) {
                FD_SET(fd, &writeFds); // 等待发送响应
            }
        }
        
        /* select() call */ 
        // setup timeout to periodically wake up and check running flag
        struct timeval timeout;
        timeout.tv_sec = 1; // 1 second timeout
        timeout.tv_usec = 0;
        
        // return the number of fds ready for read/write
        int activity = select(maxFd + 1, &readFds, &writeFds, NULL, &timeout);
        // error handling
        if (activity < 0) {
            if (errno == EINTR) {
                continue; // 被信号中断，继续循环
            }
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            break;
        }
        // timeout handling
        if (activity == 0) {
            // timetout occurred, fall through to connection cleanup
            // continue;
        }
        
        /* new connection handling */ 
        // if the server socket is readable, then accept new connections on all listening sockets
        for (size_t i = 0; i < servers.size(); ++i) {
            const std::vector<int>& socketFds = servers[i]->getSocketFds();
            for (size_t j = 0; j < socketFds.size(); ++j) {
                int serverFd = socketFds[j];
                if (FD_ISSET(serverFd, &readFds)) {
                    handleNewConnection(serverFd);
                }
            }
        }
        
        /* client request/response handling */
        for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
             it != clientConnections.end();)
        {
            int clientFd = it->first;
            ClientConnection* conn = it->second;
            // bool deleted = false;

            // safe iteration: save next iterators before possible deletion
            std::map<int, ClientConnection*>::iterator next_it = it;
            ++next_it;
            
            // if the client fd is readable, handle http request
            if (FD_ISSET(clientFd, &readFds)) {
                handleClientRequest(clientFd);
            }
            // if the client fd is writable, handle http response
            if (FD_ISSET(clientFd, &writeFds)) {
                handleClientResponse(clientFd);
            }

            /* connection lifecycle management */
            // if the request response is ready, and completely sent, then close or reset the connection
            if (conn->response_ready && conn->bytes_sent >= conn->response_buffer.size()) {
                // For HTTP/1.1, keep the connection alive by default unless "Connection: close"
                bool keep_alive = true;
                if (conn->http_response) {
                    // check of response header reset the connection to close
                    std::string response_connection = conn->http_response->getHeader("Connection");
                    std::cout << "🚧 DEBUG: response connection: " << response_connection << std::endl;
                    std::cout << "🚧 DEBUG: request connection: " << conn->http_request->getConnection() << std::endl;
                    if (response_connection == "close")
                        keep_alive = false;
                    else
                        keep_alive = true;
                } 
                else if (conn->http_request && conn->http_request->getIsParsed())
                        keep_alive = conn->http_request->getConnection();
                std::cout << "🚧 DEBUG: keep_alive: " << keep_alive << std::endl;

                
                if (!keep_alive)
                    closeClientConnection(clientFd); // close connection
                else
                    resetConnectionForResue(conn); // reset for next request
            }
            it = next_it; // move to next client in map
        }
        /* handle connection timeout
            - auto close the connection when idle +30 seconds
        */
        time_t current_time = time(NULL);
        for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
            it != clientConnections.end();) 
            {
                ClientConnection* conn = it->second;
                time_t elapse = current_time - conn->last_active;

                // 30 seconds timeout
                if (elapse > 30)
                {
                    int fd = it->first;
                    std::cout << "Connection timed out: fd=" << fd << std::endl;
                    std::map<int, ClientConnection*>::iterator next_it = it;
                    ++next_it;
                    closeClientConnection(fd);
                    it = next_it; // move to next client in map
                }
                else
                    ++it;
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
    conn->last_active = time(NULL); // init last active time
    clientConnections[clientFd] = conn;
    
    // 更新maxFd
    if (clientFd > maxFd) {
        maxFd = clientFd;
    }
    
    std::cout << "New connection accepted: fd=" << clientFd << std::endl;
}

/* helper function for handleClientRequest */
static void trimValidateRequestBuffer(std::string& request_buffer) {
    // trim leading CRLF (valid between requests)
    size_t start = 0;
    while (start < request_buffer.length())
    {
        char c = request_buffer[start];
        if (c == '\r' || c == '\n')
            start++; // skip leading CRLF
        else
            break; // stop when found non-CRLF char
    }
    // apply trim if found leading CRLF
    if (start > 0)
    {
        request_buffer = request_buffer.substr(start);
        // std::cout << "🚧 DEBUG: after trim, request_buffer: [" << request_buffer << "]" << std::endl;  
    }
}

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
    // ClientConnection* conn = clientConnections[clientFd]; // cause segfault if clientFd not found
    std::map<int, ClientConnection*>::iterator it = clientConnections.find(clientFd);
    if (it == clientConnections.end()) return;
    ClientConnection* conn = it->second;
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
    } else {
        buffer[bytesRead] = '\0';
        conn->request_buffer += buffer;
        conn->last_active = time(NULL); // update last active time
    }

    /* check for request completeness & parsing & response */
    // create HttpRequest & HttpResponse object if not exists
    if (!conn->http_request)
        conn->http_request = new HttpRequest();
    if (!conn->http_response)
        conn->http_response = new HttpResponse();

    
    // std::cout << "🚧 DEBUG: Current request_buffer: [" << conn->request_buffer << "]" << std::endl;
    // trim the request line if there is leading CRLF
    trimValidateRequestBuffer(conn->request_buffer);

    // check request completeness
    RequestStatus status = conn->http_request->isRequestComplete(conn->request_buffer);
    // std::cout << "🚧 DEBUG: isRequestComplete status: " << status << std::endl;

    if (status == REQUEST_COMPLETE) // request is complete
    {
        conn->request_complete = true;
        if (parseHttpRequest(conn)) // parse & validate request successfully
        {
            // extract host header and port
            std::string host = conn->http_request->getHost();
            int port = getPortFromClientSocket(clientFd); 
            // find the matching server instance, if not found, fall back to the first server
            if (port == -1)
                conn->server_instance = servers.empty() ? NULL : servers[0];
            else
                conn->server_instance = findServerByHost(host, port);
            // extract request uri
            std::string uri = conn->http_request->getURI();
            // find the matching location, if not found, set to NULL
            conn->matched_location = conn->server_instance->findMatchingLocation(uri);

            // build the response
            buildHttpResponse(conn);
            // mark response ready
            conn->response_ready = true;
        }
        else // parse & validate request fails
        {
            
            conn->http_response->resultToStatusCode(conn->http_request->getValidationStatus());
            conn->response_buffer = conn->http_response->buildErrorResponse(conn->http_response->getStatusCode(), "TBU", *conn->http_request);
            conn->response_ready = true;
        }
    }
    else if (status == REQUEST_TOO_LARGE)
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(413, "Content Too Large", *conn->http_request);
        conn->response_ready = true;
    }
    else if (status == INVALID_REQUEST)
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(400, "Bad Request", *conn->http_request);
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

static void generateDirListing(ClientConnection* conn, const std::string& dir_path)
{
    // generate HTML dir listing
    std::ostringstream html;
    html << "<html><head><title>Directory Listing</title></head>\r\n";
    html << "<body><h1>Index of " << dir_path << "</h1>\r\n";
    html << "</ul></body></html>";
    conn->http_response->setStatusCode(200);
    conn->http_response->setHeader("Content-Type", "text/html");
    conn->http_response->setBody(html.str());
    conn->response_buffer = conn->http_response->buildFullResponse(*conn->http_request);
}

/* helper function for handleGetResponse */
static void handleDirRequest(ClientConnection* conn, const std::string& file_path, const std::string& uri)
{
    // URI should have trailing slash (redirect if missing)
    (void)uri; // TODO

    // get index files from config (location overrides server)
    std::vector<std::string> index_files = conn->server_instance->getConfig().index;
    if (conn->matched_location && !conn->matched_location->index.empty())
        index_files = conn->matched_location->index; // location-specific index overrides server index
    // try each index file in order
    for (size_t i = 0; i < index_files.size(); ++i)
    {
        std::string index_path = file_path;
        // add trailing slash if missing
        if (index_path[index_path.length() - 1] != '/')
            index_path += "/";
        index_path += index_files[i];
        // if file exists, serve it
        if (access(index_path.c_str(), F_OK) == 0) // file exists
        {
            conn->response_buffer = conn->http_response->buildFileResponse(index_path, *conn->http_request);
            return;
        }
    }
    // no index file found, check autoindex enabled
    if (conn->matched_location && conn->matched_location->autoindex)
    {
        generateDirListing(conn, file_path);
        return;
    }
    // no index file found, no autoindex enabled
    conn->response_buffer = conn->http_response->buildErrorResponse(403, "Forbidden", *conn->http_request);
}

/* helper function for handleGetResponse
    - check for method permission in the config
*/
static bool isMethodAllowed(const std::string& method, const LocationConfig* location)
{
    // if no location matched, allow all methods by default
    if (!location)
        return true;
    // if location has no methods specified, allow all methods
    if (location->allowMethods.empty())
        return true;
    // check if method is in the allowed methods list
    for (size_t i = 0; i < location->allowMethods.size(); ++i)
    {
        if (method == location->allowMethods[i]) // method found in the list
            return true;
    }
    return false; // method not found in the list
}

/* helper function for buildHttpResponse： handle redirects */
static void handleRedirect(ClientConnection* conn)
{
    // extract redirect string from location config
    std::string redirect_str = conn->matched_location->redirect;
    // parse the redirect str, eg. "301 /newpath"
    size_t space_pos = redirect_str.find(' ');
    if (space_pos == std::string::npos)
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(500, "Internal Server Error", *conn->http_request);
        return;
    }
    int status_code = atoi(redirect_str.substr(0, space_pos).c_str());
    std::string redirect_url = redirect_str.substr(space_pos + 1);
    // build redirect response
    conn->http_response->setStatusCode(status_code);
    conn->http_response->setHeader("Location", redirect_url);
    conn->http_response->setBody("");
    conn->response_buffer = conn->http_response->buildFullResponse(*conn->http_request);
    // log redirect
    std::cout << "Redirecting to: " << redirect_url << " (" << status_code << ")" << std::endl;
}

/* helper function for buildHttpResponse： build the response for GET
    - Method not allowed -> 405
    - Redirect -> 3xx
    - Path is directory
        - index file exists -> 200 serve index file
        - no index file, autoindex on -> 200 generate dir listing
        - no index file, autoindex off -> 403
    - Path is file
        - file exists -> 200 serve file
        - file not exists -> 404
*/
static void handleGetResponse(ClientConnection* conn, std::string& uri)
{
    /* check for method permission */
    if (!isMethodAllowed("GET", conn->matched_location))
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(405, "Method Not Allowed", *conn->http_request);
        return;
    }
    /* check for redirects */
    if (conn->matched_location && !conn->matched_location->redirect.empty())
    {
        // std::cout << "🚧 DEBUG: redirect str " << conn->matched_location->redirect << std::endl;
        handleRedirect(conn);
        return;
    }
    /* determine root path 
        - extract root from server config / location config
    */
    std::string root = conn->server_instance->getConfig().root;
    if (conn->matched_location && !conn->matched_location->root.empty())
        root = conn->matched_location->root; // location-specific root overrides server root
    /* construct the full file path */
    std::string file_path = root + uri;
    /* handle directory request
        - check if it's a directory request
        - if directory, check for index files
        - if no index files, check for autoindex
    */
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) // is directory
    {
        handleDirRequest(conn, file_path, uri);
        return;
    }
    /* serve the file */
    conn->response_buffer = conn->http_response->buildFileResponse(file_path, *conn->http_request);
}

/* helper function for buildHttpResponse: build the response for POST, should process the data
    - Method not allowed -> 405
    - Client body too large -> 413
    - Success -> 200
*/
static void handlePostResponse(ClientConnection* conn, std::string& uri)
{
    /* check for method permission */
    if (!isMethodAllowed("POST", conn->matched_location)) {
        conn->request_buffer = conn->http_response->buildErrorResponse(405, "Method Not Allowed", *conn->http_request);
        return;
    }
    /* determine root path */
    std::string root = conn->server_instance->getConfig().root;
    if (conn->matched_location && !conn->matched_location->root.empty())
        root = conn->matched_location->root;
    /* construct the full file path */
    std::string file_path = root + uri;
    /* client body size validation */
    size_t configMaxBodySize = conn->server_instance->getConfig().clientMaxBodySize;
    size_t requestBodySize = conn->http_request->getBody().size();
    if (requestBodySize > configMaxBodySize) {
        conn->response_buffer = conn->http_response->buildErrorResponse(413, "Content Too Large", *conn->http_request);
        return;
    }

    /* CGI support */
    // detect CGI request based on location config

    // set up environment variables

    // execute the script with the configured interpreter

    // capture stdout and return it as HTTP response

    /* handle file upload */


    // Simple version: set success state and body for response
    conn->http_response->setStatusCode(200);
    conn->http_response->setBody("<h1>POST successful</h1>\r\n\r\n");
    conn->http_response->setHeader("Content-Type", "text/html");
    
    // update the response_buffer
    conn->response_buffer = conn->http_response->buildFullResponse(*conn->http_request);
}


/* helper function for buildHttpResponse: build the response for DELETE, should delete resources
    - Method not allowed -> 405
    - Path is directory -> 403
    - Path is file:
        - File not exists -> 404
        - Delete success -> 200
        - Delete fail -> 403
*/
static void handleDeleteResponse(ClientConnection* conn, std::string& uri)
{
    /* check for method permission */
    if (!isMethodAllowed("DELETE", conn->matched_location)) {
        conn->response_buffer = conn->http_response->buildErrorResponse(405, "Method Not Allowed", *conn->http_request);
        return;
    }
    /* determine root path */
    std::string root = conn->server_instance->getConfig().root;
    if (conn->matched_location && !conn->matched_location->root.empty())
        root = conn->matched_location->root;
    /* construct the full file path */
    std::string file_path = root + uri;
    /* check if the path is a directory */
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) // is directory
    {
        conn->response_buffer = conn->http_response->buildErrorResponse(403, "Forbidden", *conn->http_request);
        // handleDeleteDirRequest(conn, file_path);
        return;
    }
    /* check the file and try to delete */
    // if file exists
    if (access(file_path.c_str(), F_OK) == 0) {
        // delete successfully
        if (unlink(file_path.c_str()) == 0)
        {
            conn->http_response->setStatusCode(200);
            conn->response_buffer = conn->http_response->buildFullResponse(*conn->http_request);
        }
        // cannot delete
        else
            conn->response_buffer = conn->http_response->buildErrorResponse(403, "Forbidden", *conn->http_request);
    }
    else
        conn->response_buffer = conn->http_response->buildErrorResponse(404, "Not Found", *conn->http_request);
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
    // ClientConnection* conn = clientConnections[clientFd]; // cause segfault if clientFd not found
    std::map<int, ClientConnection*>::iterator it = clientConnections.find(clientFd);
    if (it == clientConnections.end()) return;
    ClientConnection* conn = it->second;
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

void WebServer::resetConnectionForResue(ClientConnection* conn) {
    // reset connection state for next request

    // std::cout << "🚧 DEBUG: before reset, request_buffer: [" << conn->request_buffer << "]" << std::endl;
    conn->request_buffer.clear();
    // std::cout << "🚧 DEBUG: afer reset, request_buffer: [" << conn->request_buffer << "]" << std::endl;
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
    conn->server_instance = NULL;
    conn->matched_location = NULL;
    conn->last_active = time(NULL); 
    // log reset
    std::cout << "Connection reset for reuse: fd=" << conn->fd << std::endl;
}

void WebServer::closeClientConnection(int clientFd) {
    // std::cout << "🚧 DEBUG: closeClientConnection called for fd=" << clientFd << std::endl;

    std::map<int, ClientConnection*>::iterator it = clientConnections.find(clientFd);
    if (it != clientConnections.end()) {
        // std::cout << "🚧 DEBUG: Found connection, deleteing..." << std::endl;
        delete it->second;
        // std::cout << "🚧 DEBUG: Erase the connection from the map..." << std::endl;
        clientConnections.erase(it);
    }
    else
    {
        // std::cout << "🚧 DEBUG: Connection not found in map!" << std::endl;
    }
    close(clientFd);
    updateMaxFd();
    std::cout << "Connection closed: fd=" << clientFd << std::endl;
}

// find the current max fd nb being used by the server and stores it in maxFd
void WebServer::updateMaxFd() {
    // reset maxFd
    maxFd = -1;
    // check all listening socket
    for (size_t i = 0; i < servers.size(); ++i) {
        const std::vector<int>& socketFds = servers[i]->getSocketFds();
        for (size_t j = 0; j < socketFds.size(); ++j) {
            if (socketFds[j] > maxFd) {
                maxFd = socketFds[j]; // update maxFd if higher
            }
        }
    }
    // check all client connecting sockets
    for (std::map<int, ClientConnection*>::iterator it = clientConnections.begin();
         it != clientConnections.end(); ++it) {
        if (it->first > maxFd) { // it->first is the client fd
            maxFd = it->first; // update maxFd if client fd is higher
        }
    }
}