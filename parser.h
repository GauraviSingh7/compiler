// ============================================================
// parser.h — Parser declaration
// ============================================================
#pragma once
#include "lexer.h"
#include <vector>
#include <string>
#include <unordered_set>

class Parser {
public:
    explicit Parser(Lexer& lexer);

    // Entry point — call this to parse the whole program
    void parse();

    std::vector<std::string> errors;   // all parse errors collected
    bool hasError() const { return !errors.empty(); }

private:
    Lexer&  lexer;
    Token   current;   // current lookahead token

    // ── Token consumption ──────────────────────────────────
    void advance();
    bool check(TokenType t) const;
    bool match(TokenType t);           // consume if match, else false
    void expect(TokenType t, const std::string& msg);  // consume or error

    // ── Error handling ─────────────────────────────────────
    void error(const std::string& msg);
    void synchronize(const std::unordered_set<TokenType>& follow);

    // ── Grammar rule functions ─────────────────────────────
    void parseProgram();
    void parseVarDeclarations();       // optional var block
    void parseVarDeclaration();        // single var line: id_list : type ;
    void parseIdentifierList();        // id { , id }
    void parseType();
    void parseStandardType();

    void parseSubprogramDeclarations();
    void parseSubprogramDeclaration();
    void parseProcedureOrFunction(TokenType kind);
    void parseArguments();
    void parseParameterList();
    void parseParameterGroup();

    void parseCompoundStatement();
    void parseStatementList();
    void parseStatement();
    void parseAssignmentStatement(const std::string& idName, int idLine);
    void parseIfStatement();
    void parseWhileStatement();
    void parseReadStatement();
    void parseWriteStatement();
    void parseProcedureCallStatement(const std::string& idName, int idLine);

    void parseExpression();
    void parseSimpleExpression();
    void parseTerm();
    void parseFactor();

    void parseOutputList();
    void parseOutputItem();
    void parseExpressionList();

    bool isRelop() const;
    bool isAddop() const;
    bool isMulop() const;
};