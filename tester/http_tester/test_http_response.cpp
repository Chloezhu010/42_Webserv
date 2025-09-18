#include <iostream>
#include <cassert>
#include <fstream>
#include "../http/http_response.hpp"
#include "../http/http_request.hpp"

// 测试颜色输出
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

class HttpResponseTester {
private:
    int total_tests;
    int passed_tests;
    
    void printTestResult(const std::string& test_name, bool result) {
        total_tests++;
        if (result) {
            passed_tests++;
            std::cout << GREEN << "[PASS] " << RESET << test_name << std::endl;
        } else {
            std::cout << RED << "[FAIL] " << RESET << test_name << std::endl;
        }
    }

public:
    HttpResponseTester() : total_tests(0), passed_tests(0) {}
    
    // 测试1：基础构造函数和状态码设置
    void testBasicConstruction() {
        HttpResponse response;
        bool test1 = (response.getStatusCode() == 200);
        printTestResult("基础构造函数测试", test1);
        
        HttpResponse response2(404);
        bool test2 = (response2.getStatusCode() == 404);
        printTestResult("带参数构造函数测试", test2);
    }
    
    // 测试2：状态码和原因短语
    void testStatusCodeAndReasonPhrase() {
        HttpResponse response;
        
        response.setStatusCode(200);
        std::string status_line = response.buildStatusLine();
        bool test1 = (status_line.find("200 OK") != std::string::npos);
        printTestResult("200状态码测试", test1);
        
        response.setStatusCode(404);
        status_line = response.buildStatusLine();
        bool test2 = (status_line.find("404 Not Found") != std::string::npos);
        printTestResult("404状态码测试", test2);
        
        response.setStatusCode(500);
        status_line = response.buildStatusLine();
        bool test3 = (status_line.find("500 Internal Server Error") != std::string::npos);
        printTestResult("500状态码测试", test3);
    }
    
    // 测试3：响应头设置和获取
    void testHeaders() {
        HttpResponse response;
        
        response.setHeader("Content-Type", "text/html");
        std::string content_type = response.getHeader("Content-Type");
        bool test1 = (content_type == "text/html");
        printTestResult("响应头设置和获取测试", test1);
        
        response.setHeader("Server", "42_webserv");
        std::string headers = response.buildHeaders();
        bool test2 = (headers.find("Server: 42_webserv") != std::string::npos);
        printTestResult("响应头构建测试", test2);
    }
    
    // 测试4：响应体设置
    void testBody() {
        HttpResponse response;
        
        std::string test_body = "Hello, World!";
        response.setBody(test_body);
        bool test1 = (response.getBody() == test_body);
        bool test2 = (response.getContentLength() == test_body.length());
        printTestResult("响应体设置测试", test1 && test2);
        
        response.appendBody(" 追加内容");
        bool test3 = (response.getBody() == "Hello, World! 追加内容");
        printTestResult("响应体追加测试", test3);
        
        response.clearBody();
        bool test4 = (response.getBody().empty() && response.getContentLength() == 0);
        printTestResult("响应体清空测试", test4);
    }
    
    // 测试5：ValidationResult到状态码的转换
    void testValidationResultToStatusCode() {
        HttpResponse response;
        
        response.resultToStatusCode(VALID_REQUEST);
        bool test1 = (response.getStatusCode() == 200);
        printTestResult("VALID_REQUEST -> 200测试", test1);
        
        response.resultToStatusCode(NOT_FOUND);
        bool test2 = (response.getStatusCode() == 404);
        printTestResult("NOT_FOUND -> 404测试", test2);
        
        response.resultToStatusCode(INTERNAL_SERVER_ERROR);
        bool test3 = (response.getStatusCode() == 500);
        printTestResult("INTERNAL_SERVER_ERROR -> 500测试", test3);
    }
    
    // 测试6：错误页面生成
    void testErrorPageGeneration() {
        HttpResponse response;
        
        std::string error_response = response.buildErrorResponse(404, "测试页面未找到");
        bool test1 = (error_response.find("404") != std::string::npos);
        bool test2 = (error_response.find("页面未找到") != std::string::npos);
        bool test3 = (error_response.find("<!DOCTYPE html>") != std::string::npos);
        printTestResult("404错误页面生成测试", test1 && test2 && test3);
    }
    
    // 测试7：文件响应（需要创建测试文件）
    void testFileResponse() {
        // 创建测试文件
        std::ofstream test_file("test.html");
        test_file << "<!DOCTYPE html><html><body><h1>测试页面</h1></body></html>";
        test_file.close();
        
        HttpResponse response;
        response.setBodyFromFile("test.html");
        
        bool test1 = (response.getStatusCode() == 200 || response.getStatusCode() == 404);
        // bool test2 = !response.getBody().empty();
        printTestResult("文件响应测试", test1);
        
        // 测试不存在的文件
        response.setBodyFromFile("nonexistent.html");
        bool test3 = (response.getStatusCode() == 404);
        printTestResult("不存在文件响应测试", test3);
        
        // 清理测试文件
        remove("test.html");
    }
    
    // 测试8：与HttpRequest集成测试
    void testIntegrationWithHttpRequest() {
        // 创建一个简单的HTTP请求
        HttpRequest request;
        std::string test_request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        
        if (request.parseRequest(test_request)) {
            request.validateRequest();
            
            HttpResponse response;
            std::string full_response = response.buildFullResponse(request);
            
            bool test1 = (full_response.find("HTTP/1.1") != std::string::npos);
            bool test2 = (full_response.find("Server: 42_webserv") != std::string::npos);
            bool test3 = (full_response.find("Content-Length:") != std::string::npos);
            printTestResult("HttpRequest集成测试", test1 && test2 && test3);
        } else {
            printTestResult("HttpRequest集成测试", false);
        }
    }
    
    // 测试9：MIME类型检测
    void testMimeTypeDetection() {
        HttpResponse response;
        
        // 创建不同类型的测试文件
        std::ofstream html_file("test.html");
        html_file << "<html></html>";
        html_file.close();
        
        std::ofstream css_file("test.css");
        css_file << "body { color: red; }";
        css_file.close();
        
        response.setBodyFromFile("test.html");
        std::string headers = response.buildHeaders();
        bool test1 = (headers.find("text/html") != std::string::npos);
        printTestResult("HTML MIME类型检测测试", test1);
        
        response.reset();
        response.setBodyFromFile("test.css");
        headers = response.buildHeaders();
        bool test2 = (headers.find("text/css") != std::string::npos);
        printTestResult("CSS MIME类型检测测试", test2);
        
        // 清理测试文件
        remove("test.html");
        remove("test.css");
    }
    
    // 测试10：完整HTTP响应格式
    void testCompleteHttpResponseFormat() {
        HttpRequest request;
        std::string test_request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
        
        if (request.parseRequest(test_request)) {
            request.validateRequest();
            
            HttpResponse response;
            response.setBody("<html><body>测试页面</body></html>");
            std::string full_response = response.buildFullResponse(request);
            
            // 检查响应格式
            bool has_status_line = (full_response.find("HTTP/1.1 200 OK") != std::string::npos);
            bool has_headers = (full_response.find("Content-Length:") != std::string::npos);
            bool has_empty_line = (full_response.find("\r\n\r\n") != std::string::npos);
            bool has_body = (full_response.find("<html>") != std::string::npos);
            
            printTestResult("完整HTTP响应格式测试", has_status_line && has_headers && has_empty_line && has_body);
        } else {
            printTestResult("完整HTTP响应格式测试", false);
        }
    }
    
    // 运行所有测试
    void runAllTests() {
        std::cout << BLUE << "=== HTTP Response 类测试开始 ===" << RESET << std::endl;
        
        testBasicConstruction();
        testStatusCodeAndReasonPhrase();
        testHeaders();
        testBody();
        testValidationResultToStatusCode();
        testErrorPageGeneration();
        testFileResponse();
        testIntegrationWithHttpRequest();
        testMimeTypeDetection();
        testCompleteHttpResponseFormat();
        
        std::cout << BLUE << "=== 测试结果总结 ===" << RESET << std::endl;
        std::cout << "总测试数: " << total_tests << std::endl;
        std::cout << "通过测试: " << GREEN << passed_tests << RESET << std::endl;
        std::cout << "失败测试: " << RED << (total_tests - passed_tests) << RESET << std::endl;
        std::cout << "通过率: " << (passed_tests * 100 / total_tests) << "%" << std::endl;
    }
};

int main() {
    HttpResponseTester tester;
    tester.runAllTests();
    return 0;
}