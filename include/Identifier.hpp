#pragma once
#include "ast_node.hpp"
#include "Visitor.hpp"
#include <string>

namespace ast {

class Identifier : public Node {
private:
    std::string name;

public:
    Identifier(const std::string& n) : name(n) {}

    const std::string& getName() const { return name; }

    void accept(Visitor& visitor) const override {
        (void)visitor; // should not be calling this
    }
};
}
