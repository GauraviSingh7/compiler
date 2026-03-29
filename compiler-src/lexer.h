// ============================================================
// lexer.h — Token types and Lexer class declaration
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// ── Token Types ────────────────────────────────────────────
enum class TokenType {
    // Literals
    IDENTIFIER, NUMBER, STRING,

    // Reserved words (exactly as per spec)
    KW_AND, KW_ARRAY, KW_BEGIN, KW_INTEGER, KW_DO,
    KW_ELSE, KW_END, KW_FUNCTION, KW_IF, KW_OF,
    KW_OR, KW_NOT, KW_PROCEDURE, KW_PROGRAM,
    KW_READ, KW_THEN, KW_VAR, KW_WHILE, KW_WRITE,

    // Single-character delimiters
    LPAREN,       // (
    RPAREN,       // )
    LBRACKET,     // [
    RBRACKET,     // ]
    SEMICOLON,    // ;
    COLON,        // :
    DOT,          // .
    COMMA,        // ,
    STAR,         // *
    MINUS,        // -
    PLUS,         // +
    SLASH,        // /
    LESS,         // 
    EQUAL,        // =
    GREATER,      // >

    // Compound symbols
    NEQ,          // <>
    ASSIGN,       // :=
    LEQ,          // <=
    GEQ,          // >=

    // Special
    EOF_TOKEN,
    ERROR
};

// ── Token Struct ───────────────────────────────────────────
struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;

    Token(TokenType t, std::string lex, int ln)
        : type(t), lexeme(std::move(lex)), line(ln) {}
};

// ── Lexer Class ────────────────────────────────────────────
class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Returns the next token from the source
    Token nextToken();

    // Tokenize entire source at once (for debug listing)
    std::vector<Token> tokenizeAll();

    // Errors accumulated during lexing
    std::vector<std::string> errors;

private:
    std::string src;      // full source text
    size_t      pos;      // current position
    int         line;     // current line number

    // Character helpers
    char peek()  const;               // look at current char
    char peekNext() const;            // look one ahead
    char advance();                   // consume & return current char
    bool isAtEnd() const;

    // Scanners for each token category
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    void  skipComment();
    void  skipWhitespace();
    Token scanDelimiterOrCompound();

    // Keyword lookup table
    static const std::unordered_map<std::string, TokenType> keywords;
};