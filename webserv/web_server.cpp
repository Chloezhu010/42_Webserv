#include "./web_server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

WebServer::WebServer() : server_fd(-1) {}

WebServer::~WebServer() {
    cleanup();
}

// 设置socket为非阻塞模式
bool WebServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "❌ fcntl F_GETFL failed" << std::endl;
        return false;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "❌ fcntl F_SETFL failed" << std::endl;
        return false;
    }
    
    return true;
}

// 初始化服务器
bool WebServer::initialize(int port) {
    // 创建socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "❌ Failed to create socket" << std::endl;
        return false;
    }
    
    // 设置SO_REUSEADDR
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "❌ setsockopt failed" << std::endl;
        return false;
    }
    
    // 设置为非阻塞
    if (!setNonBlocking(server_fd)) {
        return false;
    }
    
    // 配置地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // 绑定
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "❌ Bind failed" << std::endl;
        return false;
    }
    
    // 监听
    if (listen(server_fd, 10) < 0) {
        std::cerr << "❌ Listen failed" << std::endl;
        return false;
    }
    
    // 初始化poll数组，添加服务器socket
    struct pollfd server_poll_fd;
    server_poll_fd.fd = server_fd;
    server_poll_fd.events = POLLIN;  // 监听新连接
    server_poll_fd.revents = 0;
    poll_fds.push_back(server_poll_fd);
    
    std::cout << "🚀 Non-blocking server started at http://localhost:" << port << std::endl;
    std::cout << "📁 Serving files from ./www/ directory" << std::endl;
    std::cout << "⚡ Using event-driven architecture with poll()" << std::endl;
    
    return true;
}

// 接受新连接
void WebServer::handleNewConnection() {
    while (true) {
        int client_fd = accept(server_fd, NULL, NULL);
        
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有新连接了，这是正常的
                break;
            } else {
                std::cerr << "❌ Accept failed: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        // 设置新客户端为非阻塞
        if (!setNonBlocking(client_fd)) {
            close(client_fd);
            continue;
        }
        
        // 添加到poll监控和客户端映射
        struct pollfd client_poll_fd;
        client_poll_fd.fd = client_fd;
        client_poll_fd.events = POLLIN;  // 监听读事件
        client_poll_fd.revents = 0;
        poll_fds.push_back(client_poll_fd);
        
        // C++98兼容的方式添加客户端
        clients.insert(std::make_pair(client_fd, ClientConnection(client_fd)));
        
        std::cout << "✅ New client connected: fd=" << client_fd << std::endl;
    }
}

// 读取客户端数据
void WebServer::handleClientRead(int client_fd) {
    std::map<int, ClientConnection>::iterator it = clients.find(client_fd);
    if (it == clients.end()) {
        return;
    }
    
    ClientConnection& client = it->second;
    char buffer[1024];
    
    while (true) {
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        
        if (bytes_read > 0) {
            // 成功读取数据
            client.request_buffer.append(buffer, bytes_read);
            
            // 检查HTTP请求是否完整（简单检查：查找\r\n\r\n）
            if (client.request_buffer.find("\r\n\r\n") != std::string::npos) {
                client.request_complete = true;
                processRequest(client);
                break;
            }
        } else if (bytes_read == 0) {
            // 客户端关闭连接
            std::cout << "📤 Client disconnected: fd=" << client_fd << std::endl;
            closeClient(client_fd);
            break;
        } else {
            // bytes_read == -1
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 暂时没有数据可读，这是正常的
                break;
            } else {
                std::cerr << "❌ recv failed: " << strerror(errno) << std::endl;
                closeClient(client_fd);
                break;
            }
        }
    }
}

// 发送数据给客户端
void WebServer::handleClientWrite(int client_fd) {
    std::map<int, ClientConnection>::iterator it = clients.find(client_fd);
    if (it == clients.end()) {
        return;
    }
    
    ClientConnection& client = it->second;
    
    if (!client.response_ready) {
        return;  // 响应还没准备好
    }
    
    while (client.bytes_sent < client.response_buffer.length()) {
        ssize_t bytes_to_send = client.response_buffer.length() - client.bytes_sent;
        ssize_t bytes_sent = send(client_fd, 
                                client.response_buffer.c_str() + client.bytes_sent,
                                bytes_to_send, 0);
        
        if (bytes_sent > 0) {
            client.bytes_sent += bytes_sent;
            
            if (client.bytes_sent >= client.response_buffer.length()) {
                // 发送完成
                std::cout << "📤 Response sent completely to fd=" << client_fd << std::endl;
                // reset for next request, but keep the client connection open
                client.request_buffer.clear();
                client.response_buffer.clear();
                client.request_complete = false;
                client.response_ready = false;
                client.bytes_sent = 0;
                // switch back to reading mode
                for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it)
                {
                    if (it->fd == client_fd)
                    {
                        it->events = POLLIN;
                        it->revents = 0;
                        break;
                    }
                }
                std::cout << "✅ Client fd=" << client_fd << " is ready for next request" << std::endl;
                return;
            }
        } else if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 暂时无法发送，等待下次
                return;
            } else {
                std::cerr << "❌ send failed: " << strerror(errno) << std::endl;
                closeClient(client_fd);
                return;
            }
        }
    }
}

// 处理HTTP请求
void WebServer::processRequest(ClientConnection& client) {
    std::cout << "\n📥 Processing request from fd=" << client.fd << std::endl;
    std::cout << "Request: " << client.request_buffer.substr(0, client.request_buffer.find('\n')) << std::endl;
    
    // 解析请求路径
    std::string path = parseHttpPath(client.request_buffer);
    std::string filename = getFileName(path);
    
    std::cout << "📂 Requested path: " << path << std::endl;
    std::cout << "📄 File to serve: " << filename << std::endl;
    
    // 读取文件内容
    std::string content = readFile(filename);
    
    if (!content.empty()) {
        // 文件存在，返回200
        client.response_buffer = generateResponse(content, 200);
        std::cout << "✅ File found, preparing 200 OK response" << std::endl;
    } else {
        // 文件不存在，返回404
        std::string error_content = readFile("www/404.html");
        if (error_content.empty()) {
            error_content = "<h1>404 Not Found</h1><p>Page not found</p>";
        }
        client.response_buffer = generateResponse(error_content, 404);
        std::cout << "❌ File not found, preparing 404 response" << std::endl;
    }
    
    client.response_ready = true;
    
    // 修改poll事件：添加写事件监听（C++98兼容版本）
    for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
        if (it->fd == client.fd) {
            it->events = POLLOUT;  // 现在监听写事件
            break;
        }
    }
}

// 关闭客户端连接
void WebServer::closeClient(int client_fd) {
    // 从poll数组中移除（C++98兼容版本）
    for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
        if (it->fd == client_fd) {
            poll_fds.erase(it);
            break;
        }
    }
    
    // 从客户端映射中移除
    clients.erase(client_fd);
    
    // 关闭socket
    close(client_fd);
    
    std::cout << "🔒 Client fd=" << client_fd << " closed and cleaned up" << std::endl;
}

// 主事件循环
void WebServer::run() {
    std::cout << "🔄 Starting event loop..." << std::endl;
    
    while (true) {
        // 使用poll等待事件
        int ready = poll(&poll_fds[0], poll_fds.size(), -1);
        
        if (ready == -1) {
            std::cerr << "❌ poll failed: " << strerror(errno) << std::endl;
            break;
        }
        
        if (ready == 0) {
            continue;  // 超时，继续
        }
        
        // 处理就绪的文件描述符（C++98兼容版本）
        for (size_t i = 0; i < poll_fds.size(); ++i) {
            struct pollfd& pfd = poll_fds[i];
            
            if (pfd.revents == 0) {
                continue;  // 没有事件
            }
            
            if (pfd.fd == server_fd) {
                // 服务器socket有新连接
                if (pfd.revents & POLLIN) {
                    handleNewConnection();
                }
            } else {
                // 客户端socket事件
                if (pfd.revents & POLLIN) {
                    // 可读事件
                    handleClientRead(pfd.fd);
                } else if (pfd.revents & POLLOUT) {
                    // 可写事件
                    handleClientWrite(pfd.fd);
                } else if (pfd.revents & (POLLHUP | POLLERR)) {
                    // 连接错误或挂起
                    std::cout << "📤 Client fd=" << pfd.fd << " connection error/hangup" << std::endl;
                    closeClient(pfd.fd);
                    i--; // 因为删除了元素，调整索引
                }
            }
            
            pfd.revents = 0;  // 清除事件标志
        }
    }
}

// 清理资源
void WebServer::cleanup() {
    // C++98兼容的遍历方式
    for (std::map<int, ClientConnection>::iterator it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
    }
    clients.clear();
    
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    
    poll_fds.clear();
}

// 辅助函数（保持原有逻辑）
std::string WebServer::readFile(const std::string& filename) {
    std::ifstream file(filename.c_str());  // C++98需要c_str()
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string WebServer::getFileName(const std::string& path) {
    if (path == "/" || path == "") {
        return "www/index.html";
    }
    return "www" + path;
}

std::string WebServer::parseHttpPath(const std::string& request) {
    size_t first_space = request.find(' ');
    size_t second_space = request.find(' ', first_space + 1);
    
    if (first_space == std::string::npos || second_space == std::string::npos) {
        return "/";
    }
    
    return request.substr(first_space + 1, second_space - first_space - 1);
}

std::string WebServer::generateResponse(const std::string& content, int status_code) {
    std::string status_text;
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 404: status_text = "Not Found"; break;
        default: status_text = "Internal Server Error"; break;
    }
    
    std::stringstream response;
    response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    response << "Content-Type: text/html; charset=UTF-8\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Server: mywebserv/2.0-nonblocking\r\n";
    response << "Connection: close\r\n";  // 简化：每次请求后关闭连接
    response << "\r\n";
    response << content;
    
    return response.str();
}