#ifndef GLOBAL_H
#define GLOBAL_H

#include <map>
#include <string>
#include <memory>
#include <variant>
#include <vector>
#include <stack>
#include <tuple>

using namespace std;

typedef variant<int, string> sym_t;
typedef map<string, sym_t> symtab_t;
typedef tuple<string, string, string> labels_t;
class SymTabStack;
class FuncTab;
class WhileStack;

extern int label_cnt;
extern int ret_label_cnt;
extern int tmp_var_no;  // current temp variable number
string NewTempVar();
extern SymTabStack symtab_stack;
extern WhileStack while_stack;    // to maintain multi while
extern FuncTab functab;


// symbol table
class SymTabStack {
 private:
  int cnt = 0;  // global count for symtab, to avoid redifinition in a block
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

  // todo ~SymTabStack();
};

class FuncTab {
 private:
  map<string, string> func_map;
  bool Exist(string symbol);
 
 public:
  void Insert(string symbol, string func_type);
  
  string Lookup(unique_ptr<string>& symbol);
  // todo ~GlbSymTab();
};

class WhileStack {
 private:
  int cnt = 0;
  stack<int> stk;
  labels_t get_label(int i);

 public:
  labels_t Push();
  void Pop();
  labels_t Top();  // get current while label
};

#endif