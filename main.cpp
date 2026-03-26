// ============================================================
// main.cpp — Lexer + Parser + ICG + CodeGen + HTML Visualizer
// ============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include "lexer.h"
#include "token_printer.h"
#include "symtable.h"
#include "icg.h"
#include "parser.h"
#include "codegen.h"
#include "html_export.h"

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: compiler <source>\n"; return 1; }

    std::ifstream file(argv[1]);
    if (!file.is_open()) { std::cerr << "Cannot open file\n"; return 1; }
    std::stringstream buf; buf << file.rdbuf();
    std::string source = buf.str();

    // ── Listing file ─────────────────────────────────────────
    Lexer lexer1(source);
    auto tokens = lexer1.tokenizeAll();

    std::cout << "\n====== LISTING FILE ======\n";
    {
        std::istringstream ss(source); std::string ln; int no = 1;
        std::unordered_map<int,std::vector<std::string>> em;
        for (auto& e : lexer1.errors) { int l=0; sscanf(e.c_str(),"Line %d",&l); em[l].push_back(e); }
        while (std::getline(ss,ln)) {
            std::cout << std::setw(4) << no << "  " << ln << "\n";
            if (em.count(no)) for (auto& e : em[no]) std::cout << "      *** LEX ERROR: " << e << "\n";
            no++;
        }
    }

    // ── Token stream ─────────────────────────────────────────
    std::cout << "\n====== TOKEN STREAM ======\n";
    std::cout << std::left << std::setw(6)<<"LINE" << std::setw(16)<<"TYPE" << "LEXEME\n"
              << std::string(50,'-') << "\n";
    for (auto& t : tokens) {
        if (t.type == TokenType::EOF_TOKEN) break;
        std::cout << std::setw(6)<<t.line << std::setw(16)<<tokenTypeName(t.type) << t.lexeme << "\n";
    }

    // ── Parse + ICG ──────────────────────────────────────────
    Lexer       lexer2(source);
    SymbolTable symtable;
    ICG         icg;
    Parser      parser(lexer2, symtable, icg);
    parser.parse();

    // ── Collect ALL errors (lex + parse) ─────────────────────
    std::vector<std::string> allErrors;
    for (auto& e : lexer1.errors) allErrors.push_back(e);
    for (auto& e : parser.errors) allErrors.push_back(e);

    // ── Code Generation (always run so cg exists for HTML) ───
    CodeGen cg(icg);
    cg.generate();

    // ── Print results ─────────────────────────────────────────
    symtable.print();

    std::cout << "\n====== PARSE RESULT ======\n";
    if (allErrors.empty()) {
        std::cout << "Parsing: PASSED — no errors.\n";
        icg.print();
        cg.print();
    } else {
        std::cout << "Errors found (" << allErrors.size() << "):\n";
        for (auto& e : allErrors) std::cout << "  " << e << "\n";
        std::cout << "\n[TAC and target code not generated — errors present]\n";
    }

    // ── Export HTML (always, for both success and error cases) 
    std::string htmlPath = std::string(argv[1]) + ".html";
    exportHTML(source, tokens, symtable, allErrors, icg, cg, htmlPath);
    std::cout << "\n[Visualizer saved to: " << htmlPath << "]\n";

    return 0;
}