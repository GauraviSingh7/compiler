// ============================================================
// parser.h — Updated with ICG + SymbolTable integration
// ============================================================
#pragma once
#include "lexer.h"
#include "symtable.h"
#include "icg.h"
#include <vector>
#include <string>
#include <unordered_set>

class Parser {
public:
    Parser(Lexer& lexer, SymbolTable& sym, ICG& icg);

    void parse();

    std::vector<std::string> errors;
    bool hasError() const { return !errors.empty(); }

private:
    Lexer&       lexer;
    SymbolTable& sym;
    ICG&         icg;
    Token        current;

    void advance();
    bool check(TokenType t) const;
    bool match(TokenType t);
    void expect(TokenType t, const std::string& msg);
    void error(const std::string& msg);
    void synchronize(const std::unordered_set<TokenType>& follow);

    void        parseProgram();
    void        parseVarDeclarations();
    void        parseVarDeclaration();
    void        parseIdentifierList(std::vector<std::string>& names);
    void        parseType(SymbolKind& kind, int& arrSize);
    void        parseStandardType();

    void        parseSubprogramDeclaration();
    void        parseArguments();
    void        parseParameterList();
    void        parseParameterGroup();

    void        parseCompoundStatement();
    void        parseStatementList();
    void        parseStatement();
    void        parseAssignStatement(const std::string& name, int ln);
    void        parseProcCallStatement(const std::string& name, int ln);
    void        parseIfStatement();
    void        parseWhileStatement();
    void        parseReadStatement();
    void        parseWriteStatement();

    // Expression functions now RETURN the temp/var holding the result
    std::string parseExpression();
    std::string parseSimpleExpression();
    std::string parseTerm();
    std::string parseFactor();

    void        parseOutputList();
    void        parseOutputItem();
    std::string parseExpressionList(int& argCount);  // returns last, counts args

    bool isRelop() const;
    bool isAddop() const;
    bool isMulop() const;
    std::string currentOpStr() const;  // token → operator string for TAC
};