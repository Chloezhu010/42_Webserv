
#include <iostream>
#include <fstream>
#include <sstream>
#include "../webserv/web_server.h"
#include "../tokenize/tokenize.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::ifstream configFile(argv[1]);
    if (!configFile) {
        std::cerr << "Failed to open config file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << configFile.rdbuf();
    std::string configText = buffer.str();

    std::vector<Token> tokens = tokenize(configText);

    // 简单查找 listen 后的端口号
    int port = 8080; // 默认端口
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].value == "listen" && tokens[i+1].type == TOKEN_WORD) {
            try {
                port = std::stoi(tokens[i+1].value);
                break;
            } catch (...) {}
        }
    }

    WebServer server;
    if (!server.initialize(port)) {
        std::cerr << "❌ Failed to initialize server on port " << port << std::endl;
        return -1;
    }
    server.run();
    return 0;
}