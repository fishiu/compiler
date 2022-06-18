#include <ast.hpp>
#include <ir.hpp>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <sstream>

using namespace std;


extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];
  
  yyin = fopen(input, "r");
  ofstream fout(output);
  assert(yyin && fout);

  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  streambuf *oldcout = cout.rdbuf(fout.rdbuf());
  if (std::string(mode) == "-koopa") {
    // dump AST
    // cout << "// gen koopa" << endl;
    ast->Dump();
  } else if (std::string(mode) == "-riscv") {
    // get koopa str
    stringstream ss;
    cout.rdbuf(ss.rdbuf());
    ast->Dump();
    cout.rdbuf(fout.rdbuf());
  
    // cout << "# gen riscv" << endl;
    gen_riscv(ss.str());
  }
  cout.rdbuf(oldcout);
  fout.close();
  
  return 0;
}