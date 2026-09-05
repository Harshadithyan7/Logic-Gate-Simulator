#include "parser.h"
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <iostream>

Lexer::Lexer(const std::string& source) : src(source), pos(0), line(1) {}

char Lexer::peek() const {
    if (pos >= src.length()) return '\0';
    return src[pos];
}

char Lexer::get() {
    if (pos >= src.length()) return '\0';
    char c = src[pos++];
    if (c == '\n') line++;
    return c;
}

void Lexer::skip_whitespace_and_comments() {
    while (pos < src.length()) {
        char c = peek();
        if (std::isspace(c)) {
            get();
        } else if (c == '/' && pos + 1 < src.length() && src[pos+1] == '/') {
            get(); get(); // consume '//'
            while (pos < src.length() && peek() != '\n') {
                get();
            }
        } else if (c == '#') {
            get(); // consume '#'
            while (pos < src.length() && peek() != '\n') {
                get();
            }
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skip_whitespace_and_comments();
        if (pos >= src.length()) break;

        char c = peek();
        int token_line = line;

        if (c == ',') {
            get();
            tokens.push_back({TokenType::COMMA, ",", token_line});
        } else if (c == ';') {
            get();
            tokens.push_back({TokenType::SEMICOLON, ";", token_line});
        } else if (c == '(') {
            get();
            tokens.push_back({TokenType::LPAREN, "(", token_line});
        } else if (c == ')') {
            get();
            tokens.push_back({TokenType::RPAREN, ")", token_line});
        } else if (c == '-' && pos + 1 < src.length() && src[pos+1] == '>') {
            get(); get(); // consume '->'
            tokens.push_back({TokenType::ARROW, "->", token_line});
        } else if (std::isalpha(c) || c == '_') {
            std::string ident;
            while (std::isalnum(peek()) || peek() == '_') {
                ident += get();
            }

            std::string upper_ident = ident;
            for (auto& ch : upper_ident) ch = std::toupper(ch);

            if (upper_ident == "INPUT") {
                tokens.push_back({TokenType::KEYWORD_INPUT, ident, token_line});
            } else if (upper_ident == "OUTPUT") {
                tokens.push_back({TokenType::KEYWORD_OUTPUT, ident, token_line});
            } else if (upper_ident == "WIRE") {
                tokens.push_back({TokenType::KEYWORD_WIRE, ident, token_line});
            } else if (upper_ident == "AND" || upper_ident == "OR" || upper_ident == "NOT" ||
                       upper_ident == "XOR" || upper_ident == "NAND" || upper_ident == "NOR" ||
                       upper_ident == "XNOR") {
                tokens.push_back({TokenType::GATE_TYPE, upper_ident, token_line});
            } else {
                tokens.push_back({TokenType::IDENTIFIER, ident, token_line});
            }
        } else {
            std::stringstream ss;
            ss << "Lexical error at line " << line << ": Unexpected character '" << c << "'";
            throw std::runtime_error(ss.str());
        }
    }

    tokens.push_back({TokenType::END_OF_FILE, "", line});
    return tokens;
}

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

const Token& Parser::peek() const {
    if (pos >= tokens.size()) return tokens.back();
    return tokens[pos];
}

const Token& Parser::get() {
    const Token& t = peek();
    if (pos < tokens.size()) pos++;
    return t;
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        get();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string& err_msg) {
    if (peek().type == type) {
        get();
        return;
    }
    std::stringstream ss;
    ss << "Parse error at line " << peek().line << ": " << err_msg << ". Got '" << peek().value << "'";
    throw std::runtime_error(ss.str());
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::END_OF_FILE;
}

std::shared_ptr<Circuit> Parser::parse() {
    auto circuit = std::make_shared<Circuit>();
    while (!is_at_end()) {
        parse_statement(*circuit);
    }
    return circuit;
}

void Parser::parse_statement(Circuit& circuit) {
    Token t = peek();
    if (match(TokenType::KEYWORD_INPUT)) {
        parse_input(circuit);
    } else if (match(TokenType::KEYWORD_OUTPUT)) {
        parse_output(circuit);
    } else if (match(TokenType::KEYWORD_WIRE)) {
        parse_wire(circuit);
    } else if (peek().type == TokenType::GATE_TYPE) {
        Token gate_type_tok = get();
        parse_gate(circuit, gate_type_tok);
    } else {
        std::stringstream ss;
        ss << "Parse error at line " << t.line << ": Expected statement (INPUT, OUTPUT, WIRE, or gate type). Got '" << t.value << "'";
        throw std::runtime_error(ss.str());
    }
}

void Parser::parse_input(Circuit& circuit) {
    while (true) {
        Token name_tok = peek();
        expect(TokenType::IDENTIFIER, "Expected identifier for input name");
        circuit.mark_input(name_tok.value);

        if (match(TokenType::COMMA)) {
            continue;
        } else {
            expect(TokenType::SEMICOLON, "Expected ';' or ',' in input declaration");
            break;
        }
    }
}

void Parser::parse_output(Circuit& circuit) {
    while (true) {
        Token name_tok = peek();
        expect(TokenType::IDENTIFIER, "Expected identifier for output name");
        circuit.mark_output(name_tok.value);

        if (match(TokenType::COMMA)) {
            continue;
        } else {
            expect(TokenType::SEMICOLON, "Expected ';' or ',' in output declaration");
            break;
        }
    }
}

void Parser::parse_wire(Circuit& circuit) {
    while (true) {
        Token name_tok = peek();
        expect(TokenType::IDENTIFIER, "Expected identifier for wire name");
        circuit.get_or_create_wire(name_tok.value);

        if (match(TokenType::COMMA)) {
            continue;
        } else {
            expect(TokenType::SEMICOLON, "Expected ';' or ',' in wire declaration");
            break;
        }
    }
}

void Parser::parse_gate(Circuit& circuit, const Token& gate_type_tok) {
    Token name_tok = peek();
    expect(TokenType::IDENTIFIER, "Expected identifier for gate name");

    GateType gate_type = Gate::from_string(gate_type_tok.value);
    auto gate = std::make_shared<Gate>(name_tok.value, gate_type);

    expect(TokenType::LPAREN, "Expected '(' after gate name");

    while (true) {
        Token wire_tok = peek();
        expect(TokenType::IDENTIFIER, "Expected identifier for gate input wire");
        gate->inputs.push_back(circuit.get_or_create_wire(wire_tok.value));

        if (match(TokenType::COMMA)) {
            continue;
        } else {
            break;
        }
    }

    expect(TokenType::RPAREN, "Expected ')' or ',' after gate inputs");
    expect(TokenType::ARROW, "Expected '->' after gate input list");

    while (true) {
        Token wire_tok = peek();
        expect(TokenType::IDENTIFIER, "Expected identifier for gate output wire");
        gate->outputs.push_back(circuit.get_or_create_wire(wire_tok.value));

        if (match(TokenType::COMMA)) {
            continue;
        } else {
            break;
        }
    }

    expect(TokenType::SEMICOLON, "Expected ';' after gate declaration");

    if (gate_type == GateType::NOT) {
        if (gate->inputs.size() != 1) {
            std::stringstream ss;
            ss << "Semantic error at line " << gate_type_tok.line << ": NOT gate '" << gate->name << "' must have exactly 1 input, but has " << gate->inputs.size() << ".";
            throw std::runtime_error(ss.str());
        }
        if (gate->outputs.size() != 1) {
            std::stringstream ss;
            ss << "Semantic error at line " << gate_type_tok.line << ": NOT gate '" << gate->name << "' must have exactly 1 output, but has " << gate->outputs.size() << ".";
            throw std::runtime_error(ss.str());
        }
    } else {
        if (gate->inputs.empty()) {
            std::stringstream ss;
            ss << "Semantic error at line " << gate_type_tok.line << ": Gate '" << gate->name << "' must have at least 1 input.";
            throw std::runtime_error(ss.str());
        }
        if (gate->outputs.size() != 1) {
            std::stringstream ss;
            ss << "Semantic error at line " << gate_type_tok.line << ": Gate '" << gate->name << "' must have exactly 1 output, but has " << gate->outputs.size() << ".";
            throw std::runtime_error(ss.str());
        }
    }

    circuit.add_gate(gate);
}
