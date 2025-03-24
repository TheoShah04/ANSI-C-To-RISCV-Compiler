%option noyywrap

%{
  // A lot of this lexer is based off the ANSI C grammar:
  // https://www.lysator.liu.se/c/ANSI-C-grammar-l.html#MUL-ASSIGN
  // Avoid error "error: `fileno' was not declared in this scope"
  extern "C" int fileno(FILE *stream);

  #include "parser.tab.hpp"
   #include "ast_type_specifier.hpp"
  #include <unordered_set>
  // Suppress warning about unused function
  [[maybe_unused]] static void yyunput (int c, char * yy_bp );

  std::unordered_map<std::string, ast::TypeSpecifier> typeDefs;
  void updateTypeDefs(std::string id, ast::TypeSpecifier type){
    std::cerr << "Adding typedef: " << id << " of type: " << type << std::endl;
    typeDefs[id] = type;
    }
  ast::TypeSpecifier getTypeDefType(std::string id){
    std::cerr << "Retreiving typedef: " << id << " of type: " << typeDefs[id] << std::endl;
    return typeDefs[id];
    }
%}

D	  [0-9]
L	  [a-zA-Z_]
H   [a-fA-F0-9]
E	  [Ee][+-]?{D}+
FS  (f|F|l|L)
IS  (u|U|l|L)*

%%
"/*"			{/* consumes comment - TODO you might want to process and emit it in your assembly for debugging */}
"auto"			{return(AUTO);}
"break"			{return(BREAK);}
"case"			{return(CASE);}
"char"			{return(CHAR);}
"const"			{return(CONST);}
"continue"  {return(CONTINUE);}
"default"		{return(DEFAULT);}
"do"			  {return(DO);}
"double"		{return(DOUBLE);}
"else"			{return(ELSE);}
"enum"			{return(ENUM);}
"extern"		{return(EXTERN);}
"float"			{return(FLOAT);}
"for"			  {return(FOR);}
"goto"			{return(GOTO);}
"if"			  {return(IF);}
"int"			  {return(INT);}
"long"			{return(LONG);}
"register"	{return(REGISTER);}
"return"		{return(RETURN);}
"short"			{return(SHORT);}
"signed"		{return(SIGNED);}
"sizeof"		{return(SIZEOF);}
"static"		{return(STATIC);}
"struct"		{return(STRUCT);}
"switch"		{return(SWITCH);}
"typedef"		{return(TYPEDEF);}
"union"			{return(UNION);}
"unsigned"	{return(UNSIGNED);}
"void"			{return(VOID);}
"volatile"	{return(VOLATILE);}
"while"			{return(WHILE);}

{L}({L}|{D})*		{yylval.string = new std::string(yytext);
                if (typeDefs.find(*yylval.string) != typeDefs.end()){
                  std::cerr << "Found typedef: " << *yylval.string << std::endl;
		              return(TYPE_NAME);
                  //return IDENTIFIER;
                  }
	              else{
                  return(IDENTIFIER);
                  }
                }

0[xX]{H}+{IS}?		{yylval.number_int = (int)strtol(yytext, NULL, 0); return(INT_CONSTANT);}
0{D}+{IS}?		    {yylval.number_int = (int)strtol(yytext, NULL, 0); return(INT_CONSTANT);}
{D}+{IS}?		      {yylval.number_int = (int)strtol(yytext, NULL, 0); return(INT_CONSTANT);}
L?'(\\.|[^\\'\n])+'	{ yylval.number_int = yytext[1]; return(CHAR_CONSTANT); }
{D}+{E}{FS}		        {yylval.number_float = strtof(yytext, NULL); return(FLOAT_CONSTANT);}
{D}*"."{D}+({E})?{FS}	{yylval.number_float = strtof(yytext, NULL); return(FLOAT_CONSTANT);}
{D}+"."{D}*({E})?{FS}	{yylval.number_float = strtof(yytext, NULL); return(FLOAT_CONSTANT);}
{D}+{E}?{FS}?		{ yylval.number_double = strtod(yytext, NULL); return(DOUBLE_CONSTANT); }
{D}*"."{D}+{E}?{FS}?	{ yylval.number_double = strtod(yytext, NULL); return(DOUBLE_CONSTANT); }
{D}+"."{D}*{E}?{FS}?	{ yylval.number_double = strtod(yytext, NULL); return(DOUBLE_CONSTANT); }

L?\"(\\.|[^\\"])*\"	{ yylval.string = new std::string(yytext); return(STRING_LITERAL);}

"..."      {return(ELLIPSIS);}
">>="			 {return(RIGHT_ASSIGN);}
"<<="      {return(LEFT_ASSIGN);}
"+="			 {return(ADD_ASSIGN);}
"-="       {return(SUB_ASSIGN);}
"*="       {return(MUL_ASSIGN);}
"/="			 {return(DIV_ASSIGN);}
"%="			 {return(MOD_ASSIGN);}
"&="       {return(AND_ASSIGN);}
"^="			 {return(XOR_ASSIGN);}
"|="       {return(OR_ASSIGN);}
">>"       {return(RIGHT_OP);}
"<<"       {return(LEFT_OP);}
"++"			 {return(INC_OP);}
"--"			 {return(DEC_OP);}
"->"			 {return(PTR_OP);}
"&&"			 {return(AND_OP);}
"||"			 {return(OR_OP);}
"<="			 {return(LE_OP);}
">="			 {return(GE_OP);}
"=="			 {return(EQ_OP);}
"!="			 {return(NE_OP);}
";"			   {return(';');}
("{"|"<%") {return('{');}
("}"|"%>") {return('}');}
","			   {return(',');}
":"			   {return(':');}
"="			   {return('=');}
"("		     {return('(');}
")"			   {return(')');}
("["|"<:") {return('[');}
("]"|":>") {return(']');}
"."			   {return('.');}
"&"			   {return('&');}
"!"			   {return('!');}
"~"			   {return('~');}
"-"			   {return('-');}
"+"			   {return('+');}
"*"			   {return('*');}
"/"			   {return('/');}
"%"			   {return('%');}
"<"			   {return('<');}
">"			   {return('>');}
"^"			   {return('^');}
"|"			   {return('|');}
"?"			   {return('?');}

[ \a\b\t\v\f\n\r]		{/* ignore new lines and special sequences */}
.			              {fprintf(stderr, "Lexing error: unexpected character '%s'\n", yytext); exit(1);}

%%
