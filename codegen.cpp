// ============================================================
// codegen.cpp — Code Generator implementation
// ============================================================
#include "codegen.h"
#include <cctype>
#include <sstream>

CodeGen::CodeGen(const ICG& i) : icg(i) {}

// ── Helpers ────────────────────────────────────────────────

void CodeGen::emit(const std::string& instr) {
    targetCode.push_back(instr);
}

// A value is a constant if it's all digits (possibly negative)
bool CodeGen::isConstant(const std::string& s) const {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    for (size_t i = start; i < s.size(); i++)
        if (!std::isdigit((unsigned char)s[i])) return false;
    return true;
}

// A value is a string literal if it starts with '"'
bool CodeGen::isString(const std::string& s) const {
    return !s.empty() && s[0] == '"';
}

// Load a value into the given register (R0 or R1)
void CodeGen::loadInto(const std::string& reg, const std::string& val) {
    if (isConstant(val))
        emit("LOADI " + reg + ", " + val);    // load immediate constant
    else if (isString(val))
        emit("LOADS " + reg + ", " + val);    // load string literal
    else
        emit("LOAD  " + reg + ", " + val);    // load from memory (var/temp)
}

// ── Main translation pass ──────────────────────────────────
void CodeGen::generate() {
    targetCode.clear();
    emit("; ===== Generated Target Code =====");
    emit("; Registers: R0 (accumulator), R1 (secondary)");
    emit(";");

    for (auto& instr : icg.getCode())
        translateInstr(instr);

    emit("; ===== End of Program =====");
    emit("HALT");
}

void CodeGen::translateInstr(const TACInstr& instr) {
    const std::string& op     = instr.op;
    const std::string& arg1   = instr.arg1;
    const std::string& arg2   = instr.arg2;
    const std::string& result = instr.result;

    // ── Label ───────────────────────────────────────────────
    if (op == "label") {
        emit(result + ":");
        return;
    }

    // ── Unconditional jump ──────────────────────────────────
    if (op == "goto") {
        emit("JMP   " + result);
        return;
    }

    // ── Conditional jump: if cond goto label ───────────────
    // cond is in a temp/var — if it is 0 (false), jump
    if (op == "if") {
        loadInto("R0", arg1);
        emit("JZ    R0, " + result);   // jump if R0 == 0 (condition false)
        return;
    }

    // ── Read ────────────────────────────────────────────────
    if (op == "read") {
        emit("INPUT R0");
        emit("STORE " + arg1 + ", R0");
        return;
    }

    // ── Write ───────────────────────────────────────────────
    if (op == "write") {
        loadInto("R0", arg1);
        emit("PRINT R0");
        return;
    }

    // ── Param (push argument before a call) ─────────────────
    if (op == "param") {
        loadInto("R0", arg1);
        emit("PUSH  R0");
        return;
    }

    // ── Call ────────────────────────────────────────────────
    if (op == "call") {
        emit("CALL  " + arg1 + ", " + arg2);   // CALL procname, argc
        if (!result.empty())                    // function: store return value
            emit("STORE " + result + ", R0");
        return;
    }

    // ── Copy: result = arg1 (no op, no arg2) ────────────────
    if (op.empty()) {
        loadInto("R0", arg1);
        emit("STORE " + result + ", R0");
        return;
    }

    // ── Unary operators ─────────────────────────────────────
    if (arg2.empty()) {
        loadInto("R0", arg1);

        if      (op == "-")   emit("NEG   R0");
        else if (op == "not") emit("NOT   R0");
        else                  emit("; unknown unary: " + op);

        emit("STORE " + result + ", R0");
        return;
    }

    // ── Binary operators ────────────────────────────────────
    loadInto("R0", arg1);
    loadInto("R1", arg2);

    if      (op == "+")  emit("ADD   R0, R0, R1");
    else if (op == "-")  emit("SUB   R0, R0, R1");
    else if (op == "*")  emit("MUL   R0, R0, R1");
    else if (op == "/")  emit("DIV   R0, R0, R1");
    else if (op == "and")emit("AND   R0, R0, R1");
    else if (op == "or") emit("OR    R0, R0, R1");
    // Relational ops — result is 1 (true) or 0 (false) in R0
    else if (op == "==") emit("CEQ   R0, R0, R1");   // compare equal
    else if (op == "<>") emit("CNE   R0, R0, R1");   // compare not equal
    else if (op == "<")  emit("CLT   R0, R0, R1");   // compare less than
    else if (op == "<=") emit("CLE   R0, R0, R1");   // compare less/equal
    else if (op == ">")  emit("CGT   R0, R0, R1");   // compare greater
    else if (op == ">=") emit("CGE   R0, R0, R1");   // compare greater/equal
    else                 emit("; unknown op: " + op);

    emit("STORE " + result + ", R0");
}

// ── Print ──────────────────────────────────────────────────
void CodeGen::print() const {
    std::cout << "\n====== TARGET CODE (Pseudo-Assembly) ======\n";
    for (size_t i = 0; i < targetCode.size(); i++) {
        // Labels and comments flush-left; instructions indented
        const std::string& line = targetCode[i];
        bool isLabel   = !line.empty() && line.back() == ':';
        bool isComment = line.size() >= 1 && line[0] == ';';

        if (isLabel || isComment)
            std::cout << std::setw(4) << i << "  " << line << "\n";
        else
            std::cout << std::setw(4) << i << "      " << line << "\n";
    }
}