#include "configparser.hpp"
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>  // for atoi, strtoul
#include <cerrno>   // for errno

ConfigParser::ConfigParser() : currentTokenIndex(0), currentLine(1), currentColumn(1) {
}

ConfigParser::~ConfigParser() {
}

bool ConfigParser::parseFile(const std::string& filename, Config& config) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        lastError = "Cannot open file: " + filename;
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return parseString(content, config);
}

bool ConfigParser::parseString(const std::string& configContent, Config& config) {
    content = configContent;
    currentTokenIndex = 0;
    currentLine = 1;
    currentColumn = 1;
    tokens.clear();
    config.clear();
    
    // 词法分析
    if (!tokenize(configContent)) {
        return false;
    }
    
    // 语法分析
    return parseConfig(config);
}

bool ConfigParser::tokenize(const std::string& configContent) {
    size_t pos = 0;
    currentLine = 1;
    currentColumn = 1;
    
    while (pos < configContent.length()) {
        skipWhitespace(configContent, pos);
        if (pos >= configContent.length()) break;
        
        Token token = getNextToken(configContent, pos);
        if (token.type == TOKEN_ERROR) {
            return false;
        }
        
        if (token.type != TOKEN_COMMENT) {
            tokens.push_back(token);
        }
    }
    
    tokens.push_back(Token(TOKEN_EOF, "", currentLine, currentColumn));
    return true;
}

void ConfigParser::skipWhitespace(const std::string& content, size_t& pos) {
    while (pos < content.length() && std::isspace(content[pos])) {
        if (content[pos] == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }
        pos++;
    }
}

Token ConfigParser::getNextToken(const std::string& content, size_t& pos) {
    if (pos >= content.length()) {
        return Token(TOKEN_EOF, "", currentLine, currentColumn);
    }
    
    char c = content[pos];
    size_t tokenLine = currentLine;
    size_t tokenColumn = currentColumn;
    
    // 处理注释
    if (c == '#') {
        skipComment(content, pos);
        return Token(TOKEN_COMMENT, "", tokenLine, tokenColumn);
    }
    
    // 处理特殊字符
    switch (c) {
        case ';':
            pos++;
            currentColumn++;
            return Token(TOKEN_SEMICOLON, ";", tokenLine, tokenColumn);
        case '{':
            pos++;
            currentColumn++;
            return Token(TOKEN_LBRACE, "{", tokenLine, tokenColumn);
        case '}':
            pos++;
            currentColumn++;
            return Token(TOKEN_RBRACE, "}", tokenLine, tokenColumn);
        case '"':
        case '\'':
            return Token(TOKEN_STRING, readString(content, pos, c), tokenLine, tokenColumn);
    }
    
    // 处理数字
    if (isDigit(c)) {
        return Token(TOKEN_NUMBER, readNumber(content, pos), tokenLine, tokenColumn);
    }
    
    // 处理单词
    if (isWordChar(c)) {
        return Token(TOKEN_WORD, readWord(content, pos), tokenLine, tokenColumn);
    }
    
    // 未知字符
    lastError = "Unknown character at line " + intToString(currentLine) + 
                ", column " + intToString(currentColumn);
    return Token(TOKEN_ERROR, "", tokenLine, tokenColumn);
}

bool ConfigParser::isWordChar(char c) {
    return std::isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/' || c == ':';
}

bool ConfigParser::isDigit(char c) {
    return std::isdigit(c);
}

std::string ConfigParser::readWord(const std::string& content, size_t& pos) {
    std::string word;
    while (pos < content.length() && isWordChar(content[pos])) {
        word += content[pos];
        pos++;
        currentColumn++;
    }
    return word;
}

// 增强版 readNumber 函数，支持单位处理
std::string ConfigParser::readNumber(const std::string& content, size_t& pos) {
    std::string number;
    
    // 读取数字部分
    while (pos < content.length() && (isDigit(content[pos]) || content[pos] == '.')) {
        number += content[pos];
        pos++;
        currentColumn++;
    }
    
    // 检查是否是 IP:port 格式
    if (pos < content.length() && content[pos] == ':') {
        // 检查是否看起来像 IP 地址
        size_t dotCount = 0;
        for (size_t i = 0; i < number.length(); ++i) {
            if (number[i] == '.') dotCount++;
        }
        
        // 如果有3个点，很可能是 IP 地址，继续读取端口
        if (dotCount == 3) {
            number += content[pos]; // 添加 ':'
            pos++;
            currentColumn++;
            
            // 继续读取端口号
            while (pos < content.length() && isDigit(content[pos])) {
                number += content[pos];
                pos++;
                currentColumn++;
            }
            return number;
        }
    }
    
    // 原来的单位后缀处理
    if (pos < content.length()) {
        char nextChar = std::tolower(content[pos]);
        if (nextChar == 'k' || nextChar == 'm' || nextChar == 'g') {
            number += nextChar;
            pos++;
            currentColumn++;
        }
    }
    
    return number;
}

std::string ConfigParser::readString(const std::string& content, size_t& pos, char quote) {
    std::string str;
    pos++; // 跳过开始引号
    currentColumn++;
    
    while (pos < content.length() && content[pos] != quote) {
        if (content[pos] == '\\' && pos + 1 < content.length()) {
            pos++;
            currentColumn++;
            str += content[pos]; // 简单的转义处理
        } else {
            str += content[pos];
        }
        
        if (content[pos] == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }
        pos++;
    }
    
    if (pos < content.length()) {
        pos++; // 跳过结束引号
        currentColumn++;
    }
    
    return str;
}

void ConfigParser::skipComment(const std::string& content, size_t& pos) {
    while (pos < content.length() && content[pos] != '\n') {
        pos++;
        currentColumn++;
    }
}

bool ConfigParser::parseConfig(Config& config) {
    currentTokenIndex = 0;
    
    while (currentToken().type != TOKEN_EOF) {
        if (currentToken().type == TOKEN_WORD && currentToken().value == "server") {
            ServerConfig server;
            consumeToken(); // 消费 "server"
            
            if (!expectOpenBrace()) {
                return false;
            }
            
            if (!parseServer(server)) {
                return false;
            }
            
            if (!expectCloseBrace()) {
                return false;
            }
            
            config.addServer(server);
        } else {
            printError("Expected 'server' directive");
            return false;
        }
    }
    
    return true;
}

bool ConfigParser::parseServer(ServerConfig& server) {
    while (currentToken().type != TOKEN_RBRACE && currentToken().type != TOKEN_EOF) {
        if (currentToken().type == TOKEN_WORD) {
            std::string directive = currentToken().value;
            
            if (directive == "location") {
                consumeToken(); // 消费 "location"
                
                if (currentToken().type != TOKEN_WORD) {
                    printError("期望location路径");
                    return false;
                }
                
                LocationConfig location(currentToken().value);
                consumeToken(); // 消费路径
                
                if (!expectOpenBrace()) {
                    return false;
                }
                
                if (!parseLocation(location)) {
                    return false;
                }
                
                if (!expectCloseBrace()) {
                    return false;
                }
                
                server.addLocation(location);
            } else {
                if (!parseServerDirective(server)) {
                    return false;  // 传播错误
                }
            }
        } else {
            printError("期望指令名称");
            return false;
        }
    }
    
    return true;
}

bool ConfigParser::parseLocation(LocationConfig& location) {
    while (currentToken().type != TOKEN_RBRACE && currentToken().type != TOKEN_EOF) {
        if (!parseLocationDirective(location)) {
            return false;  // 传播错误
        }
    }
    return true;
}

bool ConfigParser::parseListenWithValidation(ServerConfig& server, const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& portStr = args[i];
        
        // 检查是否为纯数字
        if (portStr.empty()) {
            printError("端口号不能为空");
            return false;
        }
        
        // 验证字符串只包含数字
        for (size_t j = 0; j < portStr.length(); ++j) {
            if (!std::isdigit(portStr[j])) {
                printError("无效的端口号格式: " + portStr + " (只允许数字)");
                return false;
            }
        }
        
        // 转换为整数并验证范围
        long port = std::strtol(portStr.c_str(), NULL, 10);
        if (port <= 0 || port > 65535) {
            printError("端口号超出范围(1-65535): " + portStr);
            return false;
        }
        
        server.addListenPort(static_cast<int>(port));
    }
    return true;
}

bool ConfigParser::parseErrorPageWithValidation(ServerConfig& server, const std::vector<std::string>& args) {
    // 验证错误码
    const std::string& errorCodeStr = args[0];
    
    // 检查错误码是否为数字
    for (size_t i = 0; i < errorCodeStr.length(); ++i) {
        if (!std::isdigit(errorCodeStr[i])) {
            printError("无效的错误码格式: " + errorCodeStr);
            return false;
        }
    }
    
    int errorCode = stringToInt(errorCodeStr);
    if (errorCode < 100 || errorCode > 599) {
        printError("HTTP错误码超出范围(100-599): " + errorCodeStr);
        return false;
    }
    
    server.addErrorPage(errorCode, args[1]);
    return true;
}

bool ConfigParser::parseAllowMethodsWithValidation(LocationConfig& location, const std::vector<std::string>& args) {
    // 验证HTTP方法
    std::vector<std::string> validMethods;
    validMethods.push_back("GET");
    validMethods.push_back("POST");
    validMethods.push_back("PUT");
    validMethods.push_back("DELETE");
    validMethods.push_back("HEAD");
    validMethods.push_back("OPTIONS");
    validMethods.push_back("PATCH");
    
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& method = args[i];
        bool isValid = false;
        
        // 检查是否为有效的HTTP方法
        for (size_t j = 0; j < validMethods.size(); ++j) {
            if (method == validMethods[j]) {
                isValid = true;
                break;
            }
        }
        
        if (!isValid) {
            printError("无效的HTTP方法: " + method);
            return false;
        }
    }
    
    location.allowMethods = args;
    return true;
}

bool ConfigParser::parseServerDirective(ServerConfig& server) {
    std::string directive = currentToken().value;
    consumeToken();
    
    std::vector<std::string> args = getDirectiveArgs();
    
    // 为需要参数的指令添加参数数量验证
    if (directive == "listen") {
        if (args.empty()) {
            printError("listen指令至少需要一个参数");
            return false;
        }
        if (!parseListenWithValidation(server, args)) {
            return false;
        }
    } else if (directive == "server_name") {
        if (args.empty()) {
            printError("server_name指令至少需要一个参数");
            return false;
        }
        parseServerName(server, args);
    } else if (directive == "root") {
        if (args.empty()) {
            printError("root指令需要一个参数");
            return false;
        }
        parseRoot(server.root, args);
    } else if (directive == "index") {
        if (args.empty()) {
            printError("index指令至少需要一个参数");
            return false;
        }
        parseIndex(server.index, args);
    } else if (directive == "client_max_body_size") {
        if (args.empty()) {
            printError("client_max_body_size指令需要一个参数");
            return false;
        }
        parseClientMaxBodySize(server, args);
    } else if (directive == "error_page") {
        if (args.size() < 2) {
            printError("error_page指令需要至少两个参数（错误码和页面路径）");
            return false;
        }
        if (!parseErrorPageWithValidation(server, args)) {
            return false;
        }
    } else {
        printError("未知的服务器指令: " + directive);
        return false;
    }
    
    if (!expectSemicolon()) {
        return false;
    }
    
    return true;
}

void ConfigParser::parseCgiPass(LocationConfig& location, const std::vector<std::string>& args) {
    if (!args.empty()) {
        location.cgiPath = args[0];
        
        if (location.cgiExtension.empty()) {
            std::string path = args[0];
            size_t lastDot = path.find_last_of('.');
            if (lastDot != std::string::npos) {
                location.cgiExtension = path.substr(lastDot);
            }
        }
    }
}

bool ConfigParser::parseLocationDirective(LocationConfig& location) {
    std::string directive = currentToken().value;
    consumeToken();
    
    std::vector<std::string> args = getDirectiveArgs();
    
    // 为需要参数的指令添加参数验证
    if (directive == "root") {
        if (args.empty()) {
            printError("root指令需要一个参数");
            return false;
        }
        parseRoot(location.root, args);
    } else if (directive == "alias") {
        if (args.empty()) {
            printError("alias directive requires one argument");
            return false;
        }
        parseAlias(location.alias, args);
    } else if (directive == "index") {
        if (args.empty()) {
            printError("index指令至少需要一个参数");
            return false;
        }
        parseIndex(location.index, args);
    } else if (directive == "allow_methods") {
        if (args.empty()) {
            printError("allow_methods指令至少需要一个参数");
            return false;
        }
        if (!parseAllowMethodsWithValidation(location, args)) {
            return false;
        }
    } else if (directive == "autoindex") {
        if (args.empty()) {
            printError("autoindex指令需要一个参数");
            return false;
        }
        parseAutoindex(location, args);
    } else if (directive == "cgi") {
        if (args.size() < 2) {
            printError("cgi指令需要两个参数（扩展名和程序路径）");
            return false;
        }
        parseCgi(location, args);
    } else if (directive == "cgi_pass") {
        if (args.empty()) {
            printError("cgi_pass指令需要一个参数");
            return false;
        }
        parseCgiPass(location, args);
    } else if (directive == "return" || directive == "redirect") {
        if (args.empty()) {
            printError(directive + "指令需要一个参数");
            return false;
        }
        parseRedirect(location, args);
    } else {
        printError("未知的location指令: " + directive);
        return false;
    }
    
    if (!expectSemicolon()) {
        return false;
    }
    
    return true;
}

Token ConfigParser::currentToken() {
    if (currentTokenIndex >= tokens.size()) {
        return Token(TOKEN_EOF);
    }
    return tokens[currentTokenIndex];
}

Token ConfigParser::peekToken(size_t offset) {
    size_t index = currentTokenIndex + offset;
    if (index >= tokens.size()) {
        return Token(TOKEN_EOF);
    }
    return tokens[index];
}

void ConfigParser::consumeToken() {
    if (currentTokenIndex < tokens.size()) {
        currentTokenIndex++;
    }
}

bool ConfigParser::expectToken(TokenType expectedType) {
    if (currentToken().type != expectedType) {
        printError("Expected token type " + intToString(expectedType));
        return false;
    }
    consumeToken();
    return true;
}

bool ConfigParser::expectSemicolon() {
    return expectToken(TOKEN_SEMICOLON);
}

bool ConfigParser::expectOpenBrace() {
    return expectToken(TOKEN_LBRACE);
}

bool ConfigParser::expectCloseBrace() {
    return expectToken(TOKEN_RBRACE);
}

std::vector<std::string> ConfigParser::getDirectiveArgs() {
    std::vector<std::string> args;
    
    while (currentToken().type != TOKEN_SEMICOLON && 
           currentToken().type != TOKEN_EOF &&
           currentToken().type != TOKEN_LBRACE &&
           currentToken().type != TOKEN_RBRACE) {
        
        if (currentToken().type == TOKEN_WORD || 
            currentToken().type == TOKEN_STRING || 
            currentToken().type == TOKEN_NUMBER) {
            args.push_back(currentToken().value);
            consumeToken();
        } else {
            break;
        }
    }
    
    return args;
}

// C++98 compatible string to int conversion
int ConfigParser::stringToInt(const std::string& str) {
    // Using atoi for simple conversion
    return std::atoi(str.c_str());
}

// C++98 compatible int to string conversion
std::string ConfigParser::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

void ConfigParser::parseListen(ServerConfig& server, const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        int port = stringToInt(args[i]);
        if (port > 0 && port <= 65535) {
            server.addListenPort(port);
        } else {
            printError("Invalid port number: " + args[i]);
        }
    }
}

void ConfigParser::parseServerName(ServerConfig& server, const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        server.addServerName(args[i]);
    }
}

void ConfigParser::parseRoot(std::string& root, const std::vector<std::string>& args) {
    if (!args.empty()) {
        root = args[0];
    }
}

void ConfigParser::parseAlias(std::string& alias, const std::vector<std::string>& args) {
    if (!args.empty()) {
        alias = args[0];
    }
}

void ConfigParser::parseIndex(std::vector<std::string>& index, const std::vector<std::string>& args) {
    index = args;
}

void ConfigParser::parseClientMaxBodySize(ServerConfig& server, const std::vector<std::string>& args) {
    if (!args.empty()) {
        server.clientMaxBodySize = parseSize(args[0]);
    }
}

void ConfigParser::parseErrorPage(ServerConfig& server, const std::vector<std::string>& args) {
    if (args.size() >= 2) {
        int errorCode = stringToInt(args[0]);
        if (errorCode > 0) {
            server.addErrorPage(errorCode, args[1]);
        } else {
            printError("Invalid error code: " + args[0]);
        }
    }
}

void ConfigParser::parseAllowMethods(LocationConfig& location, const std::vector<std::string>& args) {
    location.allowMethods = args;
}

void ConfigParser::parseAutoindex(LocationConfig& location, const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::string value = args[0];
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        location.autoindex = (value == "on" || value == "true" || value == "1");
    }
}

void ConfigParser::parseCgi(LocationConfig& location, const std::vector<std::string>& args) {
    if (args.size() >= 2) {
        location.cgiExtension = args[0];
        location.cgiPath = args[1];
    }
}

void ConfigParser::parseRedirect(LocationConfig& location, const std::vector<std::string>& args) {
    // if (!args.empty()) {
    //     location.redirect = args[0];
    // }
    if (args.size() >= 2) {
        // store both status code and URL
        location.redirect = args[0] + " " + args[1];
    } else if (args.size() == 1) {
        // default to 302 if only URL is provided
        location.redirect = "302 " + args[0];
    }
}

// C++98 compatible size parsing
size_t ConfigParser::parseSize(const std::string& sizeStr) {
    std::string str = sizeStr;
    size_t multiplier = 1;
    
    if (!str.empty()) {
        char last = std::tolower(str[str.length() - 1]);
        if (last == 'k') {
            multiplier = 1024;
            str = str.substr(0, str.length() - 1);
        } else if (last == 'm') {
            multiplier = 1024 * 1024;
            str = str.substr(0, str.length() - 1);
        } else if (last == 'g') {
            multiplier = 1024 * 1024 * 1024;
            str = str.substr(0, str.length() - 1);
        }
    }
    
    // Using strtoul for C++98 compatibility
    char* end;
    errno = 0;
    unsigned long result = std::strtoul(str.c_str(), &end, 10);
    
    // Check for conversion errors
    if (errno == ERANGE || *end != '\0' || end == str.c_str()) {
        return 1048576; // Default 1MB
    }
    
    return static_cast<size_t>(result) * multiplier;
}

void ConfigParser::printError(const std::string& message) {
    Token token = currentToken();
    lastError = message + " at line " + intToString(token.line) + 
                ", column " + intToString(token.column);
    std::cerr << "Parse Error: " << lastError << std::endl;
}

std::string ConfigParser::getLastError() const {
    return lastError;
}

void ConfigParser::clearError() {
    lastError.clear();
}