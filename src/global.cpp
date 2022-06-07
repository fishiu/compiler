#include <global.hpp>
#include <cassert>

int tmp_var_no = 0;
SymTab symtab;

void SymTab::Insert(unique_ptr<string>& symbol, int value) {
  assert(!Exist(symbol));
  symtab[*symbol] = value;
}

bool SymTab::Exist(unique_ptr<string>& symbol) {
  return symtab.find(*symbol) != symtab.end();
}

int SymTab::Lookup(unique_ptr<string>& symbol) {
  assert(Exist(symbol));
  return symtab[*symbol];
}