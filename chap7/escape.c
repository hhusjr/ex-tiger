/*
 * escape.c - escape analysis
 */

#include <escape.h>
#include "absyn.h"
#include "env.h"
#include <env.h>

static void visitExp(S_table eenv, int level, A_exp exp);

static void visitDec(S_table eenv, int level, A_dec dec);

static void visitVar(S_table eenv, int level, A_var var);

static void visitDec(S_table eenv, int level, A_dec dec) {
    switch (dec->kind) {
        case A_functionDec:
            for (A_fundecList funs = dec->u.function; funs; funs = funs->tail) {
                S_beginScope(eenv);
                for (A_fieldList exps = funs->head->params; exps; exps = exps->tail) {
                    exps->head->escape = FALSE;
                    S_enter(eenv, exps->head->name, E_EscapeEntry(level + 1, &(exps->head->escape)));
                }
                visitExp(eenv, level + 1, funs->head->body);
                S_endScope(eenv);
            }
            break;

        case A_varDec:
            visitExp(eenv, level, dec->u.var.init);
            dec->u.var.escape = FALSE;
            S_enter(eenv, dec->u.var.var, E_EscapeEntry(level, &(dec->u.var.escape)));
            break;

        default:;
    }
}

static void visitExp(S_table eenv, int level, A_exp exp) {
    switch (exp->kind) {
        case A_varExp:
            visitVar(eenv, level, exp->u.var);
            break;

        case A_callExp:
            for (A_expList args = exp->u.call.args; args; args = args->tail) {
                visitExp(eenv, level, args->head);
            }
            break;

        case A_opExp:
            visitExp(eenv, level, exp->u.op.left);
            visitExp(eenv, level, exp->u.op.right);
            break;

        case A_recordExp:
            for (A_efieldList fields = exp->u.record.fields; fields; fields = fields->tail) {
                visitExp(eenv, level, fields->head->exp);
            }
            break;

        case A_seqExp:
            for (A_expList exps = exp->u.seq; exps; exps = exps->tail) {
                visitExp(eenv, level, exps->head);
            }
            break;

        case A_assignExp:
            visitVar(eenv, level, exp->u.assign.var);
            visitExp(eenv, level, exp->u.assign.exp);
            break;

        case A_ifExp:
            visitExp(eenv, level, exp->u.iff.test);
            visitExp(eenv, level, exp->u.iff.then);
            if (exp->u.iff.elsee) {
                visitExp(eenv, level, exp->u.iff.elsee);
            }
            break;

        case A_whileExp:
            visitExp(eenv, level, exp->u.whilee.test);
            visitExp(eenv, level, exp->u.whilee.body);
            break;

        case A_forExp:
            visitExp(eenv, level, exp->u.forr.lo);
            visitExp(eenv, level, exp->u.forr.hi);
            exp->u.forr.escape = FALSE;
            S_beginScope(eenv);
            S_enter(eenv, exp->u.forr.var, E_EscapeEntry(level, &(exp->u.forr.escape)));
            visitExp(eenv, level, exp->u.forr.body);
            S_endScope(eenv);
            break;

        case A_letExp:
            S_beginScope(eenv);
            for (A_decList decs = exp->u.let.decs; decs; decs = decs->tail) {
                visitDec(eenv, level, decs->head);
            }
            visitExp(eenv, level, exp->u.let.body);
            S_endScope(eenv);
            break;

        case A_arrayExp:
            visitExp(eenv, level, exp->u.array.size);
            visitExp(eenv, level, exp->u.array.init);
            break;

        default:;
    }
}

static void visitVar(S_table eenv, int level, A_var var) {
    switch (var->kind) {
        case A_simpleVar: {
            E_enventry esc = S_look(eenv, var->u.simple);
            if (!esc) {
                break;
            }
            assert(esc->kind == E_escapeEntry);
            if (esc->u.escape.level < level) {
                *(esc->u.escape.target) = TRUE;
            }
            break;
        }

        case A_fieldVar:
            visitVar(eenv, level, var->u.field.var);
            break;

        case A_subscriptVar:
            visitVar(eenv, level, var->u.subscript.var);
            break;
    }
}

void Esc_findEscape(A_exp exp) {
    visitExp(E_base_eenv(), 0, exp);
}