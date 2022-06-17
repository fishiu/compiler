#ifndef GLOBAL_H
#define GLOBAL_H

#include <map>
#include <string>
#include <memory>
#include <variant>
#include <vector>

using namespace std;

typedef variant<int, string> sym_t;
typedef map<string, sym_t> symtab_t;
class SymTabStack;

extern int tmp_var_no;  // current temp variable number
string NewTempVar();
extern SymTabStack symtab_stack;  // current temp variable number


// symbol table
class SymTabStack {
 private:
  int cnt = 0;  // 
  vector<symtab_t> stk;
 
 public:
  // create stack
  void Push();
  // delete stack
  void Pop();

  // def const
  void Insert(unique_ptr<string>& symbol, int value);
  // def var (whether has init, whether init is int or lval)
  string Insert(unique_ptr<string>& symbol);  // will create addr
  
  bool Exist(unique_ptr<string>& symbol, bool cur_level);
  sym_t Lookup(unique_ptr<string>& symbol);

  // ~SymTabStack();
};

#endif