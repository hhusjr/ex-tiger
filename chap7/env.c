#include "env.h"
#include "util.h"
#include <stdlib.h>

E_enventry E_VarEntry(Ty_ty ty, Tr_access access) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind = E_varEntry;
    p->u.var.ty = ty;
    p->u.var.access = access;
    p->u.var.is_loop_var = FALSE;
    return p;
}

E_enventry E_LoopVarEntry(Ty_ty ty, Tr_access access) {
    E_enventry p = E_VarEntry(ty, access);
    p->u.var.is_loop_var = TRUE;
    return p;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result, Tr_level level) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind = E_funEntry;
    p->u.fun.formals = formals;
    p->u.fun.result = result;
    p->u.fun.level = level;
    return p;
}

E_enventry E_EscapeEntry(int level, bool *target) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind = E_escapeEntry;
    p->u.escape.level = level;
    p->u.escape.target = target;
    return p;
}

S_table E_base_tenv() {
    S_table t = S_empty();
    S_enter(t, S_Symbol("int"), Ty_Int());
    S_enter(t, S_Symbol("string"), Ty_String());
    return t;
}

S_table E_base_venv() {
    S_table t = S_empty();
    S_enter(t, S_Symbol("print"), E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Void(), NULL));
    S_enter(t, S_Symbol("getchar"), E_FunEntry(NULL, Ty_String(), NULL));
    S_enter(t, S_Symbol("ord"), E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Int(), NULL));
    S_enter(t, S_Symbol("chr"), E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_String(), NULL));
    return t;
}

S_table E_base_eenv() {
    S_table t = S_empty();
    return t;
}
