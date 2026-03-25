// ============================================================
// parser.cpp — Full updated parser with TAC generation
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

Parser::Parser(Lexer& lex, SymbolTable& s, ICG& i)
    : lexer(lex), sym(s), icg(i), current(TokenType::EOF_TOKEN, "", 0) {
    advance();
}

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
bool Parser::isRelop() const {
    return check(TokenType::EQUAL) || check(TokenType::NEQ)  ||
           check(TokenType::LESS)  || check(TokenType::LEQ)  ||
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

// ── Program ────────────────────────────────────────────────
void Parser::parse() {
    parseProgram();
    expect(TokenType::EOF_TOKEN, "Expected end of file");
}

void Parser::parseProgram() {
    expect(TokenType::KW_PROGRAM, "Expected 'program'");
    std::string progName = current.lexeme;
    expect(TokenType::IDENTIFIER, "Expected program name");
    expect(TokenType::SEMICOLON,  "Expected ';'");

    if (check(TokenType::KW_VAR))
        parseVarDeclarations();

    while (check(TokenType::KW_PROCEDURE) || check(TokenType::KW_FUNCTION))
        parseSubprogramDeclaration();

    parseCompoundStatement();
    expect(TokenType::DOT, "Expected '.' at end of program");
}

// ── Variable declarations (populate symbol table) ─────────
void Parser::parseVarDeclarations() {
    expect(TokenType::KW_VAR, "Expected 'var'");
    while (check(TokenType::IDENTIFIER))
        parseVarDeclaration();
}

void Parser::parseVarDeclaration() {
    std::vector<std::string> names;
    parseIdentifierList(names);
    expect(TokenType::COLON, "Expected ':'");

    SymbolKind kind = SymbolKind::VARIABLE;
    int arrSize = 0;
    parseType(kind, arrSize);
    expect(TokenType::SEMICOLON, "Expected ';' after declaration");

    // Register all declared names in the symbol table
    for (auto& n : names)
        sym.insert(n, kind, arrSize);
}

void Parser::parseIdentifierList(std::vector<std::string>& names) {
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected identifier"); return;
    }
    names.push_back(current.lexeme);
    advance();
    while (match(TokenType::COMMA)) {
        if (!check(TokenType::IDENTIFIER)) { error("Expected identifier after ','"); break; }
        names.push_back(current.lexeme);
        advance();
    }
}

void Parser::parseType(SymbolKind& kind, int& arrSize) {
    if (check(TokenType::KW_ARRAY)) {
        advance();
        expect(TokenType::LBRACKET, "Expected '['");
        arrSize = std::stoi(current.lexeme.empty() ? "0" : current.lexeme);
        expect(TokenType::NUMBER,   "Expected array size");
        expect(TokenType::RBRACKET, "Expected ']'");
        expect(TokenType::KW_OF,    "Expected 'of'");
        parseStandardType();
        kind = SymbolKind::ARRAY;
    } else {
        parseStandardType();
        kind = SymbolKind::VARIABLE;
    }
}

void Parser::parseStandardType() {
    if (!match(TokenType::KW_INTEGER))
        error("Expected 'integer'");
}

// ── Subprograms ────────────────────────────────────────────
void Parser::parseSubprogramDeclaration() {
    SymbolKind k = check(TokenType::KW_PROCEDURE)
                   ? SymbolKind::PROCEDURE : SymbolKind::FUNCTION;
    advance();
    std::string name = current.lexeme;
    expect(TokenType::IDENTIFIER, "Expected subprogram name");
    sym.insert(name, k);

    parseArguments();

    if (k == SymbolKind::FUNCTION) {
        expect(TokenType::COLON, "Expected ':' after function args");
        parseStandardType();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after subprogram header");

    if (check(TokenType::KW_VAR)) parseVarDeclarations();
    while (check(TokenType::KW_PROCEDURE) || check(TokenType::KW_FUNCTION))
        parseSubprogramDeclaration();

    parseCompoundStatement();
    expect(TokenType::SEMICOLON, "Expected ';' after subprogram body");
}

void Parser::parseArguments() {
    if (!check(TokenType::LPAREN)) return;
    advance();
    if (!check(TokenType::RPAREN)) parseParameterList();
    expect(TokenType::RPAREN, "Expected ')'");
}

void Parser::parseParameterList() {
    parseParameterGroup();
    while (match(TokenType::SEMICOLON)) {
        if (check(TokenType::IDENTIFIER)) parseParameterGroup();
        else break;
    }
}

void Parser::parseParameterGroup() {
    std::vector<std::string> names;
    parseIdentifierList(names);
    expect(TokenType::COLON, "Expected ':' in parameter");
    SymbolKind k = SymbolKind::VARIABLE; int sz = 0;
    parseType(k, sz);
    for (auto& n : names) sym.insert(n, k, sz);
}

// ── Compound statement ─────────────────────────────────────
void Parser::parseCompoundStatement() {
    expect(TokenType::KW_BEGIN, "Expected 'begin'");
    parseStatementList();
    expect(TokenType::KW_END, "Expected 'end'");
}

void Parser::parseStatementList() {
    parseStatement();
    while (match(TokenType::SEMICOLON)) {
        if (check(TokenType::KW_END) || check(TokenType::EOF_TOKEN)) break;
        parseStatement();
    }
}

// ── Statements ─────────────────────────────────────────────
void Parser::parseStatement() {
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current.lexeme; int ln = current.line;
        advance();
        if (check(TokenType::ASSIGN) || check(TokenType::LBRACKET))
            parseAssignStatement(name, ln);
        else if (check(TokenType::LPAREN))
            parseProcCallStatement(name, ln);
        // else: bare identifier — ignore (no-arg proc call)

    } else if (check(TokenType::KW_BEGIN))  { parseCompoundStatement();
    } else if (check(TokenType::KW_IF))     { parseIfStatement();
    } else if (check(TokenType::KW_WHILE))  { parseWhileStatement();
    } else if (check(TokenType::KW_READ))   { parseReadStatement();
    } else if (check(TokenType::KW_WRITE))  { parseWriteStatement();
    } else if (check(TokenType::KW_END)     ||
               check(TokenType::KW_ELSE)    ||
               check(TokenType::SEMICOLON)  ||
               check(TokenType::EOF_TOKEN)) {
        return; // empty statement
    } else {
        error("Unexpected '" + current.lexeme + "' in statement");
        synchronize(STMT_FOLLOW);
    }
}

// id [ expr ] := expr   or   id := expr
void Parser::parseAssignStatement(const std::string& name, int /*ln*/) {
    std::string target = name;

    if (check(TokenType::LBRACKET)) {
        advance();
        std::string idx = parseExpression();
        expect(TokenType::RBRACKET, "Expected ']'");
        target = name + "[" + idx + "]";
    }
    expect(TokenType::ASSIGN, "Expected ':='");
    std::string val = parseExpression();
    icg.emitCopy(val, target);   // target = val
}

// id ( expr_list )
void Parser::parseProcCallStatement(const std::string& name, int /*ln*/) {
    expect(TokenType::LPAREN, "Expected '('");
    int argc = 0;
    if (!check(TokenType::RPAREN))
        parseExpressionList(argc);
    expect(TokenType::RPAREN, "Expected ')'");
    icg.emitCall(name, argc, false);
}

// if expr then stmt [ else stmt ]
void Parser::parseIfStatement() {
    expect(TokenType::KW_IF, "Expected 'if'");
    std::string cond = parseExpression();
    expect(TokenType::KW_THEN, "Expected 'then'");

    // if NOT cond → jump to else/end
    std::string notCond = icg.emitUnary("not", cond);
    std::string labelElse = icg.newLabel();
    std::string labelEnd  = icg.newLabel();
    icg.emitIf(notCond, labelElse);

    parseStatement();    // then-branch

    if (check(TokenType::KW_ELSE)) {
        icg.emitGoto(labelEnd);        // skip else after then
        icg.emitLabel(labelElse);
        advance();                     // consume 'else'
        parseStatement();              // else-branch
        icg.emitLabel(labelEnd);
    } else {
        icg.emitLabel(labelElse);      // no else → labelElse = labelEnd
    }
}

// while expr do stmt
void Parser::parseWhileStatement() {
    expect(TokenType::KW_WHILE, "Expected 'while'");

    std::string labelStart = icg.newLabel();
    std::string labelEnd   = icg.newLabel();
    icg.emitLabel(labelStart);

    std::string cond = parseExpression();
    expect(TokenType::KW_DO, "Expected 'do'");

    std::string notCond = icg.emitUnary("not", cond);
    icg.emitIf(notCond, labelEnd);

    parseStatement();    // loop body

    icg.emitGoto(labelStart);
    icg.emitLabel(labelEnd);
}

// read ( id_list )
void Parser::parseReadStatement() {
    expect(TokenType::KW_READ, "Expected 'read'");
    expect(TokenType::LPAREN,  "Expected '('");
    // parse identifier list and emit read for each
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected identifier in read"); return;
    }
    icg.emitRead(current.lexeme); advance();
    while (match(TokenType::COMMA)) {
        if (!check(TokenType::IDENTIFIER)) { error("Expected identifier"); break; }
        icg.emitRead(current.lexeme); advance();
    }
    expect(TokenType::RPAREN, "Expected ')'");
}

// write ( output_list )
void Parser::parseWriteStatement() {
    expect(TokenType::KW_WRITE, "Expected 'write'");
    expect(TokenType::LPAREN,   "Expected '('");
    parseOutputList();
    expect(TokenType::RPAREN, "Expected ')'");
}

void Parser::parseOutputList() {
    parseOutputItem();
    while (match(TokenType::COMMA)) parseOutputItem();
}

void Parser::parseOutputItem() {
    if (check(TokenType::STRING)) {
        icg.emitWrite("\"" + current.lexeme + "\"");
        advance();
    } else {
        std::string val = parseExpression();
        icg.emitWrite(val);
    }
}

// expression_list: emit param for each, return last result, count in argc
std::string Parser::parseExpressionList(int& argc) {
    std::string last = parseExpression();
    icg.emitParam(last);
    argc = 1;
    while (match(TokenType::COMMA)) {
        last = parseExpression();
        icg.emitParam(last);
        argc++;
    }
    return last;
}

// ── Expressions — return the temp/var holding the value ───

// expression → simple_expression [ relop simple_expression ]
std::string Parser::parseExpression() {
    std::string left = parseSimpleExpression();
    if (isRelop()) {
        std::string op = currentOpStr(); advance();
        std::string right = parseSimpleExpression();
        return icg.emitBinary(op, left, right);
    }
    return left;
}

// simple_expression → [sign] term { addop term }
std::string Parser::parseSimpleExpression() {
    bool negate = false;
    if (check(TokenType::MINUS)) { negate = true; advance(); }
    else if (check(TokenType::PLUS)) { advance(); }

    std::string result = parseTerm();

    if (negate)
        result = icg.emitUnary("-", result);

    while (isAddop()) {
        std::string op = currentOpStr(); advance();
        std::string right = parseTerm();
        result = icg.emitBinary(op, result, right);
    }
    return result;
}

// term → factor { mulop factor }
std::string Parser::parseTerm() {
    std::string result = parseFactor();
    while (isMulop()) {
        std::string op = currentOpStr(); advance();
        std::string right = parseFactor();
        result = icg.emitBinary(op, result, right);
    }
    return result;
}

// factor → id | id[expr] | id(args) | num | string | (expr) | not factor
std::string Parser::parseFactor() {
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current.lexeme; advance();

        if (check(TokenType::LBRACKET)) {
            advance();
            std::string idx = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']'");
            return name + "[" + idx + "]";
        }
        if (check(TokenType::LPAREN)) {
            advance();
            int argc = 0;
            if (!check(TokenType::RPAREN)) parseExpressionList(argc);
            expect(TokenType::RPAREN, "Expected ')'");
            return icg.emitCall(name, argc, true);
        }
        return name;

    } else if (check(TokenType::NUMBER)) {
        std::string val = current.lexeme; advance();
        return val;

    } else if (check(TokenType::STRING)) {
        std::string val = "\"" + current.lexeme + "\""; advance();
        return val;

    } else if (check(TokenType::LPAREN)) {
        advance();
        std::string val = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return val;

    } else if (check(TokenType::KW_NOT)) {
        advance();
        std::string val = parseFactor();
        return icg.emitUnary("not", val);

    } else {
        error("Expected factor — got '" + current.lexeme + "'");
        synchronize(STMT_FOLLOW);
        return "__err__";
    }
}