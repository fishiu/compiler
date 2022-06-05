#include <ir.hpp>


void gen_riscv (std::string koopa_str) {
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
  // remove @
  std::string func_name = std::string(func->name);
  func_name = func_name.substr(1);
  
  std::cout << func_name << ":" << std::endl;
  // visit all basic blocks
  Visit(func->bbs);
}

// visit basic block
void Visit(const koopa_raw_basic_block_t &bb) {
  Visit(bb->insts);
}

// visit value
void Visit(const koopa_raw_value_t &value) {
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}


// visit return
void Visit(const koopa_raw_return_t &ret) {
  std::cout << "  li a0, ";
  Visit(ret.value);

  std::cout << "  ret" << std::endl;
}


void Visit(const koopa_raw_integer_t &integer) {
  std::cout << integer.value << std::endl;
}


void Visit(const koopa_raw_aggregate_t &agg) {
  assert(false);
}
