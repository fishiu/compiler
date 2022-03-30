#include <ast.h>

#include <iostream>

void CompUnitAST::Dump() const {
  std::cout << "CompUnitAST { ";
  func_def->Dump();
  std::cout << " }";
}

void FuncDefAST::Dump() const {
  std::cout << "FuncDefAST { ";
  func_type->Dump();
  std::cout << ", " << ident << ", ";
  block->Dump();
  std::cout << " }";
}

void FuncTypeAST::Dump() const {
  std::cout << "FuncType { ";
  std::cout << type;
  std::cout << " }";
}

void BlockAST::Dump() const {
  std::cout << "Block { ";
  stmt->Dump();
  std::cout << " }";
}

void StmtAST::Dump() const {
  std::cout << "Stmt { return ";
  std::cout << number;
  std::cout << " }";
}