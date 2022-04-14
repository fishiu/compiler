#include <ast.h>

#include <iostream>

void CompUnitAST::Dump() const {
  func_def->Dump();
}

void FuncDefAST::Dump() const {
  std::cout << "fun @" << ident << "(): ";
  func_type->Dump();
  std::cout << " {" << std::endl;
  block->Dump();
  std::cout << "}";
}

void FuncTypeAST::Dump() const {
  if (type == "int") {
    std::cout << "i32";
  }
}

void BlockAST::Dump() const {
  std::cout << "\%entry:" << std::endl;
  stmt->Dump();
}

void StmtAST::Dump() const {
  std::cout << "  ret " << number << std::endl;  //ret 0
}
