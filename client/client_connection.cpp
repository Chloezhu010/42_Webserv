#include "client_connection.hpp"

// 默认构造函数
ClientConnection::ClientConnection() 
    : fd(-1), bytes_sent(0), request_complete(false), response_ready(false) {}

// 带参数构造函数
ClientConnection::ClientConnection(int socket_fd) 
    : fd(socket_fd), bytes_sent(0), request_complete(false), response_ready(false) {}