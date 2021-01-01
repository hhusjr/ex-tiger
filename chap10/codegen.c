#include "codegen.h"
#include "tree.h"
#include "assem.h"
#include <stdint.h>

/*
 * Code generator framework
 *
 * For easily extending to CSIC instructions, use
 * Dynamic Programming algorithm for instr selection
 *
 * For code generation, you must define <match, gen, cost> pair
 * and store all of them in an array "F_patterns[]"
 */

static AS_instrList instrs = NULL, instrs_tail = NULL;

static void findOptimalExp(T_exp exp);
static void findOptimalStm(T_stm stm);

extern F_pattern F_patterns[];
extern int F_nPattern;

void F_emit(AS_instr inst) {
    if (instrs == NULL) {
        instrs = instrs_tail = AS_InstrList(inst, NULL);
    } else {
        instrs_tail->tail = AS_InstrList(inst, NULL);
        instrs_tail = instrs_tail->tail;
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
            findOptimalExp(exp->u.CALL.fun);
            for (T_expList args = exp->u.CALL.args; args; args = args->tail) {
                findOptimalExp(args->head);
            }
            break;

        default: ;
    }

    for (int i = 0; i < F_nPattern; i++) {
        if (F_patterns[i].kind != PAT_EXP) {
            continue;
        }

        bool matched = FALSE;
        T_expList res = F_patterns[i].matcher.exp(exp, &matched);
        if (!matched) {
            continue;
        }

        int total_cost = 0;
        for (T_expList exps = res; exps; exps = exps->tail) {
            total_cost += exps->head->cost;
        }
        total_cost += F_patterns[i].cost;

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
            findOptimalExp(stm->u.JUMP.exp);
            break;
        case T_CJUMP:
            findOptimalExp(stm->u.CJUMP.left);
            findOptimalExp(stm->u.CJUMP.right);
            break;
        case T_MOVE:
            // Tree IR only supports MOVE(TEMP, e) or MOVE(MEM(e'), e)
            assert(stm->u.MOVE.dst->kind == T_TEMP || stm->u.MOVE.dst->kind == T_MEM);
            findOptimalExp(stm->u.MOVE.src);
            break;
        case T_EXP:
            findOptimalExp(stm->u.EXP);
            break;

        default: ;
    }

    for (int i = 0; i < F_nPattern; i++) {
        if (F_patterns[i].kind != PAT_STM) {
            continue;
        }

        bool matched = FALSE;
        T_expList res = F_patterns[i].matcher.stm(stm, &matched);
        if (!matched) {
            continue;
        }

        int total_cost = 0;
        for (T_expList exps = res; exps; exps = exps->tail) {
            total_cost += exps->head->cost;
        }
        total_cost += F_patterns[i].cost;

        if (stm->cost == -1 || total_cost < stm->cost) {
            stm->cost = total_cost;
            stm->selection = i;
        }
    }
}

Temp_temp F_doExp(T_exp exp) {
    assert(exp->selection != -1);
    return F_patterns[exp->selection].gen.exp(exp);
}

void F_doStm(T_stm stm) {
    assert(stm->selection != -1);
    F_patterns[stm->selection].gen.stm(stm);
}

AS_instrList F_codegen(F_frame f, T_stmList stmts) {
    instrs = instrs_tail = NULL;
    for (; stmts; stmts = stmts->tail) {
        findOptimalStm(stmts->head);
        F_doStm(stmts->head);
    }
    return F_procEntryExit2(instrs);
}