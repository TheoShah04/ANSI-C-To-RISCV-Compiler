#pragma once
#include "ast_node.hpp"
#include "ast_type_specifier.hpp"
#include "Identifier.hpp"
#include "Visitor.hpp"
#include <vector>
#include <memory>
#include <string>

namespace ast {

class Expression;

class Statement : public Node {
public:
    virtual ~Statement() = default;
};

using StmtPtr = std::shared_ptr<Statement>;

// function call or assignment
class ExpressionStatement : public Statement {
private:
    std::shared_ptr<Expression> expression;

public:
    ExpressionStatement(std::shared_ptr<Expression> expr = nullptr)
        : expression(expr) {}

    std::shared_ptr<Expression> getExpression() const {return expression;}
    void setExpression(std::shared_ptr<Expression> expr) {expression = expr;}

    void accept(Visitor& visitor) const override;
};

class CompoundStatement : public Statement {
private:
    std::vector<StmtPtr> statements;
    std::shared_ptr<NodeList> declarationList;

public:
    CompoundStatement() = default;

    CompoundStatement(NodeList* stmtList) {
        if (stmtList) {
            for (auto& node : stmtList->getNodes()) {
                auto stmtPtr = std::dynamic_pointer_cast<Statement>(node);
                if (stmtPtr) {
                    statements.push_back(stmtPtr);
                }
            }
        }
    }

    void setDeclarationList(std::shared_ptr<NodeList> list) {
        declarationList = std::move(list);
    }

    const NodeList* getDeclarationList() const {
        return declarationList.get();
    }

    void addStatement(StmtPtr stmt) {
        statements.push_back(std::move(stmt));
    }

    const std::vector<StmtPtr>& getStatements() const {return statements;}

    void accept(Visitor& visitor) const override;
};

class IfStatement : public Statement {
private:
    std::shared_ptr<Expression> condition;
    StmtPtr thenStatement;
    StmtPtr elseStatement;

public:
    IfStatement(std::shared_ptr<Expression> cond, StmtPtr thenStmt, StmtPtr elseStmt = nullptr)
        : condition(std::move(cond)),
          thenStatement(std::move(thenStmt)),
          elseStatement(std::move(elseStmt)) {}

    std::shared_ptr<Expression> getCondition() const { return condition; }
    const Statement* getThenStatement() const { return thenStatement.get(); }
    const Statement* getElseStatement() const { return elseStatement.get(); }
    bool hasElseStatement() const { return elseStatement != nullptr; }

    void accept(Visitor& visitor) const override;
};

class SwitchStatement : public Statement {
private:
    std::shared_ptr<Expression> condition;
    StmtPtr body;

public:
    SwitchStatement(std::shared_ptr<Expression> cond, StmtPtr bodyStmt)
        : condition(std::move(cond)), body(std::move(bodyStmt)) {}

    std::shared_ptr<Expression> getCondition() const { return condition; }
    const Statement* getBody() const { return body.get(); }

    void accept(Visitor& visitor) const override;
};

// Case statement (part of switch)
class CaseStatement : public Statement {
private:
    std::shared_ptr<Expression> caseValue;
    StmtPtr statement;

public:
    CaseStatement(std::shared_ptr<Expression> value, StmtPtr stmt)
        : caseValue(std::move(value)), statement(std::move(stmt)) {}

    CaseStatement(StmtPtr stmt)
        : caseValue(nullptr), statement(std::move(stmt)) {}

    std::shared_ptr<Expression> getCaseValue() const { return caseValue; }
    bool isDefault() const { return caseValue == nullptr; }
    const Statement* getStatement() const { return statement.get(); }

    void accept(Visitor& visitor) const override;
};

// special case of CaseStatement
class DefaultStatement : public CaseStatement {
public:
    DefaultStatement(StmtPtr stmt) : CaseStatement(stmt) {}
};

class WhileStatement : public Statement {
private:
    std::shared_ptr<Expression> condition;
    StmtPtr body;

public:
    WhileStatement(std::shared_ptr<Expression> cond, StmtPtr bodyStmt)
        : condition(std::move(cond)), body(std::move(bodyStmt)) {}

    std::shared_ptr<Expression> getCondition() const { return condition; }
    const Statement* getBody() const { return body.get(); }

    void accept(Visitor& visitor) const override;
};

class DoWhileStatement : public Statement {
private:
    StmtPtr body;
    std::shared_ptr<Expression> condition;

public:
    DoWhileStatement(StmtPtr bodyStmt, std::shared_ptr<Expression> cond)
        : body(std::move(bodyStmt)), condition(std::move(cond)) {}

    const Statement* getBody() const { return body.get(); }
    std::shared_ptr<Expression> getCondition() const { return condition; }

    void accept(Visitor& visitor) const override;
};

class ForStatement : public Statement {
private:
    // all optional except body
    std::shared_ptr<Expression> initialization;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> increment;
    StmtPtr body;

public:
    ForStatement(std::shared_ptr<Expression> init, std::shared_ptr<Expression> cond,
                 std::shared_ptr<Expression> inc, StmtPtr bodyStmt)
        : initialization(std::move(init)),
          condition(std::move(cond)),
          increment(std::move(inc)),
          body(std::move(bodyStmt)) {}

    std::shared_ptr<Expression> getInitialization() const { return initialization; }
    std::shared_ptr<Expression> getCondition() const { return condition; }
    std::shared_ptr<Expression> getIncrement() const { return increment; }
    const Statement* getBody() const { return body.get(); }

    bool hasInitialization() const { return initialization != nullptr; }
    bool hasCondition() const { return condition != nullptr; }
    bool hasIncrement() const { return increment != nullptr; }

    void accept(Visitor& visitor) const override;
};

class ReturnStatement : public Statement {
private:
    std::shared_ptr<Expression> expression;  // Optional (void functions don't have a return value)

public:
    ReturnStatement(std::shared_ptr<Expression> expr = nullptr) : expression(std::move(expr)) {}

    std::shared_ptr<Expression> getExpression() const { return expression; }
    bool hasExpression() const { return expression != nullptr; }

    void accept(Visitor& visitor) const override;
};

class BreakStatement : public Statement {
public:
    BreakStatement() = default;

    void accept(Visitor& visitor) const override;
};

class ContinueStatement : public Statement {
public:
    ContinueStatement() = default;

    void accept(Visitor& visitor) const override;
};

class GotoStatement : public Statement {
private:
    std::shared_ptr<Identifier> label;

public:
    GotoStatement(Identifier* l)
        : label(std::shared_ptr<Identifier>(l)) {}

    GotoStatement(std::shared_ptr<Identifier> l)
        : label(std::move(l)) {}

    const Identifier* getLabel() const { return label.get(); }

    void accept(Visitor& visitor) const override;
};

class LabeledStatement : public Statement {
private:
    std::shared_ptr<Identifier> label;
    StmtPtr statement;

public:
    LabeledStatement(Identifier* l, Statement* stmt)
        : label(std::shared_ptr<Identifier>(l)), statement(StmtPtr(stmt)) {}

    LabeledStatement(std::shared_ptr<Identifier> l, StmtPtr stmt)
        : label(std::move(l)), statement(std::move(stmt)) {}

    const Identifier* getLabel() const { return label.get(); }
    const Statement* getStatement() const { return statement.get(); }

    void accept(Visitor& visitor) const override;
};

inline void LabeledStatement::accept(Visitor& visitor) const { visitor.visitLabeledStatement(*this); }
inline void GotoStatement::accept(Visitor& visitor) const { visitor.visitGotoStatement(*this); }
inline void ExpressionStatement::accept(Visitor& visitor) const { visitor.visitExpressionStatement(*this); }
inline void CompoundStatement::accept(Visitor& visitor) const { visitor.visitCompoundStatement(*this); }
inline void IfStatement::accept(Visitor& visitor) const { visitor.visitIfStatement(*this); }
inline void SwitchStatement::accept(Visitor& visitor) const { visitor.visitSwitchStatement(*this); }
inline void CaseStatement::accept(Visitor& visitor) const { visitor.visitCaseStatement(*this); }
inline void WhileStatement::accept(Visitor& visitor) const { visitor.visitWhileStatement(*this); }
inline void DoWhileStatement::accept(Visitor& visitor) const { visitor.visitDoWhileStatement(*this); }
inline void ForStatement::accept(Visitor& visitor) const { visitor.visitForStatement(*this); }
inline void ReturnStatement::accept(Visitor& visitor) const { visitor.visitReturnStatement(*this); }
inline void BreakStatement::accept(Visitor& visitor) const { visitor.visitBreakStatement(*this); }
inline void ContinueStatement::accept(Visitor& visitor) const { visitor.visitContinueStatement(*this); }

} // namespace ast
