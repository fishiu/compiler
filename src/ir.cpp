#include <ir.hpp>
#include <map>
#include <cmath>

using namespace std;

// some global variables
int stack_size = 0;  // stack length
int reg_cnt = 0;
int int_reg_cnt = 0;  // int_reg will be cleaned after visiting a value

string format_reg(int reg_num) {
  string reg_str;
  if (reg_num < 7) {  // t0 ~ t6
    reg_str = "t" + to_string(reg_num);
  } else if (reg_num < 15) {  // a0 ~ a7
    reg_str = "a" + to_string(reg_num - 7);
  } else {
    assert(false);
  }
  return reg_str;
}

// must use a value map, so when referred to a value pointer
// it won't be dump twice
map<const koopa_raw_value_t, string> vmap;

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
  if (stack_size <= 2048)
    cout << "  addi sp, sp, " << to_string(-stack_size) << endl;
  else {
    cout << "  li t0, " << to_string(-stack_size) << endl;
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
string Visit(const koopa_raw_value_t &value) {
  if (vmap[value] != "") {
    return vmap[value];
  }

  printf("visit value\n");
  const auto &kind = value->kind;
  string reg;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      reg = Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      reg = Visit(kind.data.binary);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }

  vmap[value] = reg;
  return reg;
}

// visit return
void Visit(const koopa_raw_return_t &ret) {
  printf("visit return\n");
  string reg = Visit(ret.value);
  cout << "  # ret" << endl;
  cout << "  mv a0, " << reg << endl;
  cout << "  ret" << endl;
}

// visit integer
string Visit(const koopa_raw_integer_t &integer) {
  printf("visit integer\n");
  if (integer.value == 0)
    return "x0";
  string reg = format_reg(int_reg_cnt++);
  cout << "  # li integer" << endl;
  cout << "  " << "li " << reg << ", " << integer.value << endl;
  return reg;
}

// visit binary expression
string Visit(const koopa_raw_binary_t &binary) {
  printf("visit binary\n");
  koopa_raw_binary_op_t op = binary.op;
  // assume: only int will assign new reg when Visit(koopa_value)
  int_reg_cnt = reg_cnt;
  string left = Visit(binary.lhs);
  string right = Visit(binary.rhs);
  int_reg_cnt = reg_cnt;  // restore int_reg_cnt
  string reg = format_reg(reg_cnt++);
  string op_str;
  
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
      cout << "  " << op_str << " " << reg << ", " << left << ", " << right << endl;
      break;
    case KOOPA_RBO_EQ:  // eq
      cout << "  # eq" << endl;
      cout << "  xor " << reg << ", " << left << ", " << right << endl;
      cout << "  seqz " << reg << ", " << reg << endl;
      break;
    case KOOPA_RBO_NOT_EQ:  // neq
      cout << "  xor " << reg << ", " << left << ", " << right << endl;
      cout << "  snez " << reg << ", " << reg << endl;
      break;
    case KOOPA_RBO_LE:  // le
      cout << "  sgt " << reg << ", " << left << ", " << right << endl;
      cout << "  xori " << reg << ", " << reg << ", 1" << endl; 
      break;
    case KOOPA_RBO_GE:  // ge
      cout << "  slt " << reg << ", " << left << ", " << right << endl;
      cout << "  xori " << reg << ", " << reg << ", 1" << endl; 
      break;
    default:
      assert(false);
  }

  return reg;
}
