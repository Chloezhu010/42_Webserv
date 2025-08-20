#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

int main() {
    // 1. 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	//AF_INET是IPv4网络协议
	//SOCK_STREAM是TCP，http必须选择TCP
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }
    
    // 2. 设置地址重用（避免"Address already in use"错误）
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//是为了防止遇到TIME_WAIT的情况，每当关闭服务器的时候，都得等2分钟左右去重新连接，加上了setsickopt就可以直接连接了
    
    // 3. 配置服务器地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;		   // 使用IPv4协议
    address.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    address.sin_port = htons(8080);        // 端口8080
    
    // 4. 绑定socket到地址
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }
    
    // 5. 开始监听连接
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }
    
    std::cout << "Server listening on http://localhost:8080" << std::endl;
    
    // 6. 主循环：接受连接并处理请求
    while (true) {
        // 接受新连接
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        // 读取客户端请求
        char buffer[1024] = {0};
        recv(client_fd, buffer, sizeof(buffer), 0);
        
        // 打印收到的请求（用于调试）
        std::cout << "Received request:\n" << buffer << std::endl;
        
        // 发送HTTP响应
        const char* response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 138\r\n"
            "\r\n"
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>My First Server</title></head>"
            "<body>"
            "<h1>Hello from my webserv!</h1>"
            "<p>This is working!</p>"
            "</body>"
            "</html>";
        
        send(client_fd, response, strlen(response), 0);
        
        // 关闭连接
        close(client_fd);
        
        std::cout << "Response sent!" << std::endl;
    }
    
    close(server_fd);
    return 0;
}