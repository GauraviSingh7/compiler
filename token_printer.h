// ============================================================
// token_printer.h — Helper to print token type as string
// ============================================================
#pragma once
#include "lexer.h"
#include <string>

inline std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::STRING:      return "STRING";
        case TokenType::KW_AND:      return "KW_AND";
        case TokenType::KW_ARRAY:    return "KW_ARRAY";
        case TokenType::KW_BEGIN:    return "KW_BEGIN";
        case TokenType::KW_INTEGER:  return "KW_INTEGER";
        case TokenType::KW_DO:       return "KW_DO";
        case TokenType::KW_ELSE:     return "KW_ELSE";
        case TokenType::KW_END:      return "KW_END";
        case TokenType::KW_FUNCTION: return "KW_FUNCTION";
        case TokenType::KW_IF:       return "KW_IF";
        case TokenType::KW_OF:       return "KW_OF";
        case TokenType::KW_OR:       return "KW_OR";
        case TokenType::KW_NOT:      return "KW_NOT";
        case TokenType::KW_PROCEDURE:return "KW_PROCEDURE";
        case TokenType::KW_PROGRAM:  return "KW_PROGRAM";
        case TokenType::KW_READ:     return "KW_READ";
        case TokenType::KW_THEN:     return "KW_THEN";
        case TokenType::KW_VAR:      return "KW_VAR";
        case TokenType::KW_WHILE:    return "KW_WHILE";
        case TokenType::KW_WRITE:    return "KW_WRITE";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACKET:    return "LBRACKET";
        case TokenType::RBRACKET:    return "RBRACKET";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::COLON:       return "COLON";
        case TokenType::DOT:         return "DOT";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::STAR:        return "STAR";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::LESS:        return "LESS";
        case TokenType::EQUAL:       return "EQUAL";
        case TokenType::GREATER:     return "GREATER";
        case TokenType::NEQ:         return "NEQ";
        case TokenType::ASSIGN:      return "ASSIGN";
        case TokenType::LEQ:         return "LEQ";
        case TokenType::GEQ:         return "GEQ";
        case TokenType::EOF_TOKEN:   return "EOF";
        case TokenType::ERROR:       return "ERROR";
        default:                     return "UNKNOWN";
    }
}