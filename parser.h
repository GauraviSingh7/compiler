// ============================================================
// parser.h — Parser with ICG + SymbolTable + AST Tree
// ============================================================
#pragma once
#include "lexer.h"
#include "token_printer.h"
#include "symtable.h"
#include "icg.h"
#include "ast.h"
#include <vector>
#include <string>
#include <unordered_set>

class Parser {
public:
    Parser(Lexer& lexer, SymbolTable& sym, ICG& icg);

    void parse();

    std::vector<std::string> errors;
    bool       hasError()  const { return !errors.empty(); }
    ASTNodePtr getTree()   const { return treeRoot; }

private:
    Lexer&       lexer;
    SymbolTable& sym;
    ICG&         icg;
    Token        current;
    ASTNodePtr   treeRoot;

    // ── Token helpers ──────────────────────────────────────
    void advance();
    bool check(TokenType t) const;
    bool match(TokenType t);
    void expect(TokenType t, const std::string& msg);
    void error(const std::string& msg);
    void synchronize(const std::unordered_set<TokenType>& follow);

    // ── AST leaf factories ─────────────────────────────────
    ASTNodePtr makeLeaf(const std::string& label) {
        return std::make_shared<ASTNode>(label, "token");
    }
    ASTNodePtr makeLeaf(const Token& tok) {
        return std::make_shared<ASTNode>(
            tok.lexeme + "\n[" + tokenTypeName(tok.type) + "]", "token");
    }

    // ── Grammar rules (each takes its parent AST node) ────
    void parseProgram            (ASTNodePtr parent);
    void parseVarDeclarations    (ASTNodePtr parent);
    void parseVarDeclaration     (ASTNodePtr parent);
    void parseIdentifierList     (ASTNodePtr parent, std::vector<std::string>& names);
    void parseType               (ASTNodePtr parent, SymbolKind& kind, int& arrSize);
    void parseStandardType       (ASTNodePtr parent);

    void parseSubprogramDeclaration(ASTNodePtr parent);
    void parseArguments          (ASTNodePtr parent);
    void parseParameterList      (ASTNodePtr parent);
    void parseParameterGroup     (ASTNodePtr parent);

    void parseCompoundStatement  (ASTNodePtr parent);
    void parseStatementList      (ASTNodePtr parent);
    void parseStatement          (ASTNodePtr parent);
    void parseAssignStatement    (ASTNodePtr parent, const std::string& name, int ln);
    void parseProcCallStatement  (ASTNodePtr parent, const std::string& name, int ln);
    void parseIfStatement        (ASTNodePtr parent);
    void parseWhileStatement     (ASTNodePtr parent);
    void parseReadStatement      (ASTNodePtr parent);
    void parseWriteStatement     (ASTNodePtr parent);
    void parseOutputList         (ASTNodePtr parent);
    void parseOutputItem         (ASTNodePtr parent);

    // Expression rules return the temp/var holding the result
    std::string parseExpression      (ASTNodePtr parent);
    std::string parseSimpleExpression(ASTNodePtr parent);
    std::string parseTerm            (ASTNodePtr parent);
    std::string parseFactor          (ASTNodePtr parent);
    std::string parseExpressionList  (ASTNodePtr parent, int& argc);

    // ── Operator helpers ───────────────────────────────────
    bool isRelop() const;
    bool isAddop() const;
    bool isMulop() const;
    std::string currentOpStr() const;
};