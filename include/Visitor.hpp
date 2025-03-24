#pragma once

namespace ast {

// Forward declarations for all AST node types
class VariableDeclaration;
class FunctionDeclaration;
class EnumDeclaration;

class IdentifierDeclarator;
class PointerDeclarator;
class ArrayDeclarator;
class FunctionDeclarator;
class ParameterList;
class ParameterDeclaration;
class InitDeclarator;
class InitDeclaratorList;
class InitializerList;
class EnumValue;

// Expressions
class BinaryExpression;
class UnaryExpression;
class LiteralExpression;
class StringLiteralExpression;
class IdentifierExpression;
class CallExpression;
class AssignmentExpression;
class ArrayAccessExpression;
class MemberAccessExpression;
class PointerMemberAccessExpression;
class CastExpression;
class ConditionalExpression;
class CommaExpression;
class SizeofExpression;
class SizeofTypeExpression;

// Statements
class ExpressionStatement;
class CompoundStatement;
class IfStatement;
class SwitchStatement;
class CaseStatement;
class WhileStatement;
class DoWhileStatement;
class ForStatement;
class ReturnStatement;
class BreakStatement;
class ContinueStatement;
class GotoStatement;
class LabeledStatement;
class DefaultStatement;

class Visitor {
public:
    virtual ~Visitor() = default;

    // Declarations
    virtual void visitVariableDeclaration(const VariableDeclaration& decl) = 0;
    virtual void visitFunctionDeclaration(const FunctionDeclaration& decl) = 0;

    // Expressions
    virtual void visitBinaryExpression(const BinaryExpression& expr) = 0;
    virtual void visitUnaryExpression(const UnaryExpression& expr) = 0;
    virtual void visitLiteralExpression(const LiteralExpression& expr) = 0;
    virtual void visitIdentifierExpression(const IdentifierExpression& expr) = 0;
    virtual void visitCallExpression(const CallExpression& expr) = 0;
    virtual void visitAssignmentExpression(const AssignmentExpression& expr) = 0;
    virtual void visitStringLiteralExpression(const StringLiteralExpression& expr) = 0;
    virtual void visitArrayAccessExpression(const ArrayAccessExpression& expr) = 0;
    virtual void visitMemberAccessExpression(const MemberAccessExpression& expr) = 0;
    virtual void visitPointerMemberAccessExpression(const PointerMemberAccessExpression& expr) = 0;
    virtual void visitCastExpression(const CastExpression& expr) = 0;
    virtual void visitConditionalExpression(const ConditionalExpression& expr) = 0;
    virtual void visitCommaExpression(const CommaExpression& expr) = 0;
    virtual void visitSizeofExpression(const SizeofExpression& expr) = 0;
    virtual void visitSizeofTypeExpression(const SizeofTypeExpression& expr) = 0;

    // Statements
    virtual void visitExpressionStatement(const ExpressionStatement& stmt) = 0;
    virtual void visitCompoundStatement(const CompoundStatement& stmt) = 0;
    virtual void visitIfStatement(const IfStatement& stmt) = 0;
    virtual void visitWhileStatement(const WhileStatement& stmt) = 0;
    virtual void visitForStatement(const ForStatement& stmt) = 0;
    virtual void visitReturnStatement(const ReturnStatement& stmt) = 0;
    virtual void visitBreakStatement(const BreakStatement& stmt) = 0;
    virtual void visitContinueStatement(const ContinueStatement& stmt) = 0;
    virtual void visitSwitchStatement(const SwitchStatement& stmt) = 0;
    virtual void visitCaseStatement(const CaseStatement& stmt) = 0;
    virtual void visitDoWhileStatement(const DoWhileStatement& stmt) = 0;
    virtual void visitGotoStatement(const GotoStatement& stmt) = 0;
    virtual void visitLabeledStatement(const LabeledStatement& stmt) = 0;
    virtual void visitDefaultStatement(const DefaultStatement& stmt) = 0;

    // declarators
    virtual void visitIdentifierDeclarator(const IdentifierDeclarator& decl) = 0;
    virtual void visitParameterList(const ParameterList& list) = 0;
    virtual void visitParameterDeclaration(const ParameterDeclaration& decl) = 0;
    virtual void visitArrayDeclarator(const ArrayDeclarator& decl) = 0;
    virtual void visitFunctionDeclarator(const FunctionDeclarator& decl) = 0;
    virtual void visitPointerDeclarator(const PointerDeclarator& decl) = 0;
    virtual void visitInitDeclarator(const InitDeclarator& decl) = 0;
    virtual void visitInitializerList(const ast::InitializerList& list) = 0;

    virtual void visitEnumValue(const EnumValue& value) = 0;
    virtual void visitEnumDeclaration(const EnumDeclaration& decl) = 0;
};

}
