#include <cassert>
#include <global.hpp>

int label_cnt = 0;
int ret_label_cnt = 0;
int tmp_var_no = 0;
string NewTempVar() {
  printf(" [debug] renew tmp var: %d\n", tmp_var_no);
  return "%" + to_string(tmp_var_no++);
}
SymTabStack symtab_stack;
WhileStack while_stack;

// SymTabStack methods
void SymTabStack::Push() {
  cnt++;
  printf(" [debug] push symtab#%d\n", cnt);
  stk.push_back(symtab_t());
}

void SymTabStack::Pop() {
  printf(" [debug] pop symtab#%d\n", cnt);
  assert(cnt > 0);
  stk.pop_back();
  // cnt--;
}

void SymTabStack::Insert(unique_ptr<string>& symbol, int value) {
  assert(!Exist(symbol, true));
  stk.back()[*symbol] = value;
}

string SymTabStack::Insert(unique_ptr<string>& symbol) {
  assert(!Exist(symbol, true));
  string name = "@" + *symbol + "_" + to_string(cnt);
  printf(" [debug] alloc %s\n", name.c_str());
  stk.back()[*symbol] = name;
  return name;
}

// currently
bool SymTabStack::Exist(unique_ptr<string>& symbol, bool cur_level=false) {
  int layer = 0;
  // reverse
  for (auto symtab_it = stk.rbegin(); symtab_it != stk.rend(); symtab_it++) {
    auto it = symtab_it->find(*symbol);
    if (it != symtab_it->end())
      return true;
    layer++;

    if (cur_level && layer >= 1)
      break;
  }
  return false;
}

sym_t SymTabStack::Lookup(unique_ptr<string>& symbol) {
  // todo use const and refer in future
  // assert(Exist(symbol));
  // reverse
  for (auto symtab_it = stk.rbegin(); symtab_it != stk.rend(); symtab_it++) {
    auto it = symtab_it->find(*symbol);
    if (it != symtab_it->end()) {
      return it->second;
    }
  }
  assert(false);
}


// while stack implementations
labels_t WhileStack::get_label(int i) {
  // get current while label  
  string label_entry = "%while_entry_" + to_string(i);
  string label_body = "%while_body_" + to_string(i);
  string label_end = "%while_end_" + to_string(i);
  return make_tuple(label_entry, label_body, label_end);
}

labels_t WhileStack::Push() {
  // printf(" [debug] push while#%d\n", cnt);
  stk.push(cnt);
  labels_t labels = get_label(cnt);
  cnt++;
  
  return labels;
}

void WhileStack::Pop() {
  // printf(" [debug] pop while#%d\n", cnt);
  stk.pop();
}

labels_t WhileStack::Top() {
  labels_t labels = get_label(stk.top());
  return labels;
}
