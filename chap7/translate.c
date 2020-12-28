/*
 * translate.c - translate code to IR
 */

#include "translate.h"
#include "frame.h"
#include "tree.h"

#define SL_OFFSET (-4)

static F_fragList frags = NULL, frags_tail = NULL;

static void insertFrag(F_frag frag) {
    F_fragList node = F_FragList(frag, NULL);
    if (!frags) {
        frags = frags_tail = node;
    } else {
        frags_tail->tail = node;
        frags_tail = node;
    }
}

static patchList PatchList(Temp_label *head, patchList tail) {
    patchList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

static Cx newCx(patchList trues, patchList falses, T_stm stm) {
    Cx p = {
            .trues = trues,
            .falses = falses,
            .stm = stm
    };
    return p;
}

static void doPatch(patchList list, Temp_label label) {
    for (; list; list = list->tail) {
        *(list->head) = label;
    }
}

static patchList joinPatch(patchList p1, patchList p2) {
    patchList res = NULL;
    for (; p1; p1 = p1->tail) {
        res = PatchList(p1->head, res);
    }
    for (; p2; p2 = p2->tail) {
        res = PatchList(p2->head, res);
    }
    return res;
}

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
    Tr_expList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

static Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp p = checked_malloc(sizeof(*p));
    p->kind = Tr_ex;
    p->u.ex = ex;
    return p;
}

static Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp p = checked_malloc(sizeof(*p));
    p->kind = Tr_nx;
    p->u.nx = nx;
    return p;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
    Tr_exp p = checked_malloc(sizeof(*p));
    p->kind = Tr_cx;
    p->u.cx.stm = stm;
    p->u.cx.trues = trues;
    p->u.cx.falses = falses;
    return p;
}

static T_exp convertToEx(Tr_exp exp) {
    switch (exp->kind) {
        case Tr_ex:
            return exp->u.ex;

        case Tr_nx:
            return T_Eseq(exp->u.nx, T_Const(0));

        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(exp->u.cx.trues, t);
            doPatch(exp->u.cx.falses, f);
            return T_Eseq(T_Move(T_Temp(r), T_Const(1)),     // MOVE r 1
                          T_Eseq(exp->u.cx.stm,                    // Generate statement
                                 T_Eseq(T_Label(f),                      // f:
                                        T_Eseq(T_Move(T_Temp(r), T_Const(0)),  // MOVE r 0
                                               T_Eseq(T_Label(t), T_Temp(r))))));    // t: r
        }

        default:
            assert(0);
    }
}

static T_stm convertToNx(Tr_exp exp) {
    switch (exp->kind) {
        case Tr_ex:
            return T_Exp(exp->u.ex);

        case Tr_nx:
            return exp->u.nx;

        case Tr_cx: {
            Temp_label all = Temp_newlabel();
            doPatch(exp->u.cx.trues, all);
            doPatch(exp->u.cx.falses, all);
            return T_Seq(exp->u.cx.stm, T_Label(all));
        }

        default:
            assert(0);
    }
}

static Cx convertToCx(Tr_exp exp) {
    switch (exp->kind) {
        case Tr_ex: {
            // First consider special cases like T_Const(0) or T_Const(not 0)
            if (exp->u.ex->kind == T_CONST && exp->u.ex->u.CONST == 0) {
                // No matter what, goto false branch
                T_stm stm = T_Jump(T_Name(NULL), Temp_LabelList(NULL, NULL));
                return newCx(NULL, PatchList(&stm->u.JUMP.exp->u.NAME, PatchList(&stm->u.JUMP.jumps->head, NULL)), stm);
            } else if (exp->u.ex->kind == T_CONST && exp->u.ex->u.CONST != 0) {
                // No matter what, goto true branch
                T_stm stm = T_Jump(T_Name(NULL), Temp_LabelList(NULL, NULL));
                return newCx(PatchList(&stm->u.JUMP.exp->u.NAME, PatchList(&stm->u.JUMP.jumps->head, NULL)), NULL, stm);
            }

            T_stm stm = T_Cjump(T_ne, exp->u.ex, T_Const(0), NULL, NULL);
            return newCx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_cx:
            return exp->u.cx;

        default:
            assert(0);
    }
}

static struct Tr_level_ troutmost = {NULL, NULL};

Tr_access Tr_Access(Tr_level level, F_access access) {
    Tr_access p = checked_malloc(sizeof(*p));
    p->level = level;
    p->access = access;
    return p;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

Tr_level Tr_outermost() { return &troutmost; }

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    Tr_level p = checked_malloc(sizeof(*p));
    p->parent = parent;
    p->frame = F_newFrame(name, U_BoolList(TRUE, formals));
    p->formals = NULL;
    Tr_accessList tail = NULL;
    for (F_accessList f_formals = F_formals(p->frame); f_formals; f_formals = f_formals->tail) {
        Tr_accessList entry = Tr_AccessList(Tr_Access(p, f_formals->head), NULL);
        if (!p->formals) {
            p->formals = tail = entry;
        } else {
            tail->tail = entry;
            tail = entry;
        }
    }
    return p;
}

Tr_accessList Tr_formals(Tr_level level) {
    return level->formals->tail; // Skip static link
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    return Tr_Access(level, F_allocLocal(level->frame, escape));
}

Tr_exp Tr_simpleVar(Tr_access access, Tr_level cur_level) {
    T_exp real_fp = T_Temp(F_FP());
    while (cur_level != access->level) {
        T_exp sl = T_Binop(T_plus, real_fp, T_Const(SL_OFFSET));
        real_fp = T_Mem(sl);
        cur_level = cur_level->parent;
    }
    return Tr_Ex(F_exp(access->access, real_fp));
}

Tr_exp Tr_subscriptVar(Tr_exp var, Tr_exp idx) {
    T_exp ex = convertToEx(idx);
    T_exp varex = convertToEx(var);
    return Tr_Ex(
            T_Mem(T_Binop(T_plus, varex,
                          T_Binop(T_mul, ex, T_Const(F_wordSize))
            ))
    );
}

Tr_exp Tr_fieldVar(Tr_exp var, int field_idx) {
    T_exp varex = convertToEx(var);
    return Tr_Ex(
            T_Mem(T_Binop(T_plus, varex, T_Const(F_wordSize * field_idx)))
    );
}

Tr_exp Tr_strCmp(Tr_oper op, Tr_exp l, Tr_exp r) {
    switch (op) {
        case Tr_eq: {
            T_stm stm = T_Cjump(T_eq, F_externalCall("strCmp", T_ExpList(convertToEx(l), T_ExpList(convertToEx(r), NULL))), T_Const(0), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_neq: {
            T_stm stm = T_Cjump(T_ne, F_externalCall("strCmp", T_ExpList(convertToEx(l), T_ExpList(convertToEx(r), NULL))), T_Const(0), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_lt: {
            T_stm stm = T_Cjump(T_lt, F_externalCall("strCmp", T_ExpList(convertToEx(l), T_ExpList(convertToEx(r), NULL))), T_Const(0), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_le: {
            T_stm stm = T_Cjump(T_le, F_externalCall("strCmp", T_ExpList(convertToEx(l), T_ExpList(convertToEx(r), NULL))), T_Const(0), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_gt: {
            T_stm stm = T_Cjump(T_gt, F_externalCall("strCmp", T_ExpList(convertToEx(l), T_ExpList(convertToEx(r), NULL))), T_Const(0), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_ge: {
            T_stm stm = T_Cjump(T_ge, F_externalCall("strCmp", T_ExpList(convertToEx(l), T_ExpList(convertToEx(r), NULL))), T_Const(0), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        default:
            assert(0);
    }
}

Tr_exp Tr_op(Tr_oper op, Tr_exp l, Tr_exp r) {
    switch (op) {
        case Tr_plus:
            return Tr_Ex(T_Binop(T_plus, convertToEx(l), convertToEx(r)));

        case Tr_minus:
            return Tr_Ex(T_Binop(T_minus, convertToEx(l), convertToEx(r)));

        case Tr_times:
            return Tr_Ex(T_Binop(T_mul, convertToEx(l), convertToEx(r)));

        case Tr_divide:
            return Tr_Ex(T_Binop(T_div, convertToEx(l), convertToEx(r)));

        case Tr_eq: {
            T_stm stm = T_Cjump(T_eq, convertToEx(l), convertToEx(r), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_neq: {
            T_stm stm = T_Cjump(T_ne, convertToEx(l), convertToEx(r), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_lt: {
            T_stm stm = T_Cjump(T_lt, convertToEx(l), convertToEx(r), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_le: {
            T_stm stm = T_Cjump(T_le, convertToEx(l), convertToEx(r), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_gt: {
            T_stm stm = T_Cjump(T_gt, convertToEx(l), convertToEx(r), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        case Tr_ge: {
            T_stm stm = T_Cjump(T_ge, convertToEx(l), convertToEx(r), NULL, NULL);
            return Tr_Cx(PatchList(&stm->u.CJUMP.true, NULL), PatchList(&stm->u.CJUMP.false, NULL), stm);
        }

        default:
            assert(0);
    }
}

Tr_exp Tr_ifthenelse(Tr_exp test, Tr_exp then, Tr_exp elsee) {
    Cx cx = convertToCx(test);

    // Condition1: Both nx (without return value)
    // for example: if a then print("x") else print("y")
    if (then->kind == Tr_nx && elsee->kind == Tr_nx) {
        Temp_label t = Temp_newlabel(), f = Temp_newlabel(), conj = Temp_newlabel();
        doPatch(cx.trues, t);
        doPatch(cx.falses, f);
        return Tr_Nx(T_Seq(cx.stm,
                           T_Seq(T_Label(t),
                                 T_Seq(convertToNx(then),
                                       T_Seq(T_Jump(T_Name(conj), Temp_LabelList(conj, NULL)),
                                             T_Seq(T_Label(f),
                                                   T_Seq(convertToNx(elsee),
                                                         T_Label(conj))))))));
    }

    // Condition3: Both cx, or 0/1+cx, or 1/0+cx
    // for example: if a then x > y else p > q
    // for example: if a then 1 else p > q
    // for example: if a then p < q else 0
    if ((then->kind == Tr_cx && (elsee->kind == Tr_cx || (elsee->kind == Tr_ex && elsee->u.ex->kind == T_CONST && (elsee->u.ex->u.CONST == 0 || elsee->u.ex->u.CONST == 1))))
    ||  (elsee->kind == Tr_cx && (then->kind == Tr_cx || (then->kind == Tr_ex && then->u.ex->kind == T_CONST && (then->u.ex->u.CONST == 0 || then->u.ex->u.CONST == 1))))) {
        Cx then_c = convertToCx(then), elses_c = convertToCx(elsee);
        Temp_label t = Temp_newlabel(), f = Temp_newlabel(), conj = Temp_newlabel();
        doPatch(cx.trues, t);
        doPatch(cx.falses, f);
        T_stm stm = T_Seq(cx.stm,
                          T_Seq(T_Label(t),
                                T_Seq(then_c.stm,
                                      T_Seq(T_Jump(T_Name(conj), Temp_LabelList(conj, NULL)),
                                            T_Seq(T_Label(f),
                                                  T_Seq(elses_c.stm,
                                                        T_Label(conj)))))));

        return Tr_Cx(joinPatch(then_c.trues, elses_c.trues),
                     joinPatch(then_c.falses, elses_c.falses),
                     stm);
    }

    // Condition4: nx+ex
    // Impossible
    assert(!(then->kind == Tr_nx && elsee->kind != Tr_ex));

    // Condition5: ex+nx
    // Impossible
    assert(!(then->kind == Tr_ex && elsee->kind != Tr_nx));

    // Condition2: Both ex (with return value)
    // for example: if a then 3 else 4
    // And Other situations
    // We all convert them to ex
    Temp_label t = Temp_newlabel(), f = Temp_newlabel(), conj = Temp_newlabel();
    doPatch(cx.trues, t);
    doPatch(cx.falses, f);
    Temp_temp r = Temp_newtemp();
    return Tr_Ex(T_Eseq(cx.stm,
                        T_Eseq(T_Label(t),
                               T_Eseq(T_Move(T_Temp(r), convertToEx(then)),
                                      T_Eseq(T_Jump(T_Name(conj), Temp_LabelList(conj, NULL)),
                                             T_Eseq(T_Label(f),
                                                    T_Eseq(T_Move(T_Temp(r), convertToEx(elsee)),
                                                           T_Eseq(T_Label(conj),
                                                                  T_Temp(r)))))))));
}

Tr_exp Tr_ifthen(Tr_exp test, Tr_exp then) {
    Cx cx = convertToCx(test);
    Temp_label t = Temp_newlabel(), f = Temp_newlabel();
    doPatch(cx.trues, t);
    doPatch(cx.falses, f);
    return Tr_Nx(T_Seq(cx.stm, T_Seq(T_Label(t), T_Seq(convertToNx(then), T_Label(f)))));
}

Tr_exp Tr_newRecord(int n_field, Tr_expList initializers) {
    Temp_temp r = Temp_newtemp();
    T_stm alloca = T_Move(T_Temp(r),
                          F_externalCall("malloc",
                                         T_ExpList(T_Const(n_field * F_wordSize), NULL)
                          ));
    Temp_temp p = Temp_newtemp();
    T_stm init = T_Move(T_Temp(p), T_Temp(r));
    T_stm seq, seq_tail;
    seq = seq_tail = T_Seq(T_Seq(alloca, init), NULL);
    Tr_expList cur = initializers;
    for (int i = 0; i < n_field; i++) {
        assert(cur);

        T_stm field_init = T_Move(T_Mem(T_Temp(p)), convertToEx(cur->head));
        seq_tail->u.SEQ.right = T_Seq(
                T_Seq(field_init, T_Move(T_Temp(p), T_Binop(T_plus, T_Temp(p), T_Const(F_wordSize)))), NULL);
        seq_tail = seq_tail->u.SEQ.right;

        cur = cur->tail;
    }
    seq_tail->u.SEQ.right = convertToNx(Tr_nop());
    assert(!cur);
    return Tr_Ex(T_Eseq(seq, T_Temp(r)));
}

Tr_exp Tr_newArray(Tr_exp n, Tr_exp initializer) {
    Temp_temp size_e = Temp_newtemp();
    T_stm calc_size = T_Move(T_Temp(size_e), T_Binop(T_mul, convertToEx(n), T_Const(F_wordSize)));

    Temp_temp r = Temp_newtemp();
    T_stm alloca = T_Move(T_Temp(r),
                          F_externalCall("malloc",
                                         T_ExpList(T_Temp(size_e), NULL)
                          ));
    T_exp init = F_externalCall("initArray", T_ExpList(T_Temp(r), T_ExpList(T_Temp(size_e), NULL)));

    return Tr_Ex(T_Eseq(T_Seq(calc_size, T_Seq(alloca, T_Exp(init))), T_Temp(r)));
}

Tr_exp Tr_while(Tr_exp test, Tr_exp body, Temp_label done) {
    Temp_label test_lbl = Temp_newlabel();
    Temp_label continue_lbl = Temp_newlabel();
    Cx test_cx = convertToCx(test);
    doPatch(test_cx.trues, continue_lbl);
    doPatch(test_cx.falses, done);
    return Tr_Nx(T_Seq(T_Label(test_lbl),
                       T_Seq(test_cx.stm,
                             T_Seq(T_Label(continue_lbl),
                                   T_Seq(convertToNx(body),
                                         T_Seq(T_Jump(T_Name(test_lbl), Temp_LabelList(test_lbl, NULL)),
                                               T_Label(done)))))));
}

Tr_exp Tr_for(Tr_access var, Tr_level cur_level, Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done) {
    Temp_label for_loop = Temp_newlabel();
    Temp_label enter_lbl = Temp_newlabel();
    T_exp loop_var = convertToEx(Tr_simpleVar(var, cur_level));
    return Tr_Nx(T_Seq(T_Label(for_loop),
                       T_Seq(T_Move(loop_var, convertToEx(lo)),
                             T_Seq(T_Cjump(T_le, loop_var, convertToEx(hi), enter_lbl, done),
                                   T_Seq(T_Label(enter_lbl),
                                         T_Seq(convertToNx(body),
                                               T_Seq(T_Jump(T_Name(enter_lbl), Temp_LabelList(enter_lbl, NULL)),
                                                     T_Label(done))))))));
}

Tr_exp Tr_functionCall(S_symbol name, Tr_level callee, Tr_level caller, Tr_expList args, bool is_proc) {
    Tr_level p;

    // args
    T_expList converted_args_tail, converted_args;
    converted_args = converted_args_tail = NULL;
    for (; args; args = args->tail) {
        T_expList node = T_ExpList(convertToEx(args->head), NULL);

        if (!converted_args) {
            converted_args = converted_args_tail = node;
        } else {
            converted_args_tail->tail = node;
            converted_args_tail = node;
        }
    }

    if (!callee) {
        // If no level, then it must be a intrinsic function, without static link
        return Tr_Ex(F_externalCall(name->name, converted_args));
    }

    // calculate static link first
    int callee_depth = 0, caller_depth = 0;
    p = callee;
    while (p) {
        callee_depth++;
        p = p->parent;
    }
    p = caller;
    while (p) {
        caller_depth++;
        p = p->parent;
    }

    T_exp sl = T_Temp(F_FP());

    /*
     * To calculate the FP of the parent level of callee
     *
     * 1. fun C() { fun callee() { ... fun caller() { callee() } ... } } -- callee_depth < caller_depth
     * 2. fun C() { fun callee(); fun caller() { callee() } } -- callee_depth = caller_depth
     * 3. fun C() { fun caller() { fun callee() {} callee() } } -- callee_depth = caller_depth + 1
     *
     * so, callee_depth <= caller_depth + 1
     */
    assert(callee_depth <= caller_depth + 1);

    for (int i = 0; i < caller_depth - callee_depth + 1; i++) {
        sl = T_Mem(T_Binop(T_plus, sl, T_Const(SL_OFFSET)));
    }

    T_exp call = T_Call(T_Name(F_name(callee->frame)), T_ExpList(sl, converted_args));
    return !is_proc ? Tr_Ex(call) : Tr_Nx(T_Exp(call));
}

Tr_exp Tr_assign(Tr_exp lhs, Tr_exp rhs) {
    return Tr_Nx(T_Move(convertToEx(lhs), convertToEx(rhs)));
}

Tr_exp Tr_const(int n) {
    return Tr_Ex(T_Const(n));
}

Tr_exp Tr_nop() {
    return Tr_Nx(T_Exp(T_Const(0)));
}

Tr_exp Tr_string(string s) {
    Temp_label label = Temp_newlabel();
    insertFrag(F_StringFrag(label, s));
    return Tr_Ex(T_Name(label));
}

Tr_exp Tr_break(Temp_label done) {
    return Tr_Nx(T_Jump(T_Name(done), Temp_LabelList(done, NULL)));
}

Tr_exp Tr_seq(Tr_exp stm, Tr_exp res) {
    return Tr_Ex(T_Eseq(convertToNx(stm), convertToEx(res)));
}

Tr_exp Tr_stmtSeq(Tr_exp left, Tr_exp right) {
    return Tr_Nx(T_Seq(convertToNx(left), convertToNx(right)));
}

F_fragList Tr_getResult() {
    return frags;
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals) {
    insertFrag(F_ProcFrag(T_Move(T_Temp(F_RV()), convertToEx(body)), level->frame));
}
