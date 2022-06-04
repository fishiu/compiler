#include <ast.h>

static list<int> tmp_var_list;

//新增临时变量
static inline int new_temp() {
  tmp_var_list.push_front(tmp_var_no++);
  return tmp_var_list.front();
}

void CompUnitAST::Dump() { func_def->Dump(); }

void FuncDefAST::Dump() {
  cout << "fun @" << ident << "(): ";
  func_type->Dump();
  cout << " {" << endl;
  block->Dump();
  cout << "}";
}

void FuncTypeAST::Dump() {
  if (type == "int") {
    cout << "i32";
  }
}

void BlockAST::Dump() {
  cout << "\%entry:" << endl;
  stmt->Dump();
}

void StmtAST::Dump() {
  exp->Eval();
  exp->Dump();
  cout << "  ret " << exp->get_repr() << endl;  // ret 0
}

// if number than return val, else return tmp var
string ExpBaseAST::get_repr() {
  if (is_number) {
    return to_string(val);
  } else {
    return "%" + to_string(addr);
  }
}

void ExpAST::Dump() {
  unary->Dump();
}

void ExpAST::Eval() {
  unary->Eval();
  is_number = unary->is_number;
  val = unary->val;
  addr = unary->addr;
}

void UnaryAST::Dump() {
  if (op == "") {
    primary->Dump();
  } else if (op == "+") {
    unary->Dump();
  } else if (op == "-") {
    unary->Dump();
    cout << "  " << get_repr() << " = sub 0, " << unary->get_repr() << endl;
  } else if (op == "!") {
    unary->Dump();
    cout << "  " << get_repr() << " = eq " << unary->get_repr() << ", 0"
         << endl;
  }
}

void UnaryAST::Eval() {
  if (op == "") {
    // unary -> primary
    primary->Eval();
    is_number = primary->is_number;
    val = primary->val;
    addr = primary->addr;
  } else if (op == "+") {
    // unary -> + primary (do nothing)
    unary->Eval();
    is_number = unary->is_number;
    val = unary->val;
    addr = unary->addr;
  } else if (op == "-") {
    unary->Eval();
    is_number = false;  // number with op is not number
    addr = new_temp();
  } else if (op == "!") {
    unary->Eval();
    is_number = false;  // number with op is not number
    addr = new_temp();
  }
}

void PrimaryAST::Dump() {
  if (!is_number) exp->Dump();
}

void PrimaryAST::Eval() {
  // number case is already evaluated
  if (!is_number) {
    exp->Eval();
    is_number = exp->is_number;
    val = exp->val;
    addr = exp->addr;
  }
}