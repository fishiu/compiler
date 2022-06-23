#ifndef AST_H
#define AST_H

#include <global.hpp>
#include <koopa.h>

#include <cassert>
#include <iostream>
#include <list>
#include <vector>
#include <memory>
#include <sstream>
#include <string>

using namespace std;

class BaseAST;
class ExpBaseAST;
class VecAST;

class FuncDefAST;
class FuncTypeAST;
class FuncFParamAST;
class BlockAST;
class BlockItemAST;
class StmtAST;
class AssignAST;    // Another StmtAST
class RetAST;       // Another StmtAST
class IfAST;        // Another StmtAST
class WhileAST;     // Another StmtAST
class BreakAST;     // Another StmtAST
class ContinueAST;  // Another StmtAST
class DeclAST;
class ConstDeclAST;
class VarDeclAST;
class BTypeAST;

class ConstDefAST;      // expbase
class VarDefAST;        // expbase
class ConstInitValAST;  // expbase
class ConstExpAST;      // expbase
class InitValAST;       // expbase

class ExpAST;
class UnaryAST;
class PrimaryAST;
class LValAST;
class MulAST;
class AddAST;
class RelAST;
class EqAST;
class LAndAST;
class LorAST;
class FuncCallAST;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;
  virtual void Dump() = 0;  // overide <<
};

class VecAST {
 public:
  vector<unique_ptr<BaseAST>> vec;

  void push_back(unique_ptr<BaseAST>& ast) {
    vec.push_back(move(ast));
  }
};

// currently, it only contains one unary expression
class ExpBaseAST : public BaseAST {
 public:
  bool evaluated = false;  // avoid re-evaluation (only for const exp)
  bool is_number = false;  // const number
  bool is_const = false;   // is const expression (so no re-evaluate)
  int val;
  string addr;

  // go through all the downstream nodes and get the value and address
  // 1. whether the node is a number node
  // 2. get the address: if it is a number node, the address is the number, else
  // the address is the register
  virtual void Eval() = 0;
  
  // if number than return val, else return tmp var
  virtual string get_repr() {
    if (is_number) {
      return to_string(val);
    } else {
      return addr;
    }
  }
  
  void CopyInfo(unique_ptr<ExpBaseAST>& exp) {
    evaluated = exp->evaluated;
    is_number = exp->is_number;
    is_const = exp->is_const;
    val = exp->val;
    addr = exp->addr;
  }
  
  virtual string DebugInfo() {
    stringstream buffer;
    if (is_number) {
      buffer << "is number: " << val;
    } else {
      buffer << "not number, addr: " << addr;
    }
    return buffer.str();
  }
};

class CompUnitAST : public BaseAST {
 public:
  unique_ptr<VecAST> func_def_list;

  CompUnitAST(unique_ptr<VecAST>& func_def_list)
      : func_def_list(move(func_def_list)) {}
  // TODO is there any way to keep const?
  virtual void Dump() override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  unique_ptr<BaseAST> func_type;
  unique_ptr<string> ident;
  unique_ptr<VecAST> params;
  unique_ptr<BaseAST> block;
  bool has_param = false;
  bool is_void = false;

  // TODO 有空了梳理一下string内存管理
  FuncDefAST(unique_ptr<BaseAST>& func_type, unique_ptr<string>& ident, unique_ptr<BaseAST>& block)
      : func_type(move(func_type)), ident(move(ident)), block(move(block)) {
    Init();
  }
  FuncDefAST(unique_ptr<BaseAST>& func_type, unique_ptr<string>& ident, unique_ptr<VecAST>& params, unique_ptr<BaseAST>& block)
      : func_type(move(func_type)), ident(move(ident)), params(move(params)), block(move(block)), has_param(true) {
    Init();
  }

  virtual void Dump() override;

 private:
  void Init();
};

class FuncTypeAST : public BaseAST {
 public:
  string type;

  virtual void Dump() override;
};

class FuncFParamAST : public BaseAST {
 public:
  unique_ptr<BaseAST> type;
  unique_ptr<string> ident;

  FuncFParamAST(unique_ptr<BaseAST>& type, unique_ptr<string>& ident)
      : type(move(type)), ident(move(ident)) {}

  virtual void Dump() override;
};

class BlockAST : public BaseAST {
 public:
  unique_ptr<VecAST> blocks;  // block item list
  VecAST* func_params = nullptr;

  BlockAST(unique_ptr<VecAST>& blocks) : blocks(move(blocks)) {}
  virtual void Dump() override;
};

class BlockItemAST : public BaseAST {
 public:
  unique_ptr<BaseAST> ast;
  bool is_stmt;  // stmt or decl

  BlockItemAST(unique_ptr<BaseAST>& ast, bool is_stmt)
      : ast(move(ast)), is_stmt(is_stmt) {}
  virtual void Dump() override;
};

class StmtAST : public BaseAST {
 public:
  unique_ptr<ExpBaseAST> exp;
  bool has_exp = false;

  StmtAST() {}
  StmtAST(unique_ptr<ExpBaseAST>& exp) : exp(move(exp)), has_exp(true) {}
  virtual void Dump() override;
};

// Another StmtAST
class AssignAST : public BaseAST {
 public:
  unique_ptr<ExpBaseAST> lval;
  unique_ptr<ExpBaseAST> exp;

  AssignAST(unique_ptr<ExpBaseAST>& lval, unique_ptr<ExpBaseAST>& exp)
      : lval(move(lval)), exp(move(exp)) {}
  virtual void Dump() override;
};

// Another StmtAST
class RetAST : public BaseAST {
 public:
  unique_ptr<ExpBaseAST> exp;
  bool has_exp = false;

  RetAST() {}
  RetAST(unique_ptr<ExpBaseAST>& exp) : exp(move(exp)), has_exp(true) {}
  virtual void Dump() override;
};

class IfAST : public BaseAST {
 public:
  unique_ptr<ExpBaseAST> cond;
  unique_ptr<BaseAST> if_stmt;
  unique_ptr<BaseAST> else_stmt;
  bool has_else = false;

  IfAST(unique_ptr<ExpBaseAST>& cond, unique_ptr<BaseAST>& if_stmt)
      : cond(move(cond)), if_stmt(move(if_stmt)) {}
  IfAST(unique_ptr<ExpBaseAST>& cond, unique_ptr<BaseAST>& if_stmt,
        unique_ptr<BaseAST>& else_stmt)
      : cond(move(cond)), if_stmt(move(if_stmt)), else_stmt(move(else_stmt)),
        has_else(true) {}
  virtual void Dump() override;
};

class WhileAST : public BaseAST {
 public:
  unique_ptr<ExpBaseAST> cond;
  unique_ptr<BaseAST> body;

  WhileAST(unique_ptr<ExpBaseAST>& cond, unique_ptr<BaseAST>& stmt)
      : cond(move(cond)), body(move(stmt)) {}
  virtual void Dump() override;
};

class BreakAST : public BaseAST {
 public:
  virtual void Dump() override;
};

class ContinueAST : public BaseAST {
 public:
  virtual void Dump() override;
};

class DeclAST : public BaseAST {
 public:
  unique_ptr<BaseAST> decl;
  bool is_var;  // todo use global type in future

  DeclAST(unique_ptr<BaseAST>& decl, bool is_var) : decl(move(decl)), is_var(is_var) {}
  virtual void Dump() override;
};

class ConstDeclAST : public BaseAST {
 public:
  unique_ptr<BaseAST> btype;
  unique_ptr<VecAST> def_list;

  ConstDeclAST(unique_ptr<BaseAST>& btype, unique_ptr<VecAST>& def_list)
      : btype(move(btype)), def_list(move(def_list)) {}
  virtual void Dump() override;
};

class BTypeAST : public BaseAST {
 public:
  string type;

  BTypeAST(string type) : type(move(type)) {}
  virtual void Dump() override;
};

class ConstDefAST : public BaseAST {
 public:
  unique_ptr<string> ident;     // symtab
  unique_ptr<ExpBaseAST> init;  // init value

  ConstDefAST(unique_ptr<string>& ident_, unique_ptr<ExpBaseAST>& init_)
      : ident(move(ident_)), init(move(init_)) {}
  virtual void Dump() override;
};

class ConstInitValAST : public ExpBaseAST {
 public:
  unique_ptr<ExpBaseAST> exp;  // const exp

  ConstInitValAST(unique_ptr<ExpBaseAST>& exp) : exp(move(exp)) {}
  virtual void Dump() override;
  virtual void Eval() override;
};

class ConstExpAST : public ExpBaseAST {
 public:
  unique_ptr<ExpBaseAST> exp;

  ConstExpAST(unique_ptr<ExpBaseAST>& exp) : exp(move(exp)) {}
  virtual void Dump() override;
  virtual void Eval() override;
};

class VarDeclAST : public BaseAST {
 public:
  unique_ptr<BaseAST> btype;
  unique_ptr<VecAST> def_list;

  VarDeclAST(unique_ptr<BaseAST>& btype, unique_ptr<VecAST>& def_list)
      : btype(move(btype)), def_list(move(def_list)) {}
  virtual void Dump() override;
};

class VarDefAST : public BaseAST {
 public:
  unique_ptr<string> ident;
  unique_ptr<ExpBaseAST> init;
  string mem_addr;
  bool has_init;

  VarDefAST(unique_ptr<string>& ident_) : ident(move(ident_)), has_init(false) {}

  VarDefAST(unique_ptr<string>& ident_, unique_ptr<ExpBaseAST>& init_)
      : ident(move(ident_)), init(move(init_)), has_init(true) {}

  virtual void Dump() override;
};

class InitValAST : public ExpBaseAST {
 public:
  unique_ptr<ExpBaseAST> exp;

  InitValAST(unique_ptr<ExpBaseAST>& exp) : exp(move(exp)) {}
  virtual void Dump() override;
  virtual void Eval() override;
};

class ExpAST : public ExpBaseAST {
 public:
  unique_ptr<ExpBaseAST> lor;

  ExpAST(unique_ptr<ExpBaseAST>& add) : lor(move(add)) {}
  virtual void Dump() override;
  virtual void Eval() override;
};

// unary expression, op could be none
class UnaryAST : public ExpBaseAST {
 public:
  string op;  // maybe I can use enum here, default none string
  // union {
  //   unique_ptr<ExpBaseAST> primary;  // primary expression
  //   unique_ptr<ExpBaseAST> unary;    // unary
  // };
  unique_ptr<ExpBaseAST> primary;  // primary expression
  unique_ptr<ExpBaseAST> unary;    // unary

  // todo what is move and unique_ptr
  UnaryAST(unique_ptr<ExpBaseAST>& primary) : primary(move(primary)) {}
  UnaryAST(string* op, unique_ptr<ExpBaseAST>& unary)
      : op(*move(op)), unary(move(unary)) {}
  virtual void Dump() override;
  virtual void Eval() override;
  virtual string DebugInfo() override {
    string base_debug_info = ExpBaseAST::DebugInfo();
    stringstream buffer;
    buffer << base_debug_info << " op: " << op;
    return buffer.str();
  }
};

// number or parathesis expression
class PrimaryAST : public ExpBaseAST {
 public:
  bool is_lval = false;
  bool is_exp = false;
  unique_ptr<ExpBaseAST> exp;
  unique_ptr<ExpBaseAST> lval;

  PrimaryAST(unique_ptr<ExpBaseAST>& ast, bool is_lval) {
    this->is_lval = is_lval;
    this->is_exp = !is_lval;
    if (is_lval) {
      lval = move(ast);
    } else {
      exp = move(ast);
    }
  }
  PrimaryAST(int value) {
    is_number = true;
    is_const = true;
    evaluated = true;
    val = value;
  }
  virtual void Dump() override;
  virtual void Eval() override;
};

class LValAST : public ExpBaseAST {
 public:
  unique_ptr<string> ident;
  sym_t sym;
  string mem_addr;  // address in memory
  bool at_left;

  LValAST(unique_ptr<string>& ident_) : ident(move(ident_)) {}
  virtual void Dump() override;
  virtual void Eval() override;
};

class MulAST : public ExpBaseAST {
 public:
  string op;
  unique_ptr<ExpBaseAST> mul;
  unique_ptr<ExpBaseAST> unary;

  MulAST(unique_ptr<ExpBaseAST>& unary) : unary(move(unary)) {}
  MulAST(string op, unique_ptr<ExpBaseAST>& mul, unique_ptr<ExpBaseAST>& unary)
      : op(move(op)), mul(move(mul)), unary(move(unary)) {}
  virtual void Dump() override;
  virtual void Eval() override;
};

class AddAST : public ExpBaseAST {
 public:
  string op;
  unique_ptr<ExpBaseAST> add;
  unique_ptr<ExpBaseAST> mul;

  AddAST(unique_ptr<ExpBaseAST>& mul) : mul(move(mul)) {}
  AddAST(string op, unique_ptr<ExpBaseAST>& add, unique_ptr<ExpBaseAST>& mul)
      : op(move(op)), add(move(add)), mul(move(mul)) {}

  virtual void Dump() override;
  virtual void Eval() override;
};

class RelAST : public ExpBaseAST {
 public:
  string op;
  unique_ptr<ExpBaseAST> rel;
  unique_ptr<ExpBaseAST> add;

  RelAST(unique_ptr<ExpBaseAST>& add) : add(move(add)) {}
  RelAST(string* op, unique_ptr<ExpBaseAST>& rel, unique_ptr<ExpBaseAST>& add)
      : op(*move(op)), rel(move(rel)), add(move(add)) {}

  virtual void Dump() override;
  virtual void Eval() override;
};

class EqAST : public ExpBaseAST {
 public:
  string op;
  unique_ptr<ExpBaseAST> eq;
  unique_ptr<ExpBaseAST> rel;

  EqAST(unique_ptr<ExpBaseAST>& rel) : rel(move(rel)) {}
  EqAST(string* op, unique_ptr<ExpBaseAST>& eq, unique_ptr<ExpBaseAST>& rel)
      : op(*move(op)), eq(move(eq)), rel(move(rel)) {}

  virtual void Dump() override;
  virtual void Eval() override;
};

class LAndAST : public ExpBaseAST {
 public:
  bool is_single;
  unique_ptr<ExpBaseAST> land;
  unique_ptr<ExpBaseAST> eq;

  LAndAST(unique_ptr<ExpBaseAST>& eq) :is_single(true), eq(move(eq)) {}
  LAndAST(unique_ptr<ExpBaseAST>& land, unique_ptr<ExpBaseAST>& eq)
      : is_single(false), land(move(land)), eq(move(eq)) {}

  virtual void Dump() override;
  virtual void Eval() override;
};

class LOrAST : public ExpBaseAST {
 public:
  bool is_single;
  unique_ptr<ExpBaseAST> lor;
  unique_ptr<ExpBaseAST> land;

  LOrAST(unique_ptr<ExpBaseAST>& land) : is_single(true), land(move(land)) {}
  LOrAST(unique_ptr<ExpBaseAST>& lor, unique_ptr<ExpBaseAST>& land)
      : is_single(false), lor(move(lor)), land(move(land)) {}

  virtual void Dump() override;
  virtual void Eval() override;
};

class FuncCallAST : public ExpBaseAST {
 public:
  unique_ptr<string> ident;
  unique_ptr<VecAST> rparams;
  bool has_rparams = false;

  FuncCallAST(unique_ptr<string>& ident) : ident(move(ident)) {}
  FuncCallAST(unique_ptr<string>& ident, unique_ptr<VecAST>& rparams) 
      : ident(move(ident)), rparams(move(rparams)), has_rparams(true) {}
  virtual void Dump() override;
  virtual void Eval() override;
};

#endif
