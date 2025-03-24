// Declaration.hpp
#pragma once
#include "ast_node.hpp"
#include "ast_type_specifier.hpp"
#include "Declarator.hpp"
#include <string>
#include <memory>
#include <vector>

namespace ast {

class Declaration : public Node {
public:
    virtual ~Declaration() = default;
    virtual TypeSpecifier getType() const = 0;
};

class VariableDeclaration : public Declaration {
private:
    TypeSpecifier type;
    std::shared_ptr<Declarator> declarator;
    std::shared_ptr<Expression> initializer; // Optional

public:
    VariableDeclaration(TypeSpecifier t, std::shared_ptr<Declarator> decl,
                        std::shared_ptr<Expression> init = nullptr)
        : type(t), declarator(std::move(decl)), initializer(std::move(init)) {}

    TypeSpecifier getType() const override { return type; }
    std::string getIdentifier() const { return declarator->getIdentifier(); }
    std::shared_ptr<Declarator> getDeclarator() const {return declarator;}
    bool isPointer() const { return declarator && declarator->isPointer(); }
    bool isArray() const { return declarator->isArray(); }
    const Expression* getInitializer() const { return initializer.get(); }
    bool hasInitializer() const { return initializer != nullptr; }

    void accept(Visitor& visitor) const {
        visitor.visitVariableDeclaration(*this);
    }
};

class FunctionDeclaration : public Declaration {
private:
    TypeSpecifier returnType;
    std::shared_ptr<FunctionDeclarator> declarator;
    std::shared_ptr<CompoundStatement> body; // Optional (for definition)
    std::vector<std::shared_ptr<VariableDeclaration>> parameters;

public:
    FunctionDeclaration(TypeSpecifier retType, std::shared_ptr<FunctionDeclarator> decl,
                   std::shared_ptr<CompoundStatement> functionBody = nullptr)
    : returnType(retType), declarator(std::move(decl)), body(std::move(functionBody)) {

    if (declarator->getParameters()) {
        const auto& params = declarator->getParameters()->getParameters();
        for (const auto& param : params) {
            auto paramDecl = std::dynamic_pointer_cast<ParameterDeclaration>(param);
            if (paramDecl) {
                // Create a VariableDeclaration from the ParameterDeclaration
                auto varDecl = std::make_shared<VariableDeclaration>(
                    paramDecl->getType(),
                    paramDecl->getDeclaratorPtr()
                );
                parameters.push_back(varDecl);
            }
        }
    }
}

    TypeSpecifier getType() const override { return returnType; }
    bool getRetPtr() const { return declarator->isPointer(); }
    std::string getIdentifier() const { return declarator->getIdentifier(); }
    const std::vector<std::shared_ptr<VariableDeclaration>>& getParameters() const { return parameters; }
    const CompoundStatement* getBody() const { return body.get(); }
    bool hasBody() const { return body != nullptr; }

    void accept(Visitor& visitor) const {
        visitor.visitFunctionDeclaration(*this);
    }
};


} // namespace ast
