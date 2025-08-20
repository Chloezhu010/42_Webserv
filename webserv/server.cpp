#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

// 读取文件内容
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";  // 文件不存在
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 根据路径获取文件名
std::string getFileName(const std::string& path) {
    if (path == "/" || path == "") {
        return "www/index.html";  // 默认首页
    }
    return "www" + path;  // 其他路径
}

// 解析HTTP请求，提取路径
std::string parseHttpPath(const std::string& request) {
    // 解析第一行：GET /path HTTP/1.1
    size_t first_space = request.find(' ');
    size_t second_space = request.find(' ', first_space + 1);
    
    if (first_space == std::string::npos || second_space == std::string::npos) {
        return "/";  // 默认路径
    }
    
    return request.substr(first_space + 1, second_space - first_space - 1);
}

// 生成HTTP响应
std::string generateResponse(const std::string& content, int status_code = 200) {
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
    response << "Server: mywebserv/1.0\r\n";
    response << "\r\n";
    response << content;
    
    return response.str();
}

int main() {
    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }
    
    // 设置SO_REUSEADDR
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 配置地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    // 绑定
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }
    
    // 监听
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }
    
    std::cout << "🚀 Server started at http://localhost:8080" << std::endl;
    std::cout << "📁 Serving files from ./www/ directory" << std::endl;
    
    // 主循环
    while (true) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        // 读取HTTP请求
        char buffer[1024] = {0};
        recv(client_fd, buffer, sizeof(buffer), 0);
        
        std::string request(buffer);
        std::cout << "\n📥 Request received:" << std::endl;
        std::cout << request.substr(0, request.find('\n')) << std::endl;
        
        // 解析请求路径
        std::string path = parseHttpPath(request);
        std::string filename = getFileName(path);
        
        std::cout << "📂 Requested path: " << path << std::endl;
        std::cout << "📄 File to serve: " << filename << std::endl;
        
        // 读取文件内容
        std::string content = readFile(filename);
        std::string response;
        
        if (!content.empty()) {
            // 文件存在，返回200
            response = generateResponse(content, 200);
            std::cout << "✅ File found, sending 200 OK" << std::endl;
        } else {
            // 文件不存在，返回404
            std::string error_content = readFile("www/404.html");
            if (error_content.empty()) {
                // 连404页面都没有，返回简单错误
                error_content = "<h1>404 Not Found</h1><p>Page not found</p>";
            }
            response = generateResponse(error_content, 404);
            std::cout << "❌ File not found, sending 404" << std::endl;
        }
        
        // 发送响应
        send(client_fd, response.c_str(), response.length(), 0);
        std::cout << "📤 Response sent!" << std::endl;
        
        // 关闭连接
        close(client_fd);
    }
    
    close(server_fd);
    return 0;
}