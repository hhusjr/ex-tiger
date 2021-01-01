#include "codegen.h"
#include "tree.h"

/*
 * MIPS code generator
 * Pattern definitions
 *
 * MIPS Instr               Tiger IR                                    Cost
 *
 * addi rt, zero, imm       CONST(i)                                    1
 * (... save args ...)      CALL(e,args)                                1
 * jalr rs, ra
 * (... save args ...)      CALL(NAME l, args)                          1
 * jal label
 * la rd, label             NAME(l)                                     1
 * /                        TEMP(t)                                     0
 * /                        EXP(e)                                      0
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
 * jr rs                    JUMP(e)                                     1
 * j label                  JUMP(NAME l1)                               1
 * lw rt, off(rs)           MEM(BINOP(PLUS,e1,CONST))                   1
 * lw rt, off(rs)           MEM(BINOP(PLUS,CONST,e1))                   1
 * lw rt, 0(rs)             MEM(e1)                                     1
 * add rt, rs, zero         MOVE(TEMP,e)                        Move    1
 * label:                   LABEL l1                            Label   1
 * sw rt, off(rs)           MOVE(MEM(BINOP(PLUS,e1,const)),e)           1
 * sw rt, off(rs)           MOVE(MEM(BINOP(PLUS,const,e1)),e)           1
 * sw rt, 0(rs)             MOVE(MEM(e1),e)                             1
 */

static T_expList matchConst(T_exp exp, bool* matched) {
    if (exp->kind == T_CONST) {
        *matched = TRUE;
        return NULL;
    }
    return NULL;
}

static T_expList matchCall(T_exp exp, bool* matched) {
    if (exp->kind == T_CALL) {
        *matched = TRUE;
        return T_ExpList(exp->u.CALL.fun, exp->u.CALL.args);
    }
    return NULL;
}

static T_expList matchNamedCall(T_exp exp, bool* matched) {
    if (exp->kind == T_CALL && exp->u.CALL.fun->kind == T_NAME) {
        *matched = TRUE;
        return exp->u.CALL.args;
    }
    return NULL;
}

static T_expList matchName(T_exp exp, bool* matched) {
    if (exp->kind == T_NAME) {
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
    if (stm->kind == T_JUMP) {
        *matched = TRUE;
        return T_ExpList(stm->u.JUMP.exp, NULL);
    }
    return NULL;
}

static T_expList matchNamedJump(T_stm stm, bool* matched) {
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
        return T_ExpList(stm->u.MOVE.dst->u.MEM,
                         T_ExpList(stm->u.MOVE.dst->u.MEM,
                                   T_ExpList(stm->u.MOVE.src, NULL)));
    }
    return NULL;
}

static char cbuf[1024];

Temp_tempList L(Temp_temp h, Temp_tempList t) {
    return Temp_TempList(h, t);
}

static void genFrameArg(T_expList args, int off) {
    if (!args) {
        if (off) {
            // Reserve stack space
            sprintf(cbuf, "addiu `d0, `s0, %d\n", -off);
            string i = String(cbuf);
            F_emit(AS_Oper(i, L(F_SP(), NULL), L(F_SP(), NULL), NULL));
        }
        return;
    }

    genFrameArg(args->tail, off + F_wordSize);

    // Well, args suck
    sprintf(cbuf, "sw `s0, %d(`s1)\n", off);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(args->head), L(F_SP(), NULL)), NULL));
}

static void genArg(T_expList args) {
    for (Temp_tempList argregs = F_Argregs(); args && argregs; argregs = argregs->tail) {
        F_emit(AS_Oper("add `d0, `s0, `s1\n", L(argregs->head, NULL), L(F_doExp(args->head), L(F_ZERO(), NULL)), NULL));
        args = args->tail;
    }
    if (args) {
        genFrameArg(args, 0);
    }
}

static Temp_temp genConst(T_exp exp) {
    sprintf(cbuf, "addi `d0, `s0, %d\n", exp->u.CONST);
    Temp_temp r = Temp_newtemp();
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(r, NULL), L(F_ZERO(), NULL), NULL));
    return r;
}

static Temp_temp genCall(T_exp exp) {
    genArg(exp->u.CALL.args);
    // TODO: Add Caller-save registers, return value register, and so on (Collect them in calldefs), to param d
    F_emit(AS_Oper("jalr `s0, `d0\n", L(F_RA(), NULL), L(F_doExp(exp->u.CALL.fun), NULL), NULL));
    return F_RV();
}

static Temp_temp genNamedCall(T_exp exp) {
    genArg(exp->u.CALL.args);
    sprintf(cbuf, "jal %s\n", exp->u.CALL.fun->u.NAME->name);
    string i = String(cbuf);
    // TODO: Add Caller-save registers, return value register, and so on (Collect them in calldefs), to param d
    F_emit(AS_Oper(i, L(F_RA(), NULL), NULL, NULL));
    return F_RV();
}

static Temp_temp genName(T_exp exp) {
    sprintf(cbuf, "la `d0, %s\n", exp->u.NAME->name);
    string i = String(cbuf);
    Temp_temp r = Temp_newtemp();
    F_emit(AS_Oper(i, L(r, NULL), NULL, NULL));
    return r;
}

static Temp_temp genTemp(T_exp exp) {
    return exp->u.TEMP;
}

static void genExp(T_stm stm) {
    F_doExp(stm->u.EXP);
}

static Temp_temp genBinopPlus(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    F_emit(AS_Oper("add `d0, `s0, `s1\n", L(r, NULL),
                   L(F_doExp(exp->u.BINOP.left), L(F_doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static Temp_temp genBinopPlusImmEC(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    sprintf(cbuf, "addi `d0, `s0, %d\n", exp->u.BINOP.right->u.CONST);
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(r, NULL), L(F_doExp(exp->u.BINOP.left), NULL), NULL));
    return r;
}

static Temp_temp genBinopPlusImmCE(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    sprintf(cbuf, "addi `d0, `s0, %d\n", exp->u.BINOP.left->u.CONST);
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(r, NULL), L(F_doExp(exp->u.BINOP.right), NULL), NULL));
    return r;
}

static Temp_temp genBinopMinus(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    F_emit(AS_Oper("sub `d0, `s0, `s1\n", L(r, NULL),
                   L(F_doExp(exp->u.BINOP.left), L(F_doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static Temp_temp genBinopMinusImm(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    sprintf(cbuf, "subi `d0, `s0, %d\n", exp->u.BINOP.right->u.CONST);
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(r, NULL), L(F_doExp(exp->u.BINOP.left), NULL), NULL));
    return r;
}

static Temp_temp genBinopDiv(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    F_emit(AS_Oper("div `s0, `s1\nmflo `d0\n", L(r, NULL),
                   L(F_doExp(exp->u.BINOP.left), L(F_doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static Temp_temp genBinopMul(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    F_emit(AS_Oper("mult `s0, `s1\nmflo `d0\n", L(r, NULL),
                   L(F_doExp(exp->u.BINOP.left), L(F_doExp(exp->u.BINOP.right), NULL)), NULL));
    return r;
}

static void genBinopCjumpEq(T_stm stm) {
    sprintf(cbuf, "beq `s0, `s1, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.CJUMP.left), L(F_doExp(stm->u.CJUMP.right), NULL)),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpNe(T_stm stm) {
    sprintf(cbuf, "bne `s0, `s1, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.CJUMP.left), L(F_doExp(stm->u.CJUMP.right), NULL)),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLt(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s0, `s1\nbne `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(F_AT(), NULL), L(F_doExp(stm->u.CJUMP.left), L(F_doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGe(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s0, `s1\nbeq `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(F_AT(), NULL), L(F_doExp(stm->u.CJUMP.left), L(F_doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGt(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s1, `s0\nbne `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(F_AT(), NULL), L(F_doExp(stm->u.CJUMP.left), L(F_doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLe(T_stm stm) {
    sprintf(cbuf, "slt `d0, `s1, `s0\nbeq `d0, `s2, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(F_AT(), NULL), L(F_doExp(stm->u.CJUMP.left), L(F_doExp(stm->u.CJUMP.right), L(F_ZERO(), NULL))),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLt0(T_stm stm) {
    sprintf(cbuf, "bltz `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.CJUMP.left), NULL),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpLe0(T_stm stm) {
    sprintf(cbuf, "blez `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.CJUMP.left), NULL),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGt0(T_stm stm) {
    sprintf(cbuf, "bgtz `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.CJUMP.left), NULL),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genBinopCjumpGe0(T_stm stm) {
    sprintf(cbuf, "bgez `s0, %s\n", stm->u.CJUMP.true->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.CJUMP.left), NULL),
                   AS_Targets(Temp_LabelList(stm->u.CJUMP.false, Temp_LabelList(stm->u.CJUMP.true, NULL)))));
}

static void genJump(T_stm stm) {
    // TODO: In this situation, what targets will be?
    F_emit(AS_Oper("jr `s0\n", NULL, L(F_doExp(stm->u.JUMP.exp), NULL), AS_Targets(stm->u.JUMP.jumps)));
}

static void genNamedJump(T_stm stm) {
    sprintf(cbuf, "j %s\n", stm->u.JUMP.exp->u.NAME->name);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, NULL, AS_Targets(stm->u.JUMP.jumps)));
}

static Temp_temp genMemLoadOffsetEC(T_exp exp) {
    sprintf(cbuf, "lw `d0, %d(`s0)\n", exp->u.MEM->u.BINOP.right->u.CONST);
    Temp_temp r = Temp_newtemp();
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(r, NULL), L(F_doExp(exp->u.MEM->u.BINOP.left), NULL), NULL));
    return r;
}

static Temp_temp genMemLoadOffsetCE(T_exp exp) {
    sprintf(cbuf, "lw `d0, %d(`s0)\n", exp->u.MEM->u.BINOP.left->u.CONST);
    Temp_temp r = Temp_newtemp();
    string i = String(cbuf);
    F_emit(AS_Oper(i, L(r, NULL), L(F_doExp(exp->u.MEM->u.BINOP.right), NULL), NULL));
    return r;
}

static Temp_temp genMemLoadExp(T_exp exp) {
    Temp_temp r = Temp_newtemp();
    F_emit(AS_Oper("lw `d0, 0(`s0)\n", L(r, NULL), L(F_doExp(exp->u.MEM), NULL), NULL));
    return r;
}

static void genMoveTemp(T_stm stm) {
    F_emit(AS_Oper("add `d0, `s0, `s1\n", L(stm->u.MOVE.dst->u.TEMP, NULL),
                   L(F_doExp(stm->u.MOVE.src), L(F_ZERO(), NULL)), NULL));
}

static void genLabel(T_stm stm) {
    sprintf(cbuf, "%s:\n", stm->u.LABEL->name);
    string i = String(cbuf);
    F_emit(AS_Label(i, stm->u.LABEL));
}

static void genMemStoreOffsetEC(T_stm stm) {
    sprintf(cbuf, "sw `s1, %d(`s0)\n", stm->u.MOVE.dst->u.BINOP.right->u.CONST);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.MOVE.dst->u.BINOP.left), L(F_doExp(stm->u.MOVE.src), NULL)), NULL));
}

static void genMemStoreOffsetCE(T_stm stm) {
    sprintf(cbuf, "sw `s1, %d(`s0)\n", stm->u.MOVE.dst->u.BINOP.left->u.CONST);
    string i = String(cbuf);
    F_emit(AS_Oper(i, NULL, L(F_doExp(stm->u.MOVE.dst->u.BINOP.right), L(F_doExp(stm->u.MOVE.src), NULL)), NULL));
}

static void genMemStoreExp(T_stm stm) {
    F_emit(AS_Oper("sw `s1, 0(`s0)\n", NULL, L(F_doExp(stm->u.MOVE.dst), L(F_doExp(stm->u.MOVE.src), NULL)), NULL));
}

F_pattern F_patterns[] = {
        {PAT_EXP, {.exp = matchConst}, {.exp = genConst}, 1},
        {PAT_EXP, {.exp = matchCall}, {.exp = genCall}, 1},
        {PAT_EXP, {.exp = matchNamedCall}, {.exp = genNamedCall}, 1},
        {PAT_EXP, {.exp = matchName}, {.exp = genName}, 1},
        {PAT_EXP, {.exp = matchTemp}, {.exp = genTemp}, 0},
        {PAT_STM, {.stm = matchExp}, {.stm = genExp}, 0},
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
        {PAT_STM, {.stm = matchNamedJump}, {.stm = genNamedJump}, 1},
        {PAT_EXP, {.exp = matchMemLoadOffsetEC}, {.exp = genMemLoadOffsetEC}, 1},
        {PAT_EXP, {.exp = matchMemLoadOffsetCE}, {.exp = genMemLoadOffsetCE}, 1},
        {PAT_EXP, {.exp = matchMemLoadExp}, {.exp = genMemLoadExp}, 1},
        {PAT_STM, {.stm = matchMoveTemp}, {.stm = genMoveTemp}, 1},
        {PAT_STM, {.stm = matchLabel}, {.stm = genLabel}, 1},
        {PAT_STM, {.stm = matchMemStoreOffsetEC}, {.stm = genMemStoreOffsetEC}, 1},
        {PAT_STM, {.stm = matchMemStoreOffsetCE}, {.stm = genMemStoreOffsetCE}, 1},
        {PAT_STM, {.stm = matchMemStoreExp}, {.stm = genMemStoreExp}, 1},
};

int F_nPattern = sizeof(F_patterns) / sizeof(F_pattern);
