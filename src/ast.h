#ifndef AST_H
#define AST_H

#include <memory>
#include <string>

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;
  virtual void Dump() const = 0;  // overide <<
};


// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  std::unique_ptr<BaseAST> func_def;

  virtual void Dump() const override;
};


// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  
  virtual void Dump() const override;
};


class FuncTypeAST : public BaseAST {
 public:
  std::string type;

  virtual void Dump() const override;
};


class BlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;

  virtual void Dump() const override;
};


class StmtAST : public BaseAST {
 public:
  int number;

  virtual void Dump() const override;
};

#endif
