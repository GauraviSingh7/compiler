// ============================================================
// icg.h — Three Address Code + ICG engine
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

// ── One TAC instruction ────────────────────────────────────
struct TACInstr {
    std::string op;      // operator or instruction keyword
    std::string arg1;
    std::string arg2;
    std::string result;

    // Formats the instruction for display
    std::string toString() const {
        // Labels:   "L1:"
        if (op == "label")
            return result + ":";

        // Goto:     "goto L1"
        if (op == "goto")
            return "goto " + result;

        // Conditional: "if arg1 op arg2 goto result"
        if (op == "ifgoto")
            return "if " + arg1 + " " + arg2 + " " + result + " goto " + result;

        // Proper conditional format: we'll use a dedicated op
        if (op == "if")
            return "if " + arg1 + " goto " + result;

        // param / read / write
        if (op == "param")  return "param " + arg1;
        if (op == "read")   return "read "  + arg1;
        if (op == "write")  return "write " + arg1;

        // call proc, n
        if (op == "call" && result.empty())
            return "call " + arg1 + ", " + arg2;

        // result = call func, n
        if (op == "call")
            return result + " = call " + arg1 + ", " + arg2;

        // Unary:  result = op arg1
        if (arg2.empty() && !arg1.empty())
            return result + " = " + op + " " + arg1;

        // Binary: result = arg1 op arg2
        if (!arg2.empty())
            return result + " = " + arg1 + " " + op + " " + arg2;

        // Copy:   result = arg1
        return result + " = " + arg1;
    }
};

// ── ICG engine ─────────────────────────────────────────────
class ICG {
public:
    ICG() : tempCount(0), labelCount(0) {}

    // Generate a new unique temporary: t1, t2, ...
    std::string newTemp() {
        return "t" + std::to_string(++tempCount);
    }

    // Generate a new unique label: L1, L2, ...
    std::string newLabel() {
        return "L" + std::to_string(++labelCount);
    }

    // ── Emit helpers ───────────────────────────────────────

    // result = arg1 op arg2
    std::string emitBinary(const std::string& op,
                           const std::string& a1,
                           const std::string& a2) {
        std::string t = newTemp();
        code.push_back({ op, a1, a2, t });
        return t;
    }

    // result = op arg1  (unary)
    std::string emitUnary(const std::string& op, const std::string& a1) {
        std::string t = newTemp();
        code.push_back({ op, a1, "", t });
        return t;
    }

    // result = arg1  (copy)
    std::string emitCopy(const std::string& src, const std::string& dst) {
        code.push_back({ "", src, "", dst });
        return dst;
    }

    // result = arg1  (copy to new temp)
    std::string emitCopyTemp(const std::string& src) {
        std::string t = newTemp();
        code.push_back({ "", src, "", t });
        return t;
    }

    // if condition goto label
    void emitIf(const std::string& cond, const std::string& label) {
        code.push_back({ "if", cond, "", label });
    }

    // goto label
    void emitGoto(const std::string& label) {
        code.push_back({ "goto", "", "", label });
    }

    // label:
    void emitLabel(const std::string& label) {
        code.push_back({ "label", "", "", label });
    }

    // read x
    void emitRead(const std::string& var) {
        code.push_back({ "read", var, "", "" });
    }

    // write x
    void emitWrite(const std::string& val) {
        code.push_back({ "write", val, "", "" });
    }

    // param x
    void emitParam(const std::string& val) {
        code.push_back({ "param", val, "", "" });
    }

    // call proc/func, argcount
    std::string emitCall(const std::string& name, int argCount,
                         bool hasResult = false) {
        std::string t = hasResult ? newTemp() : "";
        code.push_back({ "call", name, std::to_string(argCount), t });
        return t;
    }

    // Backpatch: fill in a label that wasn't known when jump was emitted
    void backpatch(size_t instrIndex, const std::string& label) {
        if (instrIndex < code.size())
            code[instrIndex].result = label;
    }

    // Index of next instruction to be emitted (used for backpatching)
    size_t nextInstr() const { return code.size(); }

    void print() const {
        std::cout << "\n====== THREE ADDRESS CODE ======\n";
        for (size_t i = 0; i < code.size(); i++) {
            // Labels flush left; everything else indented
            if (code[i].op == "label")
                std::cout << std::setw(4) << i << "  " << code[i].toString() << "\n";
            else
                std::cout << std::setw(4) << i << "      " << code[i].toString() << "\n";
        }
    }

    const std::vector<TACInstr>& getCode() const { return code; }

private:
    std::vector<TACInstr> code;
    int tempCount;
    int labelCount;
};