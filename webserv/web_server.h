#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include "../client/client_connection.h"

class WebServer {
private:
    int server_fd; // file descriptor for the server socket
    std::vector<struct pollfd> poll_fds; // event monitor: array of pollfd structs to monitor file descriptors
    std::map<int, ClientConnection> clients; // connection state: map of client file descriptors to their connection objects
    
    // Core server helper functions
    bool setNonBlocking(int fd);
    void handleNewConnection();
    void handleClientRead(int client_fd);
    void handleClientWrite(int client_fd);
    void processRequest(ClientConnection& client);
    void closeClient(int client_fd);
    void cleanup();
    
    // HTTP handler functions
    std::string readFile(const std::string& filename);
    std::string getFileName(const std::string& path);
    std::string parseHttpPath(const std::string& request);
    std::string generateResponse(const std::string& content, int status_code = 200);
    
public:
    WebServer();
    ~WebServer();
    
    // 公共接口
    bool initialize(int port = 8080);
    void run();
};

#endif // WEB_SERVER_H