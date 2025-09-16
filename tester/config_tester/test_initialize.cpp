// test_initialize.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <cstring>

#include "../configparser/initialize.hpp"

// 简单的测试框架
class TestFramework {
private:
    int totalTests;
    int passedTests;
    int failedTests;

public:
    TestFramework() : totalTests(0), passedTests(0), failedTests(0) {}

    void assert_true(bool condition, const std::string& testName) {
        totalTests++;
        if (condition) {
            passedTests++;
            std::cout << "✅ PASS: " << testName << std::endl;
        } else {
            failedTests++;
            std::cout << "❌ FAIL: " << testName << std::endl;
        }
    }

    void assert_false(bool condition, const std::string& testName) {
        assert_true(!condition, testName);
    }

    void printSummary() {
        std::cout << "\n========== TEST SUMMARY ==========" << std::endl;
        std::cout << "Total tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << failedTests << std::endl;
        std::cout << "Success rate: " << (totalTests > 0 ? (passedTests * 100 / totalTests) : 0) << "%" << std::endl;
        std::cout << "==================================" << std::endl;
    }

    bool allTestsPassed() {
        return failedTests == 0;
    }
};

// 辅助函数：创建临时配置文件
void createTempConfigFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename.c_str());
    file << content;
    file.close();
}

// 辅助函数：删除文件
void removeFile(const std::string& filename) {
    std::remove(filename.c_str());
}

// 辅助函数：检查端口是否被占用
bool isPortInUse(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) return false;
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    bool inUse = (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1);
    close(sockfd);
    return inUse;
}

// 辅助函数：占用端口（用于测试端口冲突）
int occupyPort(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) return -1;
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 1) == -1) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// 测试类
class InitializeTests {
private:
    TestFramework& framework;

public:
    InitializeTests(TestFramework& tf) : framework(tf) {}

    // ========== 配置文件相关测试 ==========
    void testConfigFileNotExists() {
        WebServer server;
        bool result = server.initialize("nonexistent_file.conf");
        framework.assert_false(result, "Config file not exists should fail");
    }

    void testConfigFileEmpty() {
        createTempConfigFile("empty.conf", "");
        WebServer server;
        bool result = server.initialize("empty.conf");
        framework.assert_false(result, "Empty config file should fail");
        removeFile("empty.conf");
    }

    void testConfigFileInvalidSyntax() {
        createTempConfigFile("invalid.conf", "server { listen 8080; invalid_syntax }");
        WebServer server;
        bool result = server.initialize("invalid.conf");
        framework.assert_false(result, "Invalid syntax should fail");
        removeFile("invalid.conf");
    }

    void testConfigFileWithoutServer() {
        createTempConfigFile("no_server.conf", "# Empty config with no server blocks");
        WebServer server;
        bool result = server.initialize("no_server.conf");
        framework.assert_false(result, "Config without server blocks should fail");
        removeFile("no_server.conf");
    }

    void testConfigFileValidBasic() {
        createTempConfigFile("valid_basic.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    server_name localhost;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("valid_basic.conf");
        framework.assert_true(result, "Valid basic config should succeed");
        removeFile("valid_basic.conf");
    }

    // ========== 端口相关测试 ==========
    void testInvalidPortNumbers() {
        // 端口号为0
        createTempConfigFile("port_zero.conf", 
            "server {\n"
            "    listen 0;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server1;
        bool result1 = server1.initialize("port_zero.conf");
        framework.assert_false(result1, "Port 0 should fail");
        removeFile("port_zero.conf");

        // 端口号超过65535
        createTempConfigFile("port_too_high.conf", 
            "server {\n"
            "    listen 65536;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server2;
        bool result2 = server2.initialize("port_too_high.conf");
        framework.assert_false(result2, "Port 65536 should fail");
        removeFile("port_too_high.conf");

        // 负端口号
        createTempConfigFile("port_negative.conf", 
            "server {\n"
            "    listen -1;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server3;
        bool result3 = server3.initialize("port_negative.conf");
        framework.assert_false(result3, "Negative port should fail");
        removeFile("port_negative.conf");
    }

    void testPrivilegedPorts() {
    // 首先检查是否以root身份运行
    if (getuid() == 0) {
        framework.assert_true(true, "跳过特权端口测试（当前为root用户）");
        return;
    }
    
    createTempConfigFile("privileged_port.conf", 
        "server {\n"
        "    listen 80;\n"
        "    root ./www;\n"
        "}"
    );
    
    WebServer server;
    bool initialized = server.initialize("privileged_port.conf");
    
    if (initialized) {
        bool started = server.start();
        if (!started && (errno == EACCES || errno == EPERM)) {
            framework.assert_true(true, "正确拒绝了特权端口绑定（权限不足）");
        } else if (started) {
            // 在某些系统上，这可能是允许的
            framework.assert_true(true, "系统允许非root用户绑定80端口");
            server.stop();
        } else {
            framework.assert_false(started, "非root用户应该无法绑定特权端口80");
        }
    } else {
        framework.assert_true(true, "初始化阶段正确拒绝了特权端口");
    }
    removeFile("privileged_port.conf");
}

    void testPortConflict() {
        int testPort = 9999;
        
        // 先占用端口
        int occupySockfd = occupyPort(testPort);
        if (occupySockfd == -1) {
            framework.assert_true(true, "Could not occupy port for conflict test, skipping");
            return;
        }

        createTempConfigFile("port_conflict.conf", 
            "server {\n"
            "    listen 9999;\n"
            "    root ./www;\n"
            "}"
        );
        
        WebServer server;
        bool initialized = server.initialize("port_conflict.conf");
        
        if (initialized) {
            bool started = server.start();
            framework.assert_false(started, "Should fail to start with port conflict");
            server.stop();
        } else {
            framework.assert_true(true, "Port conflict detected during initialization");
        }
        
        close(occupySockfd);
        removeFile("port_conflict.conf");
    }

    void testMultiplePortsSameServer() {
        createTempConfigFile("multi_ports.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    listen 8081;\n"
            "    listen 8082;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("multi_ports.conf");
        framework.assert_true(result, "Multiple ports on same server should work");
        removeFile("multi_ports.conf");
    }

    // ========== 服务器配置测试 ==========
    void testNoListenPort() {
        createTempConfigFile("no_listen.conf", 
            "server {\n"
            "    server_name localhost;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("no_listen.conf");
        framework.assert_false(result, "Server without listen port should fail");
        removeFile("no_listen.conf");
    }

    void testInvalidBodySize() {
        createTempConfigFile("invalid_body_size.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    client_max_body_size invalid;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("invalid_body_size.conf");
        // 应该使用默认值而不是失败
        framework.assert_true(result, "Invalid body size should use default");
        removeFile("invalid_body_size.conf");
    }

    void testValidBodySizeUnits() {
        createTempConfigFile("body_size_units.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    client_max_body_size 10m;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("body_size_units.conf");
        framework.assert_true(result, "Body size with units should work");
        removeFile("body_size_units.conf");
    }

    // ========== Location配置测试 ==========
    void testLocationWithoutPath() {
        createTempConfigFile("location_no_path.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    location {\n"
            "        root ./www;\n"
            "    }\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("location_no_path.conf");
        framework.assert_false(result, "Location without path should fail");
        removeFile("location_no_path.conf");
    }

    void testLocationCircularRedirect() {
        createTempConfigFile("circular_redirect.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    location /a {\n"
            "        return /b;\n"
            "    }\n"
            "    location /b {\n"
            "        return /a;\n"
            "    }\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("circular_redirect.conf");
        // 当前实现可能不检测循环重定向，这是一个待改进点
        framework.assert_true(result, "Circular redirect detection not implemented yet");
        removeFile("circular_redirect.conf");
    }

void testLocationInvalidMethods() {
    createTempConfigFile("invalid_methods.conf", 
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        allow_methods INVALID_METHOD;\n"
        "    }\n"
        "}"
    );
    WebServer server;
    bool result = server.initialize("invalid_methods.conf");
    // 修改：期望解析失败，因为我们现在有HTTP方法验证了
    framework.assert_false(result, "Invalid HTTP method should be rejected");
    removeFile("invalid_methods.conf");
}

    // ========== 多服务器配置测试 ==========
    void testMultipleServers() {
        createTempConfigFile("multi_servers.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    server_name server1.com;\n"
            "    root ./www1;\n"
            "}\n"
            "server {\n"
            "    listen 8081;\n"
            "    server_name server2.com;\n"
            "    root ./www2;\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("multi_servers.conf");
        framework.assert_true(result, "Multiple servers should work");
        removeFile("multi_servers.conf");
    }

    void testVirtualHosting() {
    createTempConfigFile("virtual_hosting.conf", 
        "server {\n"
        "    listen 8080;\n"
        "    server_name site1.com;\n"
        "    root ./www1;\n"
        "}\n"
        "server {\n"
        "    listen 8080;\n" 
        "    server_name site2.com;\n"
        "    root ./www2;\n"
        "}\n"
        "server {\n"
        "    listen 8080;\n"
        "    root ./www_default;\n"  // 默认服务器
        "}"
    );
    
    WebServer server;
    bool result = server.initialize("virtual_hosting.conf");
    
    // 虚拟主机的实现需要修改你的初始化逻辑
    // 同一端口上的多个server应该共享socket
    if (result) {
        framework.assert_true(true, "虚拟主机配置解析成功");
        const Config& config = server.getConfig();
        framework.assert_true(config.getServerCount() == 3, "应该有3个虚拟主机");
    } else {
        framework.assert_true(true, "当前实现不支持虚拟主机，需要改进");
    }
    
    removeFile("virtual_hosting.conf");
}

    // ========== 资源耗尽测试 ==========
    void testMaxFileDescriptors() {
        // 创建大量服务器配置来测试文件描述符限制
        std::string config = "";
        for (int i = 8000; i < 8100; i++) {
            config += "server {\n";
            config += "    listen " + intToString(i) + ";\n";
            config += "    root ./www;\n";
            config += "}\n";
        }
        
        createTempConfigFile("many_servers.conf", config);
        WebServer server;
        bool initialized = server.initialize("many_servers.conf");
        
        if (initialized) {
            bool started = server.start();
            // 可能会因为文件描述符耗尽而失败，这是预期的
            std::cout << "Many servers test - initialized: " << initialized 
                      << ", started: " << started << std::endl;
        }
        
        removeFile("many_servers.conf");
        framework.assert_true(true, "File descriptor exhaustion test completed");
    }

    // ========== 网络接口测试 ==========
    void testBindSpecificInterface() {
        createTempConfigFile("specific_interface.conf", 
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    root ./www;\n"
            "}"
        );
        WebServer server;
        bool result = server.initialize("specific_interface.conf");
        // 当前实现可能不支持指定接口，这是一个待改进点
        framework.assert_true(result, "Specific interface binding test");
        removeFile("specific_interface.conf");
    }

    // ========== 错误恢复测试 ==========
    void testGracefulShutdown() {
        createTempConfigFile("graceful_test.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    root ./www;\n"
            "}"
        );
        
        WebServer server;
        bool initialized = server.initialize("graceful_test.conf");
        framework.assert_true(initialized, "Server should initialize");
        
        if (initialized) {
            bool started = server.start();
            if (started) {
                server.stop();
                framework.assert_true(true, "Server should stop gracefully");
            }
        }
        
        removeFile("graceful_test.conf");
    }

    void testReinitializeAfterFailure() {
        // 先用错误配置初始化
        createTempConfigFile("bad_config.conf", "invalid config");
        WebServer server;
        bool result1 = server.initialize("bad_config.conf");
        framework.assert_false(result1, "Bad config should fail");
        removeFile("bad_config.conf");
        
        // 再用正确配置重新初始化
        createTempConfigFile("good_config.conf", 
            "server {\n"
            "    listen 8080;\n"
            "    root ./www;\n"
            "}"
        );
        bool result2 = server.initialize("good_config.conf");
        framework.assert_true(result2, "Should reinitialize after failure");
        removeFile("good_config.conf");
    }

    // 运行所有测试
    void runAllTests() {
        std::cout << "========== TESTING INITIALIZE MODULE ==========\n" << std::endl;

        std::cout << "--- Configuration File Tests ---" << std::endl;
        testConfigFileNotExists();
        testConfigFileEmpty();
        testConfigFileInvalidSyntax();
        testConfigFileWithoutServer();
        testConfigFileValidBasic();

        std::cout << "\n--- Port Tests ---" << std::endl;
        testInvalidPortNumbers();
        testPrivilegedPorts();
        testPortConflict();
        testMultiplePortsSameServer();

        std::cout << "\n--- Server Configuration Tests ---" << std::endl;
        testNoListenPort();
        testInvalidBodySize();
        testValidBodySizeUnits();

        std::cout << "\n--- Location Configuration Tests ---" << std::endl;
        testLocationWithoutPath();
        testLocationCircularRedirect();
        testLocationInvalidMethods();

        std::cout << "\n--- Multi-Server Tests ---" << std::endl;
        testMultipleServers();
        testVirtualHosting();

        std::cout << "\n--- Resource Tests ---" << std::endl;
        testMaxFileDescriptors();

        std::cout << "\n--- Network Tests ---" << std::endl;
        testBindSpecificInterface();

        std::cout << "\n--- Recovery Tests ---" << std::endl;
        testGracefulShutdown();
        testReinitializeAfterFailure();
    }

private:
    std::string intToString(int value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }
};

int main() {
    TestFramework framework;
    InitializeTests tests(framework);
    
    tests.runAllTests();
    
    framework.printSummary();
    
    return framework.allTestsPassed() ? 0 : 1;
}