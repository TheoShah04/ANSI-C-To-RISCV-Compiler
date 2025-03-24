#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "ast_node.hpp"
#include "ast_type_specifier.hpp"
#include "ast_context.hpp"
#include "codegen_visitor.hpp"
#include "Declaration.hpp"
#include "Declarator.hpp"
#include "DeclarationStatement.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include "Identifier.hpp"
#include "EnumDeclaration.hpp"
#include "Visitor.hpp"
#include "Factory.hpp"


ast::NodePtr ParseAST(std::string file_name);
