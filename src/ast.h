#ifndef AST_H
#define AST_H

#include <global.h>
#include <koopa.h>

#include <cassert>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>

using namespace std;

class BaseAST;
class ExpBaseAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class StmtAST;
class ExpAST;
class UnaryAST;
class PrimaryAST;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;
  virtual void Dump() = 0;  // overide <<
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  unique_ptr<BaseAST> func_def;

  // TODO is there any way to keep const?
  virtual void Dump() override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  unique_ptr<BaseAST> func_type;
  string ident;
  unique_ptr<BaseAST> block;

  virtual void Dump() override;
};

class FuncTypeAST : public BaseAST {
 public:
  string type;

  virtual void Dump() override;
};

class BlockAST : public BaseAST {
 public:
  unique_ptr<BaseAST> stmt;

  BlockAST(unique_ptr<BaseAST>& stmt) : stmt(move(stmt)) {}
  virtual void Dump() override;
};

class StmtAST : public BaseAST {
 public:
  // int number;
  unique_ptr<ExpBaseAST> exp;

  StmtAST(unique_ptr<ExpBaseAST>& exp) : exp(move(exp)) {}
  virtual void Dump() override;
};

// currently, it only contains one unary expression
class ExpBaseAST : public BaseAST {
 public:
  bool is_number;  // const number
  int val;
  int addr;

  string get_repr();
  // go through all the downstream nodes and get the value and address
  // 1. whether the node is a number node
  // 2. get the address: if it is a number node, the address is the number, else
  // the address is the register
  virtual void Eval() = 0;
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

class ExpAST : public ExpBaseAST {
 public:
  unique_ptr<ExpBaseAST> add;

  ExpAST(unique_ptr<ExpBaseAST>& add) : add(move(add)) {}
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
  unique_ptr<ExpBaseAST> exp;

  PrimaryAST(unique_ptr<ExpBaseAST>& exp) : exp(move(exp)) {}
  PrimaryAST(int value) {
    is_number = true;
    val = value;
  }
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

#endif
