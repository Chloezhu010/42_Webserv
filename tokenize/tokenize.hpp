#ifndef TOKENIZE_HPP
#define TOKENIZE_HPP

#include <string>
#include <vector>

// Token类型定义
enum TokenType {
    TOKEN_WORD,
    TOKEN_LBRACE,   // {
    TOKEN_RBRACE,   // }
    TOKEN_SEMICOLON,// ;
    TOKEN_COMMENT,  // #...
    TOKEN_STRING,   // "..."
    TOKEN_UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    Token(TokenType t, const std::string& v) : type(t), value(v) {}
};

std::vector<Token> tokenize(const std::string& configText);

#endif // TOKENIZE_HPP