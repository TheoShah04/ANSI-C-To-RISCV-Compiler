#pragma once
#include "Statement.hpp"
#include "Declaration.hpp"
#include <memory>

namespace ast {

//represents variable declarations in compound statements.
class DeclarationStatement : public Statement {
private:
    std::shared_ptr<Declaration> declaration;

public:
    DeclarationStatement(std::shared_ptr<Declaration> decl) : declaration(decl) {}

    const Declaration* getDeclaration() const { return declaration.get(); }
    std::shared_ptr<Declaration> getDeclarationPtr() const { return declaration; }

    void accept(Visitor& visitor) const override;

    void Print(std::ostream& stream) const override {
        stream << "DeclarationStatement(";
        declaration->Print(stream);
        stream << ")";
    }
};

inline void DeclarationStatement::accept(Visitor& visitor) const {
    declaration->accept(visitor);
}

} // namespace ast
