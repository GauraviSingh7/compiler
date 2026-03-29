// ============================================================
// json_export.h — Export all compiler output as structured JSON
//   Used by the IDE backend (--json flag)
// ============================================================
#pragma once
#include "symtable.h"
#include "icg.h"
#include "codegen.h"
#include "ast.h"
#include "token_printer.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// ── Tiny JSON helpers ─────────────────────────────────────

static std::string jsonEsc(const std::string& s) {
    std::string r;
    for (unsigned char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    r += buf;
                } else {
                    r += c;
                }
        }
    }
    return r;
}

static std::string jStr(const std::string& s) {
    return "\"" + jsonEsc(s) + "\"";
}

// ── Recursively build tree JSON ───────────────────────────
// Format: { "label": "...", "kind": "...", "children": [...] }
static void buildTreeJSON(std::ostringstream& out, ASTNodePtr node) {
    out << "{";
    out << "\"label\":" << jStr(node->label) << ",";
    out << "\"kind\":"  << jStr(node->kind)  << ",";
    out << "\"children\":[";
    for (size_t i = 0; i < node->children.size(); i++) {
        if (i) out << ",";
        buildTreeJSON(out, node->children[i]);
    }
    out << "]}";
}

// ── Main export function ──────────────────────────────────
inline void exportJSON(
    const std::string&              sourceCode,
    const std::vector<Token>&       tokens,
    const SymbolTable&              symtable,
    const std::vector<std::string>& allErrors,
    const ICG&                      icg,
    const CodeGen&                  cg,
    ASTNodePtr                      treeRoot,
    const std::string&              outPath)
{
    std::ofstream f(outPath);
    if (!f.is_open()) return;

    std::ostringstream out;
    out << "{\n";

    // ── source ────────────────────────────────────────────
    out << "\"source\":" << jStr(sourceCode) << ",\n";

    // ── success flag ─────────────────────────────────────
    out << "\"success\":" << (allErrors.empty() ? "true" : "false") << ",\n";

    // ── tokens ───────────────────────────────────────────
    out << "\"tokens\":[\n";
    bool firstTok = true;
    for (auto& t : tokens) {
        if (t.type == TokenType::EOF_TOKEN) break;
        if (!firstTok) out << ",\n";
        out << "{"
            << "\"line\":"   << t.line                       << ","
            << "\"type\":"   << jStr(tokenTypeName(t.type))  << ","
            << "\"lexeme\":" << jStr(t.lexeme)
            << "}";
        firstTok = false;
    }
    out << "\n],\n";

    // ── symbol table ─────────────────────────────────────
    out << "\"symbols\":[\n";
    auto entries = symtable.getEntries();
    for (size_t i = 0; i < entries.size(); i++) {
        if (i) out << ",\n";
        auto& s = entries[i];
        std::string kind;
        switch (s.kind) {
            case SymbolKind::VARIABLE:  kind = "INTEGER";   break;
            case SymbolKind::ARRAY:     kind = "ARRAY";     break;
            case SymbolKind::FUNCTION:  kind = "FUNCTION";  break;
            case SymbolKind::PROCEDURE: kind = "PROCEDURE"; break;
        }
        out << "{"
            << "\"name\":"      << jStr(s.name)               << ","
            << "\"kind\":"      << jStr(kind)                  << ","
            << "\"arraySize\":" << s.arraySize
            << "}";
    }
    out << "\n],\n";

    // ── errors ───────────────────────────────────────────
    out << "\"errors\":[\n";
    for (size_t i = 0; i < allErrors.size(); i++) {
        if (i) out << ",\n";
        out << jStr(allErrors[i]);
    }
    out << "\n],\n";

    // ── TAC ──────────────────────────────────────────────
    out << "\"tac\":[\n";
    auto& tacCode = icg.getCode();
    for (size_t i = 0; i < tacCode.size(); i++) {
        if (i) out << ",\n";
        out << "{"
            << "\"index\":"  << i                               << ","
            << "\"text\":"   << jStr(tacCode[i].toString())     << ","
            << "\"isLabel\":" << (tacCode[i].op == "label" ? "true" : "false")
            << "}";
    }
    out << "\n],\n";

    // ── target code ──────────────────────────────────────
    out << "\"targetCode\":[\n";
    auto& cgCode = cg.getCode();
    for (size_t i = 0; i < cgCode.size(); i++) {
        if (i) out << ",\n";
        const std::string& line = cgCode[i];
        bool isLabel   = !line.empty() && line.back() == ':';
        bool isComment = !line.empty() && line[0] == ';';
        out << "{"
            << "\"index\":"     << i                << ","
            << "\"text\":"      << jStr(line)        << ","
            << "\"isLabel\":"   << (isLabel   ? "true" : "false") << ","
            << "\"isComment\":" << (isComment ? "true" : "false")
            << "}";
    }
    out << "\n],\n";

    // ── parse tree ───────────────────────────────────────
    out << "\"tree\":";
    if (treeRoot) {
        buildTreeJSON(out, treeRoot);
    } else {
        out << "null";
    }
    out << "\n";

    out << "}\n";

    f << out.str();
}