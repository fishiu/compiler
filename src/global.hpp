#ifndef GLOBAL_H
#define GLOBAL_H

#include <map>
#include <string>
#include <memory>

using namespace std;

class SymTab;

extern int tmp_var_no;  // current temp variable number
extern SymTab symtab;  // current temp variable number


// symbol table
class SymTab {
 public:
  SymTab() {}
  // currently only have int const value
  void Insert(unique_ptr<string>& symbol, int value);
  bool Exist(unique_ptr<string>& symbol);
  int Lookup(unique_ptr<string>& symbol);

 private:
  map<string, int> symtab;
};

#endif