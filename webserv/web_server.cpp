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

// set socket to non-blocking mode
bool WebServer::setNonBlocking(int fd) {
    // get current file status flags
    int flags = fcntl(fd, F_GETFL, 0);
    // error case
    if (flags == -1) {
        std::cerr << "❌ fcntl F_GETFL failed" << std::endl;
        return false;
    }
    // set fd flag to non-blocking
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "❌ fcntl F_SETFL failed" << std::endl;
        return false;
    }
    return true;
}

// initialize server
bool WebServer::initialize(int port) {
    // create socket (ipv4, tcp)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "❌ Failed to create socket" << std::endl;
        return false;
    }
    
    // enable reusing the address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "❌ setsockopt failed" << std::endl;
        return false;
    }
    
    // set server socket to non-blocking mode
    if (!setNonBlocking(server_fd)) {
        return false;
    }
    
    // setup server listening address
    struct sockaddr_in address;
    address.sin_family = AF_INET; // ipv4
    address.sin_addr.s_addr = INADDR_ANY; // bind to all network interfaces
    address.sin_port = htons(port); // convert port nbr to network byte order, in short format
    
    // bind socket to address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "❌ Bind failed" << std::endl;
        return false;
    }
    
    // listen for connections: arbitrary backlog size of 10
    if (listen(server_fd, 10) < 0)
    {
        std::cerr << "❌ Listen failed" << std::endl;
        return false;
    }
    
    // create an array of pollfd structs
    // add the listening socket to the poll_fds array so that the server can detect new connections
    struct pollfd server_poll_fd; // a struct to tell poll() which fd to monitor and what events to watch for
    server_poll_fd.fd = server_fd; // monitor the listening server socket
    server_poll_fd.events = POLLIN; // listen for read events, aka incoming data/ connections
    server_poll_fd.revents = 0; // clean slate, poll() will set this to the events that occurred on the fd
    poll_fds.push_back(server_poll_fd); // add this socket to the monitoring list
    
    std::cout << "🚀 Non-blocking server started at http://localhost:" << port << std::endl;
    std::cout << "📁 Serving files from ./www/ directory" << std::endl;
    std::cout << "⚡ Using event-driven architecture with poll()" << std::endl;
    
    return true;
}

// server accept new client connection
void WebServer::handleNewConnection() {
    while (true) {
        int client_fd = accept(server_fd, NULL, NULL);
        
        if (client_fd == -1) {
            // when the socket is marked as non-blocking and no connections are present to be accepted
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // normal - no more connections waiting, try again later
            } else {
                // real error cases
                std::cerr << "❌ Accept failed: " << strerror(errno) << std::endl;
                break;
            }
        }
        // set client socket to non-blocking mode
        if (!setNonBlocking(client_fd)) {
            close(client_fd);
            continue;
        }
        // add to client poll array for event monitoring
        struct pollfd client_poll_fd;
        client_poll_fd.fd = client_fd;
        client_poll_fd.events = POLLIN;
        client_poll_fd.revents = 0;
        poll_fds.push_back(client_poll_fd);
        
        // add to client map for connection state tracking
        clients.insert(std::make_pair(client_fd, ClientConnection(client_fd)));
        
        std::cout << "✅ New client connected: fd=" << client_fd << std::endl;
    }
}

// handle read event: client sends http request
void WebServer::handleClientRead(int client_fd) {
    // find the client in the map
    std::map<int, ClientConnection>::iterator it = clients.find(client_fd);
    if (it == clients.end()) {
        return;
    }
    // if find the client, get the connection state
    ClientConnection& client = it->second;
    char buffer[1024];
    
    while (true) {
        // call recv() to read data from client
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        
        // if bytes_read > 0, read data from client
        if (bytes_read > 0) {
            // append the received data to the client's request buffer
            client.request_buffer.append(buffer, bytes_read);
            
            // check if the request is complete: simple check by finding "\r\n\r\n"
            if (client.request_buffer.find("\r\n\r\n") != std::string::npos) {
                client.request_complete = true;
                processRequest(client); // process the request
                break;
            }
        } 
        // bytes_read == 0: client disconnected, or no more data to read
        else if (bytes_read == 0) {
            // if (client.request_complete) {
            //     std::cout << "✅ Request complete, client waiting for response" << std::endl;
            //     break;
            // } else {
                std::cout << "📤 Client disconnected: fd=" << client_fd << std::endl;
                closeClient(client_fd);
                break;
            // }
        } else {
            // bytes_read == -1: error case
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // normal - no more data to read, try again later
            } else {
                // real error cases
                std::cerr << "❌ recv failed: " << strerror(errno) << std::endl;
                closeClient(client_fd);
                break;
            }
        }
    }
}

// handle write event: server sends http response to client
void WebServer::handleClientWrite(int client_fd) {
    // find the client in the map
    std::map<int, ClientConnection>::iterator it = clients.find(client_fd);
    if (it == clients.end()) {
        return;
    }
    // if find the client, get the connection state
    ClientConnection& client = it->second;
    
    // if the response is not ready, return
    // response_ready is set in processRequest(), false by default
    if (!client.response_ready) {
        return;
    }
    
    // send the response to the client - partial send handling
    /*
        - client.bytes_sent: nbr of bytes already sent to client
        - client.response_buffer.length(): total size of the http response; set in processRequest()
    */    
    while (client.bytes_sent < client.response_buffer.length()) // keep sending until all bytes are sent
    {
        // calculate how many bytes remaining to send
        ssize_t bytes_to_send = client.response_buffer.length() - client.bytes_sent;
        // send the response to the client
        ssize_t bytes_sent = send(client_fd, 
                                client.response_buffer.c_str() + client.bytes_sent, // pointer to the start of the unsent part of the http response
                                bytes_to_send, // remaining bytes to send
                                0); // 0 for no flags
        // if send successfully, update the nbr of bytes sent
        if (bytes_sent > 0) {
            client.bytes_sent += bytes_sent;
            // if all response bytes are sent
            if (client.bytes_sent >= client.response_buffer.length()) {
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
        }
        // if error case
        else if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else {
                std::cerr << "❌ send failed: " << strerror(errno) << std::endl;
                closeClient(client_fd);
                return;
            }
        }
    }
}

// process client's http request
// called within handleClientRead()
void WebServer::processRequest(ClientConnection& client) {
    std::cout << "\n📥 Processing request from fd=" << client.fd << std::endl;
    std::cout << "   >> Request: " << client.request_buffer.substr(0, client.request_buffer.find('\n')) << std::endl;
    
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

// close client connection
void WebServer::closeClient(int client_fd) {
    // remove from the poll_fds array
    for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
        if (it->fd == client_fd) {
            poll_fds.erase(it);
            break;
        }
    }
    // remove from the clients map
    clients.erase(client_fd);
    // close the socket
    close(client_fd);

    std::cout << "🔒 Client fd=" << client_fd << " closed and cleaned up" << std::endl;
} 

// main loop
void WebServer::run() {
    std::cout << "🔄 Starting event loop..." << std::endl;
    
    while (true) {
        /* call poll() to wait for events on the file descriptors in poll_fds */
        // set timeout to -1 to wait indefinitely until at least one event occurs
        // cpu usage ~0% when idle, rather than constantly polling
        int ready = poll(&poll_fds[0], poll_fds.size(), -1);
        
        // error case
        if (ready == -1)
        {
            std::cerr << "❌ poll failed: " << strerror(errno) << std::endl;
            break;
        }
        
        // // will never happen when timeout is -1, but just in case
        // if (ready == 0) {
        //     continue;
        // }
        
        // if ready > 0,process ready fd
        for (size_t i = 0; i < poll_fds.size(); ++i) {
            struct pollfd& pfd = poll_fds[i];
            
            // check revents, if no event occurred on this fd, continue to next fd
            if (pfd.revents == 0) {
                continue;
            }
            
            // for server socket event
            if (pfd.fd == server_fd) {
                // if the event is a read event
                if (pfd.revents & POLLIN) {
                    // new client wants to connect
                    handleNewConnection();
                }
            }
            else
            {
                // for client socket event
                // if the event is a read event
                if (pfd.revents & POLLIN) {
                    // handle incoming data from client
                    handleClientRead(pfd.fd);
                }
                // if the event is a write event
                else if (pfd.revents & POLLOUT) {
                    // handle outgoing data to client
                    handleClientWrite(pfd.fd);
                }
                // if the event is client closed the connection or has connectionerror
                else if (pfd.revents & (POLLHUP | POLLERR)) {
                    std::cout << "📤 Client fd=" << pfd.fd << " connection error/hangup" << std::endl;
                    closeClient(pfd.fd);
                    i--; // decrement index to avoid skipping the next fd
                }
            }
            pfd.revents = 0; // reset the event flags for this fd
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

// extract the url path from the http request
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