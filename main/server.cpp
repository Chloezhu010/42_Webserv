#include <iostream>
#include "../webserv/web_server.h"

int main() {
    WebServer server;
    
    if (!server.initialize(8080)) {
        std::cerr << "❌ Failed to initialize server" << std::endl;
        return -1;
    }
    
    server.run();
    
    return 0;
}