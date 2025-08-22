#include "tokenize.hpp"
#include <cctype>

std::vector<Token> tokenize(const std::string& configText) {
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < configText.size()) {
        char c = configText[i];
        if (isspace(c)) {
            ++i;
            continue;
        }
        if (c == '{') {
            tokens.push_back(Token(TOKEN_LBRACE, "{"));
            ++i;
            continue;
        }
        if (c == '}') {
            tokens.push_back(Token(TOKEN_RBRACE, "}"));
            ++i;
            continue;
        }
        if (c == ';') {
            tokens.push_back(Token(TOKEN_SEMICOLON, ";"));
            ++i;
            continue;
        }
        if (c == '#') {
            size_t start = i;
            while (i < configText.size() && configText[i] != '\n') ++i;
            tokens.push_back(Token(TOKEN_COMMENT, configText.substr(start, i - start)));
            continue;
        }
        // 普通单词 - 扩展支持的字符集
        if (isalnum(c) || c == '_' || c == '/' || c == '.' || c == '-' || c == ':') {
            size_t start = i;
            while (i < configText.size() && 
                   (isalnum(configText[i]) || configText[i] == '_' || 
                    configText[i] == '/' || configText[i] == '.' || 
                    configText[i] == '-' || configText[i] == ':')) {
                ++i;
            }
            tokens.push_back(Token(TOKEN_WORD, configText.substr(start, i - start)));
            continue;
        }
        // 其他未知字符
        tokens.push_back(Token(TOKEN_UNKNOWN, std::string(1, c)));
        ++i;
    }
    return tokens;
}