#include "Expression.hpp"
#include "Visitor.hpp"

namespace ast {

/* Code is here to avoid circular dependencies */

std::string AssignmentExpression::getVariableName() const {
    if (auto* idExpr = dynamic_cast<const IdentifierExpression*>(lhs.get())) {
        return idExpr->getName();
    }
    /*  For normal variables LHS is cast to identifier expression
        but for arrays, LHS is an ArrayAccessExpression, not identifierExpression
        so handle differently (as cast will fail)
    */
    if (auto* arrayExpr = dynamic_cast<const ArrayAccessExpression*>(lhs.get())) {
        if (auto* arrayId = dynamic_cast<const IdentifierExpression*>(arrayExpr->getArray())) {
            return arrayId->getName() + "[]";
        }
    }

    return "unknown";
}
}
