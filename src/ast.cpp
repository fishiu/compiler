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

void ExpAST::Dump() { lor->Dump(); }

void ExpAST::Eval() {
  lor->Eval();
  is_number = lor->is_number;
  val = lor->val;
  addr = lor->addr;
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

void MulAST::Dump() {
  if (op == "") {
    unary->Dump();
  } else {
    mul->Dump();
    unary->Dump();

    if (op == "*") {
      cout << "  " << get_repr() << " = mul " << mul->get_repr() << ", "
           << unary->get_repr() << endl;
    } else if (op == "/") {
      cout << "  " << get_repr() << " = div " << mul->get_repr() << ", "
           << unary->get_repr() << endl;
    } else if (op == "%") {
      cout << "  " << get_repr() << " = mod " << mul->get_repr() << ", "
           << unary->get_repr() << endl;
    }
  }
}

void MulAST::Eval() {
  if (op == "") {
    // mul -> unary
    unary->Eval();
    // copy info
    is_number = unary->is_number;
    val = unary->val;
    addr = unary->addr;
  } else {
    // mul -> mul op unary
    mul->Eval();
    unary->Eval();

    is_number = false;  // currently, mul is not number, improve in future
    addr = new_temp();
  }
}

void AddAST::Dump() {
  if (op == "") {
    mul->Dump();
  } else {
    add->Dump();
    mul->Dump();

    if (op == "+") {
      cout << "  " << get_repr() << " = add " << add->get_repr() << ", "
           << mul->get_repr() << endl;
    } else if (op == "-") {
      cout << "  " << get_repr() << " = sub " << add->get_repr() << ", "
           << mul->get_repr() << endl;
    }
  }
}

void AddAST::Eval() {
  if (op == "") {
    // add -> mul
    mul->Eval();
    // copy info
    is_number = mul->is_number;
    val = mul->val;
    addr = mul->addr;
  } else {
    // add -> add op mul
    add->Eval();
    mul->Eval();

    is_number = false;  // currently, add is not number, improve in future
    addr = new_temp();
  }
}

void RelAST::Dump() {
  if (op == "") {
    add->Dump();
  } else {
    rel->Dump();
    add->Dump();

    if (op == "<") {
      cout << "  " << get_repr() << " = lt " << rel->get_repr() << ", "
           << add->get_repr() << endl;
    } else if (op == ">") {
      cout << "  " << get_repr() << " = gt " << rel->get_repr() << ", "
           << add->get_repr() << endl;
    } else if (op == "<=") {
      cout << "  " << get_repr() << " = le " << rel->get_repr() << ", "
           << add->get_repr() << endl;
    } else if (op == ">=") {
      cout << "  " << get_repr() << " = ge " << rel->get_repr() << ", "
           << add->get_repr() << endl;
    }
  }
}

void RelAST::Eval() {
  if (op == "") {
    // rel -> add
    add->Eval();
    // copy info
    is_number = add->is_number;
    val = add->val;
    addr = add->addr;
  } else {
    // rel -> rel op add
    rel->Eval();
    add->Eval();

    is_number = false;  // currently, rel is not number, improve in future
    addr = new_temp();
  }
}

void EqAST::Dump() {
  if (op == "") {
    rel->Dump();
  } else {
    eq->Dump();
    rel->Dump();

    if (op == "==") {
      cout << "  " << get_repr() << " = eq " << eq->get_repr() << ", "
           << rel->get_repr() << endl;
    } else if (op == "!=") {
      cout << "  " << get_repr() << " = ne " << eq->get_repr() << ", "
           << rel->get_repr() << endl;
    }
  }
}

void EqAST::Eval() {
  if (op == "") {
    // eq -> rel
    rel->Eval();
    // copy info
    is_number = rel->is_number;
    val = rel->val;
    addr = rel->addr;
  } else {
    // eq -> eq op rel
    eq->Eval();
    rel->Eval();

    is_number = false;  // currently, eq is not number, improve in future
    addr = new_temp();
  }
}

void LAndAST::Dump() {
  if (is_single) {
    eq->Dump();
  } else {
    land->Dump();
    // check if land is 0
    int land_tmp = new_temp();
    cout << "  %" << land_tmp << " = eq " << land->get_repr() << ", 0" << endl;

    eq->Dump();
    // check if eq is 0
    int eq_tmp = new_temp();
    cout << "  %" << eq_tmp << " = eq " << eq->get_repr() << ", 0" << endl;

    // create tmp and flip on tmp
    int tmp_addr = new_temp();
    cout << "  %" << tmp_addr << " = or %" << land_tmp << ", %" << eq_tmp << endl;

    cout << "  " << get_repr() << " = eq %" << tmp_addr << ", 0 " << endl;
  }
}

void LAndAST::Eval() {
  if (is_single) {
    // land -> eq
    eq->Eval();
    // copy info
    is_number = eq->is_number;
    val = eq->val;
    addr = eq->addr;
  } else {
    // land -> land op eq
    land->Eval();
    eq->Eval();

    is_number = false;  // currently, land is not number, improve in future
    addr = new_temp();
  }
}

void LOrAST::Dump() {
  if (is_single) {
    land->Dump();
  } else {
    lor->Dump();
    // check if lor is 0
    int lor_tmp = new_temp();
    cout << "  %" << lor_tmp << " = eq " << lor->get_repr() << ", 0" << endl;

    land->Dump();
    // check if land is 0
    int land_tmp = new_temp();
    cout << "  %" << land_tmp << " = eq " << land->get_repr() << ", 0" << endl;

    // create tmp and flip on tmp
    int tmp_addr = new_temp();
    cout << "  %" << tmp_addr << " = and %" << lor_tmp << ", %" << land_tmp << endl;

    cout << "  " << get_repr() << " = eq %" << tmp_addr << ", 0 " << endl;
  }
}

void LOrAST::Eval() {
  if (is_single) {
    // lor -> land
    land->Eval();
    // copy info
    is_number = land->is_number;
    val = land->val;
    addr = land->addr;
  } else {
    // lor -> lor op land
    lor->Eval();
    land->Eval();

    is_number = false;  // currently, lor is not number, improve in future
    addr = new_temp();
  }
}
