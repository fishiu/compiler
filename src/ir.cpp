#include <ir.hpp>
#include <map>
#include <vector>
#include <cmath>

using namespace std;

// some global variables
class Stack {
 private:
  int stack_top = 0;   // stack top position
  int stack_size = 0;  // stack length
 
 public:
  int inc_top(int size) {
    printf("[debug stack] inc_stack: %d\n", size);
    return stack_top += size;
  }
  
  int get_top() {
    return stack_top;
  }

  int get_size() {
    return stack_size;
  }
  
  void set_size(int size) {
    printf("[debug stack] set_size: %d\n", size);
    stack_size = size;
  }

  void clear() {
    stack_top = 0;
    stack_size = 0;
  }
} stack;

// now reg allocator use three-level reg state to better allocate
class RegAllocator {
 private:
  // todo consider changing the regs into queue or other data structure to avoid loops
  vector<reg_t> regs;
 
 public:
  RegAllocator() {
    for (int i = 0; i < REGNUM; ++i) {
      reg_t reg = {i, false};
      regs.push_back(reg);
    }
  }

  const reg_t& alloc() {
    for (auto &reg : regs) {
      if (!reg.occupied) {
        // found free reg
        reg.occupied = true;
        return reg;
      }
    }
    // i believe that currently reg won't be full
    // todo a half written version is saved in git stash
    
    // no available register
    assert(false);
  }

  /**
   * @brief alloc specific reg
   * 
   * @param reg_id 
   */
  void alloc(int reg_id) {
    assert(reg_id < REGNUM);  // x0 cannot be allocated
    assert(regs[reg_id].occupied == false);
    regs[reg_id].occupied = true;
  }

  void free() {
    for (auto &reg : regs) {
      reg.occupied = false;
    }
  }

  /**
   * @brief free specific reg
   * 
   * @param reg_id 
   */
  void free(int reg_id) {
    assert(reg_id < REGNUM);
    regs[reg_id].occupied = false;
  }
} reg_allocator;

string format_reg(int reg_num) {
  string reg_str;
  if (reg_num < 7) {  // t0 ~ t6
    reg_str = "t" + to_string(reg_num);
  } else if (reg_num < REGNUM) {  // a0 ~ a7
    reg_str = "a" + to_string(reg_num - 7);
  } else if (reg_num == REGNUM) {  // x0
    reg_str = "x0";
  } else {
    assert(false);
  }
  return reg_str;
}

// must use a value map, so when referred to a value pointer
// it won't be dump twice
map<const koopa_raw_value_t, repr_t> vmap;
int ra_addr = -1;  // -1 means no ra address

string get_op_str(koopa_raw_binary_op_t op) {
  switch (op) {
    case KOOPA_RBO_ADD:
      return "add";
    case KOOPA_RBO_SUB:
      return "sub";
    case KOOPA_RBO_MUL:
      return "mul";
    case KOOPA_RBO_DIV:
      return "div";
    case KOOPA_RBO_MOD:
      return "rem";
    case KOOPA_RBO_LT:
      return "slt";
    case KOOPA_RBO_AND:
      return "and";
    case KOOPA_RBO_OR:
      return "or";
    case KOOPA_RBO_GT:
      return "sgt";
    default:
      assert(false);
  }
}

void gen_riscv(string koopa_str) {
  printf("%s\n", koopa_str.c_str());
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(koopa_str.c_str(), &program);
  assert(ret == KOOPA_EC_SUCCESS);

  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  koopa_delete_program(program);

  // handle raw before delete
  Visit(raw);

  koopa_delete_raw_program_builder(builder);
}

// visit raw program
void Visit(const koopa_raw_program_t &program) {
  // initwork

  Visit(program.values);

  Visit(program.funcs);
}

// visit raw slice
void Visit(const koopa_raw_slice_t &slice) {
  printf("visit slice\n");
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        assert(false);
    }
  }
}

// visit func
void Visit(const koopa_raw_function_t &func) {
  // enter new function, all things are cleard
  stack.clear();
  ra_addr = -1;
  reg_allocator.free();
  vmap.clear();

  // generate information
  cout << "  .text" << endl;
  cout << "  .globl " << func->name + 1 << endl;
  cout << func->name + 1 << ":" << endl;
  
  // prepare stack size
  int max_arg_num = 0;
  bool save_ra = false;
  int stack_size = 0;

  // loop through basic blocks in the function
  for (size_t i = 0; i < func->bbs.len; ++i) {
    koopa_raw_basic_block_t bb_ptr = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    for (size_t j = 0; j < bb_ptr->insts.len; ++j) {
      koopa_raw_value_t inst_ptr = reinterpret_cast<koopa_raw_value_t>(bb_ptr->insts.buffer[j]);
      if (inst_ptr->ty->tag != KOOPA_RTT_UNIT) {
        // only value with return is counted
        stack_size += 4;
      }
      if (inst_ptr->kind.tag == KOOPA_RVT_CALL) {
        // caller save ra
        save_ra = true;
        // get max arg num
        int arg_num = inst_ptr->kind.data.call.args.len;
        if (arg_num > max_arg_num) {
          max_arg_num = arg_num;
        }
      }
    }
  }

  // calculate stack size
  int arg_in_stack_size = 0;
  if (max_arg_num > 8) {
    arg_in_stack_size = (max_arg_num - 8) * 4;
  }
  stack_size += arg_in_stack_size;
  stack.inc_top(arg_in_stack_size);  // increase at once
  // multi calls share the same save_ra_addr
  if (save_ra)
    stack_size += 4;
  // align to 16
  stack_size = ceil(stack_size / 16.0) * 16;
  stack.set_size(stack_size);
  if (stack_size) {
    if (stack_size <= 2048)
      cout << "  addi sp, sp, " << -stack_size << endl;
    else {
      cout << "  li t0, " << -stack_size << endl;
      cout << "  addi sp, sp, t0" << endl;
    }
  }

  // assign ra address
  if (save_ra) {
    ra_addr = stack.get_size() - 4;
    cout << "  sw ra, " << ra_addr << "(sp)" << endl;
  }

  // params
  for (size_t i = 0; i < func->params.len; ++i) {
    koopa_raw_value_t param_value = reinterpret_cast<koopa_raw_value_t>(func->params.buffer[i]);
    if (i < 8) {
      int reg_id = i + 7;
      reg_allocator.alloc(reg_id);
      repr_t repr = {true, reg_id};
      vmap[param_value] = repr;
    } else {
      int addr = stack.get_size() + (i - 8) * 4;
      repr_t repr = {false, addr};
      vmap[param_value] = repr;
    }
  }

  // visit all basic blocks
  Visit(func->bbs);
}

// visit basic block
void Visit(const koopa_raw_basic_block_t &bb) {
  printf("visit bb\n");
  // +1: remove the starting % of block name
  cout << bb->name + 1 << ":" << endl;
  Visit(bb->insts);
}

// visit value
repr_t Visit(const koopa_raw_value_t &value) {
  if (vmap.count(value)) {
    // do not use reference here, no need to change stored value
    repr_t repr = vmap[value];
    if (repr.is_reg) {
      return repr;
    } else {
      // no reg binded but has addr in stack
      assert(repr.addr != -1);
      reg_t reg = reg_allocator.alloc();
      cout << "  lw " << format_reg(reg.regid) << ", " << repr.addr << "(sp)" << endl;
      repr.is_reg = true;
      repr.addr = reg.regid;
    }  // already has reg bound to value
    return repr;
  }

  printf("visit new value\n");
  const auto &kind = value->kind;
  repr_t repr = {false, -1};
  bool has_ret;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      cout << "\n  # ret" << endl;
      Visit(kind.data.ret);
      reg_allocator.free();
      break;
    case KOOPA_RVT_INTEGER:
      repr = Visit(kind.data.integer);
      // do not save ! do not clear reg!
      break;
    case KOOPA_RVT_BINARY:
      cout << "\n  # binary" << endl;
      repr = Visit(kind.data.binary);
      assert(!repr.is_reg);
      vmap[value] = repr;
      reg_allocator.free();
      break;
    case KOOPA_RVT_ALLOC:
      cout << "\n  # alloc" << endl;
      repr.addr = stack.get_top();
      stack.inc_top(4);
      vmap[value] = repr;
      reg_allocator.free();
      break;
    case KOOPA_RVT_LOAD:
      cout << "\n  # load" << endl;
      repr = Visit(kind.data.load);
      assert(!repr.is_reg);
      vmap[value] = repr;
      reg_allocator.free();
      break;
    case KOOPA_RVT_STORE:
      cout << "\n  # store" << endl;
      Visit(kind.data.store);
      reg_allocator.free();
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      reg_allocator.free();
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      reg_allocator.free();
      break;
    case KOOPA_RVT_CALL:
      has_ret = value->ty->tag != KOOPA_RTT_UNIT;
      repr = Visit(kind.data.call, has_ret);
      vmap[value] = repr;  // todo actually, if has_ret=false, won't be used anymore
      reg_allocator.free();
      break;
    case KOOPA_RVT_FUNC_ARG_REF:
      // already handle all func args in func def
      assert(false);
      break;
    default:
      // 其他类型暂时遇不到
      printf("unhandled value kind %d\n", kind.tag);
      assert(false);
  }
  // reg_allocator.free();  // todo is it ok? fuck! not ok!
  return repr;
}

// visit return
void Visit(const koopa_raw_return_t &ret) {
  koopa_raw_value_t retv = ret.value;
  if (retv) {  // not void ret
    repr_t repr = Visit(retv);
    assert(repr.is_reg);
    if (repr.addr != 7) {
      cout << "  mv a0" << ", " << format_reg(repr.addr) << endl;
    }
  }

  // load ra from sp
  if (ra_addr > 0) {  // todo
    cout << "  lw ra, " << ra_addr << "(sp)" << endl;
  }

  // modify stack pointer
  int stack_size = stack.get_size();
  if (stack_size) {
    if (stack_size <= 2048)
      cout << "  addi sp, sp, " << stack_size << endl;
    else {
      // too big stack size
      cout << "  li t0, " << stack_size << endl;
      cout << "  addi sp, sp, t0" << endl;
    }
  }

  cout << "  ret" << endl;
}

// visit integer
repr_t Visit(const koopa_raw_integer_t &integer) {
  printf("visit integer\n");
  repr_t repr = {true, -1};

  if (integer.value == 0) {
    repr.addr = REGNUM;  // x0
    return repr;
  }
  reg_t reg = reg_allocator.alloc();  // only interger won't occupy
  repr.addr = reg.regid;
  cout << "  li " << format_reg(repr.addr) << ", " << integer.value << endl;
  
  return repr;
}

// visit binary expression
repr_t Visit(const koopa_raw_binary_t &binary) {
  printf("visit binary\n");
  koopa_raw_binary_op_t op = binary.op;
  // assume: only int will assign new reg when Visit(koopa_value)
  repr_t left = Visit(binary.lhs);
  repr_t right = Visit(binary.rhs);
  repr_t result = {true, reg_allocator.alloc().regid};

  string op_str;
  // load left
  if (!left.is_reg) {
    reg_t reg = reg_allocator.alloc();
    cout << "  lw " << format_reg(reg.regid) << ", " << left.addr << "(sp)" << endl;
    left.is_reg = true;
    left.addr = reg.regid;
  }
  // load right
  if (!right.is_reg) {
    reg_t reg = reg_allocator.alloc();
    cout << "  lw " << format_reg(reg.regid) << ", " << right.addr << "(sp)" << endl;
    right.is_reg = true;
    right.addr = reg.regid;
  }
  string left_reg = format_reg(left.addr);
  string right_reg = format_reg(right.addr);
  string result_reg = format_reg(result.addr);
  
  switch (op) {
    case KOOPA_RBO_ADD:  // add
    case KOOPA_RBO_SUB:  // sub
    case KOOPA_RBO_MUL:  // mul
    case KOOPA_RBO_DIV:  // div
    case KOOPA_RBO_MOD:  // rem
    case KOOPA_RBO_LT:   // slt
    case KOOPA_RBO_AND:  // and
    case KOOPA_RBO_OR:   // or
    case KOOPA_RBO_GT:   // sgt
      op_str = get_op_str(op);
      cout << "  # " << op_str << endl;
      cout << "  " << op_str << " " << result_reg << ", " << left_reg << ", " << right_reg << endl;
      break;
    case KOOPA_RBO_EQ:  // eq
      cout << "  # eq" << endl;
      cout << "  xor " << result_reg << ", " << left_reg << ", " << right_reg << endl;
      cout << "  seqz " << result_reg << ", " << result_reg << endl;
      break;
    case KOOPA_RBO_NOT_EQ:  // neq
      cout << "  xor " << result_reg << ", " << left_reg << ", " << right_reg << endl;
      cout << "  snez " << result_reg << ", " << result_reg << endl;
      break;
    case KOOPA_RBO_LE:  // le
      cout << "  sgt " << result_reg << ", " << left_reg << ", " << right_reg << endl;
      cout << "  xori " << result_reg << ", " << result_reg << ", 1" << endl; 
      break;
    case KOOPA_RBO_GE:  // ge
      cout << "  slt " << result_reg << ", " << left_reg << ", " << right_reg << endl;
      cout << "  xori " << result_reg << ", " << result_reg << ", 1" << endl; 
      break;
    default:
      assert(false);
  }

  result.is_reg = false;
  result.addr = stack.get_top();
  stack.inc_top(4);
  cout << "  sw " << result_reg << ", " << result.addr << "(sp)" << endl;

  return result;
}

// load value from src
repr_t Visit(const koopa_raw_load_t &load) {
  printf("visit load\n");
  koopa_raw_value_t src = load.src;
  // save the load value in reg, ready to return
  repr_t src_repr = Visit(src);
  
  assert(src_repr.is_reg);
  string src_reg_name = format_reg(src_repr.addr);

  // store the reg content into stack
  repr_t dest = {false, stack.get_top()};
  stack.inc_top(4);
  cout << "  sw " << src_reg_name << ", " << dest.addr << "(sp)" << endl;

  return dest;
}

void Visit(const koopa_raw_store_t &store) {
  repr_t repr = Visit(store.value);
  koopa_raw_value_t dest = store.dest;
  assert(vmap.count(dest));
  repr_t dest_repr = vmap[dest];
  assert(!dest_repr.is_reg);

  assert(repr.is_reg);
  string reg_name = format_reg(repr.addr);
  cout << "  sw " << reg_name << ", " << dest_repr.addr << "(sp)" << endl;
}

void Visit(const koopa_raw_branch_t &branch) {
  string label_true = branch.true_bb->name + 1;
  string label_false = branch.false_bb->name + 1;
  repr_t cond_repr = Visit(branch.cond);
  assert(cond_repr.is_reg);
  cout << "  bnez " << format_reg(cond_repr.addr) << ", " << label_true << endl;
  cout << "  j " << label_false << endl;
}

void Visit(const koopa_raw_jump_t &jump) {
  string label_target = jump.target->name + 1;
  cout << "  j " << label_target << endl;
}

repr_t Visit(const koopa_raw_call_t &call, bool has_ret) {
  repr_t repr;
  for (size_t i = 0; i < call.args.len; ++i) {
    koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    repr_t arg_repr = Visit(arg);
    assert(arg_repr.is_reg);
    int reg_id = arg_repr.addr;  // currently it is saved in arg_reg_id
    if (i < 8) {
      int reg_ai = 7 + i;  // target
      // move to ax
      if (reg_id != reg_ai) {
        // todo unsafe assert here, better make sure reg_ai is available
        reg_allocator.alloc(reg_ai);
        cout << "  mv " << format_reg(reg_ai) << ", " << format_reg(reg_id) << endl;
      }
    } else {
      int addr = (i - 8) * 4;  // it is really comfortable!
      cout << "  sw " << format_reg(reg_id) << ", " << addr << "(sp)" << endl;
    }
    reg_allocator.free(reg_id);
  }
  cout << "  call " << call.callee->name + 1 << endl;
  
  // save a0
  if (has_ret) {
    repr = {false, stack.get_top()};
    stack.inc_top(4);
    cout << "  sw a0" << ", " << repr.addr << "(sp)" << endl;
  }
  return repr;
}