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
GlbSymTab glb_symtab;


// ==================== SymTabStack ==================== //

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

/**
 * @brief insert a constant var (into the top stack)
 * 
 * @param symbol 
 * @param value constant value (int)
 */
void SymTabStack::Insert(unique_ptr<string>& symbol, int value) {
  assert(!Exist(symbol, true));
  stk.back()[*symbol] = value;
}

/**
 * @brief insert a variable (into the top stack) and create memory space
 * 
 * @param symbol 
 * @return string memory address name
 */
string SymTabStack::Insert(unique_ptr<string>& symbol) {
  assert(!Exist(symbol, true));
  string name = "@" + *symbol + "_" + to_string(cnt);
  printf(" [debug] alloc %s\n", name.c_str());
  stk.back()[*symbol] = name;
  return name;
}

/**
 * @brief search through all symtab in the stack (inversely)
 * 
 * @param symbol 
 * @param cur_level only search in the current level symtab
 * @return bool whether exist 
 */
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

// ==================== Global Sym Tab ==================== //

bool GlbSymTab::Exist(unique_ptr<string>& symbol) {
  auto it = sym_map.find(*symbol);
  return it != sym_map.end();
}

void GlbSymTab::Insert(unique_ptr<string>& symbol, int value) {
  assert(!Exist(symbol));
  glb_sym_t sym;
  sym.tag = "int";
  sym.val = value;
  sym_map[*symbol] = sym;
}

/**
 * @brief insert func or var
 * 
 * all global identifier can not share name, even there are different
 * type (func, var ...)
 * todo btw: can local var share name with global func (assume no)?
 * 
 * @param symbol 
 * @param value void/int for func, int for var
 * @param is_func 
 */
void GlbSymTab::Insert(unique_ptr<string>& symbol, string value, bool is_func) {
  assert(!Exist(symbol));
  glb_sym_t sym;
  sym.tag = is_func ? "func" : "var";
  sym.val = value;
  sym_map[*symbol] = sym;
}

glb_sym_t GlbSymTab::Lookup(unique_ptr<string>& symbol) {
  assert(Exist(symbol));
  return sym_map[*symbol];
}

// ==================== WhileStack ==================== //

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
