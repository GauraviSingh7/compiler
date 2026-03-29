// ============================================================
// ast.h — Parse Tree Node
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <memory>

struct ASTNode {
    std::string label;                        // rule name or token value
    std::string kind;                         // "rule", "token", "error"
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode(const std::string& lbl, const std::string& knd = "rule")
        : label(lbl), kind(knd) {}

    // Add a child and return pointer to it
    std::shared_ptr<ASTNode> addChild(const std::string& lbl,
                                      const std::string& knd = "rule") {
        auto node = std::make_shared<ASTNode>(lbl, knd);
        children.push_back(node);
        return node;
    }

    // Add a pre-built child node
    void addChild(std::shared_ptr<ASTNode> node) {
        children.push_back(node);
    }
};

using ASTNodePtr = std::shared_ptr<ASTNode>;