// ============================================================
// parser.cpp — Recursive descent parser + TAC + AST tree
// ============================================================
#include "parser.h"
#include <iostream>

static const std::unordered_set<TokenType> STMT_FOLLOW = {
    TokenType::SEMICOLON, TokenType::KW_END,
    TokenType::KW_ELSE,   TokenType::EOF_TOKEN
};
static const std::unordered_set<TokenType> DECL_FOLLOW = {
    TokenType::KW_BEGIN, TokenType::KW_PROCEDURE,
    TokenType::KW_FUNCTION, TokenType::EOF_TOKEN
};

// ── Constructor ────────────────────────────────────────────
Parser::Parser(Lexer& lex, SymbolTable& s, ICG& i)
    : lexer(lex), sym(s), icg(i), current(TokenType::EOF_TOKEN, "", 0) {
    advance();
}

// ── Token helpers ──────────────────────────────────────────
void Parser::advance() {
    current = lexer.nextToken();
    while (current.type == TokenType::ERROR)
        current = lexer.nextToken();
}
bool Parser::check(TokenType t) const { return current.type == t; }
bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}
void Parser::expect(TokenType t, const std::string& msg) {
    if (check(t)) advance();
    else error(msg + " — got '" + current.lexeme + "'");
}
void Parser::error(const std::string& msg) {
    errors.push_back("Line " + std::to_string(current.line) + ": " + msg);
}
void Parser::synchronize(const std::unordered_set<TokenType>& follow) {
    while (!check(TokenType::EOF_TOKEN))
        if (follow.count(current.type)) return;
        else advance();
}

// ── Operator predicates ────────────────────────────────────
bool Parser::isRelop() const {
    return check(TokenType::EQUAL)   || check(TokenType::NEQ)  ||
           check(TokenType::LESS)    || check(TokenType::LEQ)  ||
           check(TokenType::GREATER) || check(TokenType::GEQ);
}
bool Parser::isAddop() const {
    return check(TokenType::PLUS) || check(TokenType::MINUS) ||
           check(TokenType::KW_OR);
}
bool Parser::isMulop() const {
    return check(TokenType::STAR) || check(TokenType::SLASH) ||
           check(TokenType::KW_AND);
}
std::string Parser::currentOpStr() const {
    switch (current.type) {
        case TokenType::PLUS:    return "+";
        case TokenType::MINUS:   return "-";
        case TokenType::STAR:    return "*";
        case TokenType::SLASH:   return "/";
        case TokenType::KW_AND:  return "and";
        case TokenType::KW_OR:   return "or";
        case TokenType::EQUAL:   return "==";
        case TokenType::NEQ:     return "<>";
        case TokenType::LESS:    return "<";
        case TokenType::LEQ:     return "<=";
        case TokenType::GREATER: return ">";
        case TokenType::GEQ:     return ">=";
        default:                 return current.lexeme;
    }
}

// ── Entry point ────────────────────────────────────────────
void Parser::parse() {
    treeRoot = std::make_shared<ASTNode>("program", "rule");
    parseProgram(treeRoot);
    expect(TokenType::EOF_TOKEN, "Expected end of file");
}

// ── Program ────────────────────────────────────────────────
// program → program id ; [var_decls] [subprograms] compound_stmt .
void Parser::parseProgram(ASTNodePtr parent) {
    auto node = parent->addChild("program-decl");

    node->addChild(makeLeaf(current));
    expect(TokenType::KW_PROGRAM, "Expected 'program'");

    node->addChild(makeLeaf(current));
    expect(TokenType::IDENTIFIER, "Expected program name");

    node->addChild(makeLeaf(current));
    expect(TokenType::SEMICOLON, "Expected ';'");

    if (check(TokenType::KW_VAR))
        parseVarDeclarations(node);

    while (check(TokenType::KW_PROCEDURE) || check(TokenType::KW_FUNCTION))
        parseSubprogramDeclaration(node);

    parseCompoundStatement(node);

    node->addChild(makeLeaf(current));
    expect(TokenType::DOT, "Expected '.' at end of program");
}

// ── Variable declarations ──────────────────────────────────
void Parser::parseVarDeclarations(ASTNodePtr parent) {
    auto node = parent->addChild("var-declarations");
    node->addChild(makeLeaf(current));
    expect(TokenType::KW_VAR, "Expected 'var'");
    while (check(TokenType::IDENTIFIER))
        parseVarDeclaration(node);
}

void Parser::parseVarDeclaration(ASTNodePtr parent) {
    auto node = parent->addChild("var-decl");
    std::vector<std::string> names;
    parseIdentifierList(node, names);
    node->addChild(makeLeaf(current));
    expect(TokenType::COLON, "Expected ':'");
    SymbolKind kind = SymbolKind::VARIABLE; int arrSize = 0;
    parseType(node, kind, arrSize);
    node->addChild(makeLeaf(current));
    expect(TokenType::SEMICOLON, "Expected ';' after declaration");
    for (auto& n : names) sym.insert(n, kind, arrSize);
}

void Parser::parseIdentifierList(ASTNodePtr parent, std::vector<std::string>& names) {
    auto node = parent->addChild("id-list");
    if (!check(TokenType::IDENTIFIER)) { error("Expected identifier"); return; }
    names.push_back(current.lexeme);
    node->addChild(makeLeaf(current)); advance();
    while (check(TokenType::COMMA)) {
        node->addChild(makeLeaf(current)); advance(); // comma
        if (!check(TokenType::IDENTIFIER)) { error("Expected identifier after ','"); break; }
        names.push_back(current.lexeme);
        node->addChild(makeLeaf(current)); advance();
    }
}

void Parser::parseType(ASTNodePtr parent, SymbolKind& kind, int& arrSize) {
    auto node = parent->addChild("type");
    if (check(TokenType::KW_ARRAY)) {
        node->addChild(makeLeaf(current)); advance();
        node->addChild(makeLeaf(current)); expect(TokenType::LBRACKET, "Expected '['");
        arrSize = std::stoi(current.lexeme.empty() ? "0" : current.lexeme);
        node->addChild(makeLeaf(current)); expect(TokenType::NUMBER, "Expected array size");
        node->addChild(makeLeaf(current)); expect(TokenType::RBRACKET, "Expected ']'");
        node->addChild(makeLeaf(current)); expect(TokenType::KW_OF, "Expected 'of'");
        parseStandardType(node);
        kind = SymbolKind::ARRAY;
    } else {
        parseStandardType(node);
        kind = SymbolKind::VARIABLE;
    }
}

void Parser::parseStandardType(ASTNodePtr parent) {
    auto node = parent->addChild("standard-type");
    if (!check(TokenType::KW_INTEGER)) { error("Expected 'integer'"); return; }
    node->addChild(makeLeaf(current)); advance();
}

// ── Subprogram declarations ────────────────────────────────
void Parser::parseSubprogramDeclaration(ASTNodePtr parent) {
    auto node = parent->addChild("subprogram-decl");
    SymbolKind k = check(TokenType::KW_PROCEDURE) ? SymbolKind::PROCEDURE : SymbolKind::FUNCTION;
    node->addChild(makeLeaf(current)); advance();
    std::string name = current.lexeme;
    node->addChild(makeLeaf(current)); expect(TokenType::IDENTIFIER, "Expected subprogram name");
    sym.insert(name, k);
    parseArguments(node);
    if (k == SymbolKind::FUNCTION) {
        node->addChild(makeLeaf(current)); expect(TokenType::COLON, "Expected ':'");
        parseStandardType(node);
    }
    node->addChild(makeLeaf(current)); expect(TokenType::SEMICOLON, "Expected ';' after header");
    if (check(TokenType::KW_VAR)) parseVarDeclarations(node);
    while (check(TokenType::KW_PROCEDURE) || check(TokenType::KW_FUNCTION))
        parseSubprogramDeclaration(node);
    parseCompoundStatement(node);
    node->addChild(makeLeaf(current)); expect(TokenType::SEMICOLON, "Expected ';' after body");
}

void Parser::parseArguments(ASTNodePtr parent) {
    if (!check(TokenType::LPAREN)) return;
    auto node = parent->addChild("arguments");
    node->addChild(makeLeaf(current)); advance();
    if (!check(TokenType::RPAREN)) parseParameterList(node);
    node->addChild(makeLeaf(current)); expect(TokenType::RPAREN, "Expected ')'");
}

void Parser::parseParameterList(ASTNodePtr parent) {
    auto node = parent->addChild("param-list");
    parseParameterGroup(node);
    while (check(TokenType::SEMICOLON)) {
        node->addChild(makeLeaf(current)); advance();
        if (check(TokenType::IDENTIFIER)) parseParameterGroup(node);
        else break;
    }
}

void Parser::parseParameterGroup(ASTNodePtr parent) {
    auto node = parent->addChild("param-group");
    std::vector<std::string> names;
    parseIdentifierList(node, names);
    node->addChild(makeLeaf(current)); expect(TokenType::COLON, "Expected ':'");
    SymbolKind k = SymbolKind::VARIABLE; int sz = 0;
    parseType(node, k, sz);
    for (auto& n : names) sym.insert(n, k, sz);
}

// ── Compound statement ─────────────────────────────────────
void Parser::parseCompoundStatement(ASTNodePtr parent) {
    auto node = parent->addChild("compound-stmt");
    node->addChild(makeLeaf(current)); expect(TokenType::KW_BEGIN, "Expected 'begin'");
    parseStatementList(node);
    node->addChild(makeLeaf(current)); expect(TokenType::KW_END, "Expected 'end'");
}

void Parser::parseStatementList(ASTNodePtr parent) {
    auto node = parent->addChild("stmt-list");
    parseStatement(node);
    while (check(TokenType::SEMICOLON)) {
        node->addChild(makeLeaf(current)); advance();
        if (check(TokenType::KW_END) || check(TokenType::EOF_TOKEN)) break;
        parseStatement(node);
    }
}

// ── Statements ─────────────────────────────────────────────
void Parser::parseStatement(ASTNodePtr parent) {
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current.lexeme; int ln = current.line;
        advance();
        if (check(TokenType::ASSIGN) || check(TokenType::LBRACKET))
            parseAssignStatement(parent, name, ln);
        else if (check(TokenType::LPAREN))
            parseProcCallStatement(parent, name, ln);
        // else: bare identifier (no-arg proc call) — skip silently

    } else if (check(TokenType::KW_BEGIN))  { parseCompoundStatement(parent);
    } else if (check(TokenType::KW_IF))     { parseIfStatement(parent);
    } else if (check(TokenType::KW_WHILE))  { parseWhileStatement(parent);
    } else if (check(TokenType::KW_READ))   { parseReadStatement(parent);
    } else if (check(TokenType::KW_WRITE))  { parseWriteStatement(parent);
    } else if (check(TokenType::KW_END)    ||
               check(TokenType::KW_ELSE)   ||
               check(TokenType::SEMICOLON) ||
               check(TokenType::EOF_TOKEN)) {
        return; // empty statement — valid
    } else {
        auto enode = parent->addChild("error: " + current.lexeme, "error");
        (void)enode;
        error("Unexpected '" + current.lexeme + "' in statement");
        synchronize(STMT_FOLLOW);
    }
}

// id [ expr ] := expr   or   id := expr
void Parser::parseAssignStatement(ASTNodePtr parent, const std::string& name, int /*ln*/) {
    auto node = parent->addChild("assign-stmt");
    node->addChild(makeLeaf(name + "\n[IDENTIFIER]"));
    std::string target = name;
    if (check(TokenType::LBRACKET)) {
        node->addChild(makeLeaf(current)); advance();
        std::string idx = parseExpression(node);
        node->addChild(makeLeaf(current)); expect(TokenType::RBRACKET, "Expected ']'");
        target = name + "[" + idx + "]";
    }
    node->addChild(makeLeaf(current)); expect(TokenType::ASSIGN, "Expected ':='");
    std::string val = parseExpression(node);
    icg.emitCopy(val, target);
}

// id ( expr_list )
void Parser::parseProcCallStatement(ASTNodePtr parent, const std::string& name, int /*ln*/) {
    auto node = parent->addChild("proc-call");
    node->addChild(makeLeaf(name + "\n[IDENTIFIER]"));
    node->addChild(makeLeaf(current)); expect(TokenType::LPAREN, "Expected '('");
    int argc = 0;
    if (!check(TokenType::RPAREN)) parseExpressionList(node, argc);
    node->addChild(makeLeaf(current)); expect(TokenType::RPAREN, "Expected ')'");
    icg.emitCall(name, argc, false);
}

// if expr then stmt [ else stmt ]
void Parser::parseIfStatement(ASTNodePtr parent) {
    auto node = parent->addChild("if-stmt");
    node->addChild(makeLeaf(current)); expect(TokenType::KW_IF, "Expected 'if'");
    std::string cond = parseExpression(node);
    node->addChild(makeLeaf(current)); expect(TokenType::KW_THEN, "Expected 'then'");

    std::string notCond   = icg.emitUnary("not", cond);
    std::string labelElse = icg.newLabel();
    std::string labelEnd  = icg.newLabel();
    icg.emitIf(notCond, labelElse);

    auto thenNode = node->addChild("then-branch");
    parseStatement(thenNode);

    if (check(TokenType::KW_ELSE)) {
        icg.emitGoto(labelEnd);
        icg.emitLabel(labelElse);
        node->addChild(makeLeaf(current)); advance(); // consume 'else'
        auto elseNode = node->addChild("else-branch");
        parseStatement(elseNode);
        icg.emitLabel(labelEnd);
    } else {
        icg.emitLabel(labelElse);
    }
}

// while expr do stmt
void Parser::parseWhileStatement(ASTNodePtr parent) {
    auto node = parent->addChild("while-stmt");
    node->addChild(makeLeaf(current)); expect(TokenType::KW_WHILE, "Expected 'while'");

    std::string labelStart = icg.newLabel();
    std::string labelEnd   = icg.newLabel();
    icg.emitLabel(labelStart);

    std::string cond = parseExpression(node);
    node->addChild(makeLeaf(current)); expect(TokenType::KW_DO, "Expected 'do'");

    std::string notCond = icg.emitUnary("not", cond);
    icg.emitIf(notCond, labelEnd);

    auto bodyNode = node->addChild("loop-body");
    parseStatement(bodyNode);

    icg.emitGoto(labelStart);
    icg.emitLabel(labelEnd);
}

// read ( id_list )
void Parser::parseReadStatement(ASTNodePtr parent) {
    auto node = parent->addChild("read-stmt");
    node->addChild(makeLeaf(current)); expect(TokenType::KW_READ, "Expected 'read'");
    node->addChild(makeLeaf(current)); expect(TokenType::LPAREN, "Expected '('");
    if (!check(TokenType::IDENTIFIER)) { error("Expected identifier in read"); return; }
    icg.emitRead(current.lexeme);
    node->addChild(makeLeaf(current)); advance();
    while (check(TokenType::COMMA)) {
        node->addChild(makeLeaf(current)); advance();
        if (!check(TokenType::IDENTIFIER)) { error("Expected identifier"); break; }
        icg.emitRead(current.lexeme);
        node->addChild(makeLeaf(current)); advance();
    }
    node->addChild(makeLeaf(current)); expect(TokenType::RPAREN, "Expected ')'");
}

// write ( output_list )
void Parser::parseWriteStatement(ASTNodePtr parent) {
    auto node = parent->addChild("write-stmt");
    node->addChild(makeLeaf(current)); expect(TokenType::KW_WRITE, "Expected 'write'");
    node->addChild(makeLeaf(current)); expect(TokenType::LPAREN, "Expected '('");
    parseOutputList(node);
    node->addChild(makeLeaf(current)); expect(TokenType::RPAREN, "Expected ')'");
}

void Parser::parseOutputList(ASTNodePtr parent) {
    auto node = parent->addChild("output-list");
    parseOutputItem(node);
    while (check(TokenType::COMMA)) {
        node->addChild(makeLeaf(current)); advance();
        parseOutputItem(node);
    }
}

void Parser::parseOutputItem(ASTNodePtr parent) {
    if (check(TokenType::STRING)) {
        icg.emitWrite("\"" + current.lexeme + "\"");
        parent->addChild(makeLeaf(current)); advance();
    } else {
        std::string val = parseExpression(parent);
        icg.emitWrite(val);
    }
}

// expression_list → expr { , expr }  — emits params, counts args
std::string Parser::parseExpressionList(ASTNodePtr parent, int& argc) {
    auto node = parent->addChild("expr-list");
    std::string last = parseExpression(node);
    icg.emitParam(last); argc = 1;
    while (check(TokenType::COMMA)) {
        node->addChild(makeLeaf(current)); advance();
        last = parseExpression(node);
        icg.emitParam(last); argc++;
    }
    return last;
}

// ── Expressions ────────────────────────────────────────────

// expression → simple_expression [ relop simple_expression ]
std::string Parser::parseExpression(ASTNodePtr parent) {
    auto node = parent->addChild("expr");
    std::string left = parseSimpleExpression(node);
    if (isRelop()) {
        std::string op = currentOpStr();
        node->addChild(makeLeaf(current)); advance();
        std::string right = parseSimpleExpression(node);
        return icg.emitBinary(op, left, right);
    }
    return left;
}

// simple_expression → [sign] term { addop term }
std::string Parser::parseSimpleExpression(ASTNodePtr parent) {
    auto node = parent->addChild("simple-expr");
    bool negate = false;
    if (check(TokenType::MINUS)) {
        negate = true;
        node->addChild(makeLeaf(current)); advance();
    } else if (check(TokenType::PLUS)) {
        node->addChild(makeLeaf(current)); advance();
    }
    std::string result = parseTerm(node);
    if (negate) result = icg.emitUnary("-", result);
    while (isAddop()) {
        std::string op = currentOpStr();
        node->addChild(makeLeaf(current)); advance();
        std::string right = parseTerm(node);
        result = icg.emitBinary(op, result, right);
    }
    return result;
}

// term → factor { mulop factor }
std::string Parser::parseTerm(ASTNodePtr parent) {
    auto node = parent->addChild("term");
    std::string result = parseFactor(node);
    while (isMulop()) {
        std::string op = currentOpStr();
        node->addChild(makeLeaf(current)); advance();
        std::string right = parseFactor(node);
        result = icg.emitBinary(op, result, right);
    }
    return result;
}

// factor → id | id[expr] | id(args) | num | string | (expr) | not factor
std::string Parser::parseFactor(ASTNodePtr parent) {
    auto node = parent->addChild("factor");

    if (check(TokenType::IDENTIFIER)) {
        std::string name = current.lexeme;
        node->addChild(makeLeaf(current)); advance();
        if (check(TokenType::LBRACKET)) {
            node->addChild(makeLeaf(current)); advance();
            std::string idx = parseExpression(node);
            node->addChild(makeLeaf(current)); expect(TokenType::RBRACKET, "Expected ']'");
            return name + "[" + idx + "]";
        }
        if (check(TokenType::LPAREN)) {
            node->addChild(makeLeaf(current)); advance();
            int argc = 0;
            if (!check(TokenType::RPAREN)) parseExpressionList(node, argc);
            node->addChild(makeLeaf(current)); expect(TokenType::RPAREN, "Expected ')'");
            return icg.emitCall(name, argc, true);
        }
        return name;

    } else if (check(TokenType::NUMBER)) {
        std::string val = current.lexeme;
        node->addChild(makeLeaf(current)); advance();
        return val;

    } else if (check(TokenType::STRING)) {
        std::string val = "\"" + current.lexeme + "\"";
        node->addChild(makeLeaf(current)); advance();
        return val;

    } else if (check(TokenType::LPAREN)) {
        node->addChild(makeLeaf(current)); advance();
        std::string val = parseExpression(node);
        node->addChild(makeLeaf(current)); expect(TokenType::RPAREN, "Expected ')'");
        return val;

    } else if (check(TokenType::KW_NOT)) {
        node->addChild(makeLeaf(current)); advance();
        std::string val = parseFactor(node);
        return icg.emitUnary("not", val);

    } else {
        node->addChild(std::make_shared<ASTNode>("error: " + current.lexeme, "error"));
        error("Expected factor — got '" + current.lexeme + "'");
        synchronize(STMT_FOLLOW);
        return "__err__";
    }
}