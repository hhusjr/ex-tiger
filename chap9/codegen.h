#ifndef TIGER_CODEGEN
#define TIGER_CODEGEN

#include "assem.h"
#include "frame.h"

typedef T_expList stmMatcher(T_stm, bool* matched);
typedef T_expList expMatcher(T_exp, bool* matched);
typedef void stmGen(T_stm);
typedef Temp_temp expGen(T_exp);
struct pattern_ {
    enum {
        PAT_STM, PAT_EXP
    } kind;
    union {
        stmMatcher* stm;
        expMatcher* exp;
    } matcher;
    union {
        stmGen* stm;
        expGen* exp;
    } gen;
    int cost;
};
typedef struct pattern_ pattern;

// For code generators
void emit();
Temp_temp doExp(T_exp exp);
void doStm(T_stm stm);

AS_instrList F_codegen(F_frame f, T_stmList stmts);

#endif //TIGER_CODEGEN
