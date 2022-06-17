#include <ast.hpp>

static list<int> tmp_var_list;

void CompUnitAST::Dump() { func_def->Dump(); }

void FuncDefAST::Dump() {
  cout << "fun @" << ident << "(): ";
  func_type->Dump();
  cout << " {" << endl << "\%entry:" << endl;
  block->Dump();
  cout << "}";
}

void FuncTypeAST::Dump() {
  if (type == "int") {
    cout << "i32";
  }
}

void BlockAST::Dump() {
  symtab_stack.Push();
  for (auto& item : blocks->vec)
    item->Dump();
  symtab_stack.Pop();
}

void BlockItemAST::Dump() {
  ast->Dump();
}

void DeclAST::Dump() {
  cout << "\n  // decl" << endl;
  decl->Dump();
}

void ConstDeclAST::Dump() {
  for (auto& def : def_list->vec) {
    def->Dump();
  }
}

void ConstDefAST::Dump() {
  init->Eval();  // evaluate
  assert(init->is_number && init->is_const);
  symtab_stack.Insert(ident, init->val);
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
  cout << "\n  // stmt(exp)" << endl;
  if (has_exp) {
    exp->Eval();
    exp->Dump();
  }
}

void AssignAST::Dump() {
  cout << "\n  // assign stmt" << endl;
  // lval = exp
  lval->Eval();
  lval->Dump();
  exp->Eval();
  exp->Dump();

  // exp repr is reg, lval repr is addr
  // cast lval to LValAST
  auto lval_ast = dynamic_cast<LValAST*>(lval.get());
  cout << "  store " << exp->get_repr() << ", " << lval_ast->mem_addr << endl;
}

void RetAST::Dump() {
  cout << "\n  // return stmt" << endl;
  if (has_exp) {
    exp->Eval();
    exp->Dump();
    cout << "  ret " << exp->get_repr() << endl;
  } else {
    cout << "  ret" << endl;
  }
}

void VarDeclAST::Dump() {
  for (auto& def : def_list->vec)
    def->Dump();
}

void VarDefAST::Dump() {
  mem_addr = symtab_stack.Insert(ident);
  cout << "  " << mem_addr << " = alloc i32" << endl;
  if (has_init) {
    init->Eval();
    init->Dump();
    // todo only father knows whether is i32
    cout << "  " << "store " << init->get_repr() << ", " << mem_addr << endl; 
  }
}

void InitValAST::Dump() {
  exp->Dump();
}

void InitValAST::Eval() {
  if (evaluated) return;
  
  exp->Eval();
  CopyInfo(exp);
  
  if (!is_const) evaluated = true;
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
    return;
  }

  unary->Dump();
  if (is_number)
    cout << "  // " << op << " " << unary->val << " is_number=true" << endl;
  else {
    // if (op == "+"): do nothing
    if (op == "-") {
      cout << "  " << get_repr() << " = sub 0, " << unary->get_repr() << endl;
    } else if (op == "!") {
      cout << "  " << get_repr() << " = eq " << unary->get_repr() << ", 0" << endl;
    }
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
  } else {
    unary->Eval();

    is_number = unary->is_number;
    is_const = unary->is_const;
    if (is_number) {
      if (op == "-") {
        val = -unary->val;
      } else if (op == "!") {
        val = !unary->val;
      }
    } else {
      NewTempVar();
    }
  }
  if (!is_const) evaluated = true;
}

void PrimaryAST::Dump() {
  if (is_lval) {
    lval->Dump();
  } else if (is_exp) {
    exp->Dump();
  }
}

void PrimaryAST::Eval() {
  if (evaluated) return;

  // number case is already evaluated when initializing
  if (is_lval) {  // case: lval
    lval->Eval();
    CopyInfo(lval);
  } else if (is_exp) {  // case: exp
    exp->Eval();
    CopyInfo(exp);
  }

  if (!is_const) evaluated = true;
}

void LValAST::Dump() {
  if (!is_number) {
    // if const: just print the number
    // todo currently we do not consider optimization when lval is a inconstant number
    if (!at_left) {
      // %0 = load @x
      cout << "  " << get_repr() << " = load " << mem_addr << endl;
    }
  } else {
    cout << "  // " << val << " is_number=true" << endl;
  }
}

void LValAST::Eval() {
  if (evaluated) return;

  sym = symtab_stack.Lookup(ident);
  if (sym.index() == 0) {
    // int
    is_number = true;
    val = get<int>(sym);
    assert(!at_left);  // must be a variable and has string
  } else if (sym.index() == 1) {
    // string
    mem_addr = get<string>(sym);
    if (!at_left)
      addr = NewTempVar();
  } else {
    assert(false);
  }

  if (!is_const) evaluated = true;
}

void MulAST::Dump() {
  if (op == "") {
    unary->Dump();
    return;
  }

  // else
  mul->Dump();
  unary->Dump();

  if (is_number) {
    cout << "  // " << mul->val << " " << op << " " << unary->val << " is_number=true" << endl;
  } else {  // not number, calculate with reg addr
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
  if (evaluated) return;

  if (op == "") {
    // mul -> unary
    unary->Eval();
    CopyInfo(unary);
  } else {
    // mul -> mul op unary
    mul->Eval();
    unary->Eval();

    is_number = mul->is_number && unary->is_number;
    is_const = mul->is_const && unary->is_const;
    if (is_number) {
      if (op == "*") {
        val = mul->val * unary->val;
      } else if (op == "/") {
        val = mul->val / unary->val;
      } else if (op == "%") {
        val = mul->val % unary->val;
      }
    } else {
      // create new tmp var
      addr = NewTempVar();
    }
  }

  if (!is_const) evaluated = true;
}

void AddAST::Dump() {
  if (op == "") {
    mul->Dump();
    return;
  }
  
  // else
  add->Dump();
  mul->Dump();

  if (is_number)
    cout << "  // " << add->val << " " << op << " " << mul->val << " is_number=true" << endl;
  else {
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
  if (evaluated) return;
  
  if (op == "") {
    // add -> mul
    mul->Eval();
    CopyInfo(mul);
  } else {
    // add -> add op mul
    add->Eval();
    mul->Eval();

    is_number = add->is_number && mul->is_number;
    is_const = add->is_const && mul->is_const;
    if (is_number) {
      if (op == "+") {
        val = add->val + mul->val;
      } else if (op == "-") {
        val = add->val - mul->val;
      }
    } else {
      // create new tmp var
      addr = NewTempVar();
    }
  }
  
  if (!is_const) evaluated = true;
}

void RelAST::Dump() {
  if (op == "") {
    add->Dump();
    return;
  }
  rel->Dump();
  add->Dump();

  if (is_number)
    cout << "  // " << rel->val << " " << op << " " << add->val << " no dump" << endl;
  else {
    if (op == "<") {
      cout << "  " << get_repr() << " = lt " << rel->get_repr() << ", " << add->get_repr() << endl;
    } else if (op == ">") {
      cout << "  " << get_repr() << " = gt " << rel->get_repr() << ", " << add->get_repr() << endl;
    } else if (op == "<=") {
      cout << "  " << get_repr() << " = le " << rel->get_repr() << ", " << add->get_repr() << endl;
    } else if (op == ">=") {
      cout << "  " << get_repr() << " = ge " << rel->get_repr() << ", " << add->get_repr() << endl;
    }
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

    is_number = rel->is_number && add->is_number;
    is_const = rel->is_const && add->is_const;
    if (is_number) {
      if (op == "<") {
        val = rel->val < add->val;
      } else if (op == ">") {
        val = rel->val > add->val;
      } else if (op == "<=") {
        val = rel->val <= add->val;
      } else if (op == ">=") {
        val = rel->val >= add->val;
      }
    } else {
      // create new tmp var
      addr = NewTempVar();
    }
  }

  if (!is_const) evaluated = true;
}

void EqAST::Dump() {
  if (op == "") {
    rel->Dump();
    return;
  }
  eq->Dump();
  rel->Dump();

  if (is_number)
    cout << "  // " << eq->val << " " << op << " " << rel->val << " no dump" << endl;
  else {
    if (op == "==") {
      cout << "  " << get_repr() << " = eq " << eq->get_repr() << ", " << rel->get_repr() << endl;
    } else if (op == "!=") {
      cout << "  " << get_repr() << " = ne " << eq->get_repr() << ", " << rel->get_repr() << endl;
    }
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

    is_number = eq->is_number && rel->is_number;
    is_const = eq->is_const && rel->is_const;
    if (is_number) {
      if (op == "==") {
        val = eq->val == rel->val;
      } else if (op == "!=") {
        val = eq->val != rel->val;
      }
    } else {
      // create new tmp var
      addr = NewTempVar();
    }
  }
  
  if (!is_const) evaluated = true;
}

void LAndAST::Dump() {
  if (is_single) {
    eq->Dump();
    return;
  }
  land->Dump();
  eq->Dump();
  if (is_number)
    cout << "  // " << land->val << " && " << eq->val << " no dump" << endl;
  else {
    land->Dump();
    // check if land is 0
    int land_tmp = tmp_var_no++;
    cout << "  %" << land_tmp << " = eq " << land->get_repr() << ", 0" << endl;

    eq->Dump();
    // check if eq is 0
    int eq_tmp = tmp_var_no++;
    cout << "  %" << eq_tmp << " = eq " << eq->get_repr() << ", 0" << endl;

    // create tmp and flip on tmp
    int tmp_addr = tmp_var_no++;
    cout << "  %" << tmp_addr << " = or %" << land_tmp << ", %" << eq_tmp << endl;
    cout << "  " << get_repr() << " = eq %" << tmp_addr << ", 0 " << endl;
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

    is_number = land->is_number && eq->is_number;
    is_const = land->is_const && eq->is_const;
    if (is_number) {
      val = land->val && eq->val;
    } else {
      // create new tmp var
      addr = NewTempVar();
    }
  }
  
  if (!is_const) evaluated = true;
}

void LOrAST::Dump() {
  if (is_single) {
    land->Dump();
    return;
  }
  lor->Dump();
  land->Dump();
  if (is_number)
    cout << "  // " << lor->val << " || " << land->val << " no dump" << endl;
  else {
    lor->Dump();
    // check if lor is 0
    int lor_tmp = tmp_var_no++;
    cout << "  %" << lor_tmp << " = eq " << lor->get_repr() << ", 0" << endl;

    land->Dump();
    // check if land is 0
    int land_tmp = tmp_var_no++;
    cout << "  %" << land_tmp << " = eq " << land->get_repr() << ", 0" << endl;

    // create tmp and flip on tmp
    int tmp_addr = tmp_var_no++;
    cout << "  %" << tmp_addr << " = and %" << lor_tmp << ", %" << land_tmp << endl;
    cout << "  " << get_repr() << " = eq %" << tmp_addr << ", 0 " << endl;
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

    is_number = lor->is_number && land->is_number;
    is_const = lor->is_const && land->is_const;
    if (is_number) {
      val = lor->val || land->val;
    } else {
      // create new tmp var
      addr = NewTempVar();
    }
  }
  
  if (!is_const) evaluated = true;
}
