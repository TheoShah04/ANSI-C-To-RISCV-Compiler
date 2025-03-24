#include "codegen_visitor.hpp"
#include "Declaration.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include "EnumDeclaration.hpp"

#include <iostream>
#include <stdexcept>
#include <memory>

namespace codegen {

std::string CodeGenVisitor::getExpressionResult() const {
    return currentExprResult;
}

void CodeGenVisitor::visitVariableDeclaration(const ast::VariableDeclaration& decl) {
    if (decl.getDeclarator() && decl.getDeclarator()->isFunction()) {
        // if no function body then just register as function in context, no codegen
        std::string funcName = decl.getIdentifier();
        context.setFunctionReturnType(funcName, decl.getType(), decl.isPointer());
        return;
    }
    bool isGlobal = context.ScopeDepth() == 0;
    std::string varName = decl.getIdentifier();
    if(isGlobal){
        // sets variable to global if not in a function scope
        context.setGlobal(varName);
        stream << "    .data" << std::endl;
        stream << "    .align 2" << std::endl;
        stream << "    .globl " << varName << std::endl;
        stream << varName << ":" << std::endl;

        // handling global arrays
        if (decl.isArray() && decl.hasInitializer()) {
            auto* arrayDecl = decl.getDeclarator()->asArrayDeclarator();
            if (arrayDecl && arrayDecl->getSize()) {
                size_t arraySize = 0;
                auto* literalExpr = arrayDecl->getSize()->asLiteralExpression();
                if (literalExpr && literalExpr->getType() == ast::TypeSpecifier::INT) {
                    arraySize = static_cast<size_t>(literalExpr->getIntValue());
                }
                //context.storeArraySize(varName, arraySize);
                context.declareArray(varName, decl.getType(), arraySize);
                auto* initList = decl.getInitializer()->asInitializerList();
                if (initList) {
                    const auto& expressions = initList->getExpressions();

                    // use byte directive for char arrays
                    if(decl.getType() == ast::TypeSpecifier::CHAR) {
                        for (size_t i = 0; i < expressions.size(); ++i) {
                            auto* literal = expressions[i]->asLiteralExpression();
                            if (literal && literal->getType() == ast::TypeSpecifier::INT) {
                                stream << "    .byte " << literal->getIntValue() << std::endl;
                            } else if (literal && literal->getType() == ast::TypeSpecifier::CHAR) {
                                stream << "    .byte " << static_cast<int>(literal->getCharValue()) << std::endl;
                            } else {
                                stream << "    .byte 0" << std::endl;
                            }
                        }
                        for (size_t i = expressions.size(); i < arraySize; ++i) {
                            stream << "    .byte 0" << std::endl;
                        }
                        if (arraySize % 4 != 0) {
                            stream << "    .align 2" << std::endl;
                        }
                    } else {
                        // int, float and double arrays
                        for (size_t i = 0; i < expressions.size(); ++i) {
                            auto* literal = expressions[i]->asLiteralExpression();
                            if (literal && literal->getType() == ast::TypeSpecifier::INT) {
                                stream << "    .word " << literal->getIntValue() << std::endl;

                            } else if (literal && literal->getType() == ast::TypeSpecifier::FLOAT) {
                                float floatVal = literal->getFloatValue();
                                uint32_t bits = *reinterpret_cast<uint32_t*>(&floatVal);
                                stream << "    .word " << bits << std::endl;

                            } else if (literal && literal->getType() == ast::TypeSpecifier::DOUBLE) {
                                double doubleVal = literal->getDoubleValue();
                                union {
                                    double d;
                                    uint32_t parts[2];
                                } doubleUnion;
                                doubleUnion.d = doubleVal;
                                stream << "    .word " << doubleUnion.parts[0] << std::endl;
                                stream << "    .word " << doubleUnion.parts[1] << std::endl;
                            } else {
                                stream << "    .word 0" << std::endl;
                            }

                        }
                        for (size_t i = expressions.size(); i < arraySize; ++i) {
                            if (decl.getType() == ast::TypeSpecifier::DOUBLE) {
                                stream << "    .word 0" << std::endl;
                                stream << "    .word 0" << std::endl;
                            } else {
                                stream << "    .word 0" << std::endl;
                            }
                        }
                    }
                }
            }
        }

        // for global normal variables
        else {
            context.declareVariable(varName, decl.getType(), decl.isPointer());
            if (decl.hasInitializer()) {
                auto* literal = decl.getInitializer()->asLiteralExpression();
                if (literal && literal->getType() == ast::TypeSpecifier::INT) {
                    stream << "    .word " << literal->getIntValue() << std::endl;
                } else if (literal && literal->getType() == ast::TypeSpecifier::FLOAT) {
                    float floatVal = literal->getFloatValue();
                    uint32_t bits = *reinterpret_cast<uint32_t*>(&floatVal);
                    stream << "    .word " << bits << std::endl;
                }
                else if (literal && literal->getType() == ast::TypeSpecifier::DOUBLE) {
                    double doubleVal = literal->getDoubleValue();
                    union {
                        double d;
                        uint32_t parts[2];
                    } doubleUnion;
                    doubleUnion.d = doubleVal;
                    stream << "    .word " << doubleUnion.parts[0] << std::endl;
                    stream << "    .word " << doubleUnion.parts[1] << std::endl;
                }
                else if (literal && literal->getType() == ast::TypeSpecifier::CHAR) {
                    stream << "    .byte " << static_cast<int>(literal->getCharValue()) << std::endl;
                }
            } else {
                unsigned int size = context.getTypeSize(decl.getType());
                stream << "    .zero " << size << std::endl;
            }
            return;
        }
    }
    // handling local identifiers

    else{
        //handling arrays first
        const auto* arrayDecl = decl.getDeclarator()->asArrayDeclarator();
        if (decl.isArray()) {
            if (arrayDecl && arrayDecl->getSize()) {
                arrayDecl->getSize()->accept(*this);
                std::string sizeReg = getExpressionResult();

                int arraySize = 0;
                auto* literalExpr = arrayDecl->getSize()->asLiteralExpression();
                if (literalExpr && literalExpr->getType() == ast::TypeSpecifier::INT) {
                    arraySize = literalExpr->getIntValue();
                }

                context.declareArray(decl.getIdentifier(), decl.getType(), arraySize);

                if (decl.hasInitializer()) {
                    initArray(decl);
                }
                context.freeRegister(sizeReg);
                currentExprResult.clear();
            }
        }
        else {
            // normal variables and pointers
            context.declareVariable(decl.getIdentifier(), decl.getType(), decl.isPointer());
            if (decl.hasInitializer()) {
                decl.getInitializer()->accept(*this);
                std::string resultReg = getExpressionResult();
                context.storeVariable(stream, resultReg, decl.getIdentifier());
                context.freeRegister(resultReg);
                currentExprResult.clear();
            }
        }
    }
}

void CodeGenVisitor::visitFunctionDeclaration(const ast::FunctionDeclaration& decl) {
    if(!decl.hasBody()){
        // just register func in context, don't do any codegen
        context.setFunctionReturnType(decl.getIdentifier(), decl.getType(), decl.getRetPtr());
        return;
    }
    stream << "    .text" << std::endl;
    stream << "    .align 2" << std::endl;
    stream << "    .globl	" << decl.getIdentifier() << std::endl;
    stream << "    .type	" << decl.getIdentifier() << ", @function" << std::endl;
    stream << decl.getIdentifier() << ":" << std::endl;

    context.beginFunction(stream, decl.getIdentifier(), decl.getType(), decl.getRetPtr());

    const auto& params = decl.getParameters();
    int intParamIdx = 0;
    int floatParamIdx = 0;

    for (size_t i = 0; i < params.size(); i++) {
        const auto& param = params[i];
        std::string paramReg;
        bool isStackParam = false;
        int paramIdx = i;

        if (param->getType() == ast::TypeSpecifier::FLOAT ||
            param->getType() == ast::TypeSpecifier::DOUBLE) {
            if (floatParamIdx < 8) {
                paramReg = "fa" + std::to_string(floatParamIdx);
                floatParamIdx++;
            } else {
                isStackParam = true;
            }
        } else {
            if (intParamIdx < 8) {
                paramReg = "a" + std::to_string(intParamIdx);
                intParamIdx++;
            } else {
                isStackParam = true;
            }
        }
        std::cerr << "DEBUG: Declaring param" << std::endl;

        context.declareParameter(param->getIdentifier(), param->getType(), paramIdx, param->isPointer());

        // storing parameters to stack
        if (!isStackParam && !paramReg.empty()) {
            context.storeVariable(stream, paramReg, param->getIdentifier());
        }
    }

    if (decl.hasBody()) {
        decl.getBody()->accept(*this);
    }
    context.endFunction(stream, decl.getIdentifier());
}

void CodeGenVisitor::visitBinaryExpression(const ast::BinaryExpression& expr) {
    expr.getLeft()->accept(*this);
    std::string leftReg = getExpressionResult();

    expr.getRight()->accept(*this);
    std::string rightReg = getExpressionResult();

    bool isLeftPtr = false;
    bool isRightPtr = false;
    int pointeeSize = 4; //int is default

    // for pointer arithmetic
    const ast::IdentifierExpression* leftId = expr.getLeft()->asIdentifierExpression();
    const ast::IdentifierExpression* rightId = expr.getRight()->asIdentifierExpression();
    if (leftId) {
        auto var = context.findVariable(leftId->getName());
        if (var && var->is_pointer) {
            isLeftPtr = true;
            pointeeSize = context.getTypeSize(var->type);
        }
    }
    if (rightId) {
        auto var = context.findVariable(rightId->getName());
        if (var && var->is_pointer) {
            isRightPtr = true;
            pointeeSize = context.getTypeSize(var->type);
        }
    }

    // Allocating proper register for type
    bool useFloatingReg = ((expr.getLeft()->getType() == ast::TypeSpecifier::FLOAT ||
                                expr.getLeft()->getType() == ast::TypeSpecifier::DOUBLE ||
                                expr.getRight()->getType() == ast::TypeSpecifier::FLOAT ||
                                expr.getRight()->getType() == ast::TypeSpecifier::DOUBLE) &&
                                (expr.getOperator() != ast::BinaryOp::Type::LT &&
                                expr.getOperator() != ast::BinaryOp::Type::GT &&
                                expr.getOperator() != ast::BinaryOp::Type::LE &&
                                expr.getOperator() != ast::BinaryOp::Type::GE));
    std::string resultReg;
    if (useFloatingReg) {
        resultReg = context.allocateFloatingRegister({leftReg, rightReg});
    } else {
        resultReg = context.allocateRegister({leftReg, rightReg});
    }

    std::string opPrefix;
    std::string opSuffix;
    std::string intPrefix;
    std::string charPrefix;
    std::string charSuffix;
    if(expr.getLeft()->getType() == ast::TypeSpecifier::INT && expr.getRight()->getType() == ast::TypeSpecifier::INT){
        opPrefix = "";
        opSuffix = "";
        intPrefix = "s";
        charPrefix = "";
        charSuffix = "";
    }
    else if(expr.getLeft()->getType() == ast::TypeSpecifier::FLOAT || expr.getRight()->getType() == ast::TypeSpecifier::FLOAT){
        opPrefix = "f";
        opSuffix = ".s";
        intPrefix = "";
        charPrefix = "";
        charSuffix = "";
    }
    else if(expr.getLeft()->getType() == ast::TypeSpecifier::DOUBLE || expr.getRight()->getType() == ast::TypeSpecifier::DOUBLE){
        opPrefix = "f";
        opSuffix = ".d";
        intPrefix = "";
        charPrefix = "";
        charSuffix = "";
    }
    else if(expr.getLeft()->getType() == ast::TypeSpecifier::CHAR || expr.getRight()->getType() == ast::TypeSpecifier::CHAR){
        opPrefix = "";
        opSuffix = "";
        intPrefix = "";
        charPrefix = "s";
        charSuffix = "u";
    }
    else{
        throw std::runtime_error("Invalid binary expression type");
    }

    if (!isLeftPtr && !isRightPtr) {
        switch (expr.getOperator()) {
            case ast::BinaryOp::Type::ADD:
                stream << "    " << opPrefix << "add" << opSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::SUB:
                stream << "    " << opPrefix << "sub" << opSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::MUL:
                stream << "    " << opPrefix << "mul" << opSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::DIV:
                stream << "    " << opPrefix << "div" << opSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::MOD:
                stream << "    rem " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::LT:
                stream << "    " << intPrefix << opPrefix << charPrefix << "lt" << opSuffix << charSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::GT:
                stream << "    " << intPrefix << opPrefix << charPrefix << "gt" << opSuffix << charSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::LE:
                if(expr.getLeft()->getType() == ast::TypeSpecifier::DOUBLE || expr.getRight()->getType() == ast::TypeSpecifier::DOUBLE
                    || expr.getLeft()->getType() == ast::TypeSpecifier::FLOAT || expr.getRight()->getType() == ast::TypeSpecifier::FLOAT){
                        stream << "    fle" << opSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    }
                else{
                    stream << "    sgt" << charSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    stream << "    xori " << resultReg << ", " << resultReg << ", 1" << std::endl;
                }
                break;
            case ast::BinaryOp::Type::GE:
                if(expr.getLeft()->getType() == ast::TypeSpecifier::DOUBLE || expr.getRight()->getType() == ast::TypeSpecifier::DOUBLE
                    || expr.getLeft()->getType() == ast::TypeSpecifier::FLOAT || expr.getRight()->getType() == ast::TypeSpecifier::FLOAT){
                        stream << "    fge" << opSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    }
                else{
                    stream << "    slt" << charSuffix << " " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    stream << "    xori " << resultReg << ", " << resultReg << ", 1" << std::endl;
                }
                break;
            case ast::BinaryOp::Type::EQ:
                if(expr.getLeft()->getType() == ast::TypeSpecifier::DOUBLE || expr.getRight()->getType() == ast::TypeSpecifier::DOUBLE
                    || expr.getLeft()->getType() == ast::TypeSpecifier::FLOAT || expr.getRight()->getType() == ast::TypeSpecifier::FLOAT){
                        std::string intResultReg = context.allocateRegister();
                        stream << "    feq" << opSuffix << " " << intResultReg << ", " << leftReg << ", " << rightReg << std::endl;
                        resultReg = intResultReg; // need integer reg for result
                        context.freeRegister(intResultReg);
                    }
                else{
                    stream << "    xor " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    stream << "    seqz " << resultReg << ", " << resultReg << std::endl;
                }
                break;
            case ast::BinaryOp::Type::NE:
                if(expr.getLeft()->getType() == ast::TypeSpecifier::DOUBLE || expr.getRight()->getType() == ast::TypeSpecifier::DOUBLE
                    || expr.getLeft()->getType() == ast::TypeSpecifier::FLOAT || expr.getRight()->getType() == ast::TypeSpecifier::FLOAT){
                        std::string intResultReg = context.allocateRegister();
                        stream << "    feq" << opSuffix << " " << intResultReg << ", " << leftReg << ", " << rightReg << std::endl;
                        stream << "    xori " << intResultReg << ", " << intResultReg << ", 1" << std::endl;
                        resultReg = intResultReg;
                        context.freeRegister(intResultReg);
                    }
                else{
                    stream << "    xor " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    stream << "    snez " << resultReg << ", " << resultReg << std::endl;
                }
                break;
            case ast::BinaryOp::Type::AND: //Doesn't support FLOAT/DOUBLE
                stream << "    and " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::OR: //Doesn't support FLOAT/DOUBLE
                stream << "    or " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::XOR: //Doesn't support FLOAT/DOUBLE
                stream << "    xor " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::LOGICAL_AND: { //Complicated to do float/double
                std::string logicalAndLabel1 = context.generateUniqueLabel("LOGICAL_AND");
                std::string logicalAndLabel2 = context.generateUniqueLabel("LOGICAL_AND");
                stream << "    mv " << resultReg << ", " << leftReg << std::endl;
                stream << "    beq " << resultReg << ", " << "zero, " << logicalAndLabel1 << std::endl;
                stream << "    mv " << resultReg << ", " << rightReg << std::endl;
                stream << "    beq " << resultReg << ", " << "zero, " << logicalAndLabel1 << std::endl;
                stream << "    li " << resultReg << ", " << "1" << std::endl;
                stream << "    j " << logicalAndLabel2 << std::endl;
                stream << logicalAndLabel1 << ":" << std::endl;
                stream << "    li " << resultReg << ", " << "0" << std::endl;
                stream << logicalAndLabel2 << ":" << std::endl;
                break;
            }
            case ast::BinaryOp::Type::LOGICAL_OR: { //Complicated to do float/double
                std::string logicalOrLabel1 = context.generateUniqueLabel("LOGICAL_OR");
                std::string logicalOrLabel2 = context.generateUniqueLabel("LOGICAL_OR");
                std::string logicalOrLabel3 = context.generateUniqueLabel("LOGICAL_OR");
                stream << "    mv " << resultReg << ", " << leftReg << std::endl;
                stream << "    bne " << resultReg << ", " << "zero, " << logicalOrLabel1 << std::endl;
                stream << "    mv " << resultReg << ", " << rightReg << std::endl;
                stream << "    beq " << resultReg << ", " << "zero, " << logicalOrLabel2 << std::endl;
                stream << logicalOrLabel1 << ":" << std::endl;
                stream << "    li " << resultReg << ", " << "1" << std::endl;
                stream << "    j " << logicalOrLabel3 << std::endl;
                stream << logicalOrLabel2 << ":" << std::endl;
                stream << "    li " << resultReg << ", " << "0" << std::endl;
                stream << logicalOrLabel3 << ":" << std::endl;
                break;
            }
            case ast::BinaryOp::Type::LEFT_SHIFT: //Doesn't support FLOAT/DOUBLE
                stream << "    sll " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::RIGHT_SHIFT: //Doesn't support FLOAT/DOUBLE
                stream << "    sra " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            default:
                throw std::runtime_error("Unsupported binary operator");
        }
    }

    else if((isLeftPtr || isRightPtr) &&
            (expr.getOperator() == ast::BinaryOp::Type::ADD ||
             expr.getOperator() == ast::BinaryOp::Type::SUB)) {

        switch (expr.getOperator()) {
            case ast::BinaryOp::Type::ADD:
                if (isLeftPtr && !isRightPtr) {
                    // have to resize integer by pointed-to size
                    std::string scaleReg = context.allocateRegister({leftReg, rightReg});
                    stream << "    li " << scaleReg << ", " << pointeeSize << std::endl;
                    stream << "    mul " << rightReg << ", " << rightReg << ", " << scaleReg << std::endl;
                    stream << "    add " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    context.freeRegister(scaleReg);
                }
                else if (!isLeftPtr && isRightPtr) {
                    std::string scaleReg = context.allocateRegister({leftReg, rightReg});
                    stream << "    li " << scaleReg << ", " << pointeeSize << std::endl;
                    stream << "    mul " << leftReg << ", " << leftReg << ", " << scaleReg << std::endl;
                    stream << "    add " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    context.freeRegister(scaleReg);
                }
                break;

            case ast::BinaryOp::Type::SUB:
                if (isLeftPtr && !isRightPtr) {
                    std::string scaleReg = context.allocateRegister({leftReg, rightReg});
                    stream << "    li " << scaleReg << ", " << pointeeSize << std::endl;
                    stream << "    mul " << rightReg << ", " << rightReg << ", " << scaleReg << std::endl;
                    stream << "    sub " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    context.freeRegister(scaleReg);
                }
                else if (isLeftPtr && isRightPtr) {
                    stream << "    sub " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    std::string divReg = context.allocateRegister({resultReg});
                    stream << "    li " << divReg << ", " << pointeeSize << std::endl;
                    stream << "    div " << resultReg << ", " << resultReg << ", " << divReg << std::endl;
                    context.freeRegister(divReg);
                }
                break;

            default:
                throw std::runtime_error("illegal pointer operation");
        }
    }

    else if(isLeftPtr && isRightPtr) {
        switch (expr.getOperator()) {
            case ast::BinaryOp::Type::EQ:
                stream << "    xor " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                stream << "    seqz " << resultReg << ", " << resultReg << std::endl;
                break;
            case ast::BinaryOp::Type::NE:
                stream << "    xor " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                stream << "    snez " << resultReg << ", " << resultReg << std::endl;
                break;
            case ast::BinaryOp::Type::LT:
                stream << "    slt " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::GT:
                stream << "    sgt " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinaryOp::Type::LE:
                stream << "    sgt " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                stream << "    xori " << resultReg << ", " << resultReg << ", 1" << std::endl;
                break;
            case ast::BinaryOp::Type::GE:
                stream << "    slt " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                stream << "    xori " << resultReg << ", " << resultReg << ", 1" << std::endl;
                break;
            default:
                throw std::runtime_error("illegal pointer comparison");
        }
    }

    if (!useFloatingReg){
        context.freeRegister(leftReg);
        context.freeRegister(rightReg);
    } else {
        context.freeFloatingRegister(leftReg);
        context.freeFloatingRegister(rightReg);
    }
    currentExprResult = resultReg;
}

void CodeGenVisitor::visitUnaryExpression(const ast::UnaryExpression& expr) {
    std::string resultReg;
    std::string varName;

    // eaiser to just seperate
    if (expr.getOperator() == ast::UnaryOp::Type::PRE_INCREMENT ||
        expr.getOperator() == ast::UnaryOp::Type::POST_INCREMENT ||
        expr.getOperator() == ast::UnaryOp::Type::PRE_DECREMENT ||
        expr.getOperator() == ast::UnaryOp::Type::POST_DECREMENT) {

        const ast::IdentifierExpression* idExpr = expr.getOperand()->asIdentifierExpression();
        if (idExpr) {
            varName = idExpr->getName();
        } else {
            throw std::runtime_error("complex expressions in increment/decrement not implemented yet");
        }

        resultReg = context.allocateRegister();
    } else {
        // otherwise evaluate the operand first
        expr.getOperand()->accept(*this);
        resultReg = getExpressionResult();
    }

    switch (expr.getOperator()) {
        case ast::UnaryOp::Type::PLUS:
            // don't do anything for this?
            break;
        case ast::UnaryOp::Type::MINUS:
            stream << "    neg " << resultReg << ", " << resultReg << std::endl;
            break;
        case ast::UnaryOp::Type::LOGICAL_NOT:
            stream << "    seqz " << resultReg << ", " << resultReg << std::endl;
            break;
        case ast::UnaryOp::Type::BITWISE_NOT:
            stream << "    not " << resultReg << ", " << resultReg << std::endl;
            break;
        case ast::UnaryOp::Type::ADDRESS_OF: {
            const IdentifierExpression* idExpr = expr.getOperand()->asIdentifierExpression();
            std::string varName = idExpr->getName();
            auto var = context.findVariable(varName);
            // getting base frame address
            stream << "    addi " << resultReg << ", s0, " << var->stack_offset << std::endl;
            break;
        }

        case ast::UnaryOp::Type::DEREFERENCE: {
            std::string addrReg = resultReg;
            auto* pointeeExpr = expr.getOperand()->asIdentifierExpression();
            TypeSpecifier pointeeType = TypeSpecifier::INT;
            if (pointeeExpr) {
                //get pointee type
                auto var = context.findVariable(pointeeExpr->getName());
                if (var && var->is_pointer) {
                    pointeeType = var->type;
                }
            }
            if (pointeeType == TypeSpecifier::CHAR) {
                stream << "    lb " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            } else if (pointeeType == TypeSpecifier::FLOAT) {
                stream << "    flw " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            } else if (pointeeType == TypeSpecifier::DOUBLE) {
                stream << "    fld " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            } else {
                stream << "    lw " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            }
            break;
        }

        case ast::UnaryOp::Type::PRE_INCREMENT: {
            // For ++x: load-> increment-> store-> return new value
            context.loadVariable(stream, resultReg, varName);
            stream << "    addi " << resultReg << ", " << resultReg << ", 1" << std::endl;
            context.storeVariable(stream, resultReg, varName);
            break;
        }

        case ast::UnaryOp::Type::POST_INCREMENT: {
            // For x++: load-> save original value-> increment-> store-> return original
            std::string tempReg = context.allocateRegister();
            context.loadVariable(stream, tempReg, varName);  // Load into temp
            stream << "    mv " << resultReg << ", " << tempReg << std::endl;  // Save original
            stream << "    addi " << tempReg << ", " << tempReg << ", 1" << std::endl;  // Increment temp
            context.storeVariable(stream, tempReg, varName);  // Store back
            context.freeRegister(tempReg);
            break;
        }

        case ast::UnaryOp::Type::PRE_DECREMENT: {
            context.loadVariable(stream, resultReg, varName);
            stream << "    addi " << resultReg << ", " << resultReg << ", -1" << std::endl;
            context.storeVariable(stream, resultReg, varName);
            break;
        }

        case ast::UnaryOp::Type::POST_DECREMENT: {
            std::string tempReg = context.allocateRegister();
            context.loadVariable(stream, tempReg, varName);
            stream << "    mv " << resultReg << ", " << tempReg << std::endl;
            stream << "    addi " << tempReg << ", " << tempReg << ", -1" << std::endl;
            context.storeVariable(stream, tempReg, varName);
            context.freeRegister(tempReg);
            break;
        }

        default:
            throw std::runtime_error("Unsupported unary operator");
    }

    currentExprResult = resultReg;
}

void CodeGenVisitor::visitLiteralExpression(const ast::LiteralExpression& expr) {
    switch (expr.getType()) {
        case ast::TypeSpecifier::INT:{
            std::string reg = context.allocateRegister();
            stream << "    li " << reg << ", " << expr.getIntValue() << std::endl;
            currentExprResult = reg;
            break;
        }
        case ast::TypeSpecifier::FLOAT:{
            float floatVal = expr.getFloatValue();
            std::string memLabel = context.getFloatLabel(floatVal);

            std::string intReg = context.allocateRegister();
            std::string floatReg = context.allocateFloatingRegister();
            stream << "    lui " << intReg << ",%hi(" << memLabel << ")" << std::endl;
            stream << "    flw " << floatReg << ",%lo(" << memLabel << ")(" << intReg << ")" << std::endl;
            context.freeRegister(intReg);
            currentExprResult = floatReg;
            context.storeFloatValue(floatVal);
            context.freeFloatingRegister(floatReg);
            break;
        }
        case ast::TypeSpecifier::DOUBLE:{
            double doubleVal = expr.getDoubleValue();
            std::string memLabel = context.getDoubleLabel(doubleVal);
            std::string intReg = context.allocateRegister();
            std::string floatReg = context.allocateFloatingRegister();
            stream << "    lui " << intReg << ",%hi(" << memLabel << ")" << std::endl;
            stream << "    fld " << floatReg << ",%lo(" << memLabel << ")(" << intReg << ")" << std::endl;
            context.freeRegister(intReg);
            currentExprResult = floatReg;
            context.storeDoubleValue(doubleVal);
            context.freeFloatingRegister(floatReg);
            break;
        }
        case ast::TypeSpecifier::CHAR:{
            std::string reg = context.allocateRegister();
            stream << "    li " << reg << ", " << static_cast<int>(expr.getCharValue()) << std::endl;
            currentExprResult = reg;
            break;
        }
        default:
            throw std::runtime_error("Unsupported literal type");
    }
}

void CodeGenVisitor::visitStringLiteralExpression(const ast::StringLiteralExpression& expr) {
    std::string stringValue = expr.getValue();
    std::string memLabel = context.getStringLabel(stringValue);
    std::string intReg = context.allocateRegister();
    stream << "    lui " << intReg << ",%hi(" << memLabel << ")" << std::endl;
    stream << "    addi " << intReg << ", " << intReg << ",%lo(" << memLabel << ")" << std::endl;
    currentExprResult = intReg;
    context.storeStringValue(stringValue);
    context.freeRegister(intReg);
}

void CodeGenVisitor::visitIdentifierExpression(const ast::IdentifierExpression& expr) {
    std::string name = expr.getName();
    bool exists = context.variableExists(name);

    //checking if identifier is an enum
    if (context.isEnumValue(name)) {
        std::string reg = context.allocateRegister();
        int value = context.getEnumValue(name);
        stream << "    li " << reg << ", " << value << std::endl;
        currentExprResult = reg;
        return;
    }

    if(exists) {
        auto& mutableExpr = const_cast<ast::IdentifierExpression&>(expr);
        mutableExpr.setType(context.getType(expr.getName()));
    }

    // first check global, then local
    if (context.isGlobal(name)) {
        if (expr.getType() == ast::TypeSpecifier::INT) {
            std::string reg1 = context.allocateRegister();
            std::string regDest = context.allocateRegister();
            stream << "    lui " << reg1 << ", %hi(" << name << ")" << std::endl;
            stream << "    lw " << regDest << ", " << "%lo(" << name << ")(" << reg1 << ")" << std::endl;
            context.freeRegister(reg1);
            currentExprResult = regDest;
        } else if (expr.getType() == ast::TypeSpecifier::FLOAT) {
            std::string reg1 = context.allocateRegister();
            std::string regDest = context.allocateFloatingRegister();
            stream << "    lui " << reg1 << ", %hi(" << name << ")" << std::endl;
            stream << "    flw " << regDest << ", " << "%lo(" << name << ")(" << reg1 << ")" << std::endl;
            context.freeRegister(reg1);
            currentExprResult = regDest;
        } else if (expr.getType() == ast::TypeSpecifier::DOUBLE) {
            std::string reg1 = context.allocateRegister();
            std::string regDest = context.allocateFloatingRegister();
            stream << "    lui " << reg1 << ", %hi(" << name << ")" << std::endl;
            stream << "    fld " << regDest << ", " << "%lo(" << name << ")(" << reg1 << ")" << std::endl;
            context.freeRegister(reg1);
            currentExprResult = regDest;
        } else if (expr.getType() == ast::TypeSpecifier::CHAR) {
            std::string reg1 = context.allocateRegister();
            std::string regDest = context.allocateRegister();
            stream << "    lui " << reg1 << ", %hi(" << name << ")" << std::endl;
            stream << "    lbu " << regDest << ", " << "%lo(" << name << ")(" << reg1 << ")" << std::endl;
            context.freeRegister(reg1);
            currentExprResult = regDest;
            }
    } else {
        if(expr.getType() == ast::TypeSpecifier::CHAR || expr.getType() == ast::TypeSpecifier::INT){
            std::string reg = context.allocateRegister();
            context.loadVariable(stream, reg, expr.getName());
            currentExprResult = reg;
        }
        else if(expr.getType() == ast::TypeSpecifier::DOUBLE || expr.getType() == ast::TypeSpecifier::FLOAT){
            std::string reg = context.allocateFloatingRegister();
            context.loadVariable(stream, reg, expr.getName());
            currentExprResult = reg;
        }
    }
}

void CodeGenVisitor::visitCallExpression(const ast::CallExpression& expr) {
    std::string result;
    int stackArgsSize = 0;

    //calculate stack space needed for arguments more than 8
    if (expr.hasArguments()) {
        const auto& argList = expr.getArguments();
        const auto& nodes = argList->getNodes();
        if (nodes.size() > 8) {
            stackArgsSize = (nodes.size() - 8) * 4;
            if (stackArgsSize % 16 != 0) {
                stackArgsSize = ((stackArgsSize + 15) / 16) * 16;
            }
            stream << "    addi sp, sp, -" << stackArgsSize << std::endl;
        }
    }

    if (expr.hasArguments()) {
        const auto& argList = expr.getArguments();
        const auto& nodes = argList->getNodes();
        // process args in reverse order for arguments stored on stack
        for (int i = nodes.size() - 1; i >= 0; i--) {
            auto* argExpr = dynamic_cast<const Expression*>(nodes[i].get());
            if(!argExpr) continue;
            std::cerr << "DEBUG: Arg expr before accept: type=" << argExpr->getType() << std::endl;
            argExpr->accept(*this);
            std::string argReg = getExpressionResult();
            if (i < 8) {
                if (argExpr->getType() == ast::TypeSpecifier::FLOAT ||
                    argExpr->getType() == ast::TypeSpecifier::DOUBLE) {
                    std::string destReg = "fa" + std::to_string(i);
                    if (argReg != destReg) {
                        if (argExpr->getType() == ast::TypeSpecifier::FLOAT) {
                            stream << "    fmv.s " << destReg << ", " << argReg << std::endl;
                        } else {
                            stream << "    fmv.d " << destReg << ", " << argReg << std::endl;
                        }
                        context.freeFloatingRegister(argReg);
                    }
                }
                // default: int argument registers
                else {
                    std::string destReg = "a" + std::to_string(i);
                    if (argReg != destReg) {
                        stream << "    mv " << destReg << ", " << argReg << std::endl;
                        context.freeRegister(argReg);
                    }
                }
            } else {
                // arguments now go on stack
                int stackOffset = (i - 8) * 4;  // 4 bytes per arg
                if (argExpr->getType() == ast::TypeSpecifier::FLOAT) {
                    stream << "    fsw " << argReg << ", " << stackOffset << "(sp)" << std::endl;
                    context.freeFloatingRegister(argReg);
                } else if (argExpr->getType() == ast::TypeSpecifier::DOUBLE) {
                    stream << "    fsd " << argReg << ", " << stackOffset << "(sp)" << std::endl;
                    context.freeFloatingRegister(argReg);
                } else {
                    stream << "    sw " << argReg << ", " << stackOffset << "(sp)" << std::endl;
                    context.freeRegister(argReg);
                }
            }
        }
    }

    context.saveRegisters(stream);

    const Expression* funcExpr = expr.getFunction();
    TypeSpecifier returnType = expr.getType(&context);

    if (const IdentifierExpression* idExpr = funcExpr->asIdentifierExpression()) {
        try {
            returnType = context.getFunctionReturnType(idExpr->getName());
        } catch (const std::runtime_error&) {
            // leave blank - can just use default int if can't find function
        }
    }

    if (const IdentifierExpression* idExpr = funcExpr->asIdentifierExpression()) {
        stream << "    call " << idExpr->getName() << std::endl;
    } else {
        funcExpr->accept(*this);
        std::string funcReg = getExpressionResult();

        stream << "    jalr " << funcReg << std::endl;
        context.freeRegister(funcReg);
    }

    if (stackArgsSize > 0) {
        stream << "    addi sp, sp, " << stackArgsSize << std::endl;
    }

    // Restore saved registers
    context.restoreRegisters(stream);

    std::string resultReg;
    if (returnType == ast::TypeSpecifier::FLOAT || returnType == ast::TypeSpecifier::DOUBLE) {
        resultReg = context.allocateFloatingRegister();
        if (resultReg != "fa0" && returnType == ast::TypeSpecifier::FLOAT) {
            stream << "    fmv.s " << resultReg << ", fa0" << std::endl;
        }
        if (resultReg != "fa0" && returnType == ast::TypeSpecifier::DOUBLE) {
            stream << "    fmv.d " << resultReg << ", fa0" << std::endl;
        }
    } else {
        resultReg = context.allocateRegister();
        if (resultReg != "a0") {
            stream << "    mv " << resultReg << ", a0" << std::endl;
        }
    }

    currentExprResult = resultReg;
}

void CodeGenVisitor::visitAssignmentExpression(const ast::AssignmentExpression& expr) {
    expr.getRHS()->accept(*this);
    std::string valueReg = getExpressionResult();

    // (=) assignment
    if (expr.getOperator() == ast::AssignOp::Type::ASSIGN) {
        const Expression* lhsExpr = expr.getLHS();

        // handling pointer assignments
        if(auto* unaryExpr = lhsExpr->asUnaryExpression()) {
            unaryExpr->getOperand()->accept(*this);
            std::string addrReg = getExpressionResult();
            stream << "    sw " << valueReg << ", 0(" << addrReg << ")" << std::endl;
            context.freeRegister(addrReg);
            currentExprResult = valueReg;
            return;
        }
        // for arrays LHS is ArrayAccessExpression
        if (auto* arrayExpr = lhsExpr->asArrayAccessExpression()) {
            if (auto* arrayID = (arrayExpr->getArray())->asIdentifierExpression()) {
                std::string arrayName = arrayID->getName();

                if(context.isGlobal(arrayName)) {
                    // global array store
                    arrayExpr->getIndex()->accept(*this);
                    std::string indexReg = getExpressionResult();
                    int elementSize = context.getTypeSize(context.getType(arrayName));
                    std::string offsetReg = context.allocateRegister({valueReg, indexReg});
                    stream << "    li " << offsetReg << ", " << elementSize << std::endl;
                    stream << "    mul " << offsetReg << ", " << indexReg << ", " << offsetReg << std::endl;

                    std::string addrReg = context.allocateRegister({valueReg, indexReg, offsetReg});
                    stream << "    lui " << addrReg << ", %hi(" << arrayName << ")" << std::endl;
                    stream << "    addi " << addrReg << ", " << addrReg << ", %lo(" << arrayName << ")" << std::endl;
                    stream << "    add " << addrReg << ", " << addrReg << ", " << offsetReg << std::endl;
                    stream << "    sw " << valueReg << ", 0(" << addrReg << ")" << std::endl;

                    context.freeRegister(indexReg);
                    context.freeRegister(offsetReg);
                    context.freeRegister(addrReg);
                } else {
                    // local array store
                    auto arrayVar = context.findVariable(arrayName);
                    arrayExpr->getIndex()->accept(*this);
                    std::string indexReg = getExpressionResult();

                    // calculating offset
                    std::string offsetReg = context.allocateRegister({valueReg, indexReg});
                    int elementSize = context.getTypeSize(arrayVar->type);
                    stream << "    li " << offsetReg << ", " << elementSize << std::endl;
                    stream << "    mul " << offsetReg << ", " << indexReg << ", " << offsetReg << std::endl;

                    // calculating final address
                    std::string addrReg = context.allocateRegister({valueReg, indexReg, offsetReg});
                    stream << "    add " << addrReg << ", s0, " << offsetReg << std::endl;
                    stream << "    add " << addrReg << ", " << addrReg << ", " << arrayVar->stack_offset << std::endl;
                    stream << "    sw " << valueReg << ", 0(" << addrReg << ")" << std::endl;

                    context.freeRegister(indexReg);
                    context.freeRegister(offsetReg);
                    context.freeRegister(addrReg);
                }


            }

        } else {
            // normal variable assignment
            auto* idExpr = lhsExpr->asIdentifierExpression();
            std::string varName = idExpr->getName();

            if (context.isGlobal(varName)) {
                std::string addrReg = context.allocateRegister({valueReg});
                stream << "    lui " << addrReg << ", %hi(" << varName << ")" << std::endl;
                stream << "    addi " << addrReg << ", " << addrReg << ", %lo(" << varName << ")" << std::endl;
                if(context.getType(varName) == TypeSpecifier::INT){
                    stream << "    sw " << valueReg << ", 0(" << addrReg << ")" << std::endl;
                }
                else if(context.getType(varName) == TypeSpecifier::FLOAT){
                    stream << "    fsw " << valueReg << ", 0(" << addrReg << ")" << std::endl;
                }
                else if(context.getType(varName) == TypeSpecifier::DOUBLE){
                    stream << "    fsd " << valueReg << ", 0(" << addrReg << ")" << std::endl;
                }
                else if(context.getType(varName) == TypeSpecifier::CHAR){
                    stream << "    sb " << valueReg << ", 0(" << addrReg << ")" << std::endl;
                }
                else{
                    throw std::runtime_error("Type not found");
                }
                context.freeRegister(addrReg);
            } else {
                context.storeVariable(stream, valueReg, varName);
            }
        }
        currentExprResult = valueReg;
    }

    // assignments like (+=, *= etc)
    else {
        const Expression* lhsExpr = expr.getLHS();
        std::string varName;

        if (auto* idExpr = lhsExpr->asIdentifierExpression()) {
            varName = idExpr->getName();

            std::string leftReg = context.allocateRegister();
            context.loadVariable(stream, leftReg, varName);

            expr.getRHS()->accept(*this);
            std::string rightReg = getExpressionResult();

            std::string resultReg = context.allocateRegister({leftReg, rightReg});

            switch (expr.getOperator()) {
                case ast::AssignOp::Type::ADD_ASSIGN:
                    stream << "    add " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::SUB_ASSIGN:
                    stream << "    sub " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::MUL_ASSIGN:
                    stream << "    mul " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::DIV_ASSIGN:
                    stream << "    div " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::MOD_ASSIGN:
                    stream << "    rem " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::AND_ASSIGN:
                    stream << "    and " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::OR_ASSIGN:
                    stream << "    or " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::XOR_ASSIGN:
                    stream << "    xor " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::LEFT_ASSIGN:
                    stream << "    sll " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;
                case ast::AssignOp::Type::RIGHT_ASSIGN:
                    stream << "    sra " << resultReg << ", " << leftReg << ", " << rightReg << std::endl;
                    break;

                default:
                    throw std::runtime_error("Unsupported compound assignment operator");
            }
            context.storeVariable(stream, resultReg, varName);
            context.freeRegister(leftReg);
            context.freeRegister(rightReg);
            currentExprResult = resultReg;
        }
    }
}

void CodeGenVisitor::visitArrayAccessExpression(const ast::ArrayAccessExpression& expr) {
    const IdentifierExpression* idExpr = expr.getArray()->asIdentifierExpression();
    if(idExpr){
        std::string arrayName = idExpr->getName();
        auto arrayVar = context.findVariable(arrayName);

        expr.getIndex()->accept(*this);
        std::string indexReg = getExpressionResult();

        int elementSize = context.getTypeSize(arrayVar->type);
        std::string offsetReg = context.allocateRegister({indexReg});
        stream << "    li " << offsetReg << ", " << elementSize << std::endl;
        stream << "    mul " << offsetReg << ", " << indexReg << ", " << offsetReg << std::endl;

        TypeSpecifier arrayType = context.getType(idExpr->getName());
        bool isFloatingType = (arrayType == ast::TypeSpecifier::FLOAT || arrayType == ast::TypeSpecifier::DOUBLE);

        std::string resultReg;

        if(isFloatingType){
            resultReg = context.allocateFloatingRegister({indexReg, offsetReg});
        } else {
            resultReg = context.allocateRegister({indexReg, offsetReg});
        }

        if (context.isGlobal(arrayName)) {
            // Global array access
            std::string addrReg = context.allocateRegister({indexReg, offsetReg, resultReg});
            stream << "    lui " << addrReg << ", %hi(" << arrayName << ")" << std::endl;
            stream << "    addi " << addrReg << ", " << addrReg << ", %lo(" << arrayName << ")" << std::endl;
            stream << "    add " << addrReg << ", " << addrReg << ", " << offsetReg << std::endl;

            if (arrayType == ast::TypeSpecifier::CHAR) {
                stream << "    lbu " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            } else if (arrayType == ast::TypeSpecifier::FLOAT) {
                stream << "    flw " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            } else if (arrayType == ast::TypeSpecifier::DOUBLE) {
                stream << "    fld " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            } else {
                stream << "    lw " << resultReg << ", 0(" << addrReg << ")" << std::endl;
            }
            context.freeRegister(addrReg);
        } else {
            // local array access
            if (arrayVar->is_array) {
                // calculating final addr - frame pointer + offset
                stream << "    add " << offsetReg << ", s0, " << offsetReg << std::endl;
                stream << "    add " << offsetReg << ", " << offsetReg << ", " << arrayVar->stack_offset << std::endl;
            } else if (arrayVar->is_pointer) {
                // Pointer indexing
                std::string ptrReg = context.allocateRegister({indexReg, offsetReg, resultReg});
                stream << "    lw " << ptrReg << ", " << arrayVar->stack_offset << "(s0)" << std::endl;
                stream << "    add " << offsetReg << ", " << ptrReg << ", " << offsetReg << std::endl;
                context.freeRegister(ptrReg);
            }

            if (arrayType == ast::TypeSpecifier::CHAR) {
                stream << "    lbu " << resultReg << ", 0(" << offsetReg << ")" << std::endl;
            } else if (arrayType == ast::TypeSpecifier::FLOAT) {
                stream << "    flw " << resultReg << ", 0(" << offsetReg << ")" << std::endl;
            } else if (arrayType == ast::TypeSpecifier::DOUBLE) {
                stream << "    fld " << resultReg << ", 0(" << offsetReg << ")" << std::endl;
            } else {
                stream << "    lw " << resultReg << ", 0(" << offsetReg << ")" << std::endl;
            }
        }

        context.freeRegister(indexReg);
        context.freeRegister(offsetReg);

        currentExprResult = resultReg;
    }
}

void CodeGenVisitor::visitMemberAccessExpression(const ast::MemberAccessExpression& expr) {
    (void)expr;
    throw std::runtime_error("Member access not implemented yet");
}

void CodeGenVisitor::visitPointerMemberAccessExpression(const ast::PointerMemberAccessExpression& expr) {
    (void)expr;
    throw std::runtime_error("Pointer member access not implemented yet");
}

void CodeGenVisitor::visitCastExpression(const ast::CastExpression& expr) {
    expr.getExpression()->accept(*this);
}

void CodeGenVisitor::visitConditionalExpression(const ast::ConditionalExpression& expr) {
    std::string falseLabel = context.generateUniqueLabel("condFalse");
    std::string endLabel = context.generateUniqueLabel("condEnd");
    expr.getThenExpression()->accept(*this); //To get type
    ast::TypeSpecifier Type = expr.getType();
    std::cerr << "Cond type: " << Type << std::endl;
    if(expr.getType() == ast::TypeSpecifier::INT){
        std::string resultReg = context.allocateRegister();
        expr.getCondition()->accept(*this);
        std::string condReg = getExpressionResult();
        stream << "    beqz " << condReg << ", " << falseLabel << std::endl;
        expr.getThenExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    mv " << resultReg << ", " << condReg << std::endl;
        stream << "    j " << endLabel << std::endl;
        stream << falseLabel << ":" << std::endl;
        expr.getElseExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    mv " << resultReg << ", " << condReg << std::endl;
        stream << endLabel << ":" << std::endl;
        currentExprResult = resultReg;
        context.freeRegister(resultReg);
    }
    else if(expr.getType() == ast::TypeSpecifier::FLOAT){
        std::string resultReg = context.allocateFloatingRegister();
        expr.getCondition()->accept(*this);
        std::string condReg = getExpressionResult();
        stream << "    beqz " << condReg << ", " << falseLabel << std::endl;
        expr.getThenExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    fmv.s " << resultReg << ", " << condReg << std::endl;
        stream << "    j " << endLabel << std::endl;
        stream << falseLabel << ":" << std::endl;
        expr.getElseExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    fmv.s " << resultReg << ", " << condReg << std::endl;
        stream << endLabel << ":" << std::endl;
        currentExprResult = resultReg;
        context.freeRegister(resultReg);

    }
    else if(expr.getType() == ast::TypeSpecifier::DOUBLE){
        std::string resultReg = context.allocateFloatingRegister();
        expr.getCondition()->accept(*this);
        std::string condReg = getExpressionResult();
        stream << "    beqz " << condReg << ", " << falseLabel << std::endl;
        expr.getThenExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    fmv.d " << resultReg << ", " << condReg << std::endl;
        stream << "    j " << endLabel << std::endl;
        stream << falseLabel << ":" << std::endl;
        expr.getElseExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    fmv.d " << resultReg << ", " << condReg << std::endl;
        stream << endLabel << ":" << std::endl;
        currentExprResult = resultReg;
        context.freeRegister(resultReg);
    }
    else if(expr.getType() == ast::TypeSpecifier::CHAR){
        std::string resultReg = context.allocateRegister();
        expr.getCondition()->accept(*this);
        std::string condReg = getExpressionResult();
        stream << "    beqz " << condReg << ", " << falseLabel << std::endl;
        expr.getThenExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    mv " << resultReg << ", " << condReg << std::endl;
        stream << "    j " << endLabel << std::endl;
        stream << falseLabel << ":" << std::endl;
        expr.getElseExpression()->accept(*this);
        condReg = getExpressionResult();
        stream << "    mv " << resultReg << ", " << condReg << std::endl;
        stream << endLabel << ":" << std::endl;
        currentExprResult = resultReg;
        context.freeRegister(resultReg);
    }
    else{
        throw std::runtime_error("Conditional op not compatible with type");
    }
}

void CodeGenVisitor::visitCommaExpression(const ast::CommaExpression& expr) {
    expr.getLeft()->accept(*this);
    context.freeRegister(currentExprResult);
    expr.getRight()->accept(*this);
}

void CodeGenVisitor::visitSizeofExpression(const ast::SizeofExpression& expr) {
    expr.getExpression()->accept(*this);
    int sizeOfValue = context.getTypeSize(expr.getType());

    const ast::Expression* sizeofExpr = expr.getExpression();
    std::string arrayName;
    if (const ast::IdentifierExpression* idExpr = sizeofExpr->asIdentifierExpression()) {
        arrayName = idExpr->getName();
    }
    int arraySizeMultiplier = context.findArraySize(arrayName);
    std::cerr << arraySizeMultiplier << std::endl;
    std::string reg = context.allocateRegister();
    stream << "    li " << reg << ", " << sizeOfValue*arraySizeMultiplier << std::endl;
    context.freeRegister(reg);
    currentExprResult = reg;
}

void CodeGenVisitor::visitSizeofTypeExpression(const ast::SizeofTypeExpression& expr) {
    int sizeOfValue = context.getTypeSize(expr.getTargetType());
    std::string reg = context.allocateRegister();
    stream << "    li " << reg << ", " << sizeOfValue << std::endl;
    context.freeRegister(reg);
    currentExprResult = reg;
}

void CodeGenVisitor::visitExpressionStatement(const ast::ExpressionStatement& stmt) {
    if (stmt.getExpression()) {
        stmt.getExpression()->accept(*this);

        if (!currentExprResult.empty()) {
            context.freeRegister(currentExprResult);
            currentExprResult.clear();
        }
    }
}

void CodeGenVisitor::visitCompoundStatement(const ast::CompoundStatement& stmt) {
    context.enterScope(false);

    const NodeList* declList = stmt.getDeclarationList();
    if (declList) {
        for (const auto& nodePtr : declList->getNodes()) {
            if (!nodePtr) {
                continue;
            }

            // direct variable declaration is tried first, then a nestedlist of declarations (nodelist)
            auto varDecl = dynamic_cast<const VariableDeclaration*>(nodePtr.get());
            if (varDecl) {
                varDecl->accept(*this);
                continue;
            }

            auto nestedList = dynamic_cast<const NodeList*>(nodePtr.get());
            if (nestedList) {
                for (const auto& declNode : nestedList->getNodes()) {
                    if (declNode) {
                        declNode->accept(*this);
                    }
                }
                continue;
            }

        }
    }

    const auto& statements = stmt.getStatements();
    for (const auto& s : statements) {
        if (s) {
            s->accept(*this);
        }
    }
    context.exitScope();
}

void CodeGenVisitor::visitIfStatement(const ast::IfStatement& stmt) {
    stmt.getCondition()->accept(*this);
    std::string condReg = getExpressionResult();

    std::string elseLabel = context.generateUniqueLabel("if_else");
    std::string endLabel = context.generateUniqueLabel("if_end");

    stream << "    beqz " << condReg << ", " << elseLabel << std::endl;
    context.freeRegister(condReg);
    stmt.getThenStatement()->accept(*this);

    if (stmt.hasElseStatement()) {
        stream << "    j " << endLabel << std::endl;
    }
    stream << elseLabel << ":" << std::endl;

    if (stmt.hasElseStatement()) {
        stmt.getElseStatement()->accept(*this);
        stream << endLabel << ":" << std::endl;
    }
}

void CodeGenVisitor::visitSwitchStatement(const ast::SwitchStatement& stmt) {
    stmt.getCondition()->accept(*this);
    std::string switchValueReg = getExpressionResult();
    context.setCurrentSwitchValue(switchValueReg);
    std::string endSwitchLabel = context.generateUniqueLabel("switch_end");
    context.pushBreakTarget(endSwitchLabel);
    stmt.getBody()->accept(*this);
    if (!pendingNextCaseLabel.empty()) {
        stream << pendingNextCaseLabel << ":" << std::endl;
        pendingNextCaseLabel.clear();
    }
    stream << endSwitchLabel << ":" << std::endl;
    context.popBreakTarget();
    context.clearCurrentSwitchValue();
    context.freeRegister(switchValueReg);
}

void CodeGenVisitor::visitCaseStatement(const ast::CaseStatement& stmt) {
    if (!pendingNextCaseLabel.empty()) {
        stream << pendingNextCaseLabel << ":" << std::endl;
        pendingNextCaseLabel.clear();
    }
    std::string caseLabel = context.generateUniqueLabel("case");
    std::string nextCaseLabel = context.generateUniqueLabel("next_case");
    std::string switchValueReg = context.getCurrentSwitchValue();
    if (stmt.isDefault()) {
        stream << caseLabel << ":" << std::endl;
    } else {
        stmt.getCaseValue()->accept(*this);
        std::string caseValueReg = getExpressionResult();
        stream << "    beq " << switchValueReg << ", " << caseValueReg << ", " << caseLabel << std::endl;
        stream << "    j " << nextCaseLabel << std::endl;
        stream << caseLabel << ":" << std::endl;
        context.freeRegister(caseValueReg);
    }
    if (stmt.getStatement()) {
        stmt.getStatement()->accept(*this);
        // if more cases
    }
    pendingNextCaseLabel = nextCaseLabel;
}

void CodeGenVisitor::visitDefaultStatement(const ast::DefaultStatement& stmt) {
    std::string defaultLabel = context.generateUniqueLabel("default");
    stream << defaultLabel << ":" << std::endl;
    stmt.getStatement()->accept(*this);
}

void CodeGenVisitor::visitWhileStatement(const ast::WhileStatement& stmt) {
    std::string startLabel = context.generateUniqueLabel("while_start");
    std::string endLabel = context.generateUniqueLabel("while_end");
    // handle potential break and continue statements
    context.pushBreakTarget(endLabel);
    context.pushContinueTarget(startLabel);
    stream << startLabel << ":" << std::endl;

    stmt.getCondition()->accept(*this);
    std::string condReg = getExpressionResult();

    stream << "    beqz " << condReg << ", " << endLabel << std::endl;
    context.freeRegister(condReg);

    stmt.getBody()->accept(*this);

    stream << "    j " << startLabel << std::endl;
    stream << endLabel << ":" << std::endl;

    context.popBreakTarget();
    context.popContinueTarget();
}

void CodeGenVisitor::visitDoWhileStatement(const ast::DoWhileStatement& stmt) {
    std::string startLabel = context.generateUniqueLabel("do_start");
    std::string condLabel = context.generateUniqueLabel("do_cond");

    stream << startLabel << ":" << std::endl;

    stmt.getBody()->accept(*this);

    stream << condLabel << ":" << std::endl;
    stmt.getCondition()->accept(*this);
    std::string condReg = getExpressionResult();

    stream << "    bnez " << condReg << ", " << startLabel << std::endl;
    context.freeRegister(condReg);
}

void CodeGenVisitor::visitForStatement(const ast::ForStatement& stmt) {
    std::string initLabel = context.generateUniqueLabel("for_init");
    std::string condLabel = context.generateUniqueLabel("for_cond");
    std::string incrLabel = context.generateUniqueLabel("for_incr");
    std::string bodyLabel = context.generateUniqueLabel("for_body");
    std::string endLabel = context.generateUniqueLabel("for_end");

    context.pushBreakTarget(endLabel);
    context.pushContinueTarget(incrLabel);

    stream << initLabel << ":" << std::endl;
    if (stmt.hasInitialization()) {
        stmt.getInitialization()->accept(*this);
        if (!currentExprResult.empty()) {
            context.freeRegister(currentExprResult);
            currentExprResult.clear();
        }
    }

    stream << "    j " << condLabel << std::endl;
    stream << bodyLabel << ":" << std::endl;
    stmt.getBody()->accept(*this);

    stream << incrLabel << ":" << std::endl;
    if (stmt.hasIncrement()) {
        stmt.getIncrement()->accept(*this);
        if (!currentExprResult.empty()) {
            context.freeRegister(currentExprResult);
            currentExprResult.clear();
        }
    }

    stream << condLabel << ":" << std::endl;
    if (stmt.hasCondition()) {
        stmt.getCondition()->accept(*this);
        std::string condReg = getExpressionResult();
        stream << "    bnez " << condReg << ", " << bodyLabel << std::endl;
        context.freeRegister(condReg);
    } else {
        stream << "    j " << bodyLabel << std::endl;
    }

    stream << endLabel << ":" << std::endl;

    context.pushBreakTarget(endLabel);
    context.pushContinueTarget(incrLabel);
}


void CodeGenVisitor::visitReturnStatement(const ast::ReturnStatement& stmt) {
    if (stmt.hasExpression()) {
        stmt.getExpression()->accept(*this);
        std::string resultReg = getExpressionResult();
        std::string currentFunc = context.getCurrentFunction();
        auto returnType = context.getFunctionReturnType(currentFunc);

        if(returnType == ast::TypeSpecifier::FLOAT) {
            stream << "    fmv.s fa0, " << resultReg << std::endl;
            context.freeFloatingRegister(resultReg);
        }
        else if(returnType == ast::TypeSpecifier::DOUBLE) {
            stream << "    fmv.d fa0, " << resultReg << std::endl;
            context.freeFloatingRegister(resultReg);
        }
        else {
            stream << "    mv a0, " << resultReg << std::endl;
            context.freeRegister(resultReg);
        }
    }

    std::string currentFunc = context.getCurrentFunction();
    stream << "    j " << context.getFunctionEndLabel(currentFunc) << std::endl;
}

void CodeGenVisitor::visitBreakStatement(const ast::BreakStatement& stmt) {
    (void)stmt;
    std::string breakTarget = context.getCurrentBreakTarget();
    stream << "    j " << breakTarget << std::endl;
}

void CodeGenVisitor::visitContinueStatement(const ast::ContinueStatement& stmt) {
    (void)stmt;
    std::string continueTarget = context.getContinueTarget();
    stream << "    j " << continueTarget << std::endl;
}

void CodeGenVisitor::visitGotoStatement(const ast::GotoStatement& stmt) {
    std::string labelName = stmt.getLabel()->getName();
    stream << "    j " << labelName << std::endl;
}

void CodeGenVisitor::visitLabeledStatement(const ast::LabeledStatement& stmt) {
    stream << stmt.getLabel()->getName() << ":" << std::endl;

    stmt.getStatement()->accept(*this);
}



/*******************  DECLARATOR FUNCTIONS **********************/

void CodeGenVisitor::visitIdentifierDeclarator(const ast::IdentifierDeclarator& decl) {
    // don't actually do codegen
    // just passing on information to higher-level visitor Declaration functions
    currentIdentifier = decl.getIdentifier();
    isPointerType = false;
    isArrayType = false;
    isFunctionType = false;
}

void CodeGenVisitor::visitFunctionDeclarator(const ast::FunctionDeclarator& decl) {
    // don't actually do codegen
    // just passing on information to higher-level visitor Declaration functions
    std::shared_ptr<Declarator> baseDecl = decl.getBaseDeclaratorPtr();
    if (baseDecl) {
        baseDecl->accept(*this);
    }

    isFunctionType = true;

    parameterList.clear();

    if (decl.getParameters()) {
        const auto& params = decl.getParameters()->getParameters();

        for (size_t i = 0; i < params.size(); ++i) {
            auto paramDecl = std::dynamic_pointer_cast<const ParameterDeclaration>(params[i]);
            if (paramDecl) {
                ParameterInfo info;
                info.name = paramDecl->getIdentifier();
                info.type = paramDecl->getType();
                info.isPointer = paramDecl->isPointer();
                info.index = i;
                parameterList.push_back(info);
            }
        }
    }
}

void CodeGenVisitor::visitArrayDeclarator(const ast::ArrayDeclarator& decl) {
    decl.getBaseDeclaratorPtr()->accept(*this);
    if (decl.getSize()) {
        decl.getSize()->accept(*this);
        std::string sizeReg = getExpressionResult();
        currentArraySize = sizeReg;
    } else {
        currentArraySize = "0"; // Or some default value
    }
    isArrayType = true;
}


void CodeGenVisitor::visitPointerDeclarator(const ast::PointerDeclarator& decl) {
    decl.getBaseDeclaratorPtr()->accept(*this);

    isPointerType = true;
}


void CodeGenVisitor::visitParameterDeclaration(const ast::ParameterDeclaration& decl) {
    if (decl.hasDeclarator()) {
        context.declareParameter(decl.getIdentifier(), decl.getType(), decl.isPointer());
    } else {
        // Handle unnamed parameter (like in function prototypes)
        context.declareUnnamedParameter(decl.getType());
    }
}

void CodeGenVisitor::visitParameterList(const ast::ParameterList& list) {
    for (const auto& param : list.getParameters()) {
        param->accept(*this);
    }
}

void CodeGenVisitor::visitInitDeclarator(const ast::InitDeclarator& decl) {
    if (decl.getDeclarator()) {
        decl.getDeclarator()->accept(*this);
    }

    if (decl.getInitializer()) {
        decl.getInitializer()->accept(*this);
        initializerValue = getExpressionResult();
    } else {
        initializerValue.clear();
    }
}

void CodeGenVisitor::visitInitializerList(const ast::InitializerList& list) {
    /* used when passing arrays as a parameter
       initArray is used for array initializations */
    (void)list;
    std::string reg = context.allocateRegister();
    stream << "    addi " << reg << ", s0, 0" << std::endl;
    currentExprResult = reg;
}

void CodeGenVisitor::initArray(const ast::VariableDeclaration& decl) {
    auto* initList = decl.getInitializer()->asInitializerList();
    std::string arrayName = decl.getIdentifier();
    auto var = context.findVariable(arrayName);
    int elementSize = context.getTypeSize(decl.getType());
    int baseAddress = var->stack_offset;

    const auto& expressions = initList->getExpressions();
    for (size_t i = 0; i < expressions.size(); ++i) {
        expressions[i]->accept(*this);
        std::string valueReg = getExpressionResult();
        // calculate offset at compile time
        int offset = baseAddress + (i * elementSize);

        if (decl.getType() == ast::TypeSpecifier::FLOAT) {
            stream << "    fsw " << valueReg << ", " << offset << "(s0)" << std::endl;
            context.freeFloatingRegister(valueReg);

        } else if (decl.getType() == ast::TypeSpecifier::DOUBLE) {
            stream << "    fsd " << valueReg << ", " << offset << "(s0)" << std::endl;
            context.freeFloatingRegister(valueReg);

        } else if (decl.getType() == ast::TypeSpecifier::CHAR) {
            stream << "    sb " << valueReg << ", " << offset << "(s0)" << std::endl;
            context.freeRegister(valueReg);

        } else {
            stream << "    sw " << valueReg << ", " << offset << "(s0)" << std::endl;
            context.freeRegister(valueReg);
        }
    }
}

void CodeGenVisitor::visitEnumValue(const ast::EnumValue& value) {
    // should be handled in the EnumDeclaration visitor
    (void)value;
}

void CodeGenVisitor::visitEnumDeclaration(const ast::EnumDeclaration& decl) {
    EnumType enumType(decl.getName());

    int nextValue = 0;
    for (const auto& valuePtr : decl.getValues()) {
        if (valuePtr->hasValue()) {
            valuePtr->getValue()->accept(*this);
            std::string resultReg = getExpressionResult();

            const auto* literalExpr = valuePtr->getValue()->asLiteralExpression();
            if (literalExpr && literalExpr->getType() == ast::TypeSpecifier::INT) {
                nextValue = literalExpr->getIntValue();
            } else {
                nextValue = 0;
            }

            if (!currentExprResult.empty()) {
                context.freeRegister(currentExprResult);
                currentExprResult.clear();
            }
        }

        enumType.addValue(valuePtr->getName(), nextValue);
        nextValue++;
    }
    context.addEnumType(enumType);
}

} // namespace codegen
