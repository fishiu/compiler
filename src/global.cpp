#include <cassert>
#include <global.hpp>

int tmp_var_no = 0;
string NewTempVar() {
  return "%" + to_string(tmp_var_no++);
}
SymTab symtab;

void SymTab::Insert(unique_ptr<string>& symbol, int value) {
  assert(!Exist(symbol));
  symtab[*symbol] = value;
}

string SymTab::Insert(unique_ptr<string>& symbol) {
  assert(!Exist(symbol));
  string name = "@" + *symbol + "_" + to_string(var_cnt++);
  printf("[debug] alloc %s\n", name.c_str());
  symtab[*symbol] = name;
  return name;
}

bool SymTab::Exist(unique_ptr<string>& symbol) {
  auto it = symtab.find(*symbol);
  auto end = symtab.end();
  if (it == end) {
    return false;
  } else {
    return true;
  }
}

sym_t SymTab::Lookup(unique_ptr<string>& symbol) {
  // todo use const and refer in future
  assert(Exist(symbol));
  return symtab[*symbol];
}