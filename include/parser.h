#ifndef PARSER_H
#define PARSER_H

#include "circuit.h"
#include <string>
#include <vector>
#include <memory>

enum class TokenType {
    KEYWORD_INPUT,
    KEYWORD_OUTPUT,
    KEYWORD_WIRE,
    GATE_TYPE,
    IDENTIFIER,
    COMMA,
    SEMICOLON,
    LPAREN,
    RPAREN,
    ARROW,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t pos;
    int line;

    char peek() const;
    char get();
    void skip_whitespace_and_comments();
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::shared_ptr<Circuit> parse();

private:
    std::vector<Token> tokens;
    size_t pos;

    const Token& peek() const;
    const Token& get();
    bool match(TokenType type);
    void expect(TokenType type, const std::string& err_msg);
    bool is_at_end() const;

    void parse_statement(Circuit& circuit);
    void parse_input(Circuit& circuit);
    void parse_output(Circuit& circuit);
    void parse_wire(Circuit& circuit);
    void parse_gate(Circuit& circuit, const Token& gate_type_tok);
};

#endif // PARSER_H
