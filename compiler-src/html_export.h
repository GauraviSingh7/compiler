// ============================================================
// html_export.h — Compiler output → styled HTML with Parse Tree
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

// ── Recursively emit JS tree-construction calls ────────────
inline void buildTreeJS(std::ofstream& f, ASTNodePtr node,
                        const std::string& parentVar, int& id)
{
    int myId = id++;
    std::string var = "n" + std::to_string(myId);

    // Escape label for single-quoted JS string
    std::string escaped;
    for (char c : node->label) {
        if      (c == '\'') escaped += "\\'";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '"')  escaped += "\\\"";
        else                escaped += c;
    }

    f << "var " << var << " = addNode("
      << parentVar << ", '" << escaped << "', '" << node->kind << "');\n";

    for (auto& child : node->children)
        buildTreeJS(f, child, var, id);
}

// ── Main export function ───────────────────────────────────
inline void exportHTML(
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

    // HTML-escape helper
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

    // ── HTML head + styles ────────────────────────────────
    f << R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Compiler Visualizer</title>
<style>
  :root {
    --bg-base:    #ffffff;
    --bg-light:   #f8f8f8;
    --panel-bg:   #ffffff;
    --border:     #e5e7eb;
    --border-light: #d1d5db;
    --accent:     #606060;
    --accent-glow: #404040;
    --green:      #059669;
    --red:        #dc2626;
    --yellow:     #d97706;
    --cyan:       #0891b2;
    --text-main:  #111827;
    --text-sec:   #4b5563;
    --text-muted: #9ca3af;
    --mono:       'Fira Code','Cascadia Code','Consolas',monospace;
    --sans:       -apple-system,'Segoe UI','Helvetica Neue',sans-serif;
  }
 
  * { box-sizing:border-box; margin:0; padding:0; }
 
  body {
    background: var(--bg-base);
    color: var(--text-main);
    font-family: var(--sans);
    min-height: 100vh;
    padding: 48px 32px;
    line-height: 1.6;
    position: relative;
  }
 
  h1 {
    text-align: center;
    font-size: 2.4rem;
    font-weight: 600;
    letter-spacing: 2px;
    color: var(--text-main);
    margin-bottom: 8px;
    position: relative;
    z-index: 1;
  }
 
  .subtitle {
    text-align: center;
    color: var(--text-muted);
    font-size: 0.95rem;
    margin-bottom: 36px;
    font-weight: 400;
    letter-spacing: 0.5px;
    position: relative;
    z-index: 1;
  }
 
  .status {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px 26px;
    border-radius: 50px;
    font-size: 0.9rem;
    font-weight: 600;
    margin: 0 auto 36px;
    width: fit-content;
    position: relative;
    z-index: 1;
    border: 1px solid #e5e7eb;
  }
 
  .status.ok {
    background: #f0fdf4;
    border: 1px solid #d1fae5;
    color: #059669;
  }
 
  .status.err {
    background: #fef2f2;
    border: 1px solid #fecaca;
    color: #dc2626;
  }
 
  /* Tab bar */
  .tabs {
    display: flex;
    gap: 0;
    flex-wrap: wrap;
    border-bottom: 2px solid var(--border-light);
    position: relative;
    z-index: 1;
  }
 
  .tab {
    padding: 14px 26px;
    cursor: pointer;
    font-size: 0.9rem;
    font-weight: 500;
    background: transparent;
    border: none;
    color: var(--text-muted);
    transition: all 0.25s cubic-bezier(0.4,0,0.2,1);
    position: relative;
    letter-spacing: 0.3px;
  }
 
  .tab:hover {
    color: var(--text-main);
    background: #f3f4f6;
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
    height: 3px;
    background: var(--accent);
  }
 
  /* Panels */
  .panel-wrap {
    background: var(--panel-bg);
    padding: 32px;
    border: 1px solid var(--border-light);
    border-radius: 12px;
    min-height: 400px;
    position: relative;
    z-index: 1;
    box-shadow: 0 1px 3px rgba(0,0,0,0.1);
  }
 
  .panel { display: none; }
  .panel.active { display: block; animation: slideIn 0.3s cubic-bezier(0.4,0,0.2,1); }
  @keyframes slideIn { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }
 
  /* Source listing */
  pre {
    font-family: var(--mono);
    font-size: 0.85rem;
    line-height: 1.9;
    white-space: pre-wrap;
    word-break: break-word;
  }
 
  .line {
    display: flex;
    gap: 16px;
    padding: 2px 0;
    transition: background 0.15s;
  }
 
  .line:hover { background: #f9fafb; }
 
  .ln {
    color: var(--text-muted);
    min-width: 42px;
    text-align: right;
    user-select: none;
    flex-shrink: 0;
    font-weight: 500;
  }
 
  .src { color: var(--text-main); flex: 1; }
 
  /* Token table */
  table {
    width: 100%;
    border-collapse: collapse;
    font-family: var(--mono);
    font-size: 0.85rem;
  }
 
  th {
    text-align: left;
    padding: 12px 16px;
    color: var(--text-muted);
    border-bottom: 2px solid var(--border-light);
    font-size: 0.7rem;
    text-transform: uppercase;
    letter-spacing: 0.8px;
    font-weight: 700;
    background: #f9fafb;
  }
 
  td {
    padding: 10px 16px;
    border-bottom: 1px solid var(--border);
    transition: background 0.15s;
  }
 
  tr:hover td { background: #f9fafb; }
 
  .kw { color: var(--accent); font-weight: 700; }
  .id { color: var(--cyan); font-weight: 500; }
  .num { color: var(--yellow); }
  .str { color: var(--green); }
  .sym { color: var(--text-main); }
  .err { color: var(--red); font-weight: 700; }
 
  /* Symbol table */
  .sym-row {
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 12px 16px;
    border-bottom: 1px solid var(--border);
    transition: background 0.15s;
  }
 
  .sym-row:hover { background: #f9fafb; }
 
  .sym-name {
    font-family: var(--mono);
    color: var(--cyan);
    min-width: 140px;
    font-weight: 600;
  }
 
  .sym-kind {
    font-size: 0.7rem;
    padding: 4px 12px;
    border-radius: 6px;
    background: #f0f4f8;
    color: var(--accent);
    border: 1px solid var(--border-light);
    font-weight: 700;
    letter-spacing: 0.3px;
  }
 
  /* Error list */
  .err-item {
    display: flex;
    gap: 12px;
    align-items: flex-start;
    padding: 12px 16px;
    margin-bottom: 10px;
    background: #fef2f2;
    border: 1px solid #fecaca;
    border-radius: 8px;
    font-family: var(--mono);
    font-size: 0.85rem;
    transition: all 0.2s;
  }
 
  .err-item:hover {
    background: #fee2e2;
    border-color: #fca5a5;
  }
 
  .err-icon { color: var(--red); font-weight: 700; flex-shrink: 0; margin-top: 2px; }
 
  /* TAC + target two-column */
  .two-col {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 32px;
  }
 
  @media(max-width:900px) { .two-col { grid-template-columns: 1fr; } }
 
  .col-title {
    color: var(--text-muted);
    font-size: 0.7rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 1.2px;
    margin-bottom: 14px;
    padding-bottom: 10px;
    border-bottom: 2px solid var(--border-light);
    display: flex;
    align-items: center;
    gap: 8px;
  }
 
  .col-title::before {
    content: '';
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--accent);
  }
 
  .code-line {
    display: flex;
    gap: 16px;
    padding: 2px 0;
    transition: background 0.15s;
  }
 
  .code-line:hover { background: #f9fafb; }
 
  .code-ln {
    color: var(--text-muted);
    min-width: 40px;
    text-align: right;
    user-select: none;
    flex-shrink: 0;
    font-weight: 500;
  }
 
  .label-line .code-args {
    color: var(--yellow);
    font-weight: 700;
  }
 
  .comment-line { color: var(--text-muted); font-style: italic; }
  .code-args { color: var(--text-main); font-family: var(--mono); font-size: 0.85rem; }
 
  .empty-state {
    text-align: center;
    padding: 60px 20px;
    color: var(--text-muted);
    font-size: 0.95rem;
  }
 
  /* Parse Tree panel */
  #treepanel { overflow: auto; }
 
  .tree-hint {
    color: var(--text-muted);
    font-size: 0.75rem;
    margin-bottom: 16px;
    padding: 10px 14px;
    background: #f0f4f8;
    border-radius: 8px;
    border: 1px solid var(--border-light);
    display: flex;
    gap: 10px;
    align-items: center;
  }
 
  #treesvg { overflow: auto; cursor: grab; }
  #treesvg:active { cursor: grabbing; }
 
  /* Legend */
  .legend {
    display: flex;
    gap: 20px;
    flex-wrap: wrap;
    margin-bottom: 16px;
  }
 
  .legend-item {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 0.8rem;
    color: var(--text-muted);
  }
 
  .legend-dot {
    width: 14px;
    height: 14px;
    border-radius: 4px;
    flex-shrink: 0;
  }
</style>
</head>
<body>

<h1>&#9881; Compiler Visualizer</h1>
<p class="subtitle">Lexer &rarr; Parser &rarr; ICG &rarr; Code Generator</p>
)html";

    // ── Status badge ──────────────────────────────────────
    if (allErrors.empty())
        f << "<div class=\"status ok\">&#10003; Compilation successful &mdash; no errors</div>\n";
    else
        f << "<div class=\"status err\">&#10007; " << allErrors.size()
          << " error" << (allErrors.size() == 1 ? "" : "s") << " found</div>\n";

    // ── Tab bar ───────────────────────────────────────────
    f << R"html(
<div class="tabs">
  <div class="tab active" onclick="show('source',this)">Source</div>
  <div class="tab"        onclick="show('tokens',this)">Tokens</div>
  <div class="tab"        onclick="show('symtab',this)">Symbol Table</div>
  <div class="tab"        onclick="show('errors',this)">Errors</div>
  <div class="tab"        onclick="show('treepanel',this)">Parse Tree</div>
  <div class="tab"        onclick="show('codepanel',this)">TAC &amp; Target</div>
</div>
<div class="panel-wrap">
)html";

    // ── Tab 1: Source ─────────────────────────────────────
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

    // ── Tab 2: Tokens ─────────────────────────────────────
    f << "<div class='panel' id='tokens'>"
      << "<table><tr><th>Line</th><th>Type</th><th>Lexeme</th></tr>\n";
    for (auto& t : tokens) {
        if (t.type == TokenType::EOF_TOKEN) break;
        std::string cls = "sym";
        std::string tn  = tokenTypeName(t.type);
        if      (tn.size() >= 2 && tn.substr(0,2) == "KW") cls = "kw";
        else if (tn == "IDENTIFIER") cls = "id";
        else if (tn == "NUMBER")     cls = "num";
        else if (tn == "STRING")     cls = "str";
        else if (tn == "ERROR")      cls = "err";
        f << "<tr><td>" << t.line << "</td>"
          << "<td class='" << cls << "'>" << esc(tn) << "</td>"
          << "<td class='" << cls << "'>" << esc(t.lexeme) << "</td></tr>\n";
    }
    f << "</table></div>\n";

    // ── Tab 3: Symbol Table ───────────────────────────────
    f << "<div class='panel' id='symtab'>\n";
    if (symtable.getEntries().empty()) {
        f << "<div class='empty-state'>No symbols defined.</div>\n";
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

    // ── Tab 4: Errors ─────────────────────────────────────
    f << "<div class='panel' id='errors'>\n";
    if (allErrors.empty()) {
        f << "<div class='empty-state'>&#10003; No errors detected.</div>\n";
    } else {
        for (auto& e : allErrors)
            f << "<div class='err-item'>"
              << "<span class='err-icon'>&#10007;</span>"
              << "<span>" << esc(e) << "</span></div>\n";
    }
    f << "</div>\n";

    // ── Tab 5: Parse Tree ─────────────────────────────────
    f << "<div class='panel' id='treepanel'>\n";
    f << "<div class='legend'>"
      << "<span class='legend-item'><span class='legend-dot' style='background:#1e1b4b;border:1.5px solid #7c6af7'></span>Grammar rule</span>"
      << "<span class='legend-item'><span class='legend-dot' style='background:#0c4a6e;border:1.5px solid #22d3ee'></span>Token leaf</span>"
      << "<span class='legend-item'><span class='legend-dot' style='background:#450a0a;border:1.5px solid #f87171'></span>Error node</span>"
      << "</div>\n";
    f << "<div class='tree-hint'>&#128161; Click any node to collapse / expand its subtree.</div>\n";
    f << "<div id='treesvg'></div>\n";

    // ── Emit JS tree data ─────────────────────────────────
    f << "<script>\n";
    f << "const NODE_W=150, NODE_H=40, H_GAP=18, V_GAP=56;\n\n";

    f << "function addNode(parent, label, kind) {\n"
      << "  var n={label:label, kind:kind, children:[], _collapsed:false, _hidden:false};\n"
      << "  if(parent) parent.children.push(n);\n"
      << "  return n;\n"
      << "}\n\n";

    // Emit tree structure from AST
    f << "var root = addNode(null, '" << (treeRoot ? treeRoot->label : "program") << "', 'rule');\n";
    if (treeRoot) {
        int id = 1;
        for (auto& child : treeRoot->children)
            buildTreeJS(f, child, "root", id);
    }

    // Layout + render engine
    f << R"js(
function layout(node, depth, counter) {
  node._y = depth * (NODE_H + V_GAP);
  if (!node.children || node.children.length === 0 || node._collapsed) {
    node._x = counter.v++ * (NODE_W + H_GAP);
  } else {
    var visible = node.children.filter(function(c){ return !c._hidden; });
    for (var c of visible) layout(c, depth+1, counter);
    if (visible.length > 0) {
      node._x = (visible[0]._x + visible[visible.length-1]._x) / 2;
    } else {
      node._x = counter.v++ * (NODE_W + H_GAP);
    }
  }
}

function maxDepth(n) {
  if (!n.children || n.children.length === 0 || n._collapsed) return 0;
  var visible = n.children.filter(function(c){ return !c._hidden; });
  if (visible.length === 0) return 0;
  return 1 + Math.max.apply(null, visible.map(maxDepth));
}

function countLeaves(n) {
  if (!n.children || n.children.length === 0 || n._collapsed) return 1;
  var visible = n.children.filter(function(c){ return !c._hidden; });
  if (visible.length === 0) return 1;
  return visible.reduce(function(s,c){ return s + countLeaves(c); }, 0);
}

function render() {
  var counter = {v:0};
  layout(root, 0, counter);
  var W = Math.max(counter.v * (NODE_W + H_GAP) + 40, 600);
  var H = (maxDepth(root)+1) * (NODE_H + V_GAP) + 60;

  var svg = document.createElementNS('http://www.w3.org/2000/svg','svg');
  svg.setAttribute('width', W);
  svg.setAttribute('height', H);

  drawEdges(svg, root);
  drawNodes(svg, root);
  document.getElementById('treesvg').innerHTML = '';
  document.getElementById('treesvg').appendChild(svg);
}

var COLORS = { rule:'#606060', token:'#0891b2', error:'#dc2626' };
var BG     = { rule:'#f9fafb', token:'#f0f9ff', error:'#fef2f2' };

function drawEdges(svg, node) {
  if (!node.children || node._collapsed) return;
  var visible = node.children.filter(function(c){ return !c._hidden; });
  for (var c of visible) {
    var line = document.createElementNS('http://www.w3.org/2000/svg','line');
    line.setAttribute('x1', node._x + NODE_W/2);
    line.setAttribute('y1', node._y + NODE_H);
    line.setAttribute('x2', c._x + NODE_W/2);
    line.setAttribute('y2', c._y);
    line.setAttribute('stroke','#d1d5db');
    line.setAttribute('stroke-width','1.5');
    svg.appendChild(line);
    drawEdges(svg, c);
  }
}

function drawNodes(svg, node) {
  if (node._hidden) return;

  var g = document.createElementNS('http://www.w3.org/2000/svg','g');
  g.style.cursor = node.children && node.children.length > 0 ? 'pointer' : 'default';
  (function(n){ g.onclick = function(e){ e.stopPropagation(); toggleCollapse(n); }; })(node);

  // Box
  var rect = document.createElementNS('http://www.w3.org/2000/svg','rect');
  rect.setAttribute('x', node._x);
  rect.setAttribute('y', node._y);
  rect.setAttribute('width', NODE_W);
  rect.setAttribute('height', NODE_H);
  rect.setAttribute('rx', '7');
  rect.setAttribute('fill', BG[node.kind] || '#1e293b');
  rect.setAttribute('stroke', COLORS[node.kind] || '#334155');
  rect.setAttribute('stroke-width', node._collapsed ? '2.5' : '1.5');
  g.appendChild(rect);

  // Collapse indicator dot
  if (node.children && node.children.length > 0) {
    var dot = document.createElementNS('http://www.w3.org/2000/svg','circle');
    dot.setAttribute('cx', node._x + NODE_W - 9);
    dot.setAttribute('cy', node._y + 9);
    dot.setAttribute('r', '4');
    dot.setAttribute('fill', node._collapsed ? COLORS[node.kind] : 'transparent');
    dot.setAttribute('stroke', COLORS[node.kind] || '#334155');
    dot.setAttribute('stroke-width','1.5');
    g.appendChild(dot);
  }

  // Label — split on \n for two-line display
  var parts = node.label.split('\\n');
  var mainLabel = parts[0];
  var subLabel  = parts.length > 1 ? parts[1] : '';
  var fontSize  = mainLabel.length > 16 ? 9 : (mainLabel.length > 11 ? 10 : 12);
  var yMain = node._y + (subLabel ? 15 : 24);

  var txt = document.createElementNS('http://www.w3.org/2000/svg','text');
  txt.setAttribute('x', node._x + NODE_W/2);
  txt.setAttribute('y', yMain);
  txt.setAttribute('text-anchor','middle');
  txt.setAttribute('font-size', fontSize);
  txt.setAttribute('font-family','Consolas,monospace');
  txt.setAttribute('fill', COLORS[node.kind] || '#e2e8f0');
  txt.textContent = mainLabel;
  g.appendChild(txt);

  if (subLabel) {
    var sub = document.createElementNS('http://www.w3.org/2000/svg','text');
    sub.setAttribute('x', node._x + NODE_W/2);
    sub.setAttribute('y', node._y + 30);
    sub.setAttribute('text-anchor','middle');
    sub.setAttribute('font-size','8');
    sub.setAttribute('font-family','Consolas,monospace');
    sub.setAttribute('fill','#64748b');
    sub.textContent = subLabel;
    g.appendChild(sub);
  }

  svg.appendChild(g);

  if (!node._collapsed) {
    var visible = node.children ? node.children.filter(function(c){ return !c._hidden; }) : [];
    for (var c of visible) drawNodes(svg, c);
  }
}

function toggleCollapse(node) {
  if (!node.children || node.children.length === 0) return;
  node._collapsed = !node._collapsed;
  setHidden(node.children, node._collapsed);
  render();
}

function setHidden(children, val) {
  for (var c of children) {
    c._hidden = val;
    if (c.children) setHidden(c.children, val);
  }
}

// Initial render after page load
window.addEventListener('load', function() { render(); });
)js";

    f << "</script></div>\n"; // close treepanel

    // ── Tab 6: TAC + Target ───────────────────────────────
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
              << "<span class='code-args'>" << line << "</span>"
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
        f << "</div>\n";
    } else {
        f << "<div class='empty-state'>Code generation skipped due to errors.</div>\n";
    }
    f << "</div>\n";

    // ── Close panel-wrap + tab switcher JS ────────────────
    f << R"html(
</div>
<script>
function show(id, el) {
  document.querySelectorAll('.panel').forEach(function(p){ p.classList.remove('active'); });
  document.querySelectorAll('.tab').forEach(function(t){ t.classList.remove('active'); });
  document.getElementById(id).classList.add('active');
  el.classList.add('active');
  if (id === 'treepanel') render();
}
</script>
</body></html>
)html";
}