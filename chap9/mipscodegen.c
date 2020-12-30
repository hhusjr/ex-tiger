#include "codegen.h"
#include "tree.h"
#include <stdint.h>

/*
 * MIPS tiles used for CG
 *
 * For easily extending to CSIC instructions, use
 * Dynamic Programming algorithm for instr selection
 */

typedef T_expList stmMatcher(T_stm, bool* matched);
typedef T_expList expMatcher(T_exp, bool* matched);

struct pattern_ {
    enum {
        PAT_STM, PAT_EXP
    } kind;
    union {
        stmMatcher* stm;
        expMatcher* exp;
    } matcher;
    int cost;
};
typedef struct pattern_ pattern;

static void findOptimalExp(T_exp exp);
static void findOptimalStm(T_stm stm);

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
static T_expList matchConst(T_exp exp, bool* matched);
static T_expList matchTemp(T_exp exp, bool* matched);
static T_expList matchExp(T_stm stm, bool* matched);

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
 * add rt, ra, zero                                             Move
 * lw rt, off(rs)           MEM(BINOP(PLUS,e1,CONST))                   1
 * lw rt, off(rs)           MEM(BINOP(PLUS,CONST,e1))                   1
 * lw rt, 0(rs)             MEM(e1)                                     1
 * add rt, rs, zero         MOVE(TEMP,e)                        Move    1
 * label:                   LABEL l1                            Label   1
 * sw rt, off(rs)           MOVE(MEM(BINOP(PLUS,e1,const)),e)           1
 * sw rt, off(rs)           MOVE(MEM(BINOP(PLUS,const,e1)),e)           1
 * sw rt, 0(rs)             MOVE(MEM(e1),e)                             1
 * addi rt, zero, imm       CONST(i)                                    1
 * /                        TEMP(t)                                     0
 * /                        EXP(e)                                      0
 */

pattern patterns[] = {
        {PAT_EXP, {.exp = matchBinopPlus}, 1},
        {PAT_EXP, {.exp = matchBinopPlusImmEC}, 1},
        {PAT_EXP, {.exp = matchBinopPlusImmCE}, 1},
        {PAT_EXP, {.exp = matchBinopMinus}, 1},
        {PAT_EXP, {.exp = matchBinopMinusImm}, 1},
        {PAT_EXP, {.exp = matchBinopDiv}, 1},
        {PAT_EXP, {.exp = matchBinopMul}, 1},
        {PAT_STM, {.stm = matchBinopCjumpEq}, 1},
        {PAT_STM, {.stm = matchBinopCjumpNe}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLt}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGe}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGt}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLe}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLt0}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGe0}, 1},
        {PAT_STM, {.stm = matchBinopCjumpGt0}, 1},
        {PAT_STM, {.stm = matchBinopCjumpLe0}, 1},
        {PAT_STM, {.stm = matchJump}, 1},
        {PAT_STM, {.stm = matchProcCall}, 1},
        {PAT_STM, {.stm = matchFuncCall}, 1},
        {PAT_EXP, {.exp = matchMemLoadOffsetEC}, 1},
        {PAT_EXP, {.exp = matchMemLoadOffsetCE}, 1},
        {PAT_EXP, {.exp = matchMemLoadExp}, 1},
        {PAT_STM, {.stm = matchMoveTemp}, 1},
        {PAT_STM, {.stm = matchLabel}, 1},
        {PAT_STM, {.stm = matchMemStoreOffsetEC}, 1},
        {PAT_STM, {.stm = matchMemStoreOffsetCE}, 1},
        {PAT_STM, {.stm = matchMemStoreExp}, 1},
        {PAT_EXP, {.exp = matchConst}, 1},
        {PAT_EXP, {.exp = matchTemp}, 0},
        {PAT_STM, {.stm = matchExp}, 0},
};

#define N_PATTERN (sizeof(patterns) / sizeof(pattern))

static T_expList matchBinopPlus(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
}

static T_expList matchBinopPlusImmEC(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus && exp->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, NULL);
    }
}

static T_expList matchBinopPlusImmCE(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus && exp->u.BINOP.left->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.right, NULL);
    }
}

static T_expList matchBinopMinus(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_minus) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
}

static T_expList matchBinopMinusImm(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_minus && exp->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, NULL);
    }
}

static T_expList matchBinopDiv(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_div) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
}

static T_expList matchBinopMul(T_exp exp, bool* matched) {
    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_mul) {
        *matched = TRUE;
        return T_ExpList(exp->u.BINOP.left, T_ExpList(exp->u.BINOP.right, NULL));
    }
}

static T_expList matchBinopCjumpEq(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_eq) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpNq(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_ne) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpLt(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_lt) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpGt(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_gt) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpLe(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_le) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpGe(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_ge) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpLt0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_lt && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpGt0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_gt && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpLe0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_le && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchBinopCjumpGe0(T_stm stm, bool* matched) {
    if (stm->kind == T_CJUMP && stm->u.CJUMP.op == T_ge && stm->u.CJUMP.right->kind == T_CONST && stm->u.CJUMP.right->u.CONST == 0) {
        *matched = TRUE;
        return T_ExpList(stm->u.CJUMP.left, T_ExpList(stm->u.CJUMP.right, NULL));
    }
}

static T_expList matchJump(T_stm stm, bool* matched) {
    if (stm->kind == T_JUMP && stm->u.JUMP.exp->kind == T_NAME) {
        *matched = TRUE;
        return NULL;
    }
}

static T_expList matchMemLoadOffsetEC(T_exp exp, bool* matched) {
    if (exp->kind == T_MEM && exp->u.MEM->kind == T_BINOP && exp->u.MEM->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.MEM->u.BINOP.left, NULL);
    }
}

static T_expList matchMemLoadOffsetCE(T_exp exp, bool* matched) {
    if (exp->kind == T_MEM && exp->u.MEM->kind == T_BINOP && exp->u.MEM->u.BINOP.left->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(exp->u.MEM->u.BINOP.right, NULL);
    }
}

static T_expList matchMemLoadExp(T_exp exp, bool* matched) {
    if (exp->kind == T_MEM) {
        *matched = TRUE;
        return T_ExpList(exp->u.MEM->u.BINOP.right, NULL);
    }
}

static T_expList matchMoveTemp(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_TEMP) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.src, NULL);
    }
}

static T_expList matchLabel(T_stm stm, bool* matched) {
    if (stm->kind == T_LABEL) {
        *matched = TRUE;
        return NULL;
    }
}

static T_expList matchMemStoreOffsetEC(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_MEM
        && stm->u.MOVE.dst->u.MEM->kind == T_BINOP && stm->u.MOVE.dst->u.MEM->u.BINOP.right->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.left, T_ExpList(stm->u.MOVE.src, NULL));
    }
}

static T_expList matchMemStoreOffsetCE(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_MEM
        && stm->u.MOVE.dst->u.MEM->kind == T_BINOP && stm->u.MOVE.dst->u.MEM->u.BINOP.left->kind == T_CONST) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.right, T_ExpList(stm->u.MOVE.src, NULL));
    }
}

static T_expList matchMemStoreExp(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_MEM) {
        *matched = TRUE;
        return T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.left,
                T_ExpList(stm->u.MOVE.dst->u.MEM->u.BINOP.right,
                        T_ExpList(stm->u.MOVE.src, NULL)));
    }
}

static T_expList matchConst(T_exp exp, bool* matched) {
    if (exp->kind == T_CONST) {
        *matched = TRUE;
        return NULL;
    }
}

static T_expList matchTemp(T_exp exp, bool* matched) {
    if (exp->kind == T_TEMP) {
        *matched = TRUE;
        return NULL;
    }
}

static T_expList matchExp(T_stm stm, bool* matched) {
    if (stm->kind == T_EXP) {
        *matched = TRUE;
        return T_ExpList(stm->u.EXP, NULL);
    }
}

static T_expList matchProcCall(T_stm stm, bool* matched) {
    if (stm->kind == T_EXP && stm->u.EXP->kind == T_CALL) {
        *matched = TRUE;
        return stm->u.EXP->u.CALL.args;
    }
}

static T_expList matchFuncCall(T_stm stm, bool* matched) {
    if (stm->kind == T_MOVE && stm->u.MOVE.dst->kind == T_TEMP && stm->u.MOVE.src->kind == T_CALL) {
        *matched = TRUE;
        return stm->u.EXP->u.CALL.args;
    }
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

        int total_cost = 0;
        bool matched = FALSE;
        for (T_expList exps = patterns[i].matcher.exp(exp, &matched); exps; exps = exps->tail) {
            if (matched) {
                total_cost += exps->head->cost;
                matched = FALSE;
            }
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
            assert(stm->u.MOVE.src->kind == T_TEMP || stm->u.MOVE.src->kind == T_MEM);
            findOptimalExp(stm->u.MOVE.dst);
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

        int total_cost = 0;
        bool matched = FALSE;
        for (T_expList exps = patterns[i].matcher.stm(stm, &matched); exps; exps = exps->tail) {
            if (matched) {
                total_cost += exps->head->cost;
                matched = FALSE;
            }
        }
        total_cost += patterns[i].cost;

        if (stm->cost == -1 || total_cost < stm->cost) {
            stm->cost = total_cost;
            stm->selection = i;
        }
    }
}