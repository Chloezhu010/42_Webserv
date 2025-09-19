#include "client_connection.hpp"

// default constructor
ClientConnection::ClientConnection() 
    : fd(-1), bytes_sent(0), request_complete(false), response_ready(false), last_active(time(NULL)), http_request(NULL), http_response(NULL) {}

// constructor with param
ClientConnection::ClientConnection(int socket_fd) 
    : fd(socket_fd), bytes_sent(0), request_complete(false), response_ready(false), last_active(time(NULL)), http_request(NULL), http_response(NULL) {}

// default destructor
ClientConnection::~ClientConnection()
{
    if (http_request) {
        delete http_request;
        // http_request = NULL;
    }
    if (http_response) {
        delete http_response;
        // http_response = NULL;
    }
}