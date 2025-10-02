#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "config.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// Token类型枚举
enum TokenType {
    TOKEN_WORD,         // 单词/标识符
    TOKEN_STRING,       // 字符串
    TOKEN_NUMBER,       // 数字
    TOKEN_SEMICOLON,    // ;
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }
    TOKEN_EOF,          // 文件结束
    TOKEN_COMMENT,      // 注释
    TOKEN_ERROR         // 错误
};

// Token结构体
struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType t = TOKEN_EOF, const std::string& v = "", size_t l = 0, size_t c = 0)
        : type(t), value(v), line(l), column(c) {}
};

// 配置解析器类
class ConfigParser {
private:
    std::string content;                // 配置文件内容
    std::vector<Token> tokens;          // 词法分析后的token列表
    size_t currentTokenIndex;           // 当前处理的token索引
    size_t currentLine;                 // 当前行号
    size_t currentColumn;               // 当前列号
    
    // 词法分析相关方法
    bool tokenize(const std::string& configContent);
    void skipWhitespace(const std::string& content, size_t& pos);
    Token getNextToken(const std::string& content, size_t& pos);
    bool isWordChar(char c);
    bool isDigit(char c);
    std::string readWord(const std::string& content, size_t& pos);
    std::string readNumber(const std::string& content, size_t& pos);
    std::string readString(const std::string& content, size_t& pos, char quote);
    void skipComment(const std::string& content, size_t& pos);
    
    // 语法分析相关方法
    bool parseConfig(Config& config);
    bool parseServer(ServerConfig& server);
    bool parseLocation(LocationConfig& location);
    bool parseServerDirective(ServerConfig& server);
    bool parseLocationDirective(LocationConfig& location);
    
    // 辅助方法
    Token currentToken();
    Token peekToken(size_t offset = 1);
    void consumeToken();
    bool expectToken(TokenType expectedType);
    bool expectSemicolon();
    bool expectOpenBrace();
    bool expectCloseBrace();
    
    // 指令解析方法
    void parseListen(ServerConfig& server, const std::vector<std::string>& args);
    void parseServerName(ServerConfig& server, const std::vector<std::string>& args);
    void parseRoot(std::string& root, const std::vector<std::string>& args);
    void parseIndex(std::vector<std::string>& index, const std::vector<std::string>& args);
    void parseClientMaxBodySize(ServerConfig& server, const std::vector<std::string>& args);
    void parseClientMaxBodySize(LocationConfig& location, const std::vector<std::string>& args);
    void parseErrorPage(ServerConfig& server, const std::vector<std::string>& args);
    void parseAllowMethods(LocationConfig& location, const std::vector<std::string>& args);
    void parseAutoindex(LocationConfig& location, const std::vector<std::string>& args);
    void parseCgi(LocationConfig& location, const std::vector<std::string>& args);
    void parseRedirect(LocationConfig& location, const std::vector<std::string>& args);
    void parseCgiPass(LocationConfig& location, const std::vector<std::string>& args);
	
	bool parseListenWithValidation(ServerConfig& server, const std::vector<std::string>& args);
    bool parseErrorPageWithValidation(ServerConfig& server, const std::vector<std::string>& args);
    bool parseAllowMethodsWithValidation(LocationConfig& location, const std::vector<std::string>& args);

	bool isValidPortString(const std::string& portStr);
    bool isValidIPAddress(const std::string& ip);

    // 工具方法
    std::vector<std::string> getDirectiveArgs();
    size_t parseSize(const std::string& sizeStr);
    void printError(const std::string& message);
	int stringToInt(const std::string& str);
    std::string intToString(int value);

public:
    ConfigParser();
    ~ConfigParser();
    
    // 主要解析方法
    bool parseFile(const std::string& filename, Config& config);
    bool parseString(const std::string& configContent, Config& config);
    
    // 错误处理
    std::string getLastError() const;
    void clearError();
    
private:
    std::string lastError;
};

#endif // CONFIG_PARSER_HPP