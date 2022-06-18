#ifndef IR_H
#define IR_H

#include <string>
#include <cassert>
#include <iostream>
#include <koopa.h>
#define REGNUM 15

// a struct to save the return of a koopa value
typedef struct {
  bool is_reg;
  int addr;
} repr_t;

// register struct
typedef struct {
  int regid;
  bool occupied;
  // koopa_raw_value_t target;
} reg_t;

void gen_riscv(std::string koopa_str);
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
// return reg name
repr_t Visit(const koopa_raw_value_t &value);
repr_t Visit(const koopa_raw_integer_t &integer);
repr_t Visit(const koopa_raw_binary_t &binary);
repr_t Visit(const koopa_raw_load_t &load);

#endif