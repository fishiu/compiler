#include <ir.hpp>
#include <map>

int reg_cnt = 0;
std::map<const koopa_raw_value_t, std::string> vmap;

void gen_riscv(std::string koopa_str) {
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

  std::cout << "  .text" << std::endl;
  std::cout << "  .globl main" << std::endl;
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
  std::cout << func->name + 1 << ":" << std::endl;
  // visit all basic blocks
  Visit(func->bbs);
}

// visit basic block
void Visit(const koopa_raw_basic_block_t &bb) {
  printf("visit bb\n");
  Visit(bb->insts);
}

// visit value
std::string Visit(const koopa_raw_value_t &value) {
  if (vmap[value] != "") {
    return vmap[value];
  }

  printf("visit value\n");
  const auto &kind = value->kind;
  std::string reg;
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
  std::string reg = Visit(ret.value);
  std::cout << "  # ret" << std::endl;
  std::cout << "  mv a0, " << reg << std::endl;
  std::cout << "  ret" << std::endl;
}

// visit integer
std::string Visit(const koopa_raw_integer_t &integer) {
  printf("visit integer\n");
  if (integer.value == 0)
    return "x0";
  std::string reg = "t" + std::to_string(reg_cnt++);
  std::cout << "  # li integer" << std::endl;
  std::cout << "  " << "li " << reg << ", " << integer.value << std::endl;
  return reg;
}

// visit binary expression
std::string Visit(const koopa_raw_binary_t &binary) {
  printf("visit binary\n");
  koopa_raw_binary_op_t op = binary.op;
  std::string left = Visit(binary.lhs);
  std::string right = Visit(binary.rhs);
  std::string reg = "t" + std::to_string(reg_cnt++);

  switch (op) {
    case KOOPA_RBO_EQ:  // eq
      std::cout << "  # eq" << std::endl;
      std::cout << "  xor " << reg << ", " << left << ", " << right << std::endl;
      std::cout << "  seqz " << reg << ", " << left << std::endl;
      break;
    case KOOPA_RBO_SUB:  // sub
      std::cout << "  # sub" << std::endl;
      std::cout << "  sub " << reg << ", " << left << ", " << right << std::endl;
      break;
    default:
      assert(false);
  }

  return reg;
}
