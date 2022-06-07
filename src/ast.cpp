#include <ast.hpp>

static list<int> tmp_var_list;

// new temp var
// static inline int new_temp() {
//   tmp_var_list.push_front(tmp_var_no++);
//   return tmp_var_list.front();
// }

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
  for (auto& item : blocks->vec) {
    item->Dump();
  }
}

void BlockItemAST::Dump() {
  ast->Dump();
}

void DeclAST::Dump() {
  const_decl->Dump();
}

void ConstDeclAST::Dump() {
  for (auto& def : def_list->vec) {
    def->Dump();
  }
}

void ConstDefAST::Dump() {
  init->Eval();
  // todo current dump nothing, becuase def do not need ir
  init->Dump();
  cout << "  // const def " << *ident << " = " << init->val << endl;
}

void ConstInitValAST::Dump() {
  exp->Dump();
}

void ConstInitValAST::Eval() {
  if (evaluated) return;
  
  exp->Eval();
  CopyInfo(exp);
  
  if (!is_const) evaluated = true;
}

void ConstExpAST::Dump() {
  exp->Dump();
}

void ConstExpAST::Eval() {
  if (evaluated) return;
  
  exp->Eval();
  is_const = true;
  is_number = true;
  evaluated = true;
  val = exp->val;
  addr = exp->addr;

  if (!is_const) evaluated = true;
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
  lor->Dump();
}

void ExpAST::Eval() {
  if (evaluated) return;

  lor->Eval();
  CopyInfo(lor);
  
  if (!is_const) evaluated = true;
}

void UnaryAST::Dump() {
  if (op == "") {
    primary->Dump();
  } else {
    unary->Dump();
    // todo currently delete calculation
    // code can be recovered in git commit before lv4
    cout << "  // " << op << " " << unary->val << " no dump" << endl;
  }
}

void UnaryAST::Eval() {
  if (evaluated) return;

  if (op == "") {
    // unary -> primary
    primary->Eval();
    CopyInfo(primary);
  } else if (op == "+") {
    // unary -> + primary (do nothing)
    unary->Eval();
    CopyInfo(unary);
  } else if (op == "-") {
    unary->Eval();
    if (unary->is_number) {
      is_number = true;
      val = -unary->val;
    } else {
      assert(false);
    }
  } else if (op == "!") {
    unary->Eval();
    if (unary->is_number) {
      is_number = true;
      is_const = unary->is_const;
      val = !unary->val;
    } else {
      assert(false);
    }
  }

  if (!is_const) evaluated = true;
}

void PrimaryAST::Dump() {
  if (!is_number) {
    printf("error: primary is not number\n");
    exp->Dump();
  }
}

void PrimaryAST::Eval() {
  if (evaluated) return;

  // number case is already evaluated
  if (is_lval) {
    // todo currently lval will only be const and number
    is_const = true;
    is_number = true;
    evaluated = true;
    val = lval->val;
    addr = lval->addr;
  }
  if (!is_number) {
    // this should not be reached, because all exp will be const number
    printf("error: primary is not number\n");
    exp->Eval();
    CopyInfo(exp);
  }

  if (!is_const) evaluated = true;
}

void MulAST::Dump() {
  if (op == "") {
    unary->Dump();
  } else {
    mul->Dump();
    unary->Dump();

    // todo currently delete calculation
    // code can be recovered in git commit before lv4
    cout << "  // " << mul->val << " " << op << " " << unary->val << " no dump" << endl;
  }
}

void MulAST::Eval() {
  if (evaluated) return;

  if (op == "") {
    // mul -> unary
    unary->Eval();
    CopyInfo(unary);
  } else {
    // mul -> mul op unary
    mul->Eval();
    unary->Eval();

    is_number = true;
    is_const = mul->is_const && unary->is_const;
    if (op == "*") {
      val = mul->val * unary->val;
    } else if (op == "/") {
      val = mul->val / unary->val;
    } else if (op == "%") {
      val = mul->val % unary->val;
    }
  }

  if (!is_const) evaluated = true;
}

void AddAST::Dump() {
  if (op == "") {
    mul->Dump();
  } else {
    add->Dump();
    mul->Dump();

    // todo currently delete calculation
    // code can be recovered in git commit before lv4
    cout << "  // " << add->val << " " << op << " " << mul->val << " no dump" << endl;
  }
}

void AddAST::Eval() {
  if (evaluated) return;
  
  if (op == "") {
    // add -> mul
    mul->Eval();
    CopyInfo(mul);
  } else {
    // add -> add op mul
    add->Eval();
    mul->Eval();

    is_number = true;
    is_const = add->is_const && mul->is_const;
    if (op == "+") {
      val = add->val + mul->val;
    } else if (op == "-") {
      val = add->val - mul->val;
    }
  }
  
  if (!is_const) evaluated = true;
}

void RelAST::Dump() {
  if (op == "") {
    add->Dump();
  } else {
    rel->Dump();
    add->Dump();

    // todo currently delete calculation
    // code can be recovered in git commit before lv4
    cout << "  // " << rel->val << " " << op << " " << add->val << " no dump" << endl;
  }
}

void RelAST::Eval() {
  if (evaluated) return;

  if (op == "") {
    // rel -> add
    add->Eval();
    CopyInfo(add);
  } else {
    // rel -> rel op add
    rel->Eval();
    add->Eval();

    is_number = true;
    is_const = rel->is_const && add->is_const;
    if (op == "<") {
      val = rel->val < add->val;
    } else if (op == ">") {
      val = rel->val > add->val;
    } else if (op == "<=") {
      val = rel->val <= add->val;
    } else if (op == ">=") {
      val = rel->val >= add->val;
    }
  }
  
  if (!is_const) evaluated = true;
}

void EqAST::Dump() {
  if (op == "") {
    rel->Dump();
  } else {
    eq->Dump();
    rel->Dump();

    // todo currently delete calculation
    // code can be recovered in git commit before lv4
    cout << "  // " << eq->val << " " << op << " " << rel->val << " no dump" << endl;
  }
}

void EqAST::Eval() {
  if (evaluated) return;

  if (op == "") {
    // eq -> rel
    rel->Eval();
    CopyInfo(rel);
  } else {
    // eq -> eq op rel
    eq->Eval();
    rel->Eval();

    is_number = true;
    is_const = eq->is_const && rel->is_const;
    if (op == "==") {
      val = eq->val == rel->val;
    } else if (op == "!=") {
      val = eq->val != rel->val;
    }
  }
  
  if (!is_const) evaluated = true;
}

void LAndAST::Dump() {
  if (is_single) {
    eq->Dump();
  } else {
    land->Dump();
    eq->Dump();

    // todo currently delete calculation
    // code can be recovered in git commit before lv4
    cout << "  // " << land->val << " && " << eq->val << " no dump" << endl;
  }
}

void LAndAST::Eval() {
  if (evaluated) return;

  if (is_single) {
    // land -> eq
    eq->Eval();
    CopyInfo(eq);
  } else {
    // land -> land op eq
    land->Eval();
    eq->Eval();

    is_number = true;
    is_const = land->is_const && eq->is_const;
    val = land->val && eq->val;
  }
  
  if (!is_const) evaluated = true;
}

void LOrAST::Dump() {
  if (is_single) {
    land->Dump();
  } else {
    lor->Dump();
    land->Dump();
    
    // todo currently delete calculation
    // code can be recovered in git commit before lv4
    cout << "  // " << lor->val << " || " << land->val << " no dump" << endl;
  }
}

void LOrAST::Eval() {
  if (evaluated) return;

  if (is_single) {
    // lor -> land
    land->Eval();
    CopyInfo(land);
  } else {
    // lor -> lor op land
    lor->Eval();
    land->Eval();

    is_number = true;
    is_const = lor->is_const && land->is_const;
    val = lor->val || land->val;
  }
  
  if (!is_const) evaluated = true;
}
