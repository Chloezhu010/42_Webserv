#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <string>

// client connection state tracking
struct ClientConnection {
    int fd;
    std::string request_buffer;  // incoming http request data
    std::string response_buffer; // outgoing http request data
    size_t bytes_sent;          // nbr of bytes sent to client
    bool request_complete;      // check if the request is complete
    bool response_ready;        // check if the response is ready
    
    // default constructor
    ClientConnection();
    
    // constructor with socket fd
    ClientConnection(int socket_fd);
};

#endif // CLIENT_CONNECTION_H