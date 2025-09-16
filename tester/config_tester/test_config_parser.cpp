// test_config_parser.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "../configparser/configparser.hpp"

class ConfigParserTests {
private:
    int totalTests;
    int passedTests;

public:
    ConfigParserTests() : totalTests(0), passedTests(0) {}

    void assert_true(bool condition, const std::string& testName) {
        totalTests++;
        if (condition) {
            passedTests++;
            std::cout << "✅ PASS: " << testName << std::endl;
        } else {
            std::cout << "❌ FAIL: " << testName << std::endl;
        }
    }

    void assert_false(bool condition, const std::string& testName) {
        assert_true(!condition, testName);
    }

    // 测试词法分析corner cases
    void testLexicalAnalysis() {
        std::cout << "\n--- Lexical Analysis Tests ---" << std::endl;
        
        ConfigParser parser;
        Config config;
        
        // 测试未闭合的字符串
        std::string unclosedString = "server { listen \"8080; }";
        bool result1 = parser.parseString(unclosedString, config);
        assert_false(result1, "Unclosed string should fail");
        
        // 测试未闭合的花括号
        std::string unclosedBrace = "server { listen 8080;";
        bool result2 = parser.parseString(unclosedBrace, config);
        assert_false(result2, "Unclosed brace should fail");
        
        // 测试无效字符
        std::string invalidChar = "server { listen 8080@; }";
        bool result3 = parser.parseString(invalidChar, config);
        assert_false(result3, "Invalid character should fail");
        
        // 测试注释处理
        std::string withComments = 
            "# This is a comment\n"
            "server { # inline comment\n"
            "    listen 8080; # port comment\n"
            "    # another comment\n"
            "}";
        bool result4 = parser.parseString(withComments, config);
        assert_true(result4, "Comments should be ignored");
        
        // 测试空白字符处理
        std::string withWhitespace = 
            "   server   {   \n"
            "   \t listen  \t 8080  ;  \n"
            "   }   \n";
        bool result5 = parser.parseString(withWhitespace, config);
        assert_true(result5, "Whitespace should be handled correctly");
    }

    // 测试语法分析corner cases
    void testSyntaxAnalysis() {
        std::cout << "\n--- Syntax Analysis Tests ---" << std::endl;
        
        ConfigParser parser;
        Config config;
        
        // 测试缺少分号
        std::string missingSemicolon = "server { listen 8080 }";
        bool result1 = parser.parseString(missingSemicolon, config);
        assert_false(result1, "Missing semicolon should fail");
        
        // 测试意外的token
        std::string unexpectedToken = "server { listen 8080; } extra";
        bool result2 = parser.parseString(unexpectedToken, config);
        assert_false(result2, "Unexpected token should fail");
        
        // 测试嵌套server块
        std::string nestedServer = 
            "server {\n"
            "    listen 8080;\n"
            "    server {\n"
            "        listen 8081;\n"
            "    }\n"
            "}";
        bool result3 = parser.parseString(nestedServer, config);
        assert_false(result3, "Nested server blocks should fail");
        
        // 测试location外的location指令
        std::string invalidLocation = 
            "server {\n"
            "    listen 8080;\n"
            "    root ./www;\n"
            "}\n"
            "location / {\n"
            "    root ./other;\n"
            "}";
        bool result4 = parser.parseString(invalidLocation, config);
        assert_false(result4, "Location outside server should fail");
    }

    // 测试指令解析corner cases
    void testDirectiveProcessing() {
        std::cout << "\n--- Directive Processing Tests ---" << std::endl;
        
        ConfigParser parser;
        Config config;
        
        // 测试重复指令
        std::string duplicateDirectives = 
            "server {\n"
            "    listen 8080;\n"
            "    listen 8081;\n"  // 这应该是允许的
            "    root ./www1;\n"
            "    root ./www2;\n"  // 这会覆盖前一个
            "}";
        bool result1 = parser.parseString(duplicateDirectives, config);
        assert_true(result1, "Multiple directives should be handled");
        
        // 测试无效的数字格式
        std::string invalidNumber = 
            "server {\n"
            "    listen abc;\n"
            "}";
        bool result2 = parser.parseString(invalidNumber, config);
        assert_false(result2, "Invalid number format should fail");
        
        // 测试空指令参数
        std::string emptyArgs = 
            "server {\n"
            "    listen ;\n"
            "}";
        bool result3 = parser.parseString(emptyArgs, config);
        assert_false(result3, "Empty directive args should fail");
        
        // 测试过长的字符串
        std::string longString = "server { server_name ";
        for (int i = 0; i < 10000; i++) {
            longString += "a";
        }
        longString += "; listen 8080; }";
        bool result4 = parser.parseString(longString, config);
        assert_true(result4, "Long strings should be handled");
    }

    // 测试大小单位解析
    void testSizeUnitParsing() {
        std::cout << "\n--- Size Unit Parsing Tests ---" << std::endl;
        
        ConfigParser parser;
        Config config;
        
        // 测试各种单位
        std::string sizeUnits = 
            "server {\n"
            "    listen 8080;\n"
            "    client_max_body_size 1k;\n"
            "}\n"
            "server {\n"
            "    listen 8081;\n"
            "    client_max_body_size 10m;\n"
            "}\n"
            "server {\n"
            "    listen 8082;\n"
            "    client_max_body_size 1g;\n"
            "}";
        bool result1 = parser.parseString(sizeUnits, config);
        assert_true(result1, "Size units should be parsed correctly");
        
        // 验证解析结果
        if (result1 && config.getServerCount() >= 3) {
            const ServerConfig& server1 = config.getServer(0);
            const ServerConfig& server2 = config.getServer(1);
            const ServerConfig& server3 = config.getServer(2);
            
            assert_true(server1.clientMaxBodySize == 1024, "1k should equal 1024 bytes");
            assert_true(server2.clientMaxBodySize == 10 * 1024 * 1024, "10m should equal 10MB");
            assert_true(server3.clientMaxBodySize == 1024 * 1024 * 1024, "1g should equal 1GB");
        }
        
        // 测试无效单位
        std::string invalidUnit = 
            "server {\n"
            "    listen 8080;\n"
            "    client_max_body_size 10x;\n"
            "}";
        bool result2 = parser.parseString(invalidUnit, config);
        assert_true(result2, "Invalid unit should use default value");
    }

    // 测试错误报告
    void testErrorReporting() {
        std::cout << "\n--- Error Reporting Tests ---" << std::endl;
        
        ConfigParser parser;
        Config config;
        
        // 测试错误消息包含行号
        std::string multiLineError = 
            "server {\n"
            "    listen 8080;\n"
            "    invalid_directive;\n"  // 第3行错误
            "}";
        bool result = parser.parseString(multiLineError, config);
        assert_false(result, "Should report error with line number");
        
        std::string error = parser.getLastError();
        assert_true(error.find("line") != std::string::npos, "Error should contain line information");
        
        // 清除错误后应该可以重新解析
        parser.clearError();
        std::string validConfig = "server { listen 8080; }";
        bool result2 = parser.parseString(validConfig, config);
        assert_true(result2, "Should parse successfully after clearing error");
    }

    void runAllTests() {
        std::cout << "========== TESTING CONFIG PARSER ==========\n" << std::endl;
        
        testLexicalAnalysis();
        testSyntaxAnalysis();
        testDirectiveProcessing();
        testSizeUnitParsing();
        testErrorReporting();
        
        std::cout << "\n========== PARSER TEST SUMMARY ===========" << std::endl;
        std::cout << "Total tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
        std::cout << "Success rate: " << (totalTests > 0 ? (passedTests * 100 / totalTests) : 0) << "%" << std::endl;
        std::cout << "==========================================" << std::endl;
    }

    bool allTestsPassed() {
        return passedTests == totalTests;
    }
};

int main() {
    ConfigParserTests tests;
    tests.runAllTests();
    return tests.allTestsPassed() ? 0 : 1;
}