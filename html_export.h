// ============================================================
// html_export.h — Write compiler output to a styled HTML file
// ============================================================
#pragma once
#include "symtable.h"
#include "icg.h"
#include "codegen.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

inline void exportHTML(
    const std::string&              sourceCode,
    const std::vector<Token>&       tokens,
    const SymbolTable&              symtable,
    const std::vector<std::string>& allErrors,
    const ICG&                      icg,
    const CodeGen&                  cg,
    const std::string&              outPath)
{
    std::ofstream f(outPath);
    if (!f.is_open()) return;

    // ── helpers ──────────────────────────────────────────────
    auto esc = [](const std::string& s) {
        std::string r;
        for (char c : s) {
            if      (c == '<') r += "&lt;";
            else if (c == '>') r += "&gt;";
            else if (c == '&') r += "&amp;";
            else if (c == '"') r += "&quot;";
            else               r += c;
        }
        return r;
    };

    f << R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Compiler Visualizer</title>
<style>
  :root {
    --bg-dark:    #f8f6f1;
    --bg-light:   #fffbf7;
    --panel:      #fef9f3;
    --border:     #e8dfd5;
    --accent:     #d97706;
    --accent-dim: #f5a962;
    --accent-light: #fcd5a4;
    --success:    #059669;
    --error:      #dc2626;
    --text-dark:  #2d2621;
    --text-muted: #8b8077;
    --mono:       'Fira Code', 'JetBrains Mono', monospace;
    --serif:      'Georgia', 'Times New Roman', serif;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg-light);
    color: var(--text-dark);
    font-family: -apple-system, 'Segoe UI', 'Helvetica Neue', sans-serif;
    min-height: 100vh;
    padding: 48px 32px;
    line-height: 1.6;
  }

  h1 {
    font-family: var(--serif);
    text-align: center;
    font-size: 2.2rem;
    font-weight: 300;
    letter-spacing: 0.5px;
    color: var(--text-dark);
    margin-bottom: 8px;
    text-rendering: optimizeLegibility;
  }

  .subtitle {
    text-align: center;
    color: var(--text-muted);
    font-size: 0.95rem;
    margin-bottom: 32px;
    font-weight: 400;
    letter-spacing: 0.3px;
  }

  /* Status badge */
  .status {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    padding: 10px 24px;
    border-radius: 6px;
    font-size: 0.9rem;
    font-weight: 500;
    margin: 0 auto 32px;
    display: flex;
    justify-content: center;
    width: fit-content;
    letter-spacing: 0.3px;
    box-shadow: 0 2px 6px rgba(0,0,0,0.05);
  }

  .status.ok {
    background: #ecfdf5;
    border: 1.5px solid var(--success);
    color: var(--success);
  }

  .status.err {
    background: #fef2f2;
    border: 1.5px solid var(--error);
    color: var(--error);
  }

  /* Tab bar */
  .tabs {
    display: flex;
    gap: 0;
    margin-bottom: 0;
    flex-wrap: wrap;
    border-bottom: 2px solid var(--border);
  }

  .tab {
    padding: 12px 24px;
    cursor: pointer;
    font-size: 0.9rem;
    font-weight: 500;
    background: transparent;
    border: none;
    color: var(--text-muted);
    transition: all 0.2s ease;
    position: relative;
    letter-spacing: 0.2px;
  }

  .tab:hover {
    color: var(--text-dark);
    background: rgba(217, 119, 6, 0.04);
  }

  .tab.active {
    color: var(--accent);
  }

  .tab.active::after {
    content: '';
    position: absolute;
    bottom: -2px;
    left: 0;
    right: 0;
    height: 2px;
    background: var(--accent);
  }

  /* Panels */
  .panel-wrap {
    background: var(--panel);
    padding: 28px;
    border-radius: 8px;
    border: 1px solid var(--border);
    box-shadow: 0 1px 3px rgba(0,0,0,0.06);
    min-height: 350px;
  }

  .panel {
    display: none;
  }

  .panel.active {
    display: block;
    animation: fadeIn 0.2s ease;
  }

  @keyframes fadeIn {
    from { opacity: 0; transform: translateY(2px); }
    to { opacity: 1; transform: translateY(0); }
  }

  /* Code blocks */
  pre {
    font-family: var(--mono);
    font-size: 0.85rem;
    line-height: 1.8;
    white-space: pre-wrap;
    word-break: break-word;
    color: var(--text-dark);
  }

  .line {
    display: flex;
    gap: 16px;
    padding: 0;
  }

  .ln {
    color: var(--text-muted);
    min-width: 40px;
    text-align: right;
    user-select: none;
    flex-shrink: 0;
  }

  .src {
    color: var(--text-dark);
    flex: 1;
  }

  .err-line .src {
    color: var(--error);
    font-weight: 500;
  }

  .err-msg {
    color: var(--error);
    padding-left: 56px;
    font-size: 0.8rem;
    margin-top: 4px;
  }

  /* Token table */
  table {
    width: 100%;
    border-collapse: collapse;
    font-family: var(--mono);
    font-size: 0.85rem;
  }

  th {
    text-align: left;
    padding: 10px 14px;
    border-bottom: 2px solid var(--border);
    color: var(--text-muted);
    font-weight: 600;
    text-transform: uppercase;
    font-size: 0.75rem;
    letter-spacing: 0.5px;
    background: rgba(217, 119, 6, 0.02);
  }

  td {
    padding: 8px 14px;
    border-bottom: 1px solid var(--border);
    color: var(--text-dark);
  }

  tr:hover td {
    background: rgba(217, 119, 6, 0.04);
  }

  .kw {
    color: var(--accent);
    font-weight: 600;
  }

  .id {
    color: var(--text-dark);
    font-weight: 500;
  }

  .num {
    color: #059669;
  }

  .str {
    color: #059669;
  }

  .sym {
    color: var(--text-dark);
  }

  .err {
    color: var(--error);
    font-weight: 600;
  }

  /* Symbol table */
  .sym-row {
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--border);
  }

  .sym-row:hover {
    background: rgba(217, 119, 6, 0.04);
  }

  .sym-name {
    font-family: var(--mono);
    color: var(--text-dark);
    min-width: 140px;
    font-weight: 500;
  }

  .sym-kind {
    font-size: 0.75rem;
    padding: 4px 10px;
    border-radius: 4px;
    background: rgba(217, 119, 6, 0.1);
    color: var(--accent);
    border: 1px solid rgba(217, 119, 6, 0.2);
    font-weight: 600;
    letter-spacing: 0.2px;
  }

  /* TAC & Target code */
  .code-line {
    display: flex;
    gap: 16px;
    padding: 4px 0;
  }

  .code-ln {
    color: var(--text-muted);
    min-width: 40px;
    text-align: right;
    user-select: none;
    flex-shrink: 0;
  }

  .label-line .code-op,
  .label-line .code-args {
    color: var(--accent);
    font-weight: 600;
  }

  .comment-line {
    color: var(--text-muted);
    font-style: italic;
  }

  .code-op {
    color: var(--accent);
    font-weight: 500;
  }

  .code-args {
    color: var(--text-dark);
  }

  /* Error list */
  .err-item {
    display: flex;
    gap: 12px;
    align-items: flex-start;
    padding: 12px 14px;
    margin-bottom: 8px;
    background: #fef2f2;
    border: 1px solid rgba(220, 38, 38, 0.2);
    border-radius: 6px;
    font-family: var(--mono);
    font-size: 0.85rem;
  }

  .err-icon {
    color: var(--error);
    font-weight: 700;
    flex-shrink: 0;
    margin-top: 2px;
  }

  /* Two-col layout for TAC + Target */
  .two-col {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 28px;
  }

  .col-title {
    color: var(--text-muted);
    font-size: 0.75rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 1px;
    margin-bottom: 12px;
    padding-bottom: 8px;
    border-bottom: 2px solid var(--border);
  }

  .col-title::before {
    content: '';
    display: inline-block;
    width: 4px;
    height: 4px;
    border-radius: 50%;
    background: var(--accent);
    margin-right: 8px;
  }

  @media (max-width: 900px) {
    .two-col {
      grid-template-columns: 1fr;
      gap: 24px;
    }
  }

  /* Empty state */
  .empty-state {
    text-align: center;
    padding: 40px 20px;
    color: var(--text-muted);
  }

  .empty-state p {
    font-size: 0.95rem;
    margin: 0;
  }

  /* Responsive */
  @media (max-width: 600px) {
    body { padding: 24px 16px; }
    h1 { font-size: 1.8rem; }
    .tabs { gap: 2px; }
    .tab { padding: 10px 16px; font-size: 0.85rem; }
    pre { font-size: 0.8rem; }
    .line { gap: 8px; }
  }
</style>
</head>
<body>

<h1>Compiler Visualizer</h1>
<p class="subtitle">Full pipeline — Lexer → Parser → ICG → Code Generator</p>
)html";

    // ── Status badge ─────────────────────────────────────────
    if (allErrors.empty())
        f << R"(<div class="status ok">✓ Compilation successful</div>)" "\n";
    else
        f << "<div class=\"status err\">✕ " << allErrors.size()
          << " error" << (allErrors.size() == 1 ? "" : "s") << " found</div>\n";

    // ── Tab bar ───────────────────────────────────────────────
    f << R"html(
<div class="tabs">
  <div class="tab active" onclick="show('source',this)">Source</div>
  <div class="tab"        onclick="show('tokens',this)">Tokens</div>
  <div class="tab"        onclick="show('symtab',this)">Symbol Table</div>
  <div class="tab"        onclick="show('errors',this)">Errors</div>
  <div class="tab"        onclick="show('codepanel',this)">TAC &amp; Target</div>
</div>
<div class="panel-wrap">
)html";

    // ── Tab 1: Source listing ─────────────────────────────────
    f << "<div class='panel active' id='source'><pre>\n";
    {
        std::istringstream ss(sourceCode);
        std::string line; int no = 1;
        while (std::getline(ss, line)) {
            f << "<div class='line'>"
              << "<span class='ln'>" << no << "</span>"
              << "<span class='src'>" << esc(line) << "</span></div>\n";
            no++;
        }
    }
    f << "</pre></div>\n";

    // ── Tab 2: Token stream ───────────────────────────────────
    f << "<div class='panel' id='tokens'>"
      << "<table><tr><th>Line</th><th>Type</th><th>Lexeme</th></tr>\n";
    for (auto& t : tokens) {
        if (t.type == TokenType::EOF_TOKEN) break;
        std::string cls = "sym";
        std::string tn  = tokenTypeName(t.type);
        if      (tn.substr(0,2) == "KW") cls = "kw";
        else if (tn == "IDENTIFIER")     cls = "id";
        else if (tn == "NUMBER")         cls = "num";
        else if (tn == "STRING")         cls = "str";
        else if (tn == "ERROR")          cls = "err";
        f << "<tr><td>" << t.line << "</td>"
          << "<td class='" << cls << "'>" << esc(tn) << "</td>"
          << "<td class='" << cls << "'>" << esc(t.lexeme) << "</td></tr>\n";
    }
    f << "</table></div>\n";

    // ── Tab 3: Symbol table ───────────────────────────────────
    f << "<div class='panel' id='symtab'>\n";
    if (symtable.getEntries().empty()) {
        f << "<div class='empty-state'><p>No symbols defined.</p></div>\n";
    } else {
        for (auto& entry : symtable.getEntries()) {
            std::string kind;
            switch (entry.kind) {
                case SymbolKind::VARIABLE:  kind = "INTEGER"; break;
                case SymbolKind::ARRAY:     kind = "ARRAY[" + std::to_string(entry.arraySize) + "]"; break;
                case SymbolKind::FUNCTION:  kind = "FUNCTION"; break;
                case SymbolKind::PROCEDURE: kind = "PROCEDURE"; break;
            }
            f << "<div class='sym-row'>"
              << "<span class='sym-name'>" << esc(entry.name) << "</span>"
              << "<span class='sym-kind'>" << kind << "</span>"
              << "</div>\n";
        }
    }
    f << "</div>\n";

    // ── Tab 4: Errors ─────────────────────────────────────────
    f << "<div class='panel' id='errors'>\n";
    if (allErrors.empty()) {
        f << "<div class='empty-state'><p>✓ No errors detected.</p></div>\n";
    } else {
        for (auto& e : allErrors)
            f << "<div class='err-item'><span class='err-icon'>✕</span>"
              << "<span>" << esc(e) << "</span></div>\n";
    }
    f << "</div>\n";

    // ── Tab 5: TAC + Target code side by side ─────────────────
    f << "<div class='panel' id='codepanel'>\n";
    if (allErrors.empty()) {
        f << "<div class='two-col'>\n";

        // TAC
        f << "<div><div class='col-title'>Three Address Code</div><pre>\n";
        for (size_t i = 0; i < icg.getCode().size(); i++) {
            std::string line = esc(icg.getCode()[i].toString());
            bool isLbl = !line.empty() && line.back() == ':';
            f << "<div class='code-line" << (isLbl ? " label-line" : "") << "'>"
              << "<span class='code-ln'>" << i << "</span>"
              << "<span class='code-op'>" << line << "</span>"
              << "</div>\n";
        }
        f << "</pre></div>\n";

        // Target code
        f << "<div><div class='col-title'>Target Code (Pseudo-Assembly)</div><pre>\n";
        for (size_t i = 0; i < cg.getCode().size(); i++) {
            std::string line = esc(cg.getCode()[i]);
            bool isLbl     = !line.empty() && line.back() == ':';
            bool isComment = !line.empty() && line[0] == ';';
            std::string cls = isLbl ? "label-line" : (isComment ? "comment-line" : "");
            f << "<div class='code-line " << cls << "'>"
              << "<span class='code-ln'>" << i << "</span>"
              << "<span class='code-args'>" << line << "</span>"
              << "</div>\n";
        }
        f << "</pre></div>\n";
        f << "</div>\n"; // two-col
    } else {
        f << "<div class='empty-state'><p>Code generation skipped due to compilation errors.</p></div>\n";
    }
    f << "</div>\n"; // codepanel

    // ── Close + tab JS ────────────────────────────────────────
    f << R"html(
</div> <!-- panel-wrap -->
<script>
function show(id, el) {
  document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  el.classList.add('active');
}
</script>
</body></html>
)html";
}