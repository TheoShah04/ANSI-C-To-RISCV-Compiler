%code requires{
    #include "ast_node.hpp"
    #include "Expression.hpp"
    #include "Statement.hpp"
    #include "Declaration.hpp"
    #include "Declarator.hpp"
    #include "ast_context.hpp"
    #include "ast_type_specifier.hpp"
    #include "EnumDeclaration.hpp"
    #include "Factory.hpp"
    #include <string>

    using namespace ast;

    extern int yylineno;
    extern char* yytext;

    extern std::shared_ptr<Node> g_root;
    extern FILE* yyin;
    int yylex(void);
    void yyerror(const char*);
    int yylex_destroy(void);
    void updateTypeDefs(std::string id, ast::TypeSpecifier type);
    ast::TypeSpecifier getTypeDefType(std::string id);
}

%define parse.error detailed
%define parse.lac full

%union{
    std::shared_ptr<Node>*              node_ptr;
    std::shared_ptr<NodeList>*          node_list_ptr;
    std::shared_ptr<ParameterList>*     parameter_list_ptr;
    std::shared_ptr<Declarator>*        declarator_ptr;
    std::shared_ptr<Expression>*        expr_ptr;
    std::shared_ptr<Statement>*         stmt_ptr;
    std::shared_ptr<Identifier>*        identifier_ptr;
    int                                 number_int;
    float                               number_float;
    double                              number_double;
    char                                number_char;
    std::string*                        string;
    TypeSpecifier                       type_specifier;
    BinaryOp::Type                      binary_op;
    UnaryOp::Type                       unary_op;
    AssignOp::Type                      assign_op;
}

%token IDENTIFIER INT_CONSTANT FLOAT_CONSTANT DOUBLE_CONSTANT CHAR_CONSTANT STRING_LITERAL
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP
%token MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token TYPEDEF EXTERN STATIC AUTO REGISTER INLINE SIZEOF
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS
%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN
%token UNKNOWN
%token <string> TYPE_NAME
%token END_OF_CODE

%type <node_ptr> external_declaration function_definition enumerator
%type <node_list_ptr> declaration_list statement_list argument_list init_declarator_list translation_unit enumerator_list end_of_file
%type <parameter_list_ptr> parameter_list
%type <expr_ptr> primary_expression postfix_expression unary_expression cast_expression
%type <expr_ptr> multiplicative_expression additive_expression shift_expression relational_expression
%type <expr_ptr> equality_expression and_expression exclusive_or_expression inclusive_or_expression
%type <expr_ptr> logical_and_expression logical_or_expression conditional_expression assignment_expression
%type <expr_ptr> expression constant_expression initializer initializer_list
%type <stmt_ptr> statement compound_statement expression_statement selection_statement
%type <stmt_ptr> jump_statement labeled_statement iteration_statement
%type <node_ptr> declaration parameter_declaration enum_specifier
%type <declarator_ptr> declarator direct_declarator
%type <identifier_ptr> identifier
%type <string> IDENTIFIER STRING_LITERAL
%type <number_int> INT_CONSTANT
%type <number_float> FLOAT_CONSTANT
%type <number_double> DOUBLE_CONSTANT
%type <number_char> CHAR_CONSTANT
%type <type_specifier> type_specifier declaration_specifiers
%type <binary_op> binary_operator
%type <unary_op> unary_operator
%type <assign_op> assignment_operator

%start ROOT
%%


ROOT
    : translation_unit {g_root = *$1; delete $1;}
    ;

translation_unit
    : external_declaration
        {
            auto list = ast::makeNodeList();
            list->PushBack(*$1);
            $$ = new std::shared_ptr<NodeList>(list);
            delete $1;
        }
    | translation_unit external_declaration
        {
            (*$1)->PushBack(*$2);
            $$ = $1;
            delete $2;
        }
    ;

external_declaration
    : function_definition { $$ = $1; }
    | declaration { $$ = $1; }
    ;

function_definition
    : declaration_specifiers declarator compound_statement
        {
            auto fn = std::dynamic_pointer_cast<FunctionDeclarator>(*$2);
            if (fn) {
                auto compoundStmt = std::dynamic_pointer_cast<CompoundStatement>(*$3);
                if (compoundStmt) {
                    $$ = new std::shared_ptr<Node>(makeFunctionDeclaration($1, fn, compoundStmt));
                } else {
                    $$ = new std::shared_ptr<Node>(makeFunctionDeclaration($1, fn, nullptr));
                }

                if (fn->getParameters()) {
                    const auto& params = fn->getParameters()->getParameters();
                    std::cerr << "Function parameters: ";
                    for (const auto& param : params) {
                        auto paramDecl = std::dynamic_pointer_cast<ParameterDeclaration>(param);
                        if (paramDecl) {
                            std::cerr << paramDecl->getIdentifier() << " ";
                        }
                    }
                    std::cerr << std::endl;
                }

            } else {
                auto fnDecl = makeFunctionDeclarator(*$2, nullptr);
                auto compoundStmt = std::dynamic_pointer_cast<CompoundStatement>(*$3);
                $$ = new std::shared_ptr<Node>(makeFunctionDeclaration($1, fnDecl, compoundStmt));
            }
            delete $2;
            delete $3;
        }
    ;

declaration
    : TYPEDEF declaration_specifiers init_declarator_list ';'
        {
            for (const auto &node : (*$3)->getNodes())
            {
                auto initDecl = std::dynamic_pointer_cast<InitDeclarator>(node);
                if (initDecl) {
                    auto ident = initDecl->getDeclarator();
                    if (ident) {
                        updateTypeDefs(ident->getIdentifier(), $2);
                    }
                }
            }
            auto initDeclarator = std::dynamic_pointer_cast<InitDeclarator>((*$3)->getNodes()[0]);
            $$ = new std::shared_ptr<Node>(makeVariableDeclaration($2, initDeclarator->getDeclarator(), initDeclarator->getInitializer()));
            delete $3;
        }
    | declaration_specifiers init_declarator_list ';'
        {
            // if there is only one declaration (case most of the time) -> return single variable decl
            if ((*$2)->getNodes().size() == 1) {
                auto initDeclarator = std::dynamic_pointer_cast<InitDeclarator>((*$2)->getNodes()[0]);
                if (initDeclarator) {
                    $$ = new std::shared_ptr<Node>(makeVariableDeclaration(
                        $1, initDeclarator->getDeclarator(), initDeclarator->getInitializer()));
                }
                else {
                    $$ = new std::shared_ptr<Node>(makeNodeList());
                } /*empty list if smth wrong*/
            }
            else {
                // for multiple declarations
                auto decls = makeNodeList();
                for (auto& initDecl : (*$2)->getNodes()) {
                    auto initDeclarator = std::dynamic_pointer_cast<InitDeclarator>(initDecl);
                    if (initDeclarator) {
                        auto decl = makeVariableDeclaration(
                            $1, initDeclarator->getDeclarator(), initDeclarator->getInitializer());
                        decls->PushBack(decl);
                    }
                }
                $$ = new std::shared_ptr<Node>(decls);
            }
            delete $2;
        }
    | declaration_specifiers ';'
        { $$ = new std::shared_ptr<Node>(makeNodeList()); }
    | enum_specifier ';'
        { $$ = $1; }
    ;

enum_specifier
    : ENUM '{' enumerator_list '}'
        {
            auto enumDecl = makeEnumDeclaration(nullptr);
            for (auto& node : (*$3)->getNodes()) {
                auto enumValue = std::dynamic_pointer_cast<EnumValue>(node);
                if (enumValue) {
                    enumDecl->addValue(enumValue);
                }
            }
            $$ = new std::shared_ptr<Node>(enumDecl);
            delete $3;
        }
    | ENUM identifier '{' enumerator_list '}'
        {
            auto enumDecl = makeEnumDeclaration(*$2);
            for (auto& node : (*$4)->getNodes()) {
                auto enumValue = std::dynamic_pointer_cast<EnumValue>(node);
                if (enumValue) {
                    enumDecl->addValue(enumValue);
                }
            }
            $$ = new std::shared_ptr<Node>(enumDecl);
            delete $2; delete $4;
        }
    | ENUM identifier
        {$$ = new std::shared_ptr<Node>(makeEnumDeclaration(*$2)); delete $2;}
    ;

enumerator_list
    : enumerator
        {
            auto list = makeNodeList();
            list->PushBack(*$1);
            $$ = new std::shared_ptr<NodeList>(list);
            delete $1;
        }
    | enumerator_list ',' enumerator
        {(*$1)->PushBack(*$3); $$ = $1; delete $3;}
    ;

enumerator
    : identifier
        {$$ = new std::shared_ptr<Node>(makeEnumValue(*$1)); delete $1;}
    | identifier '=' constant_expression
        { $$ = new std::shared_ptr<Node>(makeEnumValue(*$1, *$3)); delete $1; delete $3;}
    ;

declaration_specifiers
    : type_specifier { $$ = $1; }
    | CONST type_specifier { $$ = $2; /* Ignore const for simplicity */ }
    | type_specifier CONST { $$ = $1; /* Ignore const for simplicity */ }
    ;

type_specifier
    : VOID { $$ = TypeSpecifier::VOID; }
    | CHAR { $$ = TypeSpecifier::CHAR; }
    | INT { $$ = TypeSpecifier::INT; }
    | FLOAT { $$ = TypeSpecifier::FLOAT; }
    | DOUBLE { $$ = TypeSpecifier::DOUBLE; }
    | TYPE_NAME { $$ = getTypeDefType(*$1); }
    | SIGNED { $$ = TypeSpecifier::SIGNED; }
    | UNSIGNED { $$ = TypeSpecifier::UNSIGNED; }
    ;

init_declarator_list
    : declarator
        {
            auto list = makeInitDeclaratorList();
            auto initDecl = makeInitDeclarator(*$1, nullptr);
            list->PushBack(initDecl);
            $$ = new std::shared_ptr<NodeList>(list);
            delete $1;
        }
    | init_declarator_list ',' declarator
        {
            auto initDecl = makeInitDeclarator(*$3, nullptr);
            (*$1)->PushBack(initDecl);
            $$ = $1;
            delete $3;
        }
    | declarator '=' initializer
        {
                auto list = makeInitDeclaratorList();
                auto initDecl = makeInitDeclarator(*$1, *$3);
                list->PushBack(initDecl);
                $$ = new std::shared_ptr<NodeList>(list);

                if ((*$1)->isPointer() && (*$3)->getType() == TypeSpecifier::CHAR) {
                auto stringExpr = std::dynamic_pointer_cast<ast::StringLiteralExpression>(*($3));
                if (stringExpr) {
                    int charCount = stringExpr->getSize();
                    std::cerr << "String array declaration of length: " << charCount << std::endl;
                    auto arrayDecl = makeArrayDeclarator(*$1, std::make_shared<ast::LiteralExpression>(4)); //size should be a word no matter what (4 CHARS)
                    list->PushBack(arrayDecl);
                    }
                }
                delete $1; delete $3;


        }
    | init_declarator_list ',' declarator '=' initializer
        {
            auto initDecl = makeInitDeclarator(*$3, *$5);
            (*$1)->PushBack(initDecl);
            $$ = $1;
            delete $3; delete $5;
        }
    ;

declarator
    : direct_declarator { $$ = $1; }
    | '*' declarator {
            // std::string id = (*$2)->getIdentifier();
            // std::cerr << (*$2)->getIdentifier() << std::endl;
            $$ = new std::shared_ptr<Declarator>(makePointerDeclarator(*$2));
            std::cerr << "Pointer found" << std::endl;
            delete $2;
        }
    ;

direct_declarator
    : IDENTIFIER
            {$$ = new std::shared_ptr<Declarator>(makeIdentifierDeclarator(*$1)); delete $1;}
    | '(' declarator ')' { $$ = $2; }
    | direct_declarator '[' ']'
            {$$ = new std::shared_ptr<Declarator>(makeArrayDeclarator(*$1, nullptr)); delete $1;}
    | direct_declarator '[' constant_expression ']' {
            $$ = new std::shared_ptr<Declarator>(makeArrayDeclarator(*$1, *$3));
            delete $1; delete $3;
        }
    | direct_declarator '(' parameter_list ')' {
            $$ = new std::shared_ptr<Declarator>(makeFunctionDeclarator(*$1, *$3));
            delete $1; delete $3;
        }
    | direct_declarator '(' ')'
            {$$ = new std::shared_ptr<Declarator>(makeFunctionDeclarator(*$1, nullptr)); delete $1;}
    ;

parameter_list
    : parameter_declaration
        {
            auto list = makeParameterList();
            list->addParameter(*$1);
            $$ = new std::shared_ptr<ParameterList>(list);
            delete $1;
        }
    | parameter_list ',' parameter_declaration
        {
            (*$1)->addParameter(*$3);
            $$ = $1;
            delete $3;
        }
    ;

parameter_declaration
    : declaration_specifiers declarator
        {
            $$ = new std::shared_ptr<Node>(makeParameterDeclaration($1, *$2));
            delete $2;
        }
    | declaration_specifiers
        {$$ = new std::shared_ptr<Node>(makeParameterDeclaration($1, nullptr));}
    ;

identifier
    : IDENTIFIER
            {$$ = new std::shared_ptr<Identifier>(makeIdentifier(*$1)); delete $1;}
    ;

initializer
    : assignment_expression { $$ = $1; }
    | '{' initializer_list '}' { $$ = $2; }
    | '{' initializer_list ',' '}' { $$ = $2; }
    | STRING_LITERAL {
            $$ = new std::shared_ptr<Expression>(makeStringLiteralExpression(*$1));
            delete $1;
        }
    ;

initializer_list
    : initializer
        {
            auto list = makeInitializerList();
            list->addExpression(*$1);
            $$ = new std::shared_ptr<Expression>(list);
            delete $1;
        }
    | initializer_list ',' initializer
        {
            auto list = std::dynamic_pointer_cast<InitializerList>(*$1);
            list->addExpression(*$3);
            $$ = $1;
            delete $3;
        }
    ;

statement
    : labeled_statement { $$ = $1; }
    | compound_statement { $$ = $1; }
    | expression_statement { $$ = $1; }
    | selection_statement { $$ = $1; }
    | iteration_statement { $$ = $1; }
    | jump_statement { $$ = $1; }
    ;

compound_statement
    : '{' '}' {
            $$ = new std::shared_ptr<Statement>(makeCompoundStatement());
        }
    | '{' statement_list '}' {
            $$ = new std::shared_ptr<Statement>(makeCompoundStatement(*$2));
            delete $2;
        }
    | '{' declaration_list '}'
        {
            auto compStmt = makeCompoundStatement();
            compStmt->setDeclarationList(*$2);
            $$ = new std::shared_ptr<Statement>(compStmt);
            delete $2;
        }
    | '{' declaration_list statement_list '}'
        {
            auto compStmt = makeCompoundStatement();
            compStmt->setDeclarationList(*$2);

            // Add the statements
            for (auto& node : (*$3)->getNodes()) {
                auto stmt = std::dynamic_pointer_cast<Statement>(node);
                if (stmt) {
                    compStmt->addStatement(stmt);
                }
            }

            $$ = new std::shared_ptr<Statement>(compStmt);
            delete $2;
            delete $3;
        }
    ;

statement_list
    : statement
        {
            auto list = makeNodeList();
            list->PushBack(*$1);
            $$ = new std::shared_ptr<NodeList>(list);
            delete $1;
        }
    | statement_list statement
        {
            (*$1)->PushBack(*$2);
            $$ = $1;
            delete $2;
        }
    ;

declaration_list
    : declaration
        {
            auto list = makeNodeList();
            list->PushBack(*$1);
            $$ = new std::shared_ptr<NodeList>(list);
            delete $1;
        }
    | declaration_list declaration
        {
            (*$1)->PushBack(*$2);
            $$ = $1;
            delete $2;
        }
    ;

expression_statement
    : ';' {$$ = new std::shared_ptr<Statement>(makeExpressionStatement(nullptr));}
    | expression ';' {$$ = new std::shared_ptr<Statement>(makeExpressionStatement(*$1)); delete $1;}
    ;

selection_statement
    : IF '(' expression ')' statement
        {
            $$ = new std::shared_ptr<Statement>(makeIfStatement(*$3, *$5, nullptr));
            delete $3; delete $5;
        }
    | IF '(' expression ')' statement ELSE statement
        {
            $$ = new std::shared_ptr<Statement>(makeIfStatement(*$3, *$5, *$7));
            delete $3; delete $5; delete $7;
        }
    | SWITCH '(' expression ')' statement
        {
            $$ = new std::shared_ptr<Statement>(makeSwitchStatement(*$3, *$5));
            delete $3; delete $5;
        }
    ;

labeled_statement
    : IDENTIFIER ':' statement {
            auto id = makeIdentifier(*$1);
            $$ = new std::shared_ptr<Statement>(makeLabeledStatement(id, *$3));
            delete $1; delete $3;
        }
    | CASE constant_expression ':' statement
        {
            $$ = new std::shared_ptr<Statement>(makeCaseStatement(*$2, *$4));
            delete $2; delete $4;
        }
    | DEFAULT ':' statement
        {$$ = new std::shared_ptr<Statement>(makeDefaultStatement(*$3)); delete $3;}
    ;

iteration_statement
    : WHILE '(' expression ')' statement
        {
            $$ = new std::shared_ptr<Statement>(makeWhileStatement(*$3, *$5));
            delete $3; delete $5;
        }
    | DO statement WHILE '(' expression ')' ';'
        {
            $$ = new std::shared_ptr<Statement>(makeDoWhileStatement(*$2, *$5));
            delete $2; delete $5;
        }
    | FOR '(' expression_statement expression_statement ')' statement
    {
        // Cast to ExpressionStatement
        auto initStmt = std::dynamic_pointer_cast<ExpressionStatement>(*$3);
        auto condStmt = std::dynamic_pointer_cast<ExpressionStatement>(*$4);

        std::shared_ptr<Expression> init = initStmt ? initStmt->getExpression() : nullptr;
        std::shared_ptr<Expression> cond = condStmt ? condStmt->getExpression() : nullptr;

        $$ = new std::shared_ptr<Statement>(makeForStatement(init, cond, nullptr, *$6));
        delete $3; delete $4; delete $6;
    }
    | FOR '(' expression_statement expression_statement expression ')' statement
    {
        auto initStmt = std::dynamic_pointer_cast<ExpressionStatement>(*$3);
        auto condStmt = std::dynamic_pointer_cast<ExpressionStatement>(*$4);

        std::shared_ptr<Expression> init = initStmt ? initStmt->getExpression() : nullptr;
        std::shared_ptr<Expression> cond = condStmt ? condStmt->getExpression() : nullptr;

        $$ = new std::shared_ptr<Statement>(makeForStatement(init, cond, *$5, *$7));
        delete $3; delete $4; delete $5; delete $7;
    }
    ;

jump_statement
    : GOTO IDENTIFIER ';'
        {
            auto id = makeIdentifier(*$2);
            $$ = new std::shared_ptr<Statement>(makeGotoStatement(id));
            delete $2;
        }
    | CONTINUE ';'
        {$$ = new std::shared_ptr<Statement>(makeContinueStatement());}
    | BREAK ';'
        {$$ = new std::shared_ptr<Statement>(makeBreakStatement());}
    | RETURN ';'
        { $$ = new std::shared_ptr<Statement>(makeReturnStatement(nullptr));}
    | RETURN expression ';'
        {$$ = new std::shared_ptr<Statement>(makeReturnStatement(*$2)); delete $2;}
    ;

primary_expression
    : identifier {
            $$ = new std::shared_ptr<Expression>(makeIdentifierExpression(*$1));
            delete $1;
        }
    | INT_CONSTANT {$$ = new std::shared_ptr<Expression>(makeLiteralExpression($1));}
    | FLOAT_CONSTANT {$$ = new std::shared_ptr<Expression>(makeLiteralExpression($1));}
    | DOUBLE_CONSTANT {$$ = new std::shared_ptr<Expression>(makeLiteralExpression($1));}
    | CHAR_CONSTANT {$$ = new std::shared_ptr<Expression>(makeLiteralExpression($1));
                    std::cerr << "Declaring char: " << static_cast<char>($1) << std::endl;}
    | STRING_LITERAL {$$ = new std::shared_ptr<Expression>(makeStringLiteralExpression(*$1)); delete $1;
                    std::cerr << "Declaring string" << std::endl;}
    | '(' expression ')' { $$ = $2; }
    ;

postfix_expression
    : primary_expression { $$ = $1; }
    | postfix_expression '[' expression ']' {
            $$ = new std::shared_ptr<Expression>(makeArrayAccessExpression(*$1, *$3));
            delete $1; delete $3;
        }
    | postfix_expression '(' ')' {$$ = new std::shared_ptr<Expression>(makeCallExpression(*$1, nullptr)); delete $1;}
    | postfix_expression '(' argument_list ')' {
            $$ = new std::shared_ptr<Expression>(makeCallExpression(*$1, *$3));
            delete $1; delete $3;
        }
    | postfix_expression '.' identifier {
            $$ = new std::shared_ptr<Expression>(makeMemberAccessExpression(*$1, *$3));
            delete $1; delete $3;
        }
    | postfix_expression PTR_OP identifier {
            $$ = new std::shared_ptr<Expression>(makePointerMemberAccessExpression(*$1, *$3));
            delete $1; delete $3;
        }
    | postfix_expression INC_OP {$$ = new std::shared_ptr<Expression>(makeUnaryExpression(*$1, UnaryOp::Type::POST_INCREMENT)); delete $1;}
    | postfix_expression DEC_OP {$$ = new std::shared_ptr<Expression>(makeUnaryExpression(*$1, UnaryOp::Type::POST_DECREMENT)); delete $1;}
    ;

argument_list
    : assignment_expression
        {
            auto list = makeNodeList();
            list->PushBack(*$1);
            $$ = new std::shared_ptr<NodeList>(list);
            delete $1;
        }
    | argument_list ',' assignment_expression
        {
            (*$1)->PushBack(*$3);
            $$ = $1;
            delete $3;
        }
    ;

unary_expression
    : postfix_expression { $$ = $1; }
    | INC_OP unary_expression {
            $$ = new std::shared_ptr<Expression>(makeUnaryExpression(*$2, UnaryOp::Type::PRE_INCREMENT));
            delete $2;
        }
    | DEC_OP unary_expression {
            $$ = new std::shared_ptr<Expression>(makeUnaryExpression(*$2, UnaryOp::Type::PRE_DECREMENT));
            delete $2;
        }
    | unary_operator cast_expression {
            $$ = new std::shared_ptr<Expression>(makeUnaryExpression(*$2, $1));
            delete $2;
        }
    | SIZEOF unary_expression {
            $$ = new std::shared_ptr<Expression>(makeSizeofExpression(*$2));
            delete $2;
        }
    | SIZEOF '(' type_specifier ')' {$$ = new std::shared_ptr<Expression>(makeSizeofTypeExpression($3));}
    ;

unary_operator
    : '&' { $$ = UnaryOp::Type::ADDRESS_OF; }
    | '*' { $$ = UnaryOp::Type::DEREFERENCE; }
    | '+' { $$ = UnaryOp::Type::PLUS; }
    | '-' { $$ = UnaryOp::Type::MINUS; }
    | '~' { $$ = UnaryOp::Type::BITWISE_NOT; }
    | '!' { $$ = UnaryOp::Type::LOGICAL_NOT; }
    ;

cast_expression
    : unary_expression { $$ = $1; }
    | '(' type_specifier ')' cast_expression
        {$$ = new std::shared_ptr<Expression>(makeCastExpression($2, *$4)); delete $4;}
    ;

multiplicative_expression
    : cast_expression { $$ = $1; }
    | multiplicative_expression '*' cast_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::MUL));
            delete $1; delete $3;
        }
    | multiplicative_expression '/' cast_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::DIV));
            delete $1; delete $3;
        }
    | multiplicative_expression '%' cast_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::MOD));
            delete $1; delete $3;
        }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::ADD));
            delete $1; delete $3;
        }
    | additive_expression '-' multiplicative_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::SUB));
            delete $1; delete $3;
        }
    ;

shift_expression
    : additive_expression { $$ = $1; }
    | shift_expression LEFT_OP additive_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::LEFT_SHIFT));
            delete $1; delete $3;
        }
    | shift_expression RIGHT_OP additive_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::RIGHT_SHIFT));
            delete $1; delete $3;
        }
    ;

relational_expression
    : shift_expression { $$ = $1; }
    | relational_expression '<' shift_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::LT));
            delete $1; delete $3;
        }
    | relational_expression '>' shift_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::GT));
            delete $1; delete $3;
        }
    | relational_expression LE_OP shift_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::LE));
            delete $1; delete $3;
        }
    | relational_expression GE_OP shift_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::GE));
            delete $1; delete $3;
        }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQ_OP relational_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::EQ));
            delete $1; delete $3;
        }
    | equality_expression NE_OP relational_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::NE));
            delete $1; delete $3;
        }
    ;

and_expression
    : equality_expression { $$ = $1; }
    | and_expression '&' equality_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::AND));
            delete $1; delete $3;
        }
    ;

exclusive_or_expression
    : and_expression { $$ = $1; }
    | exclusive_or_expression '^' and_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::XOR));
            delete $1; delete $3;
        }
    ;

inclusive_or_expression
    : exclusive_or_expression { $$ = $1; }
    | inclusive_or_expression '|' exclusive_or_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::OR));
            delete $1; delete $3;
        }
    ;

logical_and_expression
    : inclusive_or_expression { $$ = $1; }
    | logical_and_expression AND_OP inclusive_or_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::LOGICAL_AND));
            delete $1; delete $3;
        }
    ;

logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression OR_OP logical_and_expression {
            $$ = new std::shared_ptr<Expression>(makeBinaryExpression(*$1, *$3, BinaryOp::Type::LOGICAL_OR));
            delete $1; delete $3;
        }
    ;

conditional_expression
    : logical_or_expression { $$ = $1; }
    | logical_or_expression '?' expression ':' conditional_expression {
            $$ = new std::shared_ptr<Expression>(makeConditionalExpression(*$1, *$3, *$5));
            delete $1; delete $3; delete $5;
        }
    ;

assignment_expression
    : conditional_expression { $$ = $1; }
    | unary_expression assignment_operator assignment_expression {
            $$ = new std::shared_ptr<Expression>(makeAssignmentExpression(*$1, *$3, $2));
            delete $1; delete $3;
        }
    | unary_expression '=' STRING_LITERAL {
        std::cerr << " x = str " << std::endl;
            auto stringExpr = std::make_shared<ast::StringLiteralExpression>(*$3);
            $$ = new std::shared_ptr<Expression>(makeAssignmentExpression(*$1, stringExpr, AssignOp::ASSIGN));
            delete $1; delete $3;
        }
    ;

assignment_operator
    : '=' { $$ = AssignOp::Type::ASSIGN; }
    | MUL_ASSIGN { $$ = AssignOp::Type::MUL_ASSIGN; }
    | DIV_ASSIGN { $$ = AssignOp::Type::DIV_ASSIGN; }
    | MOD_ASSIGN { $$ = AssignOp::Type::MOD_ASSIGN; }
    | ADD_ASSIGN { $$ = AssignOp::Type::ADD_ASSIGN; }
    | SUB_ASSIGN { $$ = AssignOp::Type::SUB_ASSIGN; }
    | LEFT_ASSIGN { $$ = AssignOp::Type::LEFT_ASSIGN; }
    | RIGHT_ASSIGN { $$ = AssignOp::Type::RIGHT_ASSIGN; }
    | AND_ASSIGN { $$ = AssignOp::Type::AND_ASSIGN; }
    | XOR_ASSIGN { $$ = AssignOp::Type::XOR_ASSIGN; }
    | OR_ASSIGN { $$ = AssignOp::Type::OR_ASSIGN; }
    ;

binary_operator
    : '+'   { $$ = BinaryOp::Type::ADD; }
    | '-'   { $$ = BinaryOp::Type::SUB; }  // Changed from SUBTRACT to match the enum definition
    | '*'   { $$ = BinaryOp::Type::MUL; }
    | '/'   { $$ = BinaryOp::Type::DIV; }
    | '%'   { $$ = BinaryOp::Type::MOD; }
    | '<'   { $$ = BinaryOp::Type::LT; }
    | '>'   { $$ = BinaryOp::Type::GT; }
    | '&'   { $$ = BinaryOp::Type::AND; }
    | '|'   { $$ = BinaryOp::Type::OR; }
    | '^'   { $$ = BinaryOp::Type::XOR; }
    | OR_OP   { $$ = BinaryOp::Type::LOGICAL_OR; }
    | AND_OP   { $$ = BinaryOp::Type::LOGICAL_AND; }
    | LEFT_OP  { $$ = BinaryOp::Type::LEFT_SHIFT; }
    | RIGHT_OP { $$ = BinaryOp::Type::RIGHT_SHIFT; }
    ;

expression
    : assignment_expression { $$ = $1; }
    | expression ',' assignment_expression {
            $$ = new std::shared_ptr<Expression>(makeCommaExpression(*$1, *$3));
            delete $1; delete $3;
        }
    ;

constant_expression
    : conditional_expression { $$ = $1; }
    ;

%%

void yyerror (const char *s)
{
    std::cerr << "Error: " << s << " at line " << yylineno;
    std::cerr << " near '" << yytext << "'" << std::endl;
    std::exit(1);
}

std::shared_ptr<Node> g_root;

NodePtr ParseAST(std::string file_name)
{
  yyin = fopen(file_name.c_str(), "r");
  if(yyin == NULL){
    std::cerr << "Couldn't open input file: " << file_name << std::endl;
    exit(1);
  }
  g_root = nullptr;
  yyparse();
  fclose(yyin);
  yylex_destroy();
  return g_root;
}
