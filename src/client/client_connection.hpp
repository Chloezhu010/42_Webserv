#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <string>
#include <ctime>
#include "../http/http_request.hpp" // handle http request
#include "../http/http_response.hpp" // handle http response
#include "../configparser/config.hpp" // for server & location config

// forward declaration
class ServerInstance;
struct LocationConfig;

struct ClientConnection {
    int fd;
    std::string request_buffer;  // stores received request data
    std::string response_buffer; // stores response data to send
    size_t bytes_sent;          // number of bytes sent
    bool request_complete;      // whether request is fully received
    bool response_ready;        // whether response is ready to send
    time_t last_active;       // to deal with timeout

    // handle http request & response
    HttpRequest* http_request; // request parsing & validation
    HttpResponse* http_response; // response building

    // config context for this connection
    ServerInstance* server_instance; // which server is handling this request
    LocationConfig* matched_location; // which location matched the URI

    // Default constructor (required for C++98)
    ClientConnection();
    ~ClientConnection();

    // Constructor with parameters
    ClientConnection(int socket_fd);
};

#endif // CLIENT_CONNECTION_H