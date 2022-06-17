%code requires {
  #include <memory>
  #include <string>
  #include <ast.hpp>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cassert>
#include <vector>
#include <ast.hpp>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  ExpBaseAST *exp_ast_val;
  VecAST *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <str_val> RELOP EQOP ANDOP OROP

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Decl ConstDecl ConstDef VarDecl VarDef Block BlockItem Stmt BType
%type <exp_ast_val> ConstExp ConstInitVal InitVal Exp UnaryExp PrimaryExp LVal AddExp MulExp RelExp EqExp LAndExp LOrExp
%type <int_val> Number
%type <str_val> UnaryOp
%type <vec_val> BlockItemList ConstDefList VarDefList

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = "int";
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto blocks = unique_ptr<VecAST>($2);
    auto ast = new BlockAST(blocks);
    $$ = ast;
  }

BlockItemList
  : {
    auto vec = new VecAST();
    $$ = vec;
  }
  | BlockItemList BlockItem {
    auto vec = $1;
    auto block_item = unique_ptr<BaseAST>($2);
    vec->push_back(block_item);
    $$ = vec;
  }
  ;

BlockItem
  : Stmt {
    printf("------------BlockItem: Stmt------------\n");
    auto stmt = unique_ptr<BaseAST>($1);
    auto ast = new BlockItemAST(stmt, true);
    $$ = ast;
  }
  | Decl {
    printf("------------BlockItem: Decl------------\n");
    auto decl = unique_ptr<BaseAST>($1);
    auto ast = new BlockItemAST(decl, false);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto const_decl = unique_ptr<BaseAST>($1);
    auto ast = new DeclAST(const_decl, false);
    $$ = ast;
  }
  | VarDecl {
    auto var_decl = unique_ptr<BaseAST>($1);
    auto ast = new DeclAST(var_decl, true);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';' {
    auto btype = unique_ptr<BaseAST>($2);
    auto const_def_list = unique_ptr<VecAST>($3);
    auto ast = new ConstDeclAST(btype, const_def_list);
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST("int");
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto vec = new VecAST();
    auto const_def = unique_ptr<BaseAST>($1);
    vec->push_back(const_def);
    $$ = vec;
  }
  | ConstDefList ',' ConstDef {
    auto vec = $1;
    auto const_def = unique_ptr<BaseAST>($3);
    vec->push_back(const_def);
    $$ = vec;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ident = unique_ptr<string>($1);
    auto const_init_val = unique_ptr<ExpBaseAST>($3);
    auto ast = new ConstDefAST(ident, const_init_val);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto const_exp = unique_ptr<ExpBaseAST>($1);
    auto ast = new ConstInitValAST(const_exp);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto exp = unique_ptr<ExpBaseAST>($1);
    auto ast = new ConstExpAST(exp);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDefList ';' {
    auto btype = unique_ptr<BaseAST>($1);
    // cast btype to BTypeAST
    auto btype_ast = static_cast<BTypeAST*>(btype.get());
    printf("VarDecl -> %s VarDefList\n", btype_ast->type.c_str());
    auto var_def_list = unique_ptr<VecAST>($2);
    auto ast = new VarDeclAST(btype, var_def_list);
    $$ = ast;
  }
  ;

VarDefList
  : VarDef {
    auto vec = new VecAST();
    auto var_def = unique_ptr<BaseAST>($1);
    vec->push_back(var_def);
    $$ = vec;
  }
  | VarDefList ',' VarDef {
    auto vec = $1;
    auto var_def = unique_ptr<BaseAST>($3);
    vec->push_back(var_def);
    $$ = vec;
  }
  ;

VarDef
  : IDENT {
    auto ident = unique_ptr<string>($1);
    printf("VarDef -> %s\n", ident->c_str());
    auto ast = new VarDefAST(ident);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ident = unique_ptr<string>($1);
    printf("VarDef -> %s = InitVal", ident->c_str());
    auto init_val = unique_ptr<ExpBaseAST>($3);
    auto ast = new VarDefAST(ident, init_val);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    printf("InitVal -> Exp\n");
    auto exp = unique_ptr<ExpBaseAST>($1);
    auto ast = new InitValAST(exp);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    printf("Stmt -> return Exp\n");
    auto exp = unique_ptr<ExpBaseAST>($2);
    auto ast = new RetAST(exp);
    $$ = ast;
  }
  | RETURN ';' {
    printf("Stmt -> return\n");
    auto ast = new RetAST();
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    // assign
    printf("Stmt -> LVal = Exp\n");
    auto lval = unique_ptr<ExpBaseAST>($1);
    // cast lval to LValAST
    auto lval_ast = static_cast<LValAST*>(lval.get());
    lval_ast->at_left = true;
    auto exp = unique_ptr<ExpBaseAST>($3);
    auto ast = new AssignAST(lval, exp);
    $$ = ast;
  }
  | Block {
    printf("Stmt -> Block\n");
    // do nothing
    $$ = $1;
  }
  | Exp ';' {
    printf("Stmt -> Exp;\n");
    auto exp = unique_ptr<ExpBaseAST>($1);
    auto ast = new StmtAST(exp);
    $$ = ast;
  }
  | ';' {
    printf("Stmt -> ;\n");
    auto ast = new StmtAST();
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    printf("Exp -> LOrExp\n");
    auto lor = unique_ptr<ExpBaseAST>($1);
    auto ast = new ExpAST(lor);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    printf("UnaryExp -> PrimaryExp\n");
    auto primary = unique_ptr<ExpBaseAST>($1);
    auto ast = new UnaryAST(primary);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    printf("UnaryExp -> UnaryOp(%s) UnaryExp\n", $1->c_str());
    auto unary = unique_ptr<ExpBaseAST>($2);
    auto ast = new UnaryAST($1, unary);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    printf("PrimaryExp -> ( Exp )\n");
    auto exp = unique_ptr<ExpBaseAST>($2);
    auto ast = new PrimaryAST(exp, false);
    $$ = ast;
  }
  | Number {
    printf("PrimaryExp -> Number %d\n", $1);
    auto ast = new PrimaryAST($1);
    $$ = ast;
  }
  | LVal {
    printf("PrimaryExp -> LVal\n");
    auto lval = unique_ptr<ExpBaseAST>($1);
    auto ast = new PrimaryAST(lval, true);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    printf("LVal -> IDENT %s\n", $1->c_str());
    auto ident = unique_ptr<string>($1);
    auto ast = new LValAST(ident);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    string *op = new string("+");
    printf("UnaryOp -> +\n");
    $$ = op;
  }
  | '-' {
    string *op = new string("-");
    printf("UnaryOp -> -\n");
    $$ = op;
  }
  | '!' {
    string *op = new string("!");
    printf("UnaryOp -> !\n");
    $$ = op;
  }
  ;

MulExp
  : UnaryExp {
    printf("MulExp -> UnaryExp\n");
    auto unary = unique_ptr<ExpBaseAST>($1);
    auto ast = new MulAST(unary);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    printf("MulExp -> MulExp * UnaryExp\n");
    auto mul = unique_ptr<ExpBaseAST>($1);
    auto unary = unique_ptr<ExpBaseAST>($3);
    auto ast = new MulAST("*", mul, unary);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    printf("MulExp -> MulExp / UnaryExp\n");
    auto mul = unique_ptr<ExpBaseAST>($1);
    auto unary = unique_ptr<ExpBaseAST>($3);
    auto ast = new MulAST("/", mul, unary);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    printf("MulExp -> MulExp %% UnaryExp\n");
    auto mul = unique_ptr<ExpBaseAST>($1);
    auto unary = unique_ptr<ExpBaseAST>($3);
    auto ast = new MulAST("%", mul, unary);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    printf("AddExp -> MulExp\n");
    auto mul = unique_ptr<ExpBaseAST>($1);
    auto ast = new AddAST(mul);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    printf("AddExp -> AddExp + MulExp\n");
    auto add = unique_ptr<ExpBaseAST>($1);
    auto mul = unique_ptr<ExpBaseAST>($3);
    auto ast = new AddAST("+", add, mul);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    printf("AddExp -> AddExp - MulExp\n");
    auto add = unique_ptr<ExpBaseAST>($1);
    auto mul = unique_ptr<ExpBaseAST>($3);
    auto ast = new AddAST("-", add, mul);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    printf("RelExp -> AddExp\n");
    auto add = unique_ptr<ExpBaseAST>($1);
    auto ast = new RelAST(add);
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    printf("RelExp -> RelExp %s AddExp\n", $2->c_str());
    auto rel = unique_ptr<ExpBaseAST>($1);
    auto add = unique_ptr<ExpBaseAST>($3);
    auto ast = new RelAST($2, rel, add);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    printf("EqExp -> RelExp\n");
    auto rel = unique_ptr<ExpBaseAST>($1);
    auto ast = new EqAST(rel);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    printf("EqExp -> EqExp %s RelExp\n", $2->c_str());
    auto eq = unique_ptr<ExpBaseAST>($1);
    auto rel = unique_ptr<ExpBaseAST>($3);
    auto ast = new EqAST($2, eq, rel);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    printf("LAndExp -> EqExp\n");
    auto eq = unique_ptr<ExpBaseAST>($1);
    auto ast = new LAndAST(eq);
    $$ = ast;
  }
  | LAndExp ANDOP EqExp {
    printf("LAndExp -> LAndExp && EqExp\n");
    auto land = unique_ptr<ExpBaseAST>($1);
    auto eq = unique_ptr<ExpBaseAST>($3);
    auto ast = new LAndAST(land, eq);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    printf("LOrExp -> LAndExp\n");
    auto land = unique_ptr<ExpBaseAST>($1);
    auto ast = new LOrAST(land);
    $$ = ast;
  }
  | LOrExp OROP LAndExp {
    printf("LOrExp -> LOrExp || LAndExp\n");
    auto lor = unique_ptr<ExpBaseAST>($1);
    auto land = unique_ptr<ExpBaseAST>($3);
    auto ast = new LOrAST(lor, land);
    $$ = ast;
  }
  ;

Number
  : INT_CONST;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}