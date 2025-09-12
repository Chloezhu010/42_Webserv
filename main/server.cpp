#include <iostream>
#include <csignal>
#include "../configparser/initialize.hpp"

// 用于信号处理的全局服务器指针
WebServer* g_server = nullptr;

// 优雅关闭的信号处理函数
void signalHandler(int signal) {
    std::cout << "\n🛑 接收到信号 " << signal << std::endl;
    if (g_server && g_server->isRunning()) {
        std::cout << "正在优雅地关闭服务器..." << std::endl;
        g_server->stop();
    }
    exit(0);
}

void setupSignalHandlers() {
    // 处理常见的终止信号
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // 终止请求
    signal(SIGQUIT, signalHandler);  // 退出信号
    
    // 忽略SIGPIPE（管道破裂），在代码中处理
    signal(SIGPIPE, SIG_IGN);
}

void printUsage(const char* programName) {
    std::cout << "用法: " << programName << " <配置文件>" << std::endl;
    std::cout << "示例: " << programName << " config/webserv.conf" << std::endl;
}

int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc != 2) {
        std::cerr << "❌ 错误：参数数量无效" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // 设置信号处理器以优雅关闭
    setupSignalHandlers();

    try {
        // 创建WebServer实例
        WebServer server;
        g_server = &server; // 为信号处理器设置全局指针

        std::cout << "🚀 启动WebServer..." << std::endl;
        std::cout << "📁 配置文件: " << argv[1] << std::endl;

        // 使用配置文件初始化服务器
        if (!server.initialize(argv[1])) {
            std::cerr << "❌ 使用配置文件初始化服务器失败: " 
                      << argv[1] << std::endl;
            std::cerr << "错误详情: " << server.getLastError() << std::endl;
            return 1;
        }

        std::cout << "✅ 服务器初始化成功" << std::endl;

        // 启动服务器
        if (!server.start()) {
            std::cerr << "❌ 启动服务器失败" << std::endl;
            std::cerr << "错误详情: " << server.getLastError() << std::endl;
            return 1;
        }

        std::cout << "🌟 WebServer正在运行!" << std::endl;
        std::cout << "按Ctrl+C停止服务器" << std::endl;

        // 保持服务器运行
        // 注意：你需要在WebServer类中实现实际的事件循环
        // 这可能是一个处理客户端连接的run()方法
        while (server.isRunning()) {
            // 这里通常会有你的主事件循环
            // 现在我们只是sleep以防止忙等待
            server.run();
            
            // 你可能想要为WebServer添加run()或handleEvents()方法
            // 使用select/epoll处理客户端连接
        }

    } catch (const std::exception& e) {
        std::cerr << "❌ 捕获异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 发生未知错误" << std::endl;
        return 1;
    }

    std::cout << "👋 服务器关闭完成" << std::endl;
    return 0;
}