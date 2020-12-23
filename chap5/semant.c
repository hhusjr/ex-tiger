/*
 * semant.c - Used for semantic analysis
 */

#include <util.h>
#include <semant.h>
#include <env.h>
#include <errormsg.h>
#include <stdlib.h>
#include <absyn.h>
#include <symbol.h>
#include "types.h"
#include "env.h"
#include "absyn.h"
#include <stdbool.h>

struct expty_ {
    Tr_exp exp;
    Ty_ty ty;
};
typedef struct expty_ expty;

static expty Expty(Tr_exp exp, Ty_ty ty) {
    expty e = {
            .exp = exp,
            .ty = ty
    };
    return e;
}


static void *requireSym(int pos, S_table t, S_symbol sym, void* not_found) {
    void* look = S_look(t, sym);
    if (!look) {
        EM_error(pos, "Undefined symbol %s", sym->name);
        return not_found;
    }
    return look;
}

static expty visitExp(S_table tenv, S_table venv, A_exp exp);

static expty visitVar(S_table tenv, S_table venv, A_var var);

static expty visitNilExp();

static expty visitIntExp();

static expty visitStringExp();

static expty visitCallExp(S_table tenv, S_table venv, A_exp exp);

static expty visitOpExp(S_table tenv, S_table venv, A_exp exp);

static expty visitRecordExp(S_table tenv, S_table venv, A_exp exp);

static expty visitSeqExp(S_table tenv, S_table venv, A_expList exps);

static expty visitAssignExp(S_table tenv, S_table venv, A_exp exp);

static expty visitIfExp(S_table tenv, S_table venv, A_exp exp);

static expty visitWhileExp(S_table tenv, S_table venv, A_exp exp);

static expty visitForExp(S_table tenv, S_table venv, A_exp exp);

static expty visitBreakExp();

static expty visitLetExp(S_table tenv, S_table venv, A_exp exp);

static expty visitArrayExp(S_table tenv, S_table venv, A_exp exp);

void visitDec(S_table tenv, S_table venv, A_dec dec);

void visitTypeDec(S_table tenv, S_table venv, A_dec dec);

void visitFunctionDec(S_table tenv, S_table venv, A_dec dec);

void visitVarDec(S_table tenv, S_table venv, A_dec dec);

Ty_ty visitTy(S_table tenv, S_table venv, A_ty ty);

static Ty_ty actualTy(Ty_ty ty) {
    while (ty->kind == Ty_name) {
        ty = ty->u.name.ty;
        assert(ty);
    }
    return ty;
}

static _Bool isTypeCompat(Ty_ty lhs, Ty_ty rhs) {
    Ty_ty l = actualTy(lhs), r = actualTy(rhs);

    return (l == r && l->kind != Ty_nil && r->kind != Ty_nil)
        ||  (l->kind == Ty_nil && r->kind == Ty_record)
        ||  (l->kind == Ty_record && r->kind == Ty_nil);
}

static expty visitExp(S_table tenv, S_table venv, A_exp exp) {
    switch (exp->kind) {
        case A_varExp:
            return visitVar(tenv, venv, exp->u.var);
        case A_nilExp:
            return visitNilExp();
        case A_intExp:
            return visitIntExp();
        case A_stringExp:
            return visitStringExp();
        case A_callExp:
            return visitCallExp(tenv, venv, exp);
        case A_opExp:
            return visitOpExp(tenv, venv, exp);
        case A_recordExp:
            return visitRecordExp(tenv, venv, exp);
        case A_seqExp:
            return visitSeqExp(tenv, venv, exp->u.seq);
        case A_assignExp:
            return visitAssignExp(tenv, venv, exp);
        case A_ifExp:
            return visitIfExp(tenv, venv, exp);
        case A_whileExp:
            return visitWhileExp(tenv, venv, exp);
        case A_forExp:
            return visitForExp(tenv, venv, exp);
        case A_breakExp:
            return visitBreakExp();
        case A_letExp:
            return visitLetExp(tenv, venv, exp);
        case A_arrayExp:
            return visitArrayExp(tenv, venv, exp);
        default:
            assert(0);
    }
}

static expty visitVar(S_table tenv, S_table venv, A_var var) {
    switch (var->kind) {
        case A_simpleVar:
            return Expty(NULL, ((E_enventry) requireSym(var->pos, venv, var->u.simple, Ty_Int()))->u.var.ty);

        case A_fieldVar: {
            Ty_ty var_t = actualTy(visitVar(tenv, venv, var->u.field.var).ty);
            if (var_t->kind != Ty_record) {
                EM_error(var->pos, "variable must be record type");
                return Expty(NULL, Ty_Int());
            }

            for (Ty_fieldList p = var_t->u.record; p != NULL; p = p->tail) {
                if (p->head->name == var->u.field.sym) {
                    return Expty(NULL, p->head->ty);
                }
            }
            EM_error(var->pos, "unknown field %s", var->u.field.sym->name);
            return Expty(NULL, Ty_Int());
        }

        case A_subscriptVar: {
            Ty_ty var_t = actualTy(visitVar(tenv, venv, var->u.subscript.var).ty);
            if (var_t->kind != Ty_array) {
                EM_error(var->pos, "variable must be array type");
                return Expty(NULL, Ty_Int());
            }
            return Expty(NULL, var_t->u.array);
        }

        default:
            assert(0);
    }
}

static expty visitCallExp(S_table tenv, S_table venv, A_exp exp) {
    E_enventry func = requireSym(exp->pos, venv, exp->u.call.func, NULL);
    if (!func || func->kind != E_funEntry) {
        EM_error(exp->pos, "function name undefined or not a function");
        return Expty(NULL, Ty_Int());
    }

    A_expList arg = exp->u.call.args;

    Ty_ty result = func->u.fun.result;
    Ty_tyList formal = func->u.fun.formals;

    while (arg != NULL && formal != NULL) {
        Ty_ty ideal = formal->head;
        Ty_ty actual = visitExp(tenv, venv, arg->head).ty;

        assert(ideal->kind != Ty_nil);
        if (!isTypeCompat(ideal, actual)) {
            EM_error(arg->head->pos, "function parameter type does not match");
            return Expty(NULL, Ty_Int());
        }

        formal = formal->tail;
        arg = arg->tail;
    }

    if (arg != NULL || formal != NULL) {
        EM_error(exp->pos, "parameter passed to the function must have same length with formals in definition");
        return Expty(NULL, Ty_Int());
    }

    return Expty(NULL, result);
}

static expty visitNilExp() {
    return Expty(NULL, Ty_Nil());
}

static expty visitIntExp() {
    return Expty(NULL, Ty_Int());
}

static expty visitStringExp() {
    return Expty(NULL, Ty_String());
}

static expty visitOpExp(S_table tenv, S_table venv, A_exp exp) {
    A_exp l = exp->u.op.left, r = exp->u.op.right;
    A_oper op = exp->u.op.oper;

    switch (op) {
        case A_plusOp:
        case A_minusOp:
        case A_timesOp:
        case A_divideOp:
            if (!isTypeCompat(visitExp(tenv, venv, l).ty, Ty_Int())
                || !isTypeCompat(visitExp(tenv, venv, r).ty, Ty_Int())) {
                EM_error(exp->pos, "both left operand and right operand must be Integer value");
                return Expty(NULL, Ty_Int());
            }
            return Expty(NULL, Ty_Int());

        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp:
            if (isTypeCompat(visitExp(tenv, venv, l).ty, Ty_String()) &&
                isTypeCompat(visitExp(tenv, venv, r).ty, Ty_String())) {
                return Expty(NULL, Ty_Int());
            }
            if (isTypeCompat(visitExp(tenv, venv, l).ty, Ty_Int()) && isTypeCompat(visitExp(tenv, venv, r).ty, Ty_Int())) {
                return Expty(NULL, Ty_Int());
            }
            EM_error(exp->pos, "both left operand and right operand must be Integer value or String value");
            return Expty(NULL, Ty_Int());

        case A_eqOp:
        case A_neqOp: {
            Ty_ty l_ty = visitExp(tenv, venv, l).ty;
            Ty_ty r_ty = visitExp(tenv, venv, r).ty;
            if (isTypeCompat(l_ty, r_ty)) {
                return Expty(NULL, Ty_Int());
            }
            EM_error(exp->pos, "equality test operator can not be applied to these operators");
            return Expty(NULL, Ty_Int());
        }

        default:
            assert(0);
    }
}

static expty visitAssignExp(S_table tenv, S_table venv, A_exp exp) {
    A_var lhs = exp->u.assign.var;
    A_exp rhs = exp->u.assign.exp;

    Ty_ty lhs_t = visitVar(tenv, venv, lhs).ty;
    Ty_ty rhs_t = visitExp(tenv, venv, rhs).ty;

    assert(lhs_t->kind != Ty_nil);
    if (!isTypeCompat(lhs_t, rhs_t)) {
        EM_error(lhs->pos, "lhs and rhs does not have same type");
        return Expty(NULL, Ty_Void());
    }

    return Expty(NULL, Ty_Void());
}

static expty visitIfExp(S_table tenv, S_table venv, A_exp exp) {
    A_exp test = exp->u.iff.test;
    A_exp then = exp->u.iff.then;
    A_exp elsee = exp->u.iff.elsee;

    if (!isTypeCompat(visitExp(tenv, venv, test).ty, Ty_Int())) {
        EM_error(test->pos, "test expression must have Integer type");
        return Expty(NULL, Ty_Void());
    }

    Ty_ty then_t = visitExp(tenv, venv, then).ty;

    if (elsee == NULL) {
        if (!isTypeCompat(then_t, Ty_Void())) {
            EM_error(then->pos, "then expression must be void");
            return Expty(NULL, Ty_Void());
        }

        return Expty(NULL, Ty_Void());
    }

    Ty_ty else_t = visitExp(tenv, venv, elsee).ty;

    if (!isTypeCompat(then_t, else_t)) {
        EM_error(then->pos, "then and else must have same type (or both void)");
        return Expty(NULL, Ty_Void());
    }

    return Expty(NULL, then_t);
}

static expty visitWhileExp(S_table tenv, S_table venv, A_exp exp) {
    A_exp test = exp->u.whilee.test;
    A_exp body = exp->u.whilee.body;

    if (!isTypeCompat(visitExp(tenv, venv, test).ty, Ty_Int())) {
        EM_error(test->pos, "test expression must have Integer type");
        return Expty(NULL, Ty_Void());
    }

    if (!isTypeCompat(visitExp(tenv, venv, body).ty, Ty_Void())) {
        EM_error(test->pos, "loop body must be void");
        return Expty(NULL, Ty_Void());
    }

    return Expty(NULL, Ty_Void());
}

static expty visitForExp(S_table tenv, S_table venv, A_exp exp) {
    S_symbol id = exp->u.forr.var;
    A_exp lo = exp->u.forr.lo, hi = exp->u.forr.hi;
    A_exp body = exp->u.forr.body;

    if (!isTypeCompat(visitExp(tenv, venv, lo).ty, Ty_Int())) {
        EM_error(lo->pos, "lower bound of for loop must be Integer");
        return Expty(NULL, Ty_Void());
    }

    if (!isTypeCompat(visitExp(tenv, venv, hi).ty, Ty_Int())) {
        EM_error(lo->pos, "upper bound of for loop must be Integer");
        return Expty(NULL, Ty_Void());
    }

    S_beginScope(venv);
    S_enter(venv, id, E_VarEntry(Ty_Int()));
    if (!isTypeCompat(visitExp(tenv, venv, body).ty, Ty_Void())) {
        EM_error(body->pos, "loop body must be void");
        return Expty(NULL, Ty_Void());
    }
    S_endScope(venv);

    return Expty(NULL, Ty_Void());
}

static expty visitBreakExp() {
    return Expty(NULL, Ty_Void());
}

static expty visitSeqExp(S_table tenv, S_table venv, A_expList exps) {
    Ty_ty result = Ty_Void();

    for (A_expList p = exps; p != NULL; p = p->tail) {
        A_exp exp = p->head;
        result = visitExp(tenv, venv, exp).ty;
    }

    return Expty(NULL, result);
}

static expty visitArrayExp(S_table tenv, S_table venv, A_exp exp) {
    Ty_ty ty = requireSym(exp->pos, tenv, exp->u.array.typ, Ty_Void());
    Ty_ty actual_ty = actualTy(ty);
    if (actual_ty->kind != Ty_array) {
        EM_error(exp->pos, "array name not defined or not an array");
        return Expty(NULL, Ty_Void());
    }

    A_exp size = exp->u.array.size;
    A_exp init = exp->u.array.init;

    if (!isTypeCompat(visitExp(tenv, venv, size).ty, Ty_Int())) {
        EM_error(size->pos, "array size must be Integer");
        return Expty(NULL, ty);
    }

    if (!isTypeCompat(visitExp(tenv, venv, init).ty, actual_ty->u.array)) {
        EM_error(init->pos, "array initializer must be the same type as array elements");
        return Expty(NULL, ty);
    }

    return Expty(NULL, ty);
}

static expty visitRecordExp(S_table tenv, S_table venv, A_exp exp) {
    Ty_ty ty = requireSym(exp->pos, tenv, exp->u.record.typ, NULL);
    Ty_ty actual_ty = actualTy(ty);
    if (actual_ty->kind != Ty_record) {
        EM_error(exp->pos, "record name not defined or not a record");
        return Expty(NULL, Ty_Void());
    }

    for (A_efieldList efield_list = exp->u.record.fields; efield_list != NULL; efield_list = efield_list->tail) {
        A_efield efield = efield_list->head;

        S_symbol symbol = efield->name;
        Ty_field field = NULL;
        for (Ty_fieldList field_list = actual_ty->u.record; field_list != NULL; field_list = field_list->tail) {
            if (field_list->head->name == symbol) {
                field = field_list->head;
                break;
            }
        }

        if (field == NULL) {
            EM_error(exp->pos, "field %s not found", symbol->name);
            return Expty(NULL, ty);
        }

        expty vis_exp = visitExp(tenv, venv, efield->exp);
        if (!isTypeCompat(field->ty, vis_exp.ty)) {
            EM_error(exp->pos, "field %s has wrong type", symbol->name);
            return Expty(NULL, ty);
        }
    }

    return Expty(NULL, ty);
}

static expty visitLetExp(S_table tenv, S_table venv, A_exp exp) {
    S_beginScope(venv);
    S_beginScope(tenv);
    for (A_decList dec_list = exp->u.let.decs; dec_list != NULL; dec_list = dec_list->tail) {
        A_dec dec = dec_list->head;
        visitDec(tenv, venv, dec);
    }
    expty vis_body = visitExp(tenv, venv, exp->u.let.body);
    S_endScope(venv);
    S_endScope(tenv);
    return Expty(NULL, vis_body.ty);
}

void visitDec(S_table tenv, S_table venv, A_dec dec) {
    switch (dec->kind) {
        case A_functionDec:
            visitFunctionDec(tenv, venv, dec);
            return;

        case A_varDec:
            visitVarDec(tenv, venv, dec);
            return;

        case A_typeDec:
            visitTypeDec(tenv, venv, dec);
            return;

        default:
            assert(0);
    }
}

void visitTypeDec(S_table tenv, S_table venv, A_dec dec) {
    for (A_nametyList list = dec->u.type; list != NULL; list = list->tail) {
        S_symbol name = list->head->name;
        Ty_ty ty = visitTy(tenv, venv, list->head->ty);

        S_enter(tenv, name, ty);
    }
}

Ty_ty visitTy(S_table tenv, S_table venv, A_ty ty) {
    switch (ty->kind) {
        case A_nameTy:
            return Ty_Name(ty->u.name, requireSym(ty->pos, tenv, ty->u.name, Ty_Int()));

        case A_recordTy: {
            Ty_fieldList list_head = NULL, list_tail = NULL;
            for (A_fieldList fields = ty->u.record; fields != NULL; fields = fields->tail) {
                A_field field = fields->head;
                Ty_ty field_type = requireSym(field->pos, tenv, field->typ, Ty_Int());
                if (list_head == NULL) {
                    list_head = list_tail = Ty_FieldList(Ty_Field(field->name, field_type), NULL);
                } else {
                    list_tail->tail =  Ty_FieldList(Ty_Field(field->name, field_type), NULL);
                    list_tail = list_tail->tail;
                }
            }
            return Ty_Record(list_head);
        }

        case A_arrayTy:
            return Ty_Array(requireSym(ty->pos, tenv, ty->u.name, Ty_Int()));

        default:
            assert(0);
    }
}

void visitVarDec(S_table tenv, S_table venv, A_dec dec) {
    Ty_ty init_type = visitExp(tenv, venv, dec->u.var.init).ty;
    Ty_ty init_type_actual = actualTy(init_type);
    Ty_ty ty;
    if (dec->u.var.typ == NULL) {
        if (init_type_actual->kind == Ty_void || init_type_actual->kind == Ty_nil) {
            EM_error(dec->pos, "variable declaration initializer type error");
            S_enter(venv, dec->u.var.var, E_VarEntry(Ty_Int()));
            return;
        }
        ty = init_type;
    } else {
        ty = requireSym(dec->pos, tenv, dec->u.var.typ, init_type_actual);
    }

    if (!isTypeCompat(ty, init_type_actual)) {
        EM_error(dec->pos, "variable declaration initializer type error");
        S_enter(venv, dec->u.var.var, E_VarEntry(ty));
        return;
    }

    S_enter(venv, dec->u.var.var, E_VarEntry(ty));
}

void visitFunctionDec(S_table tenv, S_table venv, A_dec dec) {
    for (A_fundecList fundecs = dec->u.function; fundecs != NULL; fundecs = fundecs->tail) {
        A_fundec fundec = fundecs->head;
        Ty_ty result = fundec->result ? requireSym(fundec->pos, tenv, fundec->result, Ty_Void()) : Ty_Void();
        assert(result->kind != Ty_nil);

        Ty_tyList head = NULL, tail = NULL;
        for (A_fieldList fields = fundec->params; fields != NULL; fields = fields->tail) {
            A_field field = fields->head;
            if (head == NULL) {
                head = tail = Ty_TyList(requireSym(fundec->pos, tenv, field->typ, Ty_Int()), NULL);
            } else {
                tail->tail = Ty_TyList(requireSym(fundec->pos, tenv, field->typ, Ty_Int()), NULL);
                tail = tail->tail;
            }
        }

        S_enter(venv, fundec->name, E_FunEntry(head, result));

        S_beginScope(venv);
        tail = head;
        for (A_fieldList fields = fundec->params; fields != NULL; fields = fields->tail) {
            assert(tail);
            S_enter(venv, fields->head->name, E_VarEntry(tail->head));
            tail = tail->tail;
        }
        Ty_ty body_ty = visitExp(tenv, venv, fundec->body).ty;
        if (!isTypeCompat(result, body_ty)) {
            EM_error(fundec->body->pos, "function body expression type does not match the return value type");
        }
        S_endScope(venv);
    }
}

void SEM_transProg(A_exp exp) {
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    Ty_print(visitExp(tenv, venv, exp).ty);
}