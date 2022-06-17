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
} stack;

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
        // found an available registor
        reg.occupied = true;
        return reg;
      }
    }
    // no available register
    assert(false);
  }

  void free() {
    for (auto &reg : regs) {
      reg.occupied = false;
    }
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

  cout << "  .text" << endl;
  cout << "  .globl main" << endl;
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
  printf("visit func\n");
  cout << func->name + 1 << ":" << endl;
  // get stack size
  int stack_size = 0;
  for (size_t i = 0; i < func->bbs.len; ++i) {
    koopa_raw_basic_block_t bb_ptr = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    for (size_t j = 0; j < bb_ptr->insts.len; ++j) {
      koopa_raw_value_t inst_ptr = reinterpret_cast<koopa_raw_value_t>(bb_ptr->insts.buffer[j]);
      if (inst_ptr->ty->tag != KOOPA_RTT_UNIT)
        // only value with return is counted
        stack_size += 4;
    }
  }
  stack_size = ceil(stack_size / 16.0) * 16;
  stack.set_size(stack_size);
  if (stack_size <= 2048)
    cout << "  addi sp, sp, " << -stack_size << endl;
  else {
    cout << "  li t0, " << -stack_size << endl;
    cout << "  addi sp, sp, t0" << endl;
  }
  // visit all basic blocks
  Visit(func->bbs);
}

// visit basic block
void Visit(const koopa_raw_basic_block_t &bb) {
  printf("visit bb\n");
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
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      cout << "\n  # ret" << endl;
      Visit(kind.data.ret);
      reg_allocator.free();
      break;
    case KOOPA_RVT_INTEGER:
      repr = Visit(kind.data.integer);
      // do not save
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
    default:
      // 其他类型暂时遇不到
      printf("unhandled value kind %d\n", kind.tag);
      assert(false);
  }

  return repr;
}

// visit return
void Visit(const koopa_raw_return_t &ret) {
  printf("visit return\n");
  repr_t repr = Visit(ret.value);

  // cout << "  # ret" << endl;
  assert(repr.is_reg);
  cout << "  mv a0, " << format_reg(repr.addr) << endl;
  int stack_size = stack.get_size();
  if (stack_size <= 2048)
    cout << "  addi sp, sp, " << stack_size << endl;
  else {
    // too big stack size
    cout << "  li t0, " << stack_size << endl;
    cout << "  addi sp, sp, t0" << endl;
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
  repr_t dest = {true, reg_allocator.alloc().regid};
  // save the load value in reg, ready to return
  repr_t src_repr = Visit(src);
  
  assert(src_repr.is_reg);
  string reg_name = format_reg(dest.addr);
  string src_reg_name = format_reg(src_repr.addr);

  // store the reg content into stack
  dest = {false, stack.get_top()};
  stack.inc_top(4);
  cout << "  sw " << reg_name << ", " << dest.addr << "(sp)" << endl;

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
  std::cout << "  sw " << reg_name << ", " << dest_repr.addr << "(sp)" << std::endl;
}