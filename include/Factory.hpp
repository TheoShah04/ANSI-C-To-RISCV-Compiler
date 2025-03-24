#pragma once
#include "ast_node.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include "Declaration.hpp"
#include "Declarator.hpp"
#include "Identifier.hpp"
#include "EnumDeclaration.hpp"
#include <memory>
#include <string>

namespace ast {

/*
   Factory functions that consistenly return shared_ptr
   better memory management and avoids static_cast and dynamic_casting in the parser
   No more direct calls to new object and wayyy faster compile time
*/

inline std::shared_ptr<NodeList> makeNodeList() {
    return std::make_shared<NodeList>();
}

inline std::shared_ptr<Identifier> makeIdentifier(const std::string& name) {
    return std::make_shared<Identifier>(name);
}

// Declarators
inline std::shared_ptr<IdentifierDeclarator> makeIdentifierDeclarator(const std::string& id) {
    return std::make_shared<IdentifierDeclarator>(id);
}

inline std::shared_ptr<PointerDeclarator> makePointerDeclarator(std::shared_ptr<Declarator> base) {
    return std::make_shared<PointerDeclarator>(std::move(base));
}

inline std::shared_ptr<ArrayDeclarator> makeArrayDeclarator(std::shared_ptr<Declarator> base,
                                                           std::shared_ptr<Expression> size = nullptr) {
    return std::make_shared<ArrayDeclarator>(std::move(base), std::move(size));
}

inline std::shared_ptr<FunctionDeclarator> makeFunctionDeclarator(std::shared_ptr<Declarator> base,
                                                                 std::shared_ptr<ParameterList> params = nullptr) {
    return std::make_shared<FunctionDeclarator>(std::move(base), std::move(params));
}

// Parameter handling
inline std::shared_ptr<ParameterList> makeParameterList() {
    return std::make_shared<ParameterList>();
}

inline std::shared_ptr<ParameterDeclaration> makeParameterDeclaration(
    TypeSpecifier type, std::shared_ptr<Declarator> decl = nullptr) {
    return std::make_shared<ParameterDeclaration>(type, std::move(decl));
}

// Initializers
inline std::shared_ptr<InitDeclarator> makeInitDeclarator(
    std::shared_ptr<Declarator> decl, std::shared_ptr<Expression> init = nullptr) {
    return std::make_shared<InitDeclarator>(std::move(decl), std::move(init));
}

inline std::shared_ptr<InitDeclaratorList> makeInitDeclaratorList() {
    return std::make_shared<InitDeclaratorList>();
}

// Declarations
inline std::shared_ptr<VariableDeclaration> makeVariableDeclaration(
    TypeSpecifier type, std::shared_ptr<Declarator> decl, std::shared_ptr<Expression> init = nullptr) {
    return std::make_shared<VariableDeclaration>(type, std::move(decl), std::move(init));
}

inline std::shared_ptr<FunctionDeclaration> makeFunctionDeclaration(
    TypeSpecifier returnType, std::shared_ptr<FunctionDeclarator> decl,
    std::shared_ptr<CompoundStatement> body = nullptr) {
    return std::make_shared<FunctionDeclaration>(returnType, std::move(decl), std::move(body));
}

// Enum-related
inline std::shared_ptr<EnumDeclaration> makeEnumDeclaration(
    std::shared_ptr<Identifier> id = nullptr) {
    return std::make_shared<EnumDeclaration>(std::move(id));
}

inline std::shared_ptr<EnumValue> makeEnumValue(
    std::shared_ptr<Identifier> id, std::shared_ptr<Expression> value = nullptr) {
    return std::make_shared<EnumValue>(std::move(id), std::move(value));
}

// Expressions
inline std::shared_ptr<IdentifierExpression> makeIdentifierExpression(std::shared_ptr<Identifier> id) {
    return std::make_shared<IdentifierExpression>(id);
}

inline std::shared_ptr<LiteralExpression> makeLiteralExpression(int value) {
    return std::make_shared<LiteralExpression>(value);
}

inline std::shared_ptr<LiteralExpression> makeLiteralExpression(float value) {
    return std::make_shared<LiteralExpression>(value);
}

inline std::shared_ptr<LiteralExpression> makeLiteralExpression(double value) {
    return std::make_shared<LiteralExpression>(value);
}

inline std::shared_ptr<LiteralExpression> makeLiteralExpression(char value) {
    return std::make_shared<LiteralExpression>(value);
}

inline std::shared_ptr<StringLiteralExpression> makeStringLiteralExpression(const std::string& value) {
    return std::make_shared<StringLiteralExpression>(value);
}

inline std::shared_ptr<BinaryExpression> makeBinaryExpression(
    std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, BinaryOp::Type op) {
    return std::make_shared<BinaryExpression>(std::move(left), std::move(right), op);
}

inline std::shared_ptr<UnaryExpression> makeUnaryExpression(
    std::shared_ptr<Expression> expr, UnaryOp::Type op) {
    return std::make_shared<UnaryExpression>(std::move(expr), op);
}

inline std::shared_ptr<AssignmentExpression> makeAssignmentExpression(
    std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, AssignOp::Type op) {
    return std::make_shared<AssignmentExpression>(std::move(left), std::move(right), op);
}

inline std::shared_ptr<CallExpression> makeCallExpression(
    std::shared_ptr<Expression> func, std::shared_ptr<NodeList> args = nullptr) {
    return std::make_shared<CallExpression>(std::move(func), std::move(args));
}

inline std::shared_ptr<ArrayAccessExpression> makeArrayAccessExpression(
    std::shared_ptr<Expression> array, std::shared_ptr<Expression> index) {
    return std::make_shared<ArrayAccessExpression>(std::move(array), std::move(index));
}

inline std::shared_ptr<MemberAccessExpression> makeMemberAccessExpression(
    std::shared_ptr<Expression> object, std::shared_ptr<Identifier> member) {
    return std::make_shared<MemberAccessExpression>(std::move(object), std::move(member));
}

inline std::shared_ptr<PointerMemberAccessExpression> makePointerMemberAccessExpression(
    std::shared_ptr<Expression> object, std::shared_ptr<Identifier> member) {
    return std::make_shared<PointerMemberAccessExpression>(std::move(object), std::move(member));
}

inline std::shared_ptr<CastExpression> makeCastExpression(
    TypeSpecifier type, std::shared_ptr<Expression> expr) {
    return std::make_shared<CastExpression>(type, std::move(expr));
}

// inline std::shared_ptr<ConditionalExpression> makeConditionalExpression(
//     std::shared_ptr<Expression> cond, std::shared_ptr<Expression> thenExpr,
//     std::shared_ptr<Expression> elseExpr, TypeSpecifier type) {
//     return std::make_shared<ConditionalExpression>(
//         std::move(cond), std::move(thenExpr), std::move(elseExpr), type);
// }

inline std::shared_ptr<ConditionalExpression> makeConditionalExpression(
    std::shared_ptr<Expression> cond, std::shared_ptr<Expression> thenExpr,
    std::shared_ptr<Expression> elseExpr) {
    return std::make_shared<ConditionalExpression>(
        std::move(cond), std::move(thenExpr), std::move(elseExpr));
}

inline std::shared_ptr<CommaExpression> makeCommaExpression(
    std::shared_ptr<Expression> left, std::shared_ptr<Expression> right) {
    return std::make_shared<CommaExpression>(std::move(left), std::move(right));
}

inline std::shared_ptr<SizeofExpression> makeSizeofExpression(std::shared_ptr<Expression> expr) {
    return std::make_shared<SizeofExpression>(std::move(expr));
}

inline std::shared_ptr<SizeofTypeExpression> makeSizeofTypeExpression(TypeSpecifier type) {
    return std::make_shared<SizeofTypeExpression>(type);
}

// Statements
inline std::shared_ptr<ExpressionStatement> makeExpressionStatement(
    std::shared_ptr<Expression> expr = nullptr) {
    return std::make_shared<ExpressionStatement>(std::move(expr));
}

inline std::shared_ptr<CompoundStatement> makeCompoundStatement() {
    return std::make_shared<CompoundStatement>();
}

inline std::shared_ptr<CompoundStatement> makeCompoundStatement(std::shared_ptr<NodeList> stmts) {
    return std::make_shared<CompoundStatement>(stmts.get());
}

inline std::shared_ptr<IfStatement> makeIfStatement(
    std::shared_ptr<Expression> cond, std::shared_ptr<Statement> thenStmt,
    std::shared_ptr<Statement> elseStmt = nullptr) {
    return std::make_shared<IfStatement>(std::move(cond), std::move(thenStmt), std::move(elseStmt));
}

inline std::shared_ptr<WhileStatement> makeWhileStatement(
    std::shared_ptr<Expression> cond, std::shared_ptr<Statement> body) {
    return std::make_shared<WhileStatement>(std::move(cond), std::move(body));
}

inline std::shared_ptr<DoWhileStatement> makeDoWhileStatement(
    std::shared_ptr<Statement> body, std::shared_ptr<Expression> cond) {
    return std::make_shared<DoWhileStatement>(std::move(body), std::move(cond));
}

inline std::shared_ptr<ForStatement> makeForStatement(
    std::shared_ptr<Expression> init, std::shared_ptr<Expression> cond,
    std::shared_ptr<Expression> inc, std::shared_ptr<Statement> body) {
    return std::make_shared<ForStatement>(
        std::move(init), std::move(cond), std::move(inc), std::move(body));
}

inline std::shared_ptr<ReturnStatement> makeReturnStatement(
    std::shared_ptr<Expression> expr = nullptr) {
    return std::make_shared<ReturnStatement>(std::move(expr));
}

inline std::shared_ptr<BreakStatement> makeBreakStatement() {
    return std::make_shared<BreakStatement>();
}

inline std::shared_ptr<ContinueStatement> makeContinueStatement() {
    return std::make_shared<ContinueStatement>();
}

inline std::shared_ptr<SwitchStatement> makeSwitchStatement(
    std::shared_ptr<Expression> cond, std::shared_ptr<Statement> body) {
    return std::make_shared<SwitchStatement>(std::move(cond), std::move(body));
}

inline std::shared_ptr<CaseStatement> makeCaseStatement(
    std::shared_ptr<Expression> value, std::shared_ptr<Statement> stmt) {
    return std::make_shared<CaseStatement>(std::move(value), std::move(stmt));
}

inline std::shared_ptr<DefaultStatement> makeDefaultStatement(std::shared_ptr<Statement> stmt) {
    return std::make_shared<DefaultStatement>(std::move(stmt));
}

inline std::shared_ptr<GotoStatement> makeGotoStatement(std::shared_ptr<Identifier> label) {
    return std::make_shared<GotoStatement>(std::move(label));
}

inline std::shared_ptr<LabeledStatement> makeLabeledStatement(
    std::shared_ptr<Identifier> label, std::shared_ptr<Statement> stmt) {
    return std::make_shared<LabeledStatement>(std::move(label), std::move(stmt));
}

inline std::shared_ptr<InitializerList> makeInitializerList() {
    return std::make_shared<InitializerList>();
}

} // namespace ast
