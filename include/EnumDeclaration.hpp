#pragma once
#include "ast_node.hpp"
#include "ast_type_specifier.hpp"
#include "Identifier.hpp"
#include <string>
#include <memory>
#include <vector>

namespace ast {

class EnumValue : public Node {
private:
    std::shared_ptr<Identifier> name;
    std::shared_ptr<Expression> value;

public:
    EnumValue(std::shared_ptr<Identifier> name, std::shared_ptr<Expression> value = nullptr)
        : name(std::move(name)), value(std::move(value)) {}

    std::string getName() const { return name->getName(); }
    bool hasValue() const { return value != nullptr; }
    const Expression* getValue() const { return value.get(); }

    void accept(Visitor& visitor) const override {
        visitor.visitEnumValue(*this);
    }
};

class EnumDeclaration : public Declaration {
private:
    std::shared_ptr<Identifier> name;
    std::vector<std::shared_ptr<EnumValue>> values;

public:
    EnumDeclaration(std::shared_ptr<Identifier> name = nullptr)
        : name(std::move(name)) {}

    void addValue(std::shared_ptr<EnumValue> value) {
        values.push_back(std::move(value));
    }

    bool hasName() const { return name != nullptr; }
    std::string getName() const { return hasName() ? name->getName() : ""; }
    const std::vector<std::shared_ptr<EnumValue>>& getValues() const { return values; }

    TypeSpecifier getType() const override { return TypeSpecifier::ENUM; }

    void accept(Visitor& visitor) const override {
        visitor.visitEnumDeclaration(*this);
    }
};

} // namespace ast
