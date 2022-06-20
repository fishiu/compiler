# SysY Compiler

PKU Compiler Principles 2022 Spring Course Project

## TODOS

1. 如何用 enum 处理
2. Dump 如何保持const
3. 抽象出 ExpBaseAST 是否合适
4. addr 的计算方式是否 ok
5. 全局函数表
6. 重名问题：全局标识符不可以重名
7. 发现一些冗余：如果不用指针赋值的话，其实没必要用move

## 一些很愚蠢的错误
1. 逻辑运算和位运算的关系
2. 忘记把new tmp var赋值给addr
3. stack不能用层数来给变量命名
4. 去除最后一行：redir还是while parsing
5. 哪些情况需要去除最后一行？
6. string 的内存管理

我的一些想法
- 数据结构的抽象程度，比如WhileStack
- 捋清楚 Eval 和 Dump 的关系
- ExpBaseAST 是否有意义
- is_reg 到底好不好
- C++内存管理
- 编程风格：比如是否要多个构造函数？