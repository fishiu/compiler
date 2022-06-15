#ifndef GLOBAL_H
#define GLOBAL_H

#include <map>
#include <string>
#include <memory>
#include <variant>

using namespace std;

typedef variant<int, string> sym_t;
class SymTab;

extern int tmp_var_no;  // current temp variable number
string NewTempVar();
extern SymTab symtab;  // current temp variable number


// symbol table
class SymTab {
 public:
  SymTab() {}
  // def const
  void Insert(unique_ptr<string>& symbol, int value);
  // def var (whether has init, whether init is int or lval)
  string Insert(unique_ptr<string>& symbol);  // will create addr
  bool Exist(unique_ptr<string>& symbol);
  sym_t Lookup(unique_ptr<string>& symbol);

 private:
  int var_cnt = 0;  // count var to name addr
  map<string, sym_t> symtab;
};

#endif