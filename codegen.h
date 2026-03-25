// ============================================================
// codegen.h — Code Generator declaration
// ============================================================
#pragma once
#include "icg.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

class CodeGen {
public:
    explicit CodeGen(const ICG& icg);

    // Translate all TAC into target instructions
    void generate();

    // Print the generated target code
    void print() const;

    const std::vector<std::string>& getCode() const { return targetCode; }

private:
    const ICG&               icg;
    std::vector<std::string> targetCode;

    // Emit one target instruction
    void emit(const std::string& instr);

    // Translate a single TAC instruction
    void translateInstr(const TACInstr& instr);

    // Helpers
    bool isConstant(const std::string& s) const;  // is it a numeric literal?
    bool isString(const std::string& s) const;     // is it a quoted string?

    // Load a value (var, temp, or constant) into a register
    void loadInto(const std::string& reg, const std::string& val);
};