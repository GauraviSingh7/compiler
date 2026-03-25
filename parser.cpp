// ============================================================
// parser.cpp — Recursive descent parser implementation
// ============================================================
#include "parser.h"
#include <iostream>
#include <stdexcept>

// ── Sync sets used for error recovery ──────────────────────
static const std::unordered_set<TokenType> STMT_FOLLOW = {
    TokenType::SEMICOLON, TokenType::KW_END,
    TokenType::KW_ELSE,   TokenType::EOF_TOKEN
};

static const std::unordered_set<TokenType> DECL_FOLLOW = {
    TokenType::KW_BEGIN,  TokenType::KW_PROCEDURE,
    TokenType::KW_FUNCTION, TokenType::EOF_TOKEN
};

// ── Constructor: prime the lookahead ──────────────────────
Parser::Parser(Lexer& lex) : lexer(lex), current(TokenType::EOF_TOKEN, "", 0) {
    advance();   // load first token
}

// ── Token helpers ──────────────────────────────────────────
void Parser::advance() {
    current = lexer.nextToken();
    // Skip ERROR tokens from lexer, they were already reported
    while (current.type == TokenType::ERROR)
        current = lexer.nextToken();
}

bool Parser::check(TokenType t) const {
    return current.type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

// Consume expected token or record error (do NOT throw/stop)
void Parser::expect(TokenType t, const std::string& msg) {
    if (check(t)) {
        advance();
    } else {
        error(msg + " — got '" + current.lexeme + "'");
    }
}

// ── Error recording & recovery ─────────────────────────────
void Parser::error(const std::string& msg) {
    std::string full = "Line " + std::to_string(current.line) + ": " + msg;
    errors.push_back(full);
}

// Panic-mode recovery: skip tokens until one in 'follow' set
void Parser::synchronize(const std::unordered_set<TokenType>& follow) {
    while (!check(TokenType::EOF_TOKEN)) {
        if (follow.count(current.type)) return;
        advance();
    }
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

// ==============================================================
//  GRAMMAR RULES
// ==============================================================

// Entry point
// program → program id ; var_decls subprogram_decls compound_statement .
void Parser::parse() {
    parseProgram();
    expect(TokenType::EOF_TOKEN, "Expected end of file after '.'");
}

// program → program id ;
void Parser::parseProgram() {
    expect(TokenType::KW_PROGRAM, "Expected 'program'");
    expect(TokenType::IDENTIFIER, "Expected program name");
    expect(TokenType::SEMICOLON,  "Expected ';' after program name");

    // Optional var declarations
    if (check(TokenType::KW_VAR))
        parseVarDeclarations();

    // Optional subprogram (procedure/function) declarations
    while (check(TokenType::KW_PROCEDURE) || check(TokenType::KW_FUNCTION))
        parseSubprogramDeclaration();

    // Main body
    parseCompoundStatement();

    expect(TokenType::DOT, "Expected '.' at end of program");
}

// ── Variable Declarations ──────────────────────────────────
// var_declarations → var id_list : type ; { id_list : type ; }
void Parser::parseVarDeclarations() {
    expect(TokenType::KW_VAR, "Expected 'var'");

    // At least one declaration required after 'var'
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected variable name after 'var'");
        synchronize(DECL_FOLLOW);
        return;
    }

    while (check(TokenType::IDENTIFIER)) {
        parseVarDeclaration();
    }
}

// id_list : type ;
void Parser::parseVarDeclaration() {
    parseIdentifierList();
    expect(TokenType::COLON,    "Expected ':' after identifier list");
    parseType();
    expect(TokenType::SEMICOLON,"Expected ';' after variable declaration");
}

// id_list → id { , id }
void Parser::parseIdentifierList() {
    expect(TokenType::IDENTIFIER, "Expected identifier");
    while (match(TokenType::COMMA)) {
        expect(TokenType::IDENTIFIER, "Expected identifier after ','");
    }
}

// type → standard_type | array [ num ] of standard_type
void Parser::parseType() {
    if (check(TokenType::KW_ARRAY)) {
        advance();
        expect(TokenType::LBRACKET, "Expected '[' after 'array'");
        expect(TokenType::NUMBER,   "Expected array size (integer)");
        expect(TokenType::RBRACKET, "Expected ']' after array size");
        expect(TokenType::KW_OF,    "Expected 'of' after ']'");
        parseStandardType();
    } else {
        parseStandardType();
    }
}

// standard_type → integer
void Parser::parseStandardType() {
    if (!match(TokenType::KW_INTEGER)) {
        error("Expected type 'integer'");
    }
}

// ── Subprogram Declarations ────────────────────────────────
// subprogram_declaration → (procedure | function) id arguments ; body
void Parser::parseSubprogramDeclaration() {
    if (check(TokenType::KW_PROCEDURE)) {
        advance();
        expect(TokenType::IDENTIFIER, "Expected procedure name");
        parseArguments();
        expect(TokenType::SEMICOLON, "Expected ';' after procedure header");
    } else if (check(TokenType::KW_FUNCTION)) {
        advance();
        expect(TokenType::IDENTIFIER, "Expected function name");
        parseArguments();
        expect(TokenType::COLON,     "Expected ':' after function arguments");
        parseStandardType();
        expect(TokenType::SEMICOLON, "Expected ';' after function header");
    }

    // Local var declarations
    if (check(TokenType::KW_VAR))
        parseVarDeclarations();

    // Nested subprograms
    while (check(TokenType::KW_PROCEDURE) || check(TokenType::KW_FUNCTION))
        parseSubprogramDeclaration();

    parseCompoundStatement();
    expect(TokenType::SEMICOLON, "Expected ';' after subprogram body");
}

// arguments → ( parameter_list ) | ( )
void Parser::parseArguments() {
    if (!check(TokenType::LPAREN)) return;  // arguments are optional
    advance();

    if (!check(TokenType::RPAREN))
        parseParameterList();

    expect(TokenType::RPAREN, "Expected ')' after parameter list");
}

// parameter_list → id_list : type { ; id_list : type }
void Parser::parseParameterList() {
    parseParameterGroup();
    while (match(TokenType::SEMICOLON)) {
        if (check(TokenType::IDENTIFIER))
            parseParameterGroup();
        else
            break;
    }
}

void Parser::parseParameterGroup() {
    parseIdentifierList();
    expect(TokenType::COLON, "Expected ':' in parameter declaration");
    parseType();
}

// ── Statements ─────────────────────────────────────────────
// compound_statement → begin statement_list end
void Parser::parseCompoundStatement() {
    expect(TokenType::KW_BEGIN, "Expected 'begin'");
    parseStatementList();
    expect(TokenType::KW_END, "Expected 'end'");
}

// statement_list → statement { ; statement }
void Parser::parseStatementList() {
    parseStatement();
    while (match(TokenType::SEMICOLON)) {
        // After a semicolon, another statement may follow
        // But 'end' means the block is closing — stop
        if (check(TokenType::KW_END) || check(TokenType::EOF_TOKEN)) break;
        parseStatement();
    }
}

// statement → assignment | compound | if | while | read | write | proc_call
void Parser::parseStatement() {
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current.lexeme;
        int ln = current.line;
        advance();

        if (check(TokenType::ASSIGN) || check(TokenType::LBRACKET)) {
            // Assignment: x := expr  or  x[expr] := expr
            parseAssignmentStatement(name, ln);
        } else if (check(TokenType::LPAREN)) {
            // Procedure call: id ( ... )
            parseProcedureCallStatement(name, ln);
        } else {
            // Could be a standalone identifier (e.g. a no-arg procedure call)
            // Accept it silently — some grammars allow id alone as proc call
        }

    } else if (check(TokenType::KW_BEGIN)) {
        parseCompoundStatement();

    } else if (check(TokenType::KW_IF)) {
        parseIfStatement();

    } else if (check(TokenType::KW_WHILE)) {
        parseWhileStatement();

    } else if (check(TokenType::KW_READ)) {
        parseReadStatement();

    } else if (check(TokenType::KW_WRITE)) {
        parseWriteStatement();

    } else if (check(TokenType::KW_END)   ||
               check(TokenType::KW_ELSE)  ||
               check(TokenType::SEMICOLON)||
               check(TokenType::EOF_TOKEN)) {
        // Empty statement — valid in some contexts, just return
        return;

    } else {
        error("Unexpected token '" + current.lexeme + "' in statement");
        synchronize(STMT_FOLLOW);
    }
}

// variable := expression
// variable can be: id  or  id [ expression ]
void Parser::parseAssignmentStatement(const std::string& /*name*/, int /*ln*/) {
    // If array subscript: already consumed id above
    if (check(TokenType::LBRACKET)) {
        advance();
        parseExpression();
        expect(TokenType::RBRACKET, "Expected ']' in array subscript");
    }
    expect(TokenType::ASSIGN, "Expected ':=' in assignment");
    parseExpression();
}

// id ( expression_list )
void Parser::parseProcedureCallStatement(const std::string& /*name*/, int /*ln*/) {
    expect(TokenType::LPAREN, "Expected '(' in procedure call");
    if (!check(TokenType::RPAREN))
        parseExpressionList();
    expect(TokenType::RPAREN, "Expected ')' after procedure arguments");
}

// if expression then statement [ else statement ]
void Parser::parseIfStatement() {
    expect(TokenType::KW_IF,   "Expected 'if'");
    parseExpression();
    expect(TokenType::KW_THEN, "Expected 'then'");
    parseStatement();

    if (match(TokenType::KW_ELSE)) {
        parseStatement();
    }
}

// while expression do statement
void Parser::parseWhileStatement() {
    expect(TokenType::KW_WHILE, "Expected 'while'");
    parseExpression();
    expect(TokenType::KW_DO,    "Expected 'do'");
    parseStatement();
}

// read ( identifier_list )
void Parser::parseReadStatement() {
    expect(TokenType::KW_READ,  "Expected 'read'");
    expect(TokenType::LPAREN,   "Expected '(' after 'read'");
    parseIdentifierList();
    expect(TokenType::RPAREN,   "Expected ')' after read list");
}

// write ( output_list )
void Parser::parseWriteStatement() {
    expect(TokenType::KW_WRITE, "Expected 'write'");
    expect(TokenType::LPAREN,   "Expected '(' after 'write'");
    parseOutputList();
    expect(TokenType::RPAREN,   "Expected ')' after write list");
}

// output_list → output_item { , output_item }
void Parser::parseOutputList() {
    parseOutputItem();
    while (match(TokenType::COMMA))
        parseOutputItem();
}

// output_item → string | expression
void Parser::parseOutputItem() {
    if (check(TokenType::STRING))
        advance();
    else
        parseExpression();
}

// expression_list → expression { , expression }
void Parser::parseExpressionList() {
    parseExpression();
    while (match(TokenType::COMMA))
        parseExpression();
}

// ── Expressions ────────────────────────────────────────────
// expression → simple_expression [ relop simple_expression ]
void Parser::parseExpression() {
    parseSimpleExpression();
    if (isRelop()) {
        advance();   // consume relop
        parseSimpleExpression();
    }
}

// simple_expression → term { addop term }
// addop: + | - | or
void Parser::parseSimpleExpression() {
    // Optional leading sign
    if (check(TokenType::PLUS) || check(TokenType::MINUS))
        advance();

    parseTerm();

    while (isAddop()) {
        advance();  // consume addop
        parseTerm();
    }
}

// term → factor { mulop factor }
// mulop: * | / | and
void Parser::parseTerm() {
    parseFactor();
    while (isMulop()) {
        advance();  // consume mulop
        parseFactor();
    }
}

// factor → id | id[expr] | num | string | (expr) | not factor
void Parser::parseFactor() {
    if (check(TokenType::IDENTIFIER)) {
        advance();
        // Array access: id [ expression ]
        if (check(TokenType::LBRACKET)) {
            advance();
            parseExpression();
            expect(TokenType::RBRACKET, "Expected ']' after array index");
        }
        // Function call: id ( expression_list )
        else if (check(TokenType::LPAREN)) {
            advance();
            if (!check(TokenType::RPAREN))
                parseExpressionList();
            expect(TokenType::RPAREN, "Expected ')' in function call");
        }

    } else if (check(TokenType::NUMBER)) {
        advance();

    } else if (check(TokenType::STRING)) {
        advance();

    } else if (check(TokenType::LPAREN)) {
        advance();
        parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");

    } else if (check(TokenType::KW_NOT)) {
        advance();
        parseFactor();

    } else {
        error("Expected a factor (identifier, number, string, or expression) — got '"
              + current.lexeme + "'");
        synchronize(STMT_FOLLOW);
    }
}