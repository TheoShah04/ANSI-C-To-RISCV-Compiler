#pragma once
#include "ast_node.hpp"
#include "ast_type_specifier.hpp"
#include "Identifier.hpp"
#include "Declarator.hpp"
#include "Visitor.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <ostream>

namespace ast {

class Expression : public Node {
public:
    virtual ~Expression() = default;

    virtual TypeSpecifier getType() const {
        return getType(nullptr);
    }

    /* Have to pass context as an argument to get function return type*/
    virtual TypeSpecifier getType(const Context* context) const {
        (void)context;
        return TypeSpecifier::INT;
    }

    virtual TypePointer getTypePointer(const Context* context) const {
        return TypePointer(getType(context));
    }
    // pointer arithmetic
    virtual int getTypeSize(const Context* context) const {
        return context->getTypeSize(getType(context));
    }

    virtual const LiteralExpression* asLiteralExpression() const { return nullptr;}
    virtual const IdentifierExpression* asIdentifierExpression() const { return nullptr;}
    virtual const BinaryExpression* asBinaryExpression() const { return nullptr;}
    virtual const CallExpression* asCallExpression() const { return nullptr;}
    virtual const UnaryExpression* asUnaryExpression() const { return nullptr;}
    virtual const ArrayAccessExpression* asArrayAccessExpression() const { return nullptr;}
    virtual const InitializerList* asInitializerList() const { return nullptr;}
};

using ExprPtr = std::shared_ptr<Expression>;

namespace BinaryOp {
    enum Type {
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        LT,
        GT,
        LE,
        GE,
        EQ,
        NE,
        AND,
        OR,
        XOR,
        LOGICAL_AND,
        LOGICAL_OR,
        LEFT_SHIFT,
        RIGHT_SHIFT
    };
}

namespace AssignOp {
    enum Type {
        ASSIGN,
        MUL_ASSIGN,
        DIV_ASSIGN,
        MOD_ASSIGN,
        ADD_ASSIGN,
        SUB_ASSIGN,
        LEFT_ASSIGN,
        RIGHT_ASSIGN,
        AND_ASSIGN,
        XOR_ASSIGN,
        OR_ASSIGN
    };
}

class BinaryExpression : public Expression {
private:
    BinaryOp::Type op;
    ExprPtr left;
    ExprPtr right;
    TypeSpecifier resultType;

public:
    BinaryExpression(Expression* lhs, Expression* rhs, BinaryOp::Type operation)
        : op(operation),
          left(ExprPtr(lhs)),
          right(ExprPtr(rhs)) {
        resultType = left->getType();
    }

    BinaryExpression(ExprPtr lhs, ExprPtr rhs, BinaryOp::Type operation)
        : op(operation), left(std::move(lhs)), right(std::move(rhs)) {
        resultType = left->getType();
    }

    const BinaryExpression* asBinaryExpression() const override { return this; }

    BinaryOp::Type getOperator() const { return op; }
    const Expression* getLeft() const { return left.get(); }
    const Expression* getRight() const { return right.get(); }

    TypeSpecifier getType() const override { return resultType; }

    void accept(Visitor& visitor) const override;
};

// Unary operations
namespace UnaryOp {
    enum Type {
        PLUS,
        MINUS,
        LOGICAL_NOT,
        BITWISE_NOT,
        ADDRESS_OF,
        DEREFERENCE,
        PRE_INCREMENT,
        PRE_DECREMENT,
        POST_INCREMENT,
        POST_DECREMENT
    };
}

class UnaryExpression : public Expression {
private:
    UnaryOp::Type op;
    ExprPtr operand;
    TypeSpecifier resultType;

public:
    UnaryExpression(Expression* expr, UnaryOp::Type operation)
        : op(operation), operand(ExprPtr(expr)) {
        resultType = operand->getType();
    }

    UnaryExpression(ExprPtr expr, UnaryOp::Type operation)
        : op(operation), operand(std::move(expr)) {
        resultType = operand->getType();
    }

    const UnaryExpression* asUnaryExpression() const override { return this; }

    UnaryOp::Type getOperator() const { return op; }
    const Expression* getOperand() const { return operand.get(); }

    TypeSpecifier getType() const override { return resultType; }

    void accept(Visitor& visitor) const override;
};

class LiteralExpression : public Expression {
private:
    TypeSpecifier type;
    union {
        int intValue;
        float floatValue;
        double doubleValue;
        char charValue;
    };

public:
    LiteralExpression(int value)
        : type(TypeSpecifier::INT), intValue(value) {}

    LiteralExpression(float value)
        : type(TypeSpecifier::FLOAT), floatValue(value) {}

    LiteralExpression(double value)
        : type(TypeSpecifier::DOUBLE), doubleValue(value) {}

    LiteralExpression(char value)
        : type(TypeSpecifier::CHAR), charValue(value) {}

    TypeSpecifier getType() const override { return type; }

    const LiteralExpression* asLiteralExpression() const override { return this;}

    int getIntValue() const {
        if (type != TypeSpecifier::INT)
            throw std::runtime_error("Not an integer literal");
        return intValue;
    }

    float getFloatValue() const {
        if (type != TypeSpecifier::FLOAT)
            throw std::runtime_error("Not a float literal");
        return floatValue;
    }

    double getDoubleValue() const {
        if (type != TypeSpecifier::DOUBLE)
            throw std::runtime_error("Not a double literal");
        return doubleValue;
    }

    char getCharValue() const {
        if (type != TypeSpecifier::CHAR)
            throw std::runtime_error("Not a char literal");
        return charValue;
    }

    void accept(Visitor& visitor) const override;
};

class StringLiteralExpression : public Expression {
private:
    std::string value;
    int size;

public:
    StringLiteralExpression(const std::string& str) : value(str) {}

    const std::string& getValue() const { return value; }
    const int& getSize() { size = value.size() - 1;  return size; } //Excludes "" and includes null element
    TypeSpecifier getType() const override { return TypeSpecifier::CHAR; } // Char pointer technically

    void accept(Visitor& visitor) const override;
};

class IdentifierExpression : public Expression {
private:
    std::shared_ptr<Identifier> identifier;
    TypeSpecifier type;

public:
    IdentifierExpression(std::shared_ptr<Identifier> id)
        : identifier(std::move(id)), type() {}

    IdentifierExpression(Identifier* id)
        : identifier(std::shared_ptr<Identifier>(id)), type() {}

    const IdentifierExpression* asIdentifierExpression() const override { return this;}

    const Identifier* getIdentifier() const { return identifier.get(); }
    const std::string& getName() const { return identifier->getName(); }
    TypeSpecifier getType() const override { return type; }
    void setType(TypeSpecifier t) { type = t; }

    void accept(Visitor& visitor) const override;
};

class CallExpression : public Expression {
private:
    ExprPtr function;
    std::shared_ptr<NodeList> arguments;

public:
    CallExpression(Expression* func, NodeList* args = nullptr)
        : function(ExprPtr(func)), arguments(std::shared_ptr<NodeList>(args)) {}

    CallExpression(ExprPtr func, std::shared_ptr<NodeList> args = nullptr)
        : function(std::move(func)), arguments(std::move(args)) {}

    const CallExpression* asCallExpression() const override { return this;}

    const Expression* getFunction() const { return function.get(); }
    const NodeList* getArguments() const { return arguments.get(); }
    bool hasArguments() const { return arguments != nullptr; }

    TypeSpecifier getType() const override { return TypeSpecifier::INT; }

    // Have to more thoroughly check if function return types properly implemented
    TypeSpecifier getType(const Context* context) const override {
        if(!context) return TypeSpecifier::INT;
        if (const IdentifierExpression* idExpr = dynamic_cast<const IdentifierExpression*>(function.get())) {
            const std::string& funcName = idExpr->getName();
            if (context->functionExists(funcName)) {
                return context->getFunctionReturnType(funcName);
            }
        }
        return TypeSpecifier::INT;
    }

    void accept(Visitor& visitor) const override;
};

// Assignment expression (x = expr)
class AssignmentExpression : public Expression {
private:
    ExprPtr lhs;
    ExprPtr rhs;
    AssignOp::Type op;
    TypeSpecifier resultType;

public:
    AssignmentExpression(Expression* left, Expression* right, AssignOp::Type operation)
        : lhs(ExprPtr(left)), rhs(ExprPtr(right)), op(operation) {
        resultType = rhs->getType();
    }

    AssignmentExpression(ExprPtr left, ExprPtr right, AssignOp::Type operation)
        : lhs(std::move(left)), rhs(std::move(right)), op(operation) {
        resultType = rhs->getType();
    }

    const Expression* getLHS() const { return lhs.get(); }
    const Expression* getRHS() const { return rhs.get(); }
    std::string getVariableName() const;
    AssignOp::Type getOperator() const { return op; }
    TypeSpecifier getType() const override { return resultType; }

    void accept(Visitor& visitor) const override;
};

class InitializerList : public Expression {
private:
    std::vector<std::shared_ptr<Expression>> expressions;

public:
    InitializerList() = default;

    const InitializerList* asInitializerList() const override { return this;}
    void addExpression(std::shared_ptr<Expression> expr) {expressions.push_back(std::move(expr));}
    const std::vector<std::shared_ptr<Expression>>& getExpressions() const {return expressions;}
    TypeSpecifier getType() const override {return TypeSpecifier::INT;}  // TODO implement support for other types

    void accept(Visitor& visitor) const override {visitor.visitInitializerList(*this);}
};

class ArrayAccessExpression : public Expression {
private:
    ExprPtr array;
    ExprPtr index;
    TypeSpecifier resultType;

public:
    ArrayAccessExpression(Expression* arr, Expression* idx)
        : array(ExprPtr(arr)), index(ExprPtr(idx)) {
        // result type should be set to type of array element
        resultType = array->getType();
    }

    ArrayAccessExpression(ExprPtr arr, ExprPtr idx)
        : array(std::move(arr)), index(std::move(idx)) {
        resultType = array->getType();
    }

    const ArrayAccessExpression* asArrayAccessExpression() const override { return this;}

    const Expression* getArray() const { return array.get(); }
    const Expression* getIndex() const { return index.get(); }
    TypeSpecifier getType() const override {return resultType; }

    void accept(Visitor& visitor) const override;
};

// struct member access
class MemberAccessExpression : public Expression {
private:
    std::shared_ptr<Expression> object;
    std::shared_ptr<Identifier> member;
    TypeSpecifier resultType;

public:
    MemberAccessExpression(std::shared_ptr<Expression> obj, std::shared_ptr<Identifier> id)
    : object(std::move(obj)), member(std::move(id)), resultType(TypeSpecifier::INT) {}

    const Expression* getObject() const { return object.get(); }
    const Identifier* getMember() const { return member.get(); }
    TypeSpecifier getType() const override { return resultType; }
    void setType(TypeSpecifier type) { resultType = type; }

    void accept(Visitor& visitor) const override;
};

// Pointer member access expression (ptr->member)
class PointerMemberAccessExpression : public Expression {
private:
    std::shared_ptr<Expression> object;
    std::shared_ptr<Identifier> member;
    TypeSpecifier resultType;

public:
    PointerMemberAccessExpression(std::shared_ptr<Expression> obj, std::shared_ptr<Identifier> id)
        : object(std::move(obj)), member(std::move(id)), resultType(TypeSpecifier::INT) {}

    PointerMemberAccessExpression(Expression* obj, Identifier* id)
        : object(ExprPtr(obj)), member(std::shared_ptr<Identifier>(id)), resultType(TypeSpecifier::INT) {} //CHECK INT

    const Expression* getObject() const { return object.get(); }
    const Identifier* getMemberName() const { return member.get(); }
    TypeSpecifier getType() const override { return resultType; }
    void setType(TypeSpecifier type) { resultType = type; }

    void accept(Visitor& visitor) const override;
};

// Cast expression ((type)expr)
class CastExpression : public Expression {
private:
    TypeSpecifier targetType;
    ExprPtr expr;

public:
    CastExpression(TypeSpecifier type, Expression* expression)
        : targetType(type), expr(ExprPtr(expression)) {}

    CastExpression(TypeSpecifier type, ExprPtr expression)
        : targetType(type), expr(std::move(expression)) {}

    TypeSpecifier getType() const override { return targetType; }
    const Expression* getExpression() const { return expr.get(); }

    void accept(Visitor& visitor) const override;
};

class ConditionalExpression : public Expression {
private:
    ExprPtr condition;
    ExprPtr thenExpr;
    ExprPtr elseExpr;
    TypeSpecifier resultType;

public:
    ConditionalExpression(Expression* cond, Expression* thenE, Expression* elseE)
        : condition(ExprPtr(cond)), thenExpr(ExprPtr(thenE)), elseExpr(ExprPtr(elseE)) {
       // resultType = thenExpr->getType();
    }

    ConditionalExpression(ExprPtr cond, ExprPtr thenE, ExprPtr elseE)
        : condition(std::move(cond)), thenExpr(std::move(thenE)), elseExpr(std::move(elseE)) {
        // resultType = thenExpr->getType();
    }

    const Expression* getCondition() const { return condition.get(); }
    const Expression* getThenExpression() const { return thenExpr.get(); }
    const Expression* getElseExpression() const { return elseExpr.get(); }
    TypeSpecifier getType() const override {const TypeSpecifier resultType = thenExpr->getType(); return resultType; }

    void accept(Visitor& visitor) const override;
};

class CommaExpression : public Expression {
private:
    ExprPtr left;
    ExprPtr right;

public:
    CommaExpression(Expression* lhs, Expression* rhs)
        : left(ExprPtr(lhs)), right(ExprPtr(rhs)) {}

    CommaExpression(ExprPtr lhs, ExprPtr rhs)
        : left(std::move(lhs)), right(std::move(rhs)) {}

    const Expression* getLeft() const { return left.get(); }
    const Expression* getRight() const { return right.get(); }

    TypeSpecifier getType() const override { return right->getType(); }

    void accept(Visitor& visitor) const override;
};

class SizeofExpression : public Expression {
private:
    ExprPtr expr;

public:
    SizeofExpression(Expression* e) : expr(ExprPtr(e)) {}
    SizeofExpression(ExprPtr e) : expr(std::move(e)) {}

    const Expression* getExpression() const { return expr.get(); }

    TypeSpecifier getType() const override { return expr->getType(); }

    void accept(Visitor& visitor) const override;
};

class SizeofTypeExpression : public Expression {
private:
    TypeSpecifier targetType;

public:
    SizeofTypeExpression(TypeSpecifier type) : targetType(type) {}

    TypeSpecifier getTargetType() const { return targetType; }

    //TypeSpecifier getType() const override { return context.getTypeSize(); }

    void accept(Visitor& visitor) const override;
};

inline void BinaryExpression::accept(Visitor& visitor) const { visitor.visitBinaryExpression(*this); }
inline void UnaryExpression::accept(Visitor& visitor) const { visitor.visitUnaryExpression(*this); }
inline void LiteralExpression::accept(Visitor& visitor) const { visitor.visitLiteralExpression(*this); }
inline void StringLiteralExpression::accept(Visitor& visitor) const { visitor.visitStringLiteralExpression(*this); }
inline void IdentifierExpression::accept(Visitor& visitor) const { visitor.visitIdentifierExpression(*this); }
inline void CallExpression::accept(Visitor& visitor) const { visitor.visitCallExpression(*this); }
inline void AssignmentExpression::accept(Visitor& visitor) const { visitor.visitAssignmentExpression(*this); }
inline void ArrayAccessExpression::accept(Visitor& visitor) const { visitor.visitArrayAccessExpression(*this); }
inline void MemberAccessExpression::accept(Visitor& visitor) const { visitor.visitMemberAccessExpression(*this); }
inline void PointerMemberAccessExpression::accept(Visitor& visitor) const { visitor.visitPointerMemberAccessExpression(*this); }
inline void CastExpression::accept(Visitor& visitor) const { visitor.visitCastExpression(*this); }
inline void ConditionalExpression::accept(Visitor& visitor) const { visitor.visitConditionalExpression(*this); }
inline void CommaExpression::accept(Visitor& visitor) const { visitor.visitCommaExpression(*this); }
inline void SizeofExpression::accept(Visitor& visitor) const { visitor.visitSizeofExpression(*this); }
inline void SizeofTypeExpression::accept(Visitor& visitor) const { visitor.visitSizeofTypeExpression(*this); }

} // namespace ast
