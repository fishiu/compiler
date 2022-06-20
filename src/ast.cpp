#include <ast.hpp>

static list<int> tmp_var_list;

void CompUnitAST::Dump() {
  for (auto& func_def : func_def_list->vec)
    func_def->Dump();
}

void FuncDefAST::Init() {
  auto func_type_ast = dynamic_cast<FuncTypeAST*>(func_type.get());
  if (func_type_ast->type == "void") {
    is_void = true;
    glb_symtab.Insert(ident, "void", true);
  } else {
    glb_symtab.Insert(ident, "i32", true);
  }
}

void FuncDefAST::Dump() {
  // modify func ir string lines
  stringstream tmpss;
  streambuf* old_buf = cout.rdbuf();
  cout.rdbuf(tmpss.rdbuf());  // redirect to tmpss

  // func head
  cout << "fun @" << *ident << "(";
  if (has_param) {
    int param_id = 0;
    for (auto& param : params->vec) {
      if (param_id++ != 0) {
        cout << ", ";
      }
      param->Dump();
      param_id++;
    }
    // cast block to BlockAST
    auto block_ast = dynamic_cast<BlockAST*>(block.get());
    block_ast->func_params = params.get();
  }
  cout << ")";
  func_type->Dump();
  cout << " {" << endl << "\%entry:" << endl;
  block->Dump();

  // handle last empty ret block
  string ir = tmpss.str();
  string lline;  // last line
  int pt;  // pointer to ir (inverse)
  for (pt = ir.length() - 2; ir[pt] != '\n'; pt--)
    // build last line
    lline = ir[pt] + lline;
  if (lline.substr(0, 4) == "%ret")
    // if last line is ret, remove it
    ir = ir.substr(0, pt);
  
  // restore stream
  cout.rdbuf(old_buf);
  cout << ir;

  if (is_void) {
    cout << "  ret" << endl;
  }
  cout << "}" << endl << endl;
}

void FuncTypeAST::Dump() {
  if (type == "int") {
    cout << ": i32";
  } else {
    // do nothing
    assert(type == "void");
  }
}

void FuncFParamAST::Dump() {
  cout << "@" << *ident;
  type->Dump();
}

void BlockAST::Dump() {
  symtab_stack.Push();
  
  // init funcf params
  if (func_params) {
    for (auto& param : func_params->vec) {
      // cast param to FuncFParamAST
      auto fparam_ast = dynamic_cast<FuncFParamAST*>(param.get());
      string mem_addr = symtab_stack.Insert(fparam_ast->ident);
      cout << "  " << mem_addr << " = alloc i32" << endl;
      cout << "  store " << "@" << *fparam_ast->ident << ", " << mem_addr << endl;
    }
  }

  for (auto& item : blocks->vec)
    item->Dump();
  symtab_stack.Pop();
}

void BlockItemAST::Dump() {
  ast->Dump();
}

void DeclAST::Dump() {
  cout << "  // decl" << endl;
  decl->Dump();
}

void ConstDeclAST::Dump() {
  for (auto& def : def_list->vec) {
    def->Dump();
  }
}

void BTypeAST::Dump() {
  if (type == "int") {
    cout << ": i32";
  } else {
    // do nothing
    assert(type == "void");
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
  cout << "  // stmt(exp)" << endl;
  if (has_exp) {
    exp->Eval();
    exp->Dump();
  }
}

void AssignAST::Dump() {
  cout << "  // assign stmt" << endl;
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
  cout << "  // return stmt" << endl;
  if (has_exp) {
    exp->Eval();
    exp->Dump();
    cout << "  ret " << exp->get_repr() << endl;
  } else {
    cout << "  ret" << endl;
  }
  string ret_label = "%ret_" + to_string(ret_label_cnt++);
  cout << endl << ret_label << ":" << endl;
}

void IfAST::Dump() {
  cout << "  // if stmt" << endl;
  cond->Eval();
  cond->Dump();

  string label_then = "%then_" + to_string(label_cnt);
  string label_else = "%else_" + to_string(label_cnt);
  string label_end = "%end_" + to_string(label_cnt);
  label_cnt++;

  if (has_else) {
    cout << "  br " << cond->get_repr() << ", " << label_then << ", " << label_else << endl;
    cout << endl << label_then << ":" << endl;
    if_stmt->Dump();
    cout << "  jump " << label_end << endl;
    cout << endl << label_else << ":" << endl;
    else_stmt->Dump();
  } else {
    cout << "  br " << cond->get_repr() << ", " << label_then << ", " << label_end << endl;
    cout << endl << label_then << ":" << endl;
    if_stmt->Dump();
  }
  cout << "  jump " << label_end << endl;
  cout << endl << label_end << ":" << endl;
}

void WhileAST::Dump() {
  cout << "  // while stmt" << endl;
  labels_t while_labels = while_stack.Push();
  string label_entry = get<0>(while_labels);
  string label_body = get<1>(while_labels);
  string label_end = get<2>(while_labels);

  cout << "  jump " << label_entry << endl;
  // entry
  cout << endl << label_entry << ":" << endl;
  cond->Eval();
  cond->Dump();
  cout << "  br " << cond->get_repr() << ", " << label_body << ", " << label_end << endl;

  // body
  cout << endl << label_body << ":" << endl;
  body->Dump();
  cout << "  jump " << label_entry << endl;

  // end
  // todo do i need to remove redundant end label?
  cout << endl << label_end << ":" << endl;

  while_stack.Pop();
}

void BreakAST::Dump() {
  cout << "  // break stmt" << endl;
  string label_end = get<2>(while_stack.Top());
  string break_label = label_end + "_break";
  cout << "  jump " << label_end << endl;
  // to avoid empty jump in the rest of WhileAST
  cout << endl << break_label << ":" << endl;
}

void ContinueAST::Dump() {
  cout << "  // continue stmt" << endl;
  string label_entry = get<0>(while_stack.Top());
  string continue_label = label_entry + "_continue";
  cout << "  jump " << label_entry << endl;
  cout << endl << continue_label << ":" << endl;
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
      addr = NewTempVar();
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
  if (is_number) {
    cout << "  // " << land->val << " && " << eq->val << " no dump" << endl;
  } else {
    /* short circuit: 
     * int result = 0;
     * if (lhs != 0) {
     *   result = rhs != 0;
     * }
     */

    // prepare label for short circuit
    string label_then = "%then_" + to_string(label_cnt);
    string label_else = "%else_" + to_string(label_cnt);
    string label_end = "%end_" + to_string(label_cnt);
    label_cnt++;

    // create result on stack
    string result = "%" + to_string(tmp_var_no++);
    cout << "  " << result << " = alloc i32" << endl;
    // if lhs != 0 -> label_then, else label_else
    cout << "  br " << land->get_repr() << ", " << label_then << ", " << label_else << endl;
    cout << label_then << ":" << endl;
    string tmp_rhs = "%" + to_string(tmp_var_no++);
    // result = rhs != 0
    cout << "  " << tmp_rhs << " = ne " << eq->get_repr() << ", 0" << endl;
    cout << "  store " << tmp_rhs << ", " << result << endl;
    cout << "  jump " << label_end << endl;
    // label else (result = 0)
    cout << label_else << ":" << endl;
    cout << "  store 0, " << result << endl;
    cout << "  jump " << label_end << endl;
    // label end
    cout << label_end << ":" << endl;
    cout << "  " << get_repr() << " = load " << result << endl;
  }
}

void LAndAST::Eval() {
  // todo consider short-circuit in eval
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
  if (is_number) {
    cout << "  // " << lor->val << " || " << land->val << " no dump" << endl;
  }  else {
    /* short circuit: 
     * int result = 1;
     * if (lhs == 0) {
     *   result = rhs != 0;
     * }
     */

    string label_then = "%then_" + to_string(label_cnt);
    string label_else = "%else_" + to_string(label_cnt);
    string label_end = "%end_" + to_string(label_cnt);
    label_cnt++;

    // create result on stack
    string result = "%" + to_string(tmp_var_no++);
    cout << "  " << result << " = alloc i32" << endl;
    // if lhs == 0 -> label_then, else label_else
    cout << "  br " << lor->get_repr() << ", " << label_else << ", " << label_then << endl;
    cout << label_then << ":" << endl;
    string tmp_rhs = "%" + to_string(tmp_var_no++);
    // result = rhs != 0
    cout << "  " << tmp_rhs << " = ne " << land->get_repr() << ", 0" << endl;
    cout << "  store " << tmp_rhs << ", " << result << endl;
    cout << "  jump " << label_end << endl;
    // label else (result = 1)
    cout << label_else << ":" << endl;
    cout << "  store 1, " << result << endl;
    cout << "  jump " << label_end << endl;
    // label end
    cout << label_end << ":" << endl;
    cout << "  " << get_repr() << " = load " << result << endl;
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

void FuncCallAST::Dump() {
  // %0 = call @half(10, %2)
  if (has_rparams) {
    for (auto& rparam : rparams->vec)
      rparam->Dump();
  }

  glb_sym_t func_sym = glb_symtab.Lookup(ident);
  string sym_val = get<string>(func_sym.val);
  if (sym_val == "void") {
    cout << "  ";
  } else {
    assert(sym_val == "i32");
    cout << "  " << get_repr() << " = ";
  }
  cout << "call @" << *ident << "(";
  
  if (has_rparams) {
    int param_cnt = 0;
    for (auto& param : rparams->vec) {
      auto param_ast = dynamic_cast<ExpBaseAST*>(param.get());
      if (param_cnt++ > 0) {
        cout << ", ";
      }
      cout << param_ast->get_repr();
    }
  }

  cout << ")" << endl;
}

void FuncCallAST::Eval() {
  // todo need to search function table
  if (evaluated) return;

  if (has_rparams) {
    for (auto& rparam : rparams->vec) {
      auto rparam_ast = dynamic_cast<ExpBaseAST*>(rparam.get());
      rparam_ast->Eval();
    }
  }

  evaluated = true;
  is_number = false;
  is_const = false;
  addr = NewTempVar();
}
