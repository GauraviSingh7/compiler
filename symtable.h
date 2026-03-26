// ============================================================
// symtable.h — Symbol Table
// ============================================================
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

enum class SymbolKind { VARIABLE, ARRAY, FUNCTION, PROCEDURE };

struct Symbol {
    std::string name;
    SymbolKind  kind;
    int         arraySize;  // only for ARRAY kind
    // Extend later for function param types etc.
};

class SymbolTable {
public:
    // Insert a symbol; warn if already declared
    void insert(const std::string& name, SymbolKind kind, int arraySize = 0) {
        if (table.count(name)) {
            std::cerr << "Warning: '" << name << "' already declared.\n";
            return;
        }
        table[name] = { name, kind, arraySize };
        order.push_back(name);
    }

    // Lookup — returns nullptr if not found
    const Symbol* lookup(const std::string& name) const {
        auto it = table.find(name);
        if (it == table.end()) return nullptr;
        return &it->second;
    }

    void print() const {
        std::cout << "\n====== SYMBOL TABLE ======\n";
        std::cout << std::left;
        for (auto& nm : order) {
            auto& s = table.at(nm);
            std::cout << "  " << s.name;
            if (s.kind == SymbolKind::ARRAY)
                std::cout << "  [ARRAY size=" << s.arraySize << "]";
            else if (s.kind == SymbolKind::FUNCTION)
                std::cout << "  [FUNCTION]";
            else if (s.kind == SymbolKind::PROCEDURE)
                std::cout << "  [PROCEDURE]";
            else
                std::cout << "  [INTEGER]";
            std::cout << "\n";
        }
    }
    std::vector<Symbol> getEntries() const {
    std::vector<Symbol> result;
    for (auto& nm : order)
        result.push_back(table.at(nm));
    return result;
}

private:
    std::unordered_map<std::string, Symbol> table;
    std::vector<std::string> order;  // preserve insertion order for printing
};