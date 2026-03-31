# Pascal Compiler

A complete compiler pipeline for a Pascal-like programming language, implemented in C++. Covers all four classical phases — lexical analysis, recursive descent parsing, intermediate code generation, and target code generation — paired with a browser-based IDE for real-time visualization.

**Live demo:** https://compiler-frontend-333v.onrender.com

---

## Overview

The compiler accepts a Pascal-like source program and processes it through four phases:

1. **Lexical Analyzer** — tokenizes the source, handles identifiers, reserved words, integer literals, string literals, and `!` comments; reports lexical errors with line numbers and continues
2. **Recursive Descent Parser** — parses the full grammar, builds a typed AST, and drives TAC generation simultaneously; uses panic-mode error recovery to report multiple errors without aborting
3. **Intermediate Code Generator (ICG)** — emits Three Address Code with temporaries (t1, t2, …) and labels (L1, L2, …) for all expressions, assignments, control flow, and procedure/function calls
4. **Code Generator** — translates TAC to a two-register pseudo-assembly instruction set (LOADI, LOAD, STORE, ADD, SUB, MUL, DIV, JZ, JMP, CALL, PRINT, HALT, …)

Target code is generated only when the source program is error-free. The listing file interleaves source lines with any errors reported on that line, matching the output format of production compilers.

---

## Language

The language is a statically-typed Pascal-like language with the following features:

**Types:** `integer`, `array[n] of integer`

**Statements:** assignment (`:=`), `if-then`, `if-then-else`, `while-do`, `read`, `write`, compound (`begin … end`), procedure and function calls

**Expressions:** arithmetic (`+`, `-`, `*`, `/`), relational (`=`, `<>`, `<`, `<=`, `>`, `>=`), logical (`and`, `or`, `not`)

**Lexical rules:**
- Identifiers are case-insensitive, distinguishable within the first 32 characters
- Strings are apostrophe-delimited with backslash escapes (`\n`, `\t`, `\\`, `\'`)
- Comments begin with `!` and extend to end of line

**Reserved words:** `and array begin do else end function if integer not of or procedure program read then var while write`

### Sample Program

```pascal
! Simple example
program myProg;
var x, y : integer;
begin
  x := 10;
  y := x + 5;
  write(y);
  if x <> y then
    write('DON\'T panic')
  else
    write('ok')
end.
```

---

## Repository Structure

```
.
├── compiler-src/              ← C++ compiler source
│   ├── lexer.h / lexer.cpp    ← Lexical analyzer
│   ├── parser.h / parser.cpp  ← Recursive descent parser + AST + ICG driver
│   ├── icg.h                  ← TAC engine (temps, labels, instruction list)
│   ├── codegen.h / codegen.cpp← Pseudo-assembly code generator
│   ├── symtable.h             ← Symbol table
│   ├── ast.h                  ← AST node definition
│   ├── html_export.h          ← Static HTML visualizer export
│   ├── json_export.h          ← JSON export for the IDE backend
│   ├── token_printer.h        ← Token type → string utility
│   ├── main.cpp               ← Compiler driver
│   └── Makefile
│
├── backend/                   ← Node.js / Express API
│   ├── server.js              ← POST /compile endpoint
│   └── package.json
│
└── frontend/                  ← React + Vite IDE
    ├── src/
    │   ├── App.jsx            ← Split-pane layout with drag-to-resize
    │   ├── components/
    │   │   ├── Editor.jsx     ← Code editor with syntax highlighting
    │   │   ├── OutputPanel.jsx← Tab switcher
    │   │   ├── Toolbar.jsx
    │   │   ├── StatusBar.jsx
    │   │   └── tabs/
    │   │       ├── TokensTab.jsx
    │   │       ├── SymbolsTab.jsx
    │   │       ├── ErrorsTab.jsx
    │   │       ├── ParseTreeTab.jsx   ← Collapsible SVG tree
    │   │       ├── TACTab.jsx
    │   │       └── TargetTab.jsx
    └── package.json
```

---

## Building the Compiler

**Requirements:** g++ with C++17 support

```bash
cd backend
make
# Produces ./compiler binary
```

**Usage:**

```bash
# Standard output — listing file + HTML visualizer
./compiler program.pas

# JSON output for the IDE backend
./compiler program.pas --json output.json
```

The `--json` flag writes all compiler output (tokens, symbol table, errors, AST, TAC, target code) as structured JSON, which the IDE backend returns to the frontend.

---

## Running the IDE Locally

**Requirements:** Node.js 18+

### 1. Start the backend

```bash
cd backend
npm install
node server.js
# Listening on http://localhost:3001
```

The backend expects the compiled `compiler` binary at the path set by `COMPILER_PATH` (defaults to `./compiler`). Copy or symlink the binary from `compiler-src/` into `backend/`.

### 2. Start the frontend

```bash
cd frontend
npm install
npm run dev
# Opens at http://localhost:5173
```

The Vite dev server proxies `/compile` requests to `localhost:3001` automatically — no CORS configuration needed locally.

---

## Compiler Output

### Token Stream (excerpt)

| Line | Type       | Lexeme  |
|------|------------|---------|
| 2    | KW_PROGRAM | program |
| 2    | IDENTIFIER | myprog  |
| 3    | KW_VAR     | var     |
| 3    | IDENTIFIER | x       |
| 3    | KW_INTEGER | integer |

### Symbol Table

| Name | Kind    |
|------|---------|
| x    | INTEGER |
| y    | INTEGER |

### Three Address Code (excerpt)

```
0      x = 10
1      t1 = x + 5
2      y = t1
3      write y
4      t2 = x <> y
5      t3 = not t2
6      if t3 goto L1
7      write "DON'T panic"
8      goto L2
9  L1:
10     write "ok"
11 L2:
```

### Target Code / Pseudo-Assembly (excerpt)

```
0      LOADI R0, 10
1      STORE x, R0
2      LOAD  R0, x
3      LOADI R1, 5
4      ADD   R0, R0, R1
5      STORE t1, R0
...
17     LOAD  R0, t3
18     JZ    R0, L1
19     LOADS R0, "DON'T panic"
20     PRINT R0
21     JMP   L2
22 L1:
23     LOADS R0, "ok"
24     PRINT R0
25 L2:
26     HALT
```

### Error Recovery

When errors are present, the compiler reports all of them and suppresses target code generation:

```
Line 5:  Expected factor -- got ';'
Line 7:  Expected factor -- got 'then'
Line 9:  Expected 'then' -- got 'end'

[TAC and target code not generated — errors present]
```

---

## IDE Features

The browser-based IDE replaces per-compilation static HTML exports with a persistent split-pane interface:

- **Editor** — Pascal syntax highlighting, line numbers, tab key support, drag-to-resize divider
- **Tokens** — full token stream table with type and lexeme columns
- **Symbols** — declared identifiers with kind badges (INTEGER, ARRAY, FUNCTION, PROCEDURE)
- **Errors** — error list with line numbers extracted and highlighted; badge count on the tab auto-focuses on compile
- **Parse Tree** — interactive collapsible SVG tree; click any node to expand or collapse its subtree; rule, token, and error nodes are colour-coded
- **TAC** — three-address code with line indices and label highlighting
- **Target** — pseudo-assembly output with comment and label formatting

---

## Architecture Notes

**Parser and ICG are coupled by design.** Each grammar rule method accepts a parent `ASTNodePtr`, attaches child nodes as it consumes tokens, and simultaneously calls ICG emit methods. This avoids a separate tree-walking pass and keeps the implementation compact.

**Error recovery uses follow sets.** When `expect()` fails, `synchronize()` advances the token stream to the nearest token in a predefined follow set (`;`, `end`, `else`, `EOF`), then resumes parsing. This allows the compiler to report multiple independent errors in a single run rather than aborting at the first fault.

**Code generation is a single linear pass.** The code generator iterates over the flat TAC list once, mapping each instruction to pseudo-assembly using a two-register model (R0 as accumulator, R1 as secondary operand). No register allocation or optimization is performed — the output is intentionally readable for tracing by hand.

**JSON export is additive.** The `--json` flag is the only change to the driver. Without it, the compiler behaves exactly as before — stdout listing plus HTML export. The JSON path is only taken when the IDE backend is calling the binary.

---

## Tech Stack

| Layer    | Technology       |
|----------|------------------|
| Compiler | C++17            |
| Backend  | Node.js, Express |
| Frontend | React 18, Vite   |
| Styling  | CSS Modules      |
| Deploy   | Render           |