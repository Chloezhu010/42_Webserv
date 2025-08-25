#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include "../client/client_connection.hpp"

class WebServer {
private:
    int server_fd;
    std::vector<struct pollfd> poll_fds;
    std::map<int, ClientConnection> clients;
    
    // 私有辅助方法
    bool setNonBlocking(int fd);
    void handleNewConnection();
    void handleClientRead(int client_fd);
    void handleClientWrite(int client_fd);
    void processRequest(ClientConnection& client);
    void closeClient(int client_fd);
    void cleanup();
    
    // HTTP处理辅助方法
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