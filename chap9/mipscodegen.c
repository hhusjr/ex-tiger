#include "codegen.h"
#include "tree.h"
#include "assem.h"
#include <stdint.h>

/*
 * MIPS tiles used for CG
 *
 * For easily extending to CSIC instructions, use
 * Dynamic Programming algorithm for instr selection
 */

typedef T_expList stmMatcher(T_stm, bool* matched);
typedef T_expList expMatcher(T_exp, bool* matched);
typedef void stmGen(T_stm);
typedef Temp_temp expGen(T_exp);

static AS_instrList instrs = NULL, instrs_tail = NULL;

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

static void emit();

static void findOptimalExp(T_exp exp);
static void findOptimalStm(T_stm stm);
static Temp_temp doExp(T_exp exp);
static void doStm(T_stm stm);

static T_expList matchBinopPlus(T_exp exp, bool* matched);
static T_expList matchBinopPlusImmEC(T_exp exp, bool* matched);
static T_expList matchBinopPlusImmCE(T_exp exp, bool* matched);
static T_expList matchBinopMinus(T_exp exp, bool* matched);
static T_expList matchBinopMinusImm(T_exp exp, bool* matched);
static T_expList matchBinopDiv(T_exp exp, bool* matched);
static T_expList matchBinopMul(T_exp exp, bool* matched);
static T_expList matchBinopCjumpEq(T_stm stm, bool* matched);
static T_expList matchBinopCjumpNe(T_stm stm, bool* matched);
static T_expList matchBinopCjumpLt(T_stm stm, bool* matched);
static T_expList matchBinopCjumpGe(T_stm stm, bool* matched);
static T_expList matchBinopCjumpGt(T_stm stm, bool* matched);
static T_expList matchBinopCjumpLe(T_stm stm, bool* matched);
static T_expList matchBinopCjumpLt0(T_stm stm, bool* matched);
static T_expList matchBinopCjumpGe0(T_stm stm, bool* matched);
static T_expList matchBinopCjumpGt0(T_stm stm, bool* matched);
static T_expList matchBinopCjumpLe0(T_stm stm, bool* matched);
static T_expList matchJump(T_stm stm, bool* matched);
static T_expList matchProcCall(T_stm stm, bool* matched);
static T_expList matchFuncCall(T_stm stm, bool* matched);
static T_expList matchMemLoadOffsetEC(T_exp exp, bool* matched);
static T_expList matchMemLoadOffsetCE(T_exp exp, bool* matched);
static T_expList matchMemLoadExp(T_exp exp, bool* matched);
static T_expList matchMoveTemp(T_stm stm, bool* matched);
static T_expList matchLabel(T_stm stm, bool* matched);
static T_expList matchMemStoreOffsetEC(T_stm stm, bool* matched);
static T_expList matchMemStoreOffsetCE(T_stm stm, bool* matched);
static T_expList matchMemStoreExp(T_stm stm, bool* matched);
static T_expList matchCall(T_exp exp, bool* matched);
static T_expList matchConst(T_exp exp, bool* matched);
static T_expList matchTemp(T_exp exp, bool* matched);
static T_expList matchExp(T_stm stm, bool* matched);

static Temp_temp genBinopPlus(T_exp exp);
static Temp_temp genBinopPlusImmEC(T_exp exp);
static Temp_temp genBinopPlusImmCE(T_exp exp);
static Temp_temp genBinopMinus(T_exp exp);
static Temp_temp genBinopMinusImm(T_exp exp);
static Temp_temp genBinopDiv(T_exp exp);
static Temp_temp genBinopMul(T_exp exp);
static void genBinopCjumpEq(T_stm stm);
static void genBinopCjumpNe(T_stm stm);
static void genBinopCjumpLt(T_stm stm);
static void genBinopCjumpGe(T_stm stm);
static void genBinopCjumpGt(T_stm stm);
static void genBinopCjumpLe(T_stm stm);
static void genBinopCjumpLt0(T_stm stm);
static void genBinopCjumpGe0(T_stm stm);
static void genBinopCjumpGt0(T_stm stm);
static void genBinopCjumpLe0(T_stm stm);
static void genJump(T_stm stm);
static void genProcCall(T_stm stm);
static void genFuncCall(T_stm stm);
static Temp_temp genMemLoadOffsetEC(T_exp exp);
static Temp_temp genMemLoadOffsetCE(T_exp exp);
static Temp_temp genMemLoadExp(T_exp exp);
static void genMoveTemp(T_stm stm);
static void genLabel(T_stm stm);
static void genMemStoreOffsetEC(T_stm stm);
static void genMemStoreOffsetCE(T_stm stm);
static void genMemStoreExp(T_stm stm);
static Temp_temp genCall(T_exp exp);
static Temp_temp genConst(T_exp exp);
static Temp_temp genTemp(T_exp exp);
static void genExp(T_stm stm);

/*
 * Pattern definitions
 *
 * MIPS Instr               Tiger IR                                    Cost
 * add rd, rs, rt           BINOP(PLUS,e1,e2)                           1
 * addi rt, rs, imm         BINOP(PLUS,e1,const)                        1
 * addi rt, rs, imm         BINOP(PLUS,const,e1)                        1
 * sub rd, rs, rt           BINOP(MINUS,e1,e2)                          1
 * subi rt, rs, imm         BINOP(MINUS,e1,const)                       1
 * div rs, rt               BINOP(DIV,e1,e2)                            1
 * mult rs, rt              BINOP(MUL,e1,e2)                            1
 * beq rs, rt, label        CJUMP(EQ,e1,e2,NAME l1)                     1
 * bne rs, rt, label        CJUMP(NE,e1,e2,NAME l1)                     1
 * slt at, rs, rt           CJUMP(LT,e1,e2,NAME l1)                     1
 * bne at, zero, label
 * slt at, rs, rt           CJUMP(GE,e1,e2,NAME l1)                     1
 * beq at, zero, label
 * slt at, rt, rs           CJUMP(GT,e1,e2,NAME l1)                     1
 * bne at, zero, label
 * slt at, rt, rs           CJUMP(LE,e1,e2,NAME l1)                     1
 * beq at, zero, label
 * bgez rs, label           CJUMP(GE,e1,const 0,NAME l1)                1
 * bgtz rs, label           CJUMP(GT,e1,const 0,NAME l1)                1
 * blez rs, label           CJUMP(LE,e1,const 0,NAME l1)                1
 * bltz rs, label           CJUMP(LT,e1,const 0,NAME l1)                1
 * j label                  JUMP(NAME l1)                               1
 * (... Save args ...)      EXP(CALL(NAME l1))                          1
 * jal label
 * (... Save args ...)      MOVE(TEMP t,CALL(NAME l1))                  1
 * jal label
 * add rt, ra, zero
 * lw rt, off(rs)           MEM(BINOP(PLUS,e1,CONST))                   1
 * lw rt, off(rs)           MEM(BINOP(PLUS,CONST,e1))                   1
 * lw rt, 0(rs)             MEM(e1)                                     1
 * add rt, rs, zero         MOVE(TEMP,e)                        Move    1
 * label:                   LABEL l1                            Label   1
 * sw rt, off(rs)           MOVE(MEM(BINOP(PLUS,e1,const)),e)           1
 * sw rt, off(rs)           MOVE(MEM(BINOP(PLUS,const,e1)),e)           1
 * sw rt, 0(rs)             MOVE(MEM(e1),e)                             1
 * addi rt, zero, imm       CONST(i)                                    1
 * (x)                      CALL()                                      1
 * /                        TEMP(t)                                     0
 * /                        EXP(e)                                      0
 */

pattern patterns[] = {
        {PAT_EXP, {.exp = matchBinopPlus}, {.exp = genBinopPlus}, 1},
        {PAT_EXP, {.exp = matchBinopPlusImmEC}, {.exp = genBinopPlusImmEC}, 1},
        {PAT_EXP, {.exp = matchBinopPlusImmCE}, {.exp = genBinopPlusImmCE}, 1},
        {PAT_EXP, {.exp = matchBinopMinus}, {.exp = genBinopMinus}, 1},
        {PAT_EXP, {.exp = matchBinopMinusImm}, {.exp = genBinopMinusImm}, 1},
        {PAT_EXP, {.exp = matchBinopDiv}, {.exp = genBinopDiv}, 1},
        {PAT_EXP, {.exp = matchBinopMul}, {.exp = genBinopMul}, 1},
        {PAT_STM, {.stm = matchBinopCjumpEq}, {.stm = genBinopCjumpEq}, 1},
        {PAT_STM, {.stm = matchBinopCjumpNe}, {.stm = genBinopCjumpNe}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLt}, {.stm = genBinopCjumpLt}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGe}, {.stm = genBinopCjumpGe}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGt}, {.stm = genBinopCjumpGt}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLe}, {.stm = genBinopCjumpLe}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLt0}, {.stm = genBinopCjumpLt0}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGe0}, {.stm = genBinopCjumpGe0}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGt0}, {.stm = genBinopCjumpGt0}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLe0}, {.stm = genBinopCjumpLe0}, 1},
        {PAT_STM, {.stm = matchJump}, {.stm = genJump}, 1},
        {PAT_STM, {.stm = matchProcCall}, {.stm = genProcCall}, 1},
        {PAT_STM, {.stm = matchFuncCall}, {.stm = genFuncCall}, 1},
        {PAT_EXP, {.exp = matchMemLoadOffsetEC}, {.exp = genMemLoadOffsetEC}, 1},
        {PAT_EXP, {.exp = matchMemLoadOffsetCE}, {.exp = genMemLoadOffsetCE}, 1},
        {PAT_EXP, {.exp = matchMemLoadExp}, {.exp = genMemLoadExp}, 1},
        {PAT_STM, {.stm = matchMoveTemp}, {.stm = genMoveTemp}, 1},
        {PAT_STM, {.stm = matchLabel}, {.stm = genLabel}, 1},
        {PAT_STM, {.stm = matchMemStoreOffsetEC}, {.stm = genMemStoreOffsetEC}, 1},
        {PAT_STM, {.stm = matchMemStoreOffsetCE}, {.stm = genMemStoreOffsetCE}, 1},
        {PAT_STM, {.stm = matchMemStoreExp}, {.stm = genMemStoreExp}, 1},
        {PAT_EXP, {.exp = matchCall}, {.exp = genCall}, 1},
        {PAT_EXP, {.exp = matchConst}, {.exp = genConst}, 1},
        {PAT_EXP, {.exp = matchTemp}, {.exp = genTemp}, 0},
        {PAT_STM, {.stm = matchExp}, {.stm = genExp}, 0},
};

#define N_PATTERN (sizeof(patterns) / sizeof(pattern))

void emit(AS_instr inst) {
    if (instrs == NULL) {
        instrs = instrs_tail = AS_InstrList(inst, NULL);
    } else {
        instrs_tail->tail = AS_InstrList(inst, NULL);
        instrs_tail = instrs_tail->tail;
    }
}

static T_expList matchBinopPlus(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopPlusImmEC(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus && exp->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, NULL);
    }
    return NULL;
}

static T_expList matchBinopPlusImmCE(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus && exp->u.BINOP.left->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.right, NULL);
    }
    return NULL;
}

static T_expList matchBinopMinus(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_minus) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopMinusImm(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_minus && exp->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, NULL);
    }
    return NULL;
}

static T_expList matchBinopDiv(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_div) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopMul(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_mul) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpEq(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_eq) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpNe(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_ne) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpLt(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_lt) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpGt(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_gt) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpLe(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_le) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpGe(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_ge) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpLt0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_lt && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpGt0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_gt && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpLe0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_le && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchBinopCjumpGe0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_ge && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
    return NULL;
}

static T_expList matchJump(T_stm stm, bool* matched) {
    if (stm->kind == T_JUMP && stm->u.JUMP.exp->kind == T_NAME) {
        *matched = TRUE;
        return NULL;
    }
    return NULL;
}

static T_expList matchMemLoadOffsetEC(T_exp exp, bool* matched) {
    if (exp->kind == T_MEM && exp->u.MEM->kind == T_BINOP && exp->u.MEM->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.MEM->u.BINOP.left, NULL);
    }
    return NULL;
}

static T_expList matchMemLoadOffsetCE(T_exp exp, bool* matched) {
    if (exp->kind == T_MEM && exp->u.MEM->kind == T_BINOP && exp->u.MEM->u.BINOP.left->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.MEM->u.BINOP.right, NULL);
    }
    return NULL;
}

static T_expList matchMemLoadExp(T_exp exp, bool* matched) {
    if (exp->kind == T_MEM) {
        *matched = TRUE;
        return T_ExpList(exp->u.MEM->u.BINOP.right, NULL);
    }
    return NULL;
}

static T_expList matchMoveTemp(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_TEMP) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.src, NULL);
    }
    return NULL;
}

static T_expList matchLabel(T_stm stm, bool* matched) {
    if (stm->kind == T_LABEL) {
        *matched = TRUE;
        return NULL;
    }
    return NULL;
}

static T_expList matchMemStoreOffsetEC(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_MEM
        && stm->u.MOVE.dst->u.MEM->kind == T_BINOP && stm->u.MOVE.dst->u.MEM->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.left, T_ExpList(stm->u.MOVE.src, NULL));
    }
    return NULL;
}

static T_expList matchMemStoreOffsetCE(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_MEM
        && stm->u.MOVE.dst->u.MEM->kind == T_BINOP && stm->u.MOVE.dst->u.MEM->u.BINOP.left->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.right, T_ExpList(stm->u.MOVE.src, NULL));
    }
    return NULL;
}

static T_expList matchMemStoreExp(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_MEM) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.left,
                T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.right,
                        T_ExpList(stm->u.MOVE.src, NULL)));
    }
    return NULL;
}

static T_expList matchConst(T_exp exp, bool* matched) {
    if (exp->kind == T_CONST) {
        *matched = TRUE;
        return NULL;
    }
    return NULL;
}

static T_expList matchTemp(T_exp exp, bool* matched) {
    if (exp->kind == T_TEMP) {
        *matched = TRUE;
        return NULL;
    }
    return NULL;
}

static T_expList matchExp(T_stm stm, bool* matched) {
    if (stm->kind == T_EXP) {
        *matched = TRUE;
        return T_ExpList(stm->u.EXP, NULL);
    }
    return NULL;
}

static T_expList matchProcCall(T_stm stm, bool* matched) {
    if (stm->kind == T_EXP && stm->u.EXP->kind == T_CALL) {
        *matched = TRUE;
        return stm->u.EXP->u.CALL.args;
    }
    return NULL;
}

static T_expList matchFuncCall(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_TEMP && stm->u.MOVE.src->kind == T_CALL) {
        *matched = TRUE;
        return stm->u.EXP->u.CALL.args;
    }
    return NULL;
}

static T_expList matchCall(T_exp exp, bool* matched) {
    if (exp->kind == T_CALL) {
        *matched = TRUE;
        return NULL;
    }
    return NULL;
}

/*
 * Use dynamic programming to find optimum coverage method
 */

static void findOptimalExp(T_exp exp) {
    exp->cost = -1;
    exp->selection = -1;

    // Bottom up DP
    switch (exp->kind) {
        case T_BINOP:
            findOptimalExp(exp->u.BINOP.left);
            findOptimalExp(exp->u.BINOP.right);
            break;
        case T_MEM:
            findOptimalExp(exp->u.MEM);
            break;
        case T_ESEQ:
            // No ESEQ must exist in this pass
            assert(0);
            break;
        case T_CALL:
            // Now only support named function call
            assert(exp->u.CALL.fun->kind == T_NAME);
            break;

        default: ;
    }

    for (int i = 0; i < N_PATTERN; i++) {
        if (patterns[i].kind != PAT_EXP) {
            continue;
        }

        bool matched = FALSE;
        T_expList res = patterns[i].matcher.exp(exp, &matched);
        if (!matched) {
            continue;
        }

        int total_cost = 0;
        for (T_expList exps = res; exps; exps = exps->tail) {
            total_cost += exps->head->cost;
        }
        total_cost += patterns[i].cost;

        if (exp->cost == -1 || total_cost < exp->cost) {
            exp->cost = total_cost;
            exp->selection = i;
        }
    }
}

static void findOptimalStm(T_stm stm) {
    stm->cost = -1;
    stm->selection = -1;

    // Bottom up DP
    switch (stm->kind) {
        case T_SEQ:
            // No SEQ must exist in this pass
            assert(0);
            break;
        case T_JUMP:
            // Now only support named jump
            assert(stm->u.JUMP.exp->kind == T_NAME);
            break;
        case T_CJUMP:
            findOptimalExp(stm->u.CJUMP.left);
            findOptimalExp(stm->u.CJUMP.right);
            break;
        case T_MOVE:
            assert(stm->u.MOVE.dst->kind == T_TEMP || stm->u.MOVE.dst->kind == T_MEM);
            findOptimalExp(stm->u.MOVE.src);
            break;
        case T_EXP:
            findOptimalExp(stm->u.EXP);
            break;

        default: ;
    }

    for (int i = 0; i < N_PATTERN; i++) {
        if (patterns[i].kind != PAT_STM) {
            continue;
        }

        bool matched = FALSE;
        T_expList res = patterns[i].matcher.stm(stm, &matched);
        if (!matched) {
            continue;
        }

        int total_cost = 0;
        for (T_expList exps = res; exps; exps = exps->tail) {
            total_cost += exps->head->cost;
        }
        total_cost += patterns[i].cost;

        if (stm->cost == -1 || total_cost < stm->cost) {
            stm->cost = total_cost;
            stm->selection = i;
        }
    }
}

static Temp_temp doExp(T_exp exp) {
    assert(exp->selection != -1);
    return patterns[exp->selection].gen.exp(exp);
}

static void doStm(T_stm stm) {
    assert(stm->selection != -1);
    patterns[stm->selection].gen.stm(stm);
}

Temp_tempList L(Temp_temp h, Temp_tempList t) {
    return Temp_TempList(h, t);
}

char cbuf[1024];

static Temp_temp genBinopPlus(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    emit(AS_Oper("add `d0, `s0, `s1\n", L(r, NULL), L(doExp(exp->u.BINOP.left), L(doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static Temp_temp genBinopPlusImmEC(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    sprintf(cbuf, "addi `d0, `s0, %d\n", exp->u.BINOP.right->u.CONST);
    string i = String(cbuf);
    emit(AS_Oper(i, L(r, NULL), L(doExp(exp->u.BINOP.left), NULL), NULL));
    return r;
}

static Temp_temp genBinopPlusImmCE(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    sprintf(cbuf, "addi `d0, `s0, %d\n", exp->u.BINOP.left->u.CONST);
    string i = String(cbuf);
    emit(AS_Oper(i, L(r, NULL), L(doExp(exp->u.BINOP.right), NULL), NULL));
    return r;
}

static Temp_temp genBinopMinus(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    emit(AS_Oper("sub `d0, `s0, `s1\n", L(r, NULL), L(doExp(exp->u.BINOP.left), L(doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static Temp_temp genBinopMinusImm(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    sprintf(cbuf, "subi `d0, `s0, %d\n", exp->u.BINOP.right->u.CONST);
    string i = String(cbuf);
    emit(AS_Oper(i, L(r, NULL), L(doExp(exp->u.BINOP.left), NULL), NULL));
    return r;
}

static Temp_temp genBinopDiv(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    emit(AS_Oper("div `s0, `s1\nmflo `d0\n", L(r, NULL), L(doExp(exp->u.BINOP.left), L(doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static Temp_temp genBinopMul(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    emit(AS_Oper("mult `s0, `s1\nmflo `d0\n", L(r, NULL), L(doExp(exp->u.BINOP.left), L(doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static void genBinopCjumpEq(T_stm stm) {
    sprintf(cbuf, "beq `s0, `s1, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.CJUMP.left), L(doExp(stm->u.CJUMP.right), NULL)),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpNe(T_stm stm) {
    sprintf(cbuf, "bne `s0, `s1, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.CJUMP.left), L(doExp(stm->u.CJUMP.right), NULL)),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLt(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s0, `s1\nbne `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, L(F_AT(), NULL), L(doExp(stm->u.CJUMP.left), L(doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGe(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s0, `s1\nbeq `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, L(F_AT(), NULL), L(doExp(stm->u.CJUMP.left), L(doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGt(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s1, `s0\nbne `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, L(F_AT(), NULL), L(doExp(stm->u.CJUMP.left), L(doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLe(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s1, `s0\nbeq `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, L(F_AT(), NULL), L(doExp(stm->u.CJUMP.left), L(doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLt0(T_stm stm) {
    sprintf(cbuf, "bltz `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.CJUMP.left), NULL),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLe0(T_stm stm) {
    sprintf(cbuf, "blez `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.CJUMP.left), NULL),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGt0(T_stm stm) {
    sprintf(cbuf, "bgtz `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.CJUMP.left), NULL),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGe0(T_stm stm) {
    sprintf(cbuf, "bgez `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.CJUMP.left), NULL),
                 AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genJump(T_stm stm) {
    sprintf(cbuf, "j %s\n", stm->u.JUMP.exp->u.NAME->name);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, NULL, AS_Targets(Temp_LabelList(stm->u.JUMP.exp->u.NAME, NULL))));
}

static void genProcCall(T_stm stm) {
}

static void genFuncCall(T_stm stm) {
}

static Temp_temp genMemLoadOffsetEC(T_exp exp) {
    sprintf(cbuf, "lw `d0, %d(`s0)\n", exp->u.MEM->u.BINOP.right->u.CONST);
    Temp_temp r = Temp_newtemp();
    string i = String(cbuf);
    emit(AS_Oper(i, L(r, NULL), L(doExp(exp->u.MEM->u.BINOP.left), NULL), NULL));
    return r;
}

static Temp_temp genMemLoadOffsetCE(T_exp exp) {
    sprintf(cbuf, "lw `d0, %d(`s0)\n", exp->u.MEM->u.BINOP.left->u.CONST);
    Temp_temp r = Temp_newtemp();
    string i = String(cbuf);
    emit(AS_Oper(i, L(r, NULL), L(doExp(exp->u.MEM->u.BINOP.right), NULL), NULL));
    return r;
}

static Temp_temp genMemLoadExp(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    emit(AS_Oper("lw `d0, 0(`s0)\n", L(r, NULL), L(doExp(exp->u.MEM), NULL), NULL));
    return r;
}

static void genMoveTemp(T_stm stm) {
    emit(AS_Oper("add `d0, `s0, `s1\n", L(stm->u.MOVE.dst->u.TEMP, NULL), L(doExp(stm->u.MOVE.src), L(F_ZERO(), NULL)), NULL));
}

static void genLabel(T_stm stm) {
    sprintf(cbuf, "%s:\n", stm->u.LABEL->name);
    string i = String(cbuf);
    emit(AS_Label(i, stm->u.LABEL));
}

static void genMemStoreOffsetEC(T_stm stm) {
    sprintf(cbuf, "sw `s1, %d(`s0)\n", stm->u.MOVE.dst->u.BINOP.right->u.CONST);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.MOVE.dst->u.BINOP.left), L(doExp(stm->u.MOVE.src), NULL)), NULL));
}

static void genMemStoreOffsetCE(T_stm stm) {
    sprintf(cbuf, "sw `s1, %d(`s0)\n", stm->u.MOVE.dst->u.BINOP.left->u.CONST);
    string i = String(cbuf);
    emit(AS_Oper(i, NULL, L(doExp(stm->u.MOVE.dst->u.BINOP.right), L(doExp(stm->u.MOVE.src), NULL)), NULL));
}

static void genMemStoreExp(T_stm stm) {
    emit(AS_Oper("sw `s1, 0(`s0)\n", NULL, L(doExp(stm->u.MOVE.dst), L(doExp(stm->u.MOVE.src), NULL)), NULL));
}

static Temp_temp genConst(T_exp exp) {
    sprintf(cbuf, "addi `d0, `s0, %d\n", exp->u.CONST);
    Temp_temp r = Temp_newtemp();
    string i = String(cbuf);
    emit(AS_Oper(i, L(r, NULL), L(F_ZERO(), NULL), NULL));
    return r;
}

static Temp_temp genTemp(T_exp exp) {
    return exp->u.TEMP;
}

static void genExp(T_stm stm) {
    doExp(stm->u.EXP);
}

static Temp_temp genCall(T_exp exp) {
    assert(0);
}

AS_instrList F_codegen(F_frame f, T_stmList stmts) {
    instrs = instrs_tail = NULL;
    for (; stmts; stmts = stmts->tail) {
        findOptimalStm(stmts->head);
        doStm(stmts->head);
    }
    return instrs;
}