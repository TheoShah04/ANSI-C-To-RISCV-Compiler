#pragma once

#include "Visitor.hpp"
#include "ast_context.hpp"
#include "DeclarationStatement.hpp"
#include "Declarator.hpp"
#include "Declaration.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include <iostream>
#include <string>
#include <stack>

using namespace ast;
namespace codegen {

class CodeGenVisitor : public Visitor {
private:
    Context& context;
    std::ostream& stream;
    std::string currentExprResult;
    std::string pendingNextCaseLabel;

    std::string currentIdentifier;
    bool isPointerType = false;
    bool isArrayType = false;
    bool isFunctionType = false;
    std::string currentArraySize;
    std::string initializerValue;

    struct ParameterInfo {
        std::string name;
        TypeSpecifier type;
        bool isPointer;
        size_t index;
    };
    std::vector<ParameterInfo> parameterList;

    struct LoopLabels {
        std::string continue_label;
        std::string break_label;
    };

    std::stack<LoopLabels> loop_label_stack;

public:
    CodeGenVisitor(Context& ctx, std::ostream& output)
        : context(ctx), stream(output) {}

    std::string getExpressionResult() const;

    void visitVariableDeclaration(const VariableDeclaration& decl) override;
    void visitFunctionDeclaration(const FunctionDeclaration& decl) override;

    // expressions
    void visitBinaryExpression(const BinaryExpression& expr) override;
    void visitUnaryExpression(const UnaryExpression& expr) override;
    void visitLiteralExpression(const LiteralExpression& expr) override;
    void visitIdentifierExpression(const IdentifierExpression& expr) override;
    void visitCallExpression(const CallExpression& expr) override;
    void visitAssignmentExpression(const AssignmentExpression& expr) override;
    void visitStringLiteralExpression(const StringLiteralExpression& expr) override;
    void visitArrayAccessExpression(const ArrayAccessExpression& expr) override;
    void visitMemberAccessExpression(const MemberAccessExpression& expr) override;
    void visitPointerMemberAccessExpression(const PointerMemberAccessExpression& expr) override;
    void visitCastExpression(const CastExpression& expr) override;
    void visitConditionalExpression(const ConditionalExpression& expr) override;
    void visitCommaExpression(const CommaExpression& expr) override;
    void visitSizeofExpression(const SizeofExpression& expr) override;
    void visitSizeofTypeExpression(const SizeofTypeExpression& expr) override;

    // statements
    void visitExpressionStatement(const ExpressionStatement& stmt) override;
    void visitCompoundStatement(const CompoundStatement& stmt) override;
    void visitIfStatement(const IfStatement& stmt) override;
    void visitWhileStatement(const WhileStatement& stmt) override;
    void visitForStatement(const ForStatement& stmt) override;
    void visitReturnStatement(const ReturnStatement& stmt) override;
    void visitBreakStatement(const BreakStatement& stmt) override;
    void visitContinueStatement(const ContinueStatement& stmt) override;
    void visitSwitchStatement(const SwitchStatement& stmt) override;
    void visitCaseStatement(const CaseStatement& stmt) override;
    void visitDoWhileStatement(const DoWhileStatement& stmt) override;
    void visitGotoStatement(const GotoStatement& stmt) override;
    void visitLabeledStatement(const LabeledStatement& stmt) override;
    void visitDefaultStatement(const DefaultStatement& stmt) override;

    // declarators
    void visitIdentifierDeclarator(const IdentifierDeclarator& decl) override;
    void visitArrayDeclarator(const ArrayDeclarator& decl) override;
    void visitFunctionDeclarator(const FunctionDeclarator& decl) override;
    void visitPointerDeclarator(const PointerDeclarator& decl) override;
    void visitParameterDeclaration(const ParameterDeclaration& decl) override;
    void visitParameterList(const ParameterList& list) override;
    void visitInitDeclarator(const InitDeclarator& decl) override;
    void visitInitializerList(const ast::InitializerList& list) override;

    void visitEnumValue(const ast::EnumValue& value);
    void visitEnumDeclaration(const ast::EnumDeclaration& decl);

    // helper
    void initArray(const ast::VariableDeclaration& decl);
};

} //namespace codegen
