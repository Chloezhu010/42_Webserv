#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <string>
#include "../http/http_request.hpp" // handle http request
#include "../http/http_response.hpp" // handle http response

// 客户端连接状态
struct ClientConnection {
    int fd;
    std::string request_buffer;  // 存储接收到的请求数据
    std::string response_buffer; // 存储要发送的响应数据
    size_t bytes_sent;          // 已发送的字节数
    bool request_complete;      // 请求是否接收完整
    bool response_ready;        // 响应是否准备好

    // handle http request & response
    HttpRequest* http_request; // request parsing & validation
    HttpResponse* http_response; // response building
    
    // 默认构造函数（C++98需要）
    ClientConnection();
    ~ClientConnection();
    
    // 带参数构造函数
    ClientConnection(int socket_fd);
};

#endif // CLIENT_CONNECTION_H