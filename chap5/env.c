#include "env.h"
#include "util.h"
#include <stdlib.h>

E_enventry E_VarEntry(Ty_ty ty) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind = E_varEntry;
    p->u.var.ty = ty;
    return p;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind = E_funEntry;
    p->u.fun.formals = formals;
    p->u.fun.result = result;
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
    S_enter(t, S_Symbol("print"), E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Void()));
    return t;
}
