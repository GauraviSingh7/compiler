// ============================================================
// lexer.cpp — Lexer implementation
// ============================================================
#include "lexer.h"
#include <cctype>
#include <algorithm>
#include <stdexcept>

// ── Keyword table (all lowercase; identifiers are lowercased before lookup) ──
const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"and",       TokenType::KW_AND},
    {"array",     TokenType::KW_ARRAY},
    {"begin",     TokenType::KW_BEGIN},
    {"integer",   TokenType::KW_INTEGER},
    {"do",        TokenType::KW_DO},
    {"else",      TokenType::KW_ELSE},
    {"end",       TokenType::KW_END},
    {"function",  TokenType::KW_FUNCTION},
    {"if",        TokenType::KW_IF},
    {"of",        TokenType::KW_OF},
    {"or",        TokenType::KW_OR},
    {"not",       TokenType::KW_NOT},
    {"procedure", TokenType::KW_PROCEDURE},
    {"program",   TokenType::KW_PROGRAM},
    {"read",      TokenType::KW_READ},
    {"then",      TokenType::KW_THEN},
    {"var",       TokenType::KW_VAR},
    {"while",     TokenType::KW_WHILE},
    {"write",     TokenType::KW_WRITE},
};

// ── Constructor ────────────────────────────────────────────
Lexer::Lexer(const std::string& source)
    : src(source), pos(0), line(1) {}

// ── Character helpers ──────────────────────────────────────
char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return src[pos];
}

char Lexer::peekNext() const {
    if (pos + 1 >= src.size()) return '\0';
    return src[pos + 1];
}

char Lexer::advance() {
    char c = src[pos++];
    if (c == '\n') line++;
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= src.size();
}

// ── Skip whitespace (spaces, tabs, newlines) ───────────────
void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace((unsigned char)peek()))
        advance();
}

// ── Skip comment: from '!' to end of line ─────────────────
void Lexer::skipComment() {
    // consume '!'
    advance();
    while (!isAtEnd() && peek() != '\n')
        advance();
    // The '\n' is left for advance() to handle line counting
}

// ── Scan identifier or reserved word ──────────────────────
Token Lexer::scanIdentifierOrKeyword() {
    int startLine = line;
    std::string raw;

    while (!isAtEnd() && (std::isalnum((unsigned char)peek()) || peek() == '_')) {
        raw += advance();
    }

    // Spec: distinguishable within first 32 characters
    std::string key = raw.substr(0, 32);

    // Case-insensitive: lowercase for keyword lookup
    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = keywords.find(lower);
    if (it != keywords.end())
        return Token(it->second, lower, startLine);

    return Token(TokenType::IDENTIFIER, lower, startLine);
}

// ── Scan integer literal ───────────────────────────────────
Token Lexer::scanNumber() {
    int startLine = line;
    std::string num;

    while (!isAtEnd() && std::isdigit((unsigned char)peek()))
        num += advance();

    return Token(TokenType::NUMBER, num, startLine);
}

// ── Scan string literal with escape sequences ─────────────
// Format: 'characters'
// Escapes: \n (newline), \t (tab), \\ (backslash), \' (apostrophe)
// Error: newline inside string is illegal
Token Lexer::scanString() {
    int startLine = line;
    advance(); // consume opening '
    std::string value;

    while (true) {
        if (isAtEnd()) {
            errors.push_back("Line " + std::to_string(startLine) +
                             ": Unterminated string literal");
            return Token(TokenType::ERROR, value, startLine);
        }

        char c = peek();

        if (c == '\n') {
            errors.push_back("Line " + std::to_string(line) +
                             ": Newline inside string literal is illegal");
            // consume the newline and stop the string
            advance();
            return Token(TokenType::ERROR, value, startLine);
        }

        if (c == '\'') {
            advance(); // consume closing '
            return Token(TokenType::STRING, value, startLine);
        }

        if (c == '\\') {
            advance(); // consume backslash
            if (isAtEnd()) {
                errors.push_back("Line " + std::to_string(line) +
                                 ": Unexpected end after backslash in string");
                return Token(TokenType::ERROR, value, startLine);
            }
            char esc = advance();
            switch (esc) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;
                default:
                    // spec: \X denotes character X
                    value += esc;
                    break;
            }
        } else {
            value += advance();
        }
    }
}

// ── Scan delimiters and compound symbols ──────────────────
Token Lexer::scanDelimiterOrCompound() {
    int startLine = line;
    char c = advance();

    switch (c) {
        case '(': return Token(TokenType::LPAREN,    "(", startLine);
        case ')': return Token(TokenType::RPAREN,    ")", startLine);
        case '[': return Token(TokenType::LBRACKET,  "[", startLine);
        case ']': return Token(TokenType::RBRACKET,  "]", startLine);
        case ';': return Token(TokenType::SEMICOLON, ";", startLine);
        case '.': return Token(TokenType::DOT,       ".", startLine);
        case ',': return Token(TokenType::COMMA,     ",", startLine);
        case '*': return Token(TokenType::STAR,      "*", startLine);
        case '-': return Token(TokenType::MINUS,     "-", startLine);
        case '+': return Token(TokenType::PLUS,      "+", startLine);
        case '/': return Token(TokenType::SLASH,     "/", startLine);
        case '=': return Token(TokenType::EQUAL,     "=", startLine);

        case ':':
            if (peek() == '=') {
                advance();
                return Token(TokenType::ASSIGN, ":=", startLine);
            }
            return Token(TokenType::COLON, ":", startLine);

        case '<':
            if (peek() == '>') {
                advance();
                return Token(TokenType::NEQ, "<>", startLine);
            }
            if (peek() == '=') {
                advance();
                return Token(TokenType::LEQ, "<=", startLine);
            }
            return Token(TokenType::LESS, "<", startLine);

        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GEQ, ">=", startLine);
            }
            return Token(TokenType::GREATER, ">", startLine);

        default: {
            std::string msg = "Line " + std::to_string(startLine) +
                              ": Unknown character '" + c + "'";
            errors.push_back(msg);
            return Token(TokenType::ERROR, std::string(1, c), startLine);
        }
    }
}

// ── Main entry: get next token ─────────────────────────────
Token Lexer::nextToken() {
    while (true) {
        skipWhitespace();

        if (isAtEnd())
            return Token(TokenType::EOF_TOKEN, "EOF", line);

        char c = peek();

        if (c == '!') {
            skipComment();
            continue; // re-enter loop after comment
        }

        if (std::isalpha((unsigned char)c))
            return scanIdentifierOrKeyword();

        if (std::isdigit((unsigned char)c))
            return scanNumber();

        if (c == '\'')
            return scanString();

        return scanDelimiterOrCompound();
    }
}

// ── Tokenize entire source (for debug / listing file) ─────
std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    pos = 0; line = 1;

    while (true) {
        Token t = nextToken();
        tokens.push_back(t);
        if (t.type == TokenType::EOF_TOKEN) break;
    }
    return tokens;
}