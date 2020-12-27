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
#include <escape.h>
#include <translate.h>

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

struct visitorAttrs_ {
    Tr_level level;
    Temp_label done;
};
typedef struct visitorAttrs_ visitorAttrs;

static visitorAttrs VisitorAttrs(Tr_level level, Temp_label done) {
    visitorAttrs p = {
            .level = level,
            .done = done
    };
    return p;
}

static visitorAttrs VisitorAttrs_changeLevel(visitorAttrs attrs, Tr_level level) {
    attrs.level = level;
    return attrs;
}

static visitorAttrs VisitorAttrs_changeDone(visitorAttrs attrs, Temp_label done) {
    attrs.done = done;
    return attrs;
}

static expty visitExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitVar(S_table tenv, S_table venv, A_var var, visitorAttrs attrs, bool *is_loopvar_check);

static expty visitNilExp();

static expty visitIntExp(int val);

static expty visitStringExp(string s);

static expty visitCallExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitOpExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitRecordExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitSeqExp(S_table tenv, S_table venv, A_expList exps, visitorAttrs attrs);

static expty visitAssignExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitIfExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitWhileExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitForExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitBreakExp(A_exp exp, visitorAttrs attrs);

static expty visitLetExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitArrayExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs);

static expty visitDec(S_table tenv, S_table venv, A_dec dec, visitorAttrs attrs);

void visitTypeDec(S_table tenv, S_table venv, A_dec dec);

void visitFunctionDec(S_table tenv, S_table venv, A_dec dec, visitorAttrs attrs);

static expty visitVarDec(S_table tenv, S_table venv, A_dec dec, visitorAttrs attrs);

Ty_ty visitTy(S_table tenv, S_table venv, A_ty ty);

static Ty_ty actualTy(Ty_ty ty) {
    while (ty->kind == Ty_name) {
        ty = ty->u.name.ty;
        assert(ty);
    }
    return ty;
}

static bool isTypeCompat(Ty_ty lhs, Ty_ty rhs) {
    Ty_ty l = actualTy(lhs), r = actualTy(rhs);

    return (l == r && l->kind != Ty_nil && r->kind != Ty_nil)
        ||  (l->kind == Ty_nil && r->kind == Ty_record)
        ||  (l->kind == Ty_record && r->kind == Ty_nil);
}

static expty visitExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    switch (exp->kind) {
        case A_varExp:
            return visitVar(tenv, venv, exp->u.var, attrs, NULL);
        case A_nilExp:
            return visitNilExp();
        case A_intExp:
            return visitIntExp(exp->u.intt);
        case A_stringExp:
            return visitStringExp(exp->u.stringg);
        case A_callExp:
            return visitCallExp(tenv, venv, exp, attrs);
        case A_opExp:
            return visitOpExp(tenv, venv, exp, attrs);
        case A_recordExp:
            return visitRecordExp(tenv, venv, exp, attrs);
        case A_seqExp:
            return visitSeqExp(tenv, venv, exp->u.seq, attrs);
        case A_assignExp:
            return visitAssignExp(tenv, venv, exp, attrs);
        case A_ifExp:
            return visitIfExp(tenv, venv, exp, attrs);
        case A_whileExp:
            return visitWhileExp(tenv, venv, exp, attrs);
        case A_forExp:
            return visitForExp(tenv, venv, exp, attrs);
        case A_breakExp:
            return visitBreakExp(exp, attrs);
        case A_letExp:
            return visitLetExp(tenv, venv, exp, attrs);
        case A_arrayExp:
            return visitArrayExp(tenv, venv, exp, attrs);
        default:
            assert(0);
    }
}

static expty visitVar(S_table tenv, S_table venv, A_var var, visitorAttrs attrs, bool *is_loopvar_check) {
    switch (var->kind) {
        case A_simpleVar: {
            E_enventry e = (E_enventry) requireSym(var->pos, venv, var->u.simple, Ty_Int());
            if (is_loopvar_check) {
                *is_loopvar_check = e->u.var.is_loop_var;
            }
            return Expty(Tr_simpleVar(e->u.var.access, attrs.level), e->u.var.ty);
        }

        case A_fieldVar: {
            expty simple_var = visitVar(tenv, venv, var->u.field.var, attrs, NULL);
            Ty_ty var_t = actualTy(simple_var.ty);
            if (var_t->kind != Ty_record) {
                EM_error(var->pos, "variable must be record type");
                return Expty(Tr_const(0), Ty_Int());
            }

            int pos = 0;
            for (Ty_fieldList p = var_t->u.record; p; p = p->tail) {
                if (p->head->name == var->u.field.sym) {
                    return Expty(Tr_fieldVar(simple_var.exp, pos), p->head->ty);
                }
                pos++;
            }
            EM_error(var->pos, "unknown field %s", var->u.field.sym->name);
            return Expty(Tr_const(0), Ty_Int());
        }

        case A_subscriptVar: {
            expty simple_var = visitVar(tenv, venv, var->u.subscript.var, attrs, NULL);
            expty subscription = visitExp(tenv, venv, var->u.subscript.exp, attrs);
            Ty_ty var_t = actualTy(simple_var.ty);
            if (!isTypeCompat(subscription.ty, Ty_Int())) {
                EM_error(var->u.subscript.exp->pos, "array subscription type must be Integer");
                return Expty(Tr_const(0), Ty_Int());
            }
            if (var_t->kind != Ty_array) {
                EM_error(var->pos, "variable must be array type");
                return Expty(Tr_const(0), Ty_Int());
            }
            return Expty(Tr_subscriptVar(simple_var.exp, subscription.exp), var_t->u.array);
        }

        default:
            assert(0);
    }
}

static expty visitCallExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    E_enventry func = requireSym(exp->pos, venv, exp->u.call.func, NULL);
    if (!func || func->kind != E_funEntry) {
        EM_error(exp->pos, "function name undefined or not a function");
        return Expty(Tr_const(0), Ty_Int());
    }

    A_expList arg = exp->u.call.args;

    Ty_ty result = func->u.fun.result;
    Ty_tyList formal = func->u.fun.formals;

    Tr_expList arg_tr = NULL, arg_tr_tail = NULL;

    while (arg && formal) {
        expty v = visitExp(tenv, venv, arg->head, attrs);

        Ty_ty ideal = formal->head;
        Ty_ty actual = v.ty;

        assert(ideal->kind != Ty_nil);
        if (!isTypeCompat(ideal, actual)) {
            EM_error(arg->head->pos, "function parameter type does not match");
            return Expty(Tr_const(0), Ty_Int());
        }

        if (!arg_tr) {
            arg_tr = arg_tr_tail = Tr_ExpList(v.exp, NULL);
        } else {
            arg_tr_tail->tail = Tr_ExpList(v.exp, NULL);;
            arg_tr_tail = arg_tr_tail->tail;
        }

        formal = formal->tail;
        arg = arg->tail;
    }

    if (arg || formal) {
        EM_error(exp->pos, "parameter passed to the function must have same length with formals in definition");
        return Expty(Tr_const(0), Ty_Int());
    }

    return Expty(Tr_functionCall(exp->u.call.func, func->u.fun.level, attrs.level, arg_tr, result == Ty_Void()), result);
}

static expty visitNilExp() {
    return Expty(Tr_const(0), Ty_Nil());
}

static expty visitIntExp(int val) {
    return Expty(Tr_const(val), Ty_Int());
}

static expty visitStringExp(string s) {
    return Expty(Tr_string(s), Ty_String());
}

static Tr_oper opMap(A_oper op) {
    switch (op) {
        case A_plusOp:
            return Tr_plus;
            
        case A_minusOp:
            return Tr_minus;
            
        case A_timesOp:
            return Tr_times;
            
        case A_divideOp:
            return Tr_divide;
            
        case A_eqOp:
            return Tr_eq;
            
        case A_neqOp:
            return Tr_neq;
            
        case A_ltOp:
            return Tr_lt;
            
        case A_leOp:
            return Tr_le;
            
        case A_gtOp:
            return Tr_gt;
            
        case A_geOp:
            return Tr_ge;

        default: assert(0);
    }
}

static expty visitOpExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    A_exp l = exp->u.op.left, r = exp->u.op.right;
    A_oper op = exp->u.op.oper;

    expty vl = visitExp(tenv, venv, l, attrs), vr = visitExp(tenv, venv, r, attrs);

    switch (op) {
        case A_plusOp:
        case A_minusOp:
        case A_timesOp:
        case A_divideOp: {
            if (!isTypeCompat(vl.ty, Ty_Int())
                || !isTypeCompat(vr.ty, Ty_Int())) {
                EM_error(exp->pos, "both left operand and right operand must be Integer value");
                return Expty(Tr_const(0), Ty_Int());
            }
            return Expty(Tr_op(opMap(op), vl.exp, vr.exp), Ty_Int());
        }

        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp: {
            if (isTypeCompat(vl.ty, Ty_String()) &&
                isTypeCompat(vr.ty, Ty_String())) {
                // TODO: Finish string comparison
                printf("String comparison operator not implemented\n");
                assert(0);
                return Expty(NULL, Ty_Int());
            }
            if (isTypeCompat(vl.ty, Ty_Int()) &&
                isTypeCompat(vr.ty, Ty_Int())) {
                return Expty(Tr_op(opMap(op), vl.exp, vr.exp), Ty_Int());
            }
            EM_error(exp->pos, "both left operand and right operand must be Integer value or String value");
            return Expty(Tr_const(0), Ty_Int());
        }

        case A_eqOp:
        case A_neqOp: {
            if (isTypeCompat(vl.ty, vr.ty)) {
                return Expty(Tr_op(opMap(op), vl.exp, vr.exp), Ty_Int());
            }
            EM_error(exp->pos, "equality test operator can not be applied to these operators");
            return Expty(Tr_const(0), Ty_Int());
        }

        default:
            assert(0);
    }
}

static expty visitAssignExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    A_var lhs = exp->u.assign.var;
    A_exp rhs = exp->u.assign.exp;

    bool is_loop_var = FALSE;
    expty vl = visitVar(tenv, venv, lhs, attrs, &is_loop_var);
    expty vr = visitExp(tenv, venv, rhs, attrs);
    Ty_ty lhs_t = vl.ty;
    Ty_ty rhs_t = vr.ty;

    assert(lhs_t->kind != Ty_nil);
    if (!isTypeCompat(lhs_t, rhs_t)) {
        EM_error(lhs->pos, "lhs and rhs does not have same type");
        return Expty(Tr_nop(), Ty_Void());
    }

    if (is_loop_var) {
        EM_error(lhs->pos, "can not assign to loop variable");
        return Expty(Tr_nop(), Ty_Void());
    }

    return Expty(Tr_assign(vl.exp, vr.exp), Ty_Void());
}

static expty visitIfExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    A_exp test = exp->u.iff.test;
    A_exp then = exp->u.iff.then;
    A_exp elsee = exp->u.iff.elsee;

    expty test_v = visitExp(tenv, venv, test, attrs);

    if (!isTypeCompat(test_v.ty, Ty_Int())) {
        EM_error(test->pos, "test expression must have Integer type");
        return Expty(Tr_nop(), Ty_Void());
    }

    expty then_v = visitExp(tenv, venv, then, attrs);
    Ty_ty then_t = then_v.ty;

    if (elsee == NULL) {
        if (!isTypeCompat(then_t, Ty_Void())) {
            EM_error(then->pos, "then expression must be void");
            return Expty(Tr_nop(), Ty_Void());
        }

        return Expty(Tr_ifthen(test_v.exp, then_v.exp), Ty_Void());
    }

    expty else_v = visitExp(tenv, venv, elsee, attrs);
    Ty_ty else_t = else_v.ty;

    if (!isTypeCompat(then_t, else_t)) {
        EM_error(then->pos, "then and else must have same type (or both void)");
        return Expty(Tr_nop(), Ty_Void());
    }

    return Expty(Tr_ifthenelse(test_v.exp, then_v.exp, else_v.exp), then_t);
}

static expty visitWhileExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    A_exp test = exp->u.whilee.test;
    A_exp body = exp->u.whilee.body;
    expty test_v = visitExp(tenv, venv, test, attrs);

    if (!isTypeCompat(test_v.ty, Ty_Int())) {
        EM_error(test->pos, "test expression must have Integer type");
        return Expty(Tr_nop(), Ty_Void());
    }

    Temp_label done = Temp_newlabel();

    expty body_v = visitExp(tenv, venv, body, VisitorAttrs_changeDone(attrs, done));
    Ty_ty body_ty = body_v.ty;

    if (!isTypeCompat(body_ty, Ty_Void())) {
        EM_error(test->pos, "loop body must be void");
        return Expty(Tr_nop(), Ty_Void());
    }

    return Expty(Tr_while(test_v.exp, body_v.exp, done), Ty_Void());
}

static expty visitForExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    S_symbol id = exp->u.forr.var;
    A_exp lo = exp->u.forr.lo, hi = exp->u.forr.hi;
    A_exp body = exp->u.forr.body;

    expty lo_v = visitExp(tenv, venv, lo, attrs), hi_v = visitExp(tenv, venv, hi, attrs);

    if (!isTypeCompat(lo_v.ty, Ty_Int())) {
        EM_error(lo->pos, "lower bound of for loop must be Integer");
        return Expty(Tr_nop(), Ty_Void());
    }

    if (!isTypeCompat(hi_v.ty, Ty_Int())) {
        EM_error(lo->pos, "upper bound of for loop must be Integer");
        return Expty(Tr_nop(), Ty_Void());
    }

    Tr_access var = Tr_allocLocal(attrs.level, exp->u.forr.escape);

    S_beginScope(venv);
    S_enter(venv, id, E_LoopVarEntry(Ty_Int(), var));

    Temp_label done = Temp_newlabel();

    expty body_v = visitExp(tenv, venv, body, VisitorAttrs_changeDone(attrs, done));
    Ty_ty body_ty = body_v.ty;
    S_endScope(venv);

    if (!isTypeCompat(body_ty, Ty_Void())) {
        EM_error(body->pos, "loop body must be void");
        return Expty(Tr_nop(), Ty_Void());
    }
    
    return Expty(Tr_for(var, attrs.level, lo_v.exp, hi_v.exp, body_v.exp, done), Ty_Void());
}

static expty visitBreakExp(A_exp exp, visitorAttrs attrs) {
    if (!attrs.done) {
        EM_error(exp->pos, "break must be inside a loop");
    }
    return Expty(Tr_break(attrs.done), Ty_Void());
}

static expty visitSeqExp(S_table tenv, S_table venv, A_expList exps, visitorAttrs attrs) {
    expty stm_v = visitExp(tenv, venv, exps->head, attrs);
    if (!exps->tail) {
        return stm_v;
    }

    expty exp_v = visitSeqExp(tenv, venv, exps->tail, attrs);
    return Expty(Tr_seq(stm_v.exp, exp_v.exp), exp_v.ty);
}

static expty visitArrayExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    Ty_ty ty = requireSym(exp->pos, tenv, exp->u.array.typ, Ty_Void());
    Ty_ty actual_ty = actualTy(ty);
    if (actual_ty->kind != Ty_array) {
        EM_error(exp->pos, "array name not defined or not an array");
        return Expty(Tr_nop(), Ty_Void());
    }

    A_exp size = exp->u.array.size;
    A_exp init = exp->u.array.init;

    expty vsize = visitExp(tenv, venv, size, attrs), vinit = visitExp(tenv, venv, init, attrs);

    if (!isTypeCompat(vsize.ty, Ty_Int())) {
        EM_error(size->pos, "array size must be Integer");
        return Expty(Tr_const(0), ty);
    }

    if (!isTypeCompat(vinit.ty, actual_ty->u.array)) {
        EM_error(init->pos, "array initializer must be the same type as array elements");
        return Expty(Tr_const(0), ty);
    }

    return Expty(Tr_newArray(vsize.exp, vinit.exp), ty);
}

struct fieldAndInitializer_ {
    Ty_field field;
    Tr_exp initializer;
};
typedef struct fieldAndInitializer_* fieldAndInitializer;
fieldAndInitializer FieldAndInitializer(Ty_field field, Tr_exp initializer) {
    fieldAndInitializer p = checked_malloc(sizeof(*p));
    p->field = field;
    p->initializer = initializer;
    return p;
}

static expty visitRecordExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    Ty_ty ty = requireSym(exp->pos, tenv, exp->u.record.typ, NULL);
    Ty_ty actual_ty = actualTy(ty);
    if (actual_ty->kind != Ty_record) {
        EM_error(exp->pos, "record name not defined or not a record");
        return Expty(Tr_nop(), Ty_Void());
    }

    S_table fieldset = S_empty(); // Map field to initializer
    for (Ty_fieldList field_list = actual_ty->u.record; field_list; field_list = field_list->tail) {
        S_enter(fieldset, field_list->head->name, FieldAndInitializer(field_list->head, Tr_const(0)));
    }

    for (A_efieldList efield_list = exp->u.record.fields; efield_list; efield_list = efield_list->tail) {
        A_efield efield = efield_list->head;

        S_symbol symbol = efield->name;
        fieldAndInitializer field = S_look(fieldset, symbol);

        if (field == NULL) {
            EM_error(exp->pos, "field %s not found", symbol->name);
            return Expty(Tr_const(0), ty);
        }

        expty vis_exp = visitExp(tenv, venv, efield->exp, attrs);
        if (!isTypeCompat(field->field->ty, vis_exp.ty)) {
            EM_error(exp->pos, "field %s has wrong type", symbol->name);
            return Expty(Tr_const(0), ty);
        }

        field->initializer = vis_exp.exp;
    }

    int n = 0;
    Tr_expList initializers = NULL, initializers_tail = NULL;
    for (Ty_fieldList field_list = actual_ty->u.record; field_list; field_list = field_list->tail) {
        fieldAndInitializer field = S_look(fieldset, field_list->head->name);
        assert(field);
        Tr_expList p = Tr_ExpList(field->initializer, NULL);
        if (!initializers) {
            initializers = initializers_tail = p;
        } else {
            initializers_tail->tail = p;
            initializers_tail = p;
        }
        n++;
    }

    return Expty(Tr_newRecord(n, initializers), ty);
}

static expty visitLetExp(S_table tenv, S_table venv, A_exp exp, visitorAttrs attrs) {
    S_beginScope(venv);
    S_beginScope(tenv);

    {
        // merge type declarations
        A_nametyList cur = NULL;
        for (A_decList dec_list = exp->u.let.decs; dec_list && dec_list->tail;) {
            if (dec_list->head->kind == A_typeDec
                && dec_list->tail->head->kind == A_typeDec) {
                if (!cur) {
                    cur = dec_list->head->u.type;
                }
                cur->tail = dec_list->tail->head->u.type;
                cur = cur->tail;
                dec_list->tail = dec_list->tail->tail;
            } else {
                cur = NULL;
                dec_list = dec_list->tail;
            }
        }
    }

    {
        // merge function declarations
        A_fundecList cur = NULL;
        for (A_decList dec_list = exp->u.let.decs; dec_list && dec_list->tail;) {
            if (dec_list->head->kind == A_functionDec
                && dec_list->tail->head->kind == A_functionDec) {
                if (!cur) {
                    cur = dec_list->head->u.function;
                }
                cur->tail = dec_list->tail->head->u.function;
                cur = cur->tail;
                dec_list->tail = dec_list->tail->tail;
            } else {
                cur = NULL;
                dec_list = dec_list->tail;
            }
        }
    }

    Tr_exp init_stm = NULL;

    for (A_decList dec_list = exp->u.let.decs; dec_list; dec_list = dec_list->tail) {
        A_dec dec = dec_list->head;
        Tr_exp init = visitDec(tenv, venv, dec, attrs).exp;
        if (!init) {
            // For functions/types/variable declaration without initialization, skip generate initializer
            continue;
        }

        init_stm = init_stm ? Tr_stmtSeq(init_stm, init) : init;
    }
    expty vis_body = visitExp(tenv, venv, exp->u.let.body, attrs);
    S_endScope(venv);
    S_endScope(tenv);
    return Expty(init_stm ? Tr_seq(init_stm, vis_body.exp) : vis_body.exp, vis_body.ty);
}

static expty visitDec(S_table tenv, S_table venv, A_dec dec, visitorAttrs attrs) {
    switch (dec->kind) {
        case A_functionDec:
            visitFunctionDec(tenv, venv, dec, attrs);
            return Expty(NULL, NULL);

        case A_varDec:
            return visitVarDec(tenv, venv, dec, attrs);

        case A_typeDec:
            visitTypeDec(tenv, venv, dec);
            return Expty(NULL, NULL);

        default:
            assert(0);
    }
}

void visitTypeDec(S_table tenv, S_table venv, A_dec dec) {
    for (A_nametyList list = dec->u.type; list; list = list->tail) {
        S_enter(tenv, list->head->name, Ty_Name(list->head->name, NULL));
    }

    for (A_nametyList list = dec->u.type; list; list = list->tail) {
        Ty_ty name = S_look(tenv, list->head->name);
        assert(name->kind == Ty_name && name->u.name.ty == NULL);
        name->u.name.ty = visitTy(tenv, venv, list->head->ty);
    }
}

Ty_ty visitTy(S_table tenv, S_table venv, A_ty ty) {
    switch (ty->kind) {
        case A_nameTy:
            return Ty_Name(ty->u.name, requireSym(ty->pos, tenv, ty->u.name, Ty_Int()));

        case A_recordTy: {
            Ty_fieldList list_head = NULL, list_tail = NULL;
            for (A_fieldList fields = ty->u.record; fields; fields = fields->tail) {
                A_field field = fields->head;
                Ty_ty field_type = requireSym(field->pos, tenv, field->typ, Ty_Int());
                if (list_head == NULL) {
                    list_head = list_tail = Ty_FieldList(Ty_Field(field->name, field_type), NULL);
                } else {
                    list_tail->tail = Ty_FieldList(Ty_Field(field->name, field_type), NULL);
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

static expty visitVarDec(S_table tenv, S_table venv, A_dec dec, visitorAttrs attrs) {
    // Allocate local variable
    Tr_access access = Tr_allocLocal(attrs.level, dec->u.var.escape);

    expty init = visitExp(tenv, venv, dec->u.var.init, attrs);
    Ty_ty init_type = init.ty;
    Ty_ty init_type_actual = actualTy(init_type);
    Ty_ty ty;
    if (dec->u.var.typ == NULL) {
        if (init_type_actual->kind == Ty_void || init_type_actual->kind == Ty_nil) {
            EM_error(dec->pos, "variable declaration initializer type error");
            S_enter(venv, dec->u.var.var, E_VarEntry(Ty_Int(), access));
            return Expty(NULL, NULL);
        }
        ty = init_type;
    } else {
        ty = requireSym(dec->pos, tenv, dec->u.var.typ, init_type_actual);
    }

    if (!isTypeCompat(ty, init_type_actual)) {
        EM_error(dec->pos, "variable declaration initializer type error");
        S_enter(venv, dec->u.var.var, E_VarEntry(ty, access));
        return Expty(NULL, NULL);
    }

    S_enter(venv, dec->u.var.var, E_VarEntry(ty, access));
    return Expty(Tr_assign(Tr_simpleVar(access, attrs.level), init.exp), ty);
}

void visitFunctionDec(S_table tenv, S_table venv, A_dec dec, visitorAttrs attrs) {
    for (A_fundecList fundecs = dec->u.function; fundecs; fundecs = fundecs->tail) {
        A_fundec fundec = fundecs->head;
        Ty_ty result = fundec->result ? requireSym(fundec->pos, tenv, fundec->result, Ty_Void()) : Ty_Void();
        assert(result->kind != Ty_nil);

        // Calculate type of each formals
        Ty_tyList head = NULL, tail = NULL;
        for (A_fieldList fields = fundec->params; fields; fields = fields->tail) {
            A_field field = fields->head;
            if (head == NULL) {
                head = tail = Ty_TyList(requireSym(fundec->pos, tenv, field->typ, Ty_Int()), NULL);
            } else {
                tail->tail = Ty_TyList(requireSym(fundec->pos, tenv, field->typ, Ty_Int()), NULL);
                tail = tail->tail;
            }
        }

        // Convert formal escape info to a U_BoolList
        A_fieldList fields = fundec->params;
        U_boolList formals = NULL, formals_tail = NULL;
        for (; fields; fields = fields->tail) {
            if (!formals) {
                formals = formals_tail = U_BoolList(fields->head->escape, NULL);
            } else {
                formals_tail->tail = U_BoolList(fields->head->escape, NULL);
                formals_tail = formals_tail->tail;
            }
        }

        // Create activation record
        Tr_level lv = Tr_newLevel(attrs.level, Temp_newlabel(), formals);

        // Put it to symbol table
        S_enter(venv, fundec->name, E_FunEntry(head, result, lv));
    }

    for (A_fundecList fundecs = dec->u.function; fundecs; fundecs = fundecs->tail) {
        A_fundec fundec = fundecs->head;

        E_enventry e = S_look(venv, fundec->name);
        assert(e && e->kind == E_funEntry);

        // Put each formals to symbol table (their types, names, and accesses)
        Tr_accessList formal_access = Tr_formals(e->u.fun.level);
        A_fieldList fields;
        Ty_tyList p = e->u.fun.formals;
        for (fields = fundec->params; fields; fields = fields->tail) {
            assert(formal_access && p);
            S_enter(venv, fields->head->name, E_VarEntry(p->head, formal_access->head));
            formal_access = formal_access->tail;
            p = p->tail;
        }
        assert(!formal_access && !fields && !p);

        // Calculate function body
        S_beginScope(venv);
        expty body_v = visitExp(tenv, venv, fundec->body, VisitorAttrs_changeLevel(attrs, e->u.fun.level));
        Ty_ty body_ty = body_v.ty;
        if (!isTypeCompat(e->u.fun.result, body_ty)) {
            EM_error(fundec->body->pos, "function body expression type does not match the return value type");
        }
        S_endScope(venv);

        // Proc Entry Exit
        Tr_procEntryExit(e->u.fun.level, body_v.exp, formal_access);
    }
}

void SEM_transProg(A_exp exp) {
    Esc_findEscape(exp);
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    Tr_level main_level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("main"), NULL);
    Tr_procEntryExit(main_level, visitExp(tenv, venv, exp, VisitorAttrs(main_level, NULL)).exp, NULL);
}
