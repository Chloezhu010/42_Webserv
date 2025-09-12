#include "../http/http_response.hpp"
#include "../http/http_request.hpp"
#include <iostream>

void debugResponseGeneration() {
    std::cout << "=== 调试HTTP响应生成过程 ===" << std::endl;
    
    // 创建测试请求
    HttpRequest request;
    std::string test_request = "GET /index.html HTTP/1.1\r\n"
                              "Host: localhost:8080\r\n"
                              "User-Agent: DebugTest/1.0\r\n"
                              "Connection: keep-alive\r\n\r\n";
    
    std::cout << "1. 解析请求：" << std::endl;
    std::cout << test_request << std::endl;
    
    if (request.parseRequest(test_request)) {
        std::cout << "   ✓ 请求解析成功" << std::endl;
        
        ValidationResult result = request.validateRequest();
        std::cout << "   验证结果：" << result << std::endl;
        
        // 创建响应
        HttpResponse response;
        
        std::cout << "2. 设置响应体：" << std::endl;
        response.setBody("<html><head><title>调试测试</title></head><body><h1>Hello World!</h1></body></html>");
        
        std::cout << "3. 构建完整响应：" << std::endl;
        std::string full_response = response.buildFullResponse(request);
        
        std::cout << "--- 完整HTTP响应 ---" << std::endl;
        std::cout << full_response << std::endl;
        std::cout << "--- 响应结束 ---" << std::endl;
        
        std::cout << "4. 响应分析：" << std::endl;
        std::cout << "   状态码：" << response.getStatusCode() << std::endl;
        std::cout << "   内容长度：" << response.getContentLength() << std::endl;
        std::cout << "   是否成功状态：" << (response.isSuccessStatus() ? "是" : "否") << std::endl;
        
    } else {
        std::cout << "   ✗ 请求解析失败" << std::endl;
    }
}

int main() {
    debugResponseGeneration();
    return 0;
}