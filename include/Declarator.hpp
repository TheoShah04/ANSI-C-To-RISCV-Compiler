#pragma once
#include "ast_node.hpp"
#include "ast_type_specifier.hpp"
#include <string>
#include <memory>
#include <vector>

namespace ast {

class Expression;
class Visitor;

class Declarator : public Node {
public:
    virtual ~Declarator() = default;
    virtual std::string getIdentifier() const = 0;
    virtual bool isPointer() const { return false; }
    virtual bool isArray() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool isFunction() const { return false; }

    virtual const IdentifierDeclarator* asIdentifierDeclarator() const {return nullptr;}
    virtual const ArrayDeclarator* asArrayDeclarator() const {return nullptr;}
    virtual const PointerDeclarator* asPointerDeclarator() const {return nullptr;}
    virtual const FunctionDeclarator* asFunctionDeclarator() const {return nullptr;}
};

// same as variableDeclarator
class IdentifierDeclarator : public Declarator {
private:
    std::string identifier;

public:
    IdentifierDeclarator(const std::string& id) : identifier(id) {}

    const IdentifierDeclarator* asIdentifierDeclarator() const override {return this;}
    std::string getIdentifier() const override { return identifier; }

    void accept(Visitor& visitor) const override;
};

class InitDeclarator : public Node {
private:
    std::shared_ptr<Declarator> declarator;
    std::shared_ptr<Expression> initializer; // Optional

public:
    InitDeclarator(std::shared_ptr<Declarator> decl, std::shared_ptr<Expression> init = nullptr)
        : declarator(std::move(decl)), initializer(std::move(init)) {}

    std::shared_ptr<Declarator> getDeclarator() const { return declarator; }
    std::shared_ptr<Expression> getInitializer() const { return initializer; }

    void accept(Visitor& visitor) const override;

    void Print(std::ostream& stream) const override {
        stream << "InitDeclarator(";
        if (declarator) {
            declarator->Print(stream);
        }
        if (initializer) {
            stream << " = <expr>";
        }
        stream << ")";
    }
};

class InitDeclaratorList : public NodeList {
public:
    InitDeclaratorList() = default;
};

class ArrayDeclarator : public Declarator {
private:
    std::shared_ptr<Declarator> baseDeclarator;
    std::shared_ptr<Expression> size; // Optional

public:
    ArrayDeclarator(std::shared_ptr<Declarator> base, std::shared_ptr<Expression> arraySize = nullptr)
        : baseDeclarator(std::move(base)), size(std::move(arraySize)) {}

    std::string getIdentifier() const override {
        return baseDeclarator ? baseDeclarator->getIdentifier() : "";
    }
    const ArrayDeclarator* asArrayDeclarator() const override {return this;}
    bool isArray() const override { return true; }
    const Declarator* getBaseDeclarator() const {return baseDeclarator.get();}
    std::shared_ptr<Declarator> getBaseDeclaratorPtr() const {return baseDeclarator;}
    const Expression* getSize() const {return size.get();}

    void accept(Visitor& visitor) const override;
};

class ParameterList : public Node {
private:
    std::vector<std::shared_ptr<Node>> parameters;

public:
    ParameterList() = default;

    void addParameter(std::shared_ptr<Node> param) {
        parameters.push_back(std::move(param));
    }

    const std::vector<std::shared_ptr<Node>>& getParameters() const {
        return parameters;
    }

    size_t size() const {
        return parameters.size();
    }

    void accept(Visitor& visitor) const override;

    void Print(std::ostream& stream) const override {
        stream << "ParameterList(";
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) stream << ", ";
            parameters[i]->Print(stream);
        }
        stream << ")";
    }
};

class FunctionDeclarator : public Declarator {
private:
    std::shared_ptr<Declarator> baseDeclarator;
    std::shared_ptr<ParameterList> parameters; // Optional

public:
    FunctionDeclarator(Declarator* base, ParameterList* params = nullptr)
    : baseDeclarator(std::shared_ptr<Declarator>(base)),
    parameters(std::shared_ptr<ParameterList>(params)) {}

    FunctionDeclarator(std::shared_ptr<Declarator> base, std::shared_ptr<ParameterList> params = nullptr)
        : baseDeclarator(std::move(base)), parameters(std::move(params)) {}

    std::string getIdentifier() const override {
        return baseDeclarator ? baseDeclarator->getIdentifier() : "";
    }

    const FunctionDeclarator* asFunctionDeclarator() const override {return this;}

    bool isFunction() const override { return true; }
    bool isPointer() const override { return baseDeclarator->isPointer();}

    const Declarator* getBaseDeclarator() const {return baseDeclarator.get();}

    std::shared_ptr<Declarator> getBaseDeclaratorPtr() const {return baseDeclarator;}

    const ParameterList* getParameters() const {return parameters.get();}

    void accept(Visitor& visitor) const override;
};

class ParameterDeclaration : public Node {
private:
    TypeSpecifier type;
    std::shared_ptr<Declarator> declarator;

public:
    ParameterDeclaration(TypeSpecifier t, std::shared_ptr<Declarator> decl = nullptr)
        : type(t), declarator(std::move(decl)) {}

    TypeSpecifier getType() const { return type; }
    std::string getIdentifier() const {
        if (!declarator) return "";
        return declarator->getIdentifier();
    }

    bool isPointer() const {
        if (!declarator) return false;
        // direct pointer declarator?
        if (auto pointerDecl = std::dynamic_pointer_cast<PointerDeclarator>(declarator)){
            return true;
        }
        return declarator->isPointer();
    }
    bool hasDeclarator() const {return declarator != nullptr;}

    const Declarator* getDeclarator() const {return declarator.get();}
    std::shared_ptr<Declarator> getDeclaratorPtr() const {return declarator;}

    void accept(Visitor& visitor) const override;
};

class PointerDeclarator : public Declarator {
private:
    std::shared_ptr<Declarator> baseDeclarator;

public:
    PointerDeclarator(std::shared_ptr<Declarator> base)
        : baseDeclarator(std::move(base)) {}

    std::string getIdentifier() const override {
        return baseDeclarator ? baseDeclarator->getIdentifier() : "";
    }

    const PointerDeclarator* asPointerDeclarator() const override {return this;}

    bool isPointer() const override { return true; }

    const Declarator* getBaseDeclarator() const {return baseDeclarator.get();}

    std::shared_ptr<Declarator> getBaseDeclaratorPtr() const {
        return baseDeclarator;
    }

    void accept(Visitor& visitor) const override;
};

inline void IdentifierDeclarator::accept(Visitor& visitor) const { visitor.visitIdentifierDeclarator(*this); }
inline void PointerDeclarator::accept(Visitor& visitor) const { visitor.visitPointerDeclarator(*this); }
inline void ArrayDeclarator::accept(Visitor& visitor) const { visitor.visitArrayDeclarator(*this); }
inline void FunctionDeclarator::accept(Visitor& visitor) const { visitor.visitFunctionDeclarator(*this); }
inline void ParameterList::accept(Visitor& visitor) const { visitor.visitParameterList(*this); }
inline void ParameterDeclaration::accept(Visitor& visitor) const { visitor.visitParameterDeclaration(*this); }
inline void InitDeclarator::accept(Visitor& visitor) const { visitor.visitInitDeclarator(*this); }


} // namespace ast
