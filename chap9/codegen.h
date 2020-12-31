#ifndef TIGER_CODEGEN
#define TIGER_CODEGEN

#include "assem.h"
#include "frame.h"

typedef T_expList F_stmMatcher(T_stm, bool* matched);
typedef T_expList F_expMatcher(T_exp, bool* matched);
typedef void F_stmGen(T_stm);
typedef Temp_temp F_expGen(T_exp);
struct F_pattern_ {
    enum {
        PAT_STM, PAT_EXP
    } kind;
    union {
        F_stmMatcher* stm;
        F_expMatcher* exp;
    } matcher;
    union {
        F_stmGen* stm;
        F_expGen* exp;
    } gen;
    int cost;
};
typedef struct F_pattern_ F_pattern;

// For code generators
void F_emit(AS_instr inst);
Temp_temp F_doExp(T_exp exp);
void F_doStm(T_stm stm);

AS_instrList F_codegen(F_frame f, T_stmList stmts);

#endif //TIGER_CODEGEN
