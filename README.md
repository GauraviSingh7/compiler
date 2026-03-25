# 🧠 Mini Compiler Project

A complete compiler pipeline implementation that transforms a high-level source program into intermediate representation and finally into target code, walking through every classical phase of compilation.

---

## 🌊 Overview

This compiler is designed for a Pascal-like programming language.  
It processes input source code step-by-step through the following stages:

1. **Lexical Analysis** → breaks code into tokens  
2. **Syntax Analysis (Parsing)** → validates structure using grammar  
3. **Intermediate Code Generation (ICG)** → produces Three Address Code  
4. **Code Generation** → converts IR into target instructions  

Even when the program encounters errors, the compiler continues processing—reporting them with line numbers while proceeding with analysis.

---

## 📁 Project Structure

```
compiler/
├── lexer.h              # Token types + Lexer declaration
├── lexer.cpp            # Lexer implementation
├── token_printer.h      # Token type → string helper
│
├── parser.h             # Parser declaration
├── parser.cpp           # Recursive descent parser
│
├── symtable.h           # Symbol table
├── symtable.cpp
├── icg.h                # Intermediate Code Generator
├── icg.cpp
│
├── codegen.h            # Code Generator
├── codegen.cpp
│
└── main.cpp             # Driver program
```

---

## ⚙️ Features

- Tokenization of identifiers, keywords, numbers, strings, and operators  
- Recursive descent parsing based on defined grammar  
- Error handling with line numbers (continues parsing)  
- Symbol table management  
- Three Address Code (TAC) generation  
- Simple target code generation  
- Modular and extensible design  

---

## 🧾 Language Specification

### 🔤 Lexical Units
The language consists of the following lexical units:
- Identifiers
- Reserved words
- Numbers
- Strings
- Delimiters

### 🔹 Delimiters
```
( ) [ ] ; : . , * - + / < = >
```

Compound symbols:
```
<>  :=  <=  >=
```

Spaces may be freely used between tokens. Each lexical unit must fit on a single line.

---

### 🔹 Identifiers
- Begin with a letter
- Followed by letters or digits
- Case-insensitive
- Significant up to first 32 characters

---

### 🔹 Numbers
- Only integers are supported
- Sequence of digits

---

### 🔹 Strings
- Enclosed in single quotes `' '`
- Supports escape sequences:
  - `\\n` → newline
  - `\\t` → tab
  - `\\'` → single quote

Example:
```
'DON\'T'
```

---

### 🔹 Comments
- Start with `!`
- Continue until end of line

Example:
```
! this is a comment
```

---

### 🔹 Reserved Words
```
and, array, begin, integer, do, else, end, function,
if, of, or, not, procedure, program, read, then,
var, while, write
```

---

## 🔧 Grammar (Simplified)

```
program → program id ;

statement → variable := expression
          | procedure_statement
          | compound_statement
          | if expression then statement else statement
          | if expression then statement
          | while expression do statement
          | read(identifier_list)
          | write(output_list)
```

---

## 🛠️ Compilation & Execution

### 🔧 Compile
```bash
g++ -std=c++17 -Wall lexer.cpp parser.cpp symtable.cpp icg.cpp codegen.cpp main.cpp -o compiler
```

### ▶️ Run (Windows PowerShell)
```bash
.\\compiler.exe input.txt
```

### ▶️ Run (Linux / Mac)
```bash
./compiler input.txt
```

---

## 📤 Output

The compiler produces:

- Token stream (for debugging)
- Error messages with line numbers
- Intermediate code (Three Address Code)
- Final generated target code (if no errors)

---

## 🧠 Design Philosophy

Each module operates as a stage in translation:
- The **Lexer** processes raw input into tokens
- The **Parser** validates structure
- The **ICG** builds intermediate representation
- The **Code Generator** produces executable-like instructions

---

## ⚠️ Error Handling

- Errors are reported with line numbers
- Compilation continues despite errors
- Code generation occurs only if no errors are present

---

## 🚀 Future Improvements

- Type checking
- Optimization (constant folding, dead code elimination)
- Abstract Syntax Tree (AST) visualization
- GUI / Web-based interface
- Extended data types and functions

---
