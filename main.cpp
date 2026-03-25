// ============================================================
// main.cpp — Updated driver for Lexer + Parser
// ============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "lexer.h"
#include "token_printer.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: compiler <source_file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << argv[1] << "\n";
        return 1;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    // ── Lex ──────────────────────────────────────────────────
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenizeAll();

    // ── Listing file ─────────────────────────────────────────
    std::cout << "\n====== LISTING FILE ======\n";
    {
        std::istringstream ss(source);
        std::string srcLine;
        int lineNo = 1;
        std::unordered_map<int, std::vector<std::string>> errMap;
        for (auto& e : lexer.errors) {
            int ln = 0; sscanf(e.c_str(), "Line %d", &ln);
            errMap[ln].push_back(e);
        }
        while (std::getline(ss, srcLine)) {
            std::cout << std::setw(4) << lineNo << "  " << srcLine << "\n";
            if (errMap.count(lineNo))
                for (auto& err : errMap[lineNo])
                    std::cout << "      *** LEX ERROR: " << err << "\n";
            lineNo++;
        }
    }

    // ── Token stream ─────────────────────────────────────────
    std::cout << "\n====== TOKEN STREAM ======\n";
    std::cout << std::left << std::setw(6) << "LINE"
              << std::setw(16) << "TYPE" << "LEXEME\n"
              << std::string(50, '-') << "\n";
    for (auto& tok : tokens) {
        if (tok.type == TokenType::EOF_TOKEN) break;
        std::cout << std::setw(6)  << tok.line
                  << std::setw(16) << tokenTypeName(tok.type)
                  << tok.lexeme    << "\n";
    }

    // ── Parse (fresh lexer, same source) ────────────────────
    Lexer lexer2(source);
    Parser parser(lexer2);
    parser.parse();

    std::cout << "\n====== PARSE RESULT ======\n";
    if (lexer.errors.empty() && parser.errors.empty()) {
        std::cout << "Parsing: PASSED — no errors.\n";
    } else {
        if (!lexer.errors.empty()) {
            std::cout << "Lexical errors:\n";
            for (auto& e : lexer.errors) std::cout << "  " << e << "\n";
        }
        if (!parser.errors.empty()) {
            std::cout << "Parse errors:\n";
            for (auto& e : parser.errors) std::cout << "  " << e << "\n";
        }
    }

    return 0;
}