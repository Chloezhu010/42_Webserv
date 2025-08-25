#include "configparser.hpp"
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <algorithm>

ConfigParser::ConfigParser() : currentTokenIndex(0), currentLine(1), currentColumn(1) {
}

ConfigParser::~ConfigParser() {
}

bool ConfigParser::parseFile(const std::string& filename, Config& config) {
    std::ifstream file(filename);
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
    lastError = "Unknown character at line " + std::to_string(currentLine) + 
                ", column " + std::to_string(currentColumn);
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

std::string ConfigParser::readNumber(const std::string& content, size_t& pos) {
    std::string number;
    while (pos < content.length() && (isDigit(content[pos]) || content[pos] == '.')) {
        number += content[pos];
        pos++;
        currentColumn++;
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
                    printError("Expected location path");
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
                    return false;
                }
            }
        } else {
            printError("Expected directive name");
            return false;
        }
    }
    
    return true;
}

bool ConfigParser::parseLocation(LocationConfig& location) {
    while (currentToken().type != TOKEN_RBRACE && currentToken().type != TOKEN_EOF) {
        if (!parseLocationDirective(location)) {
            return false;
        }
    }
    return true;
}

bool ConfigParser::parseServerDirective(ServerConfig& server) {
    std::string directive = currentToken().value;
    consumeToken();
    
    std::vector<std::string> args = getDirectiveArgs();
    
    if (!expectSemicolon()) {
        return false;
    }
    
    if (directive == "listen") {
        parseListen(server, args);
    } else if (directive == "server_name") {
        parseServerName(server, args);
    } else if (directive == "root") {
        parseRoot(server.root, args);
    } else if (directive == "index") {
        parseIndex(server.index, args);
    } else if (directive == "client_max_body_size") {
        parseClientMaxBodySize(server, args);
    } else if (directive == "error_page") {
        parseErrorPage(server, args);
    } else {
        printError("Unknown server directive: " + directive);
        return false;
    }
    
    return true;
}

bool ConfigParser::parseLocationDirective(LocationConfig& location) {
    std::string directive = currentToken().value;
    consumeToken();
    
    std::vector<std::string> args = getDirectiveArgs();
    
    if (!expectSemicolon()) {
        return false;
    }
    
    if (directive == "root") {
        parseRoot(location.root, args);
    } else if (directive == "index") {
        parseIndex(location.index, args);
    } else if (directive == "allow_methods") {
        parseAllowMethods(location, args);
    } else if (directive == "autoindex") {
        parseAutoindex(location, args);
    } else if (directive == "cgi") {
        parseCgi(location, args);
    } else if (directive == "return" || directive == "redirect") {
        parseRedirect(location, args);
    } else {
        printError("Unknown location directive: " + directive);
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
        printError("Expected token type " + std::to_string(expectedType));
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

void ConfigParser::parseListen(ServerConfig& server, const std::vector<std::string>& args) {
    for (const std::string& arg : args) {
        try {
            int port = std::stoi(arg);
            server.addListenPort(port);
        } catch (const std::invalid_argument&) {
            printError("Invalid port number: " + arg);
        }
    }
}

void ConfigParser::parseServerName(ServerConfig& server, const std::vector<std::string>& args) {
    for (const std::string& arg : args) {
        server.addServerName(arg);
    }
}

void ConfigParser::parseRoot(std::string& root, const std::vector<std::string>& args) {
    if (!args.empty()) {
        root = args[0];
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
        try {
            int errorCode = std::stoi(args[0]);
            server.addErrorPage(errorCode, args[1]);
        } catch (const std::invalid_argument&) {
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
    if (!args.empty()) {
        location.redirect = args[0];
    }
}

size_t ConfigParser::parseSize(const std::string& sizeStr) {
    std::string str = sizeStr;
    size_t multiplier = 1;
    
    if (!str.empty()) {
        char last = std::tolower(str.back());
        if (last == 'k') {
            multiplier = 1024;
            str.pop_back();
        } else if (last == 'm') {
            multiplier = 1024 * 1024;
            str.pop_back();
        } else if (last == 'g') {
            multiplier = 1024 * 1024 * 1024;
            str.pop_back();
        }
    }
    
    try {
        return std::stoull(str) * multiplier;
    } catch (const std::invalid_argument&) {
        return 1048576; // 默认1MB
    }
}

void ConfigParser::printError(const std::string& message) {
    Token token = currentToken();
    lastError = message + " at line " + std::to_string(token.line) + 
                ", column " + std::to_string(token.column);
    std::cerr << "Parse Error: " << lastError << std::endl;
}

std::string ConfigParser::getLastError() const {
    return lastError;
}

void ConfigParser::clearError() {
    lastError.clear();
}