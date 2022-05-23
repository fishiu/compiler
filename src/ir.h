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
void Visit(const koopa_raw_value_t &value);

void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_aggregate_t &agg);

#endif