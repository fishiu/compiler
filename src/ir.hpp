#ifndef IR_H
#define IR_H

#include <string>
#include <cassert>
#include <iostream>
#include <koopa.h>

void gen_riscv(std::string koopa_str);
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_return_t &ret);
// return reg name
std::string Visit(const koopa_raw_value_t &value);
std::string Visit(const koopa_raw_integer_t &integer);
std::string Visit(const koopa_raw_binary_t &binary);

#endif