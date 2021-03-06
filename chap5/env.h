#include "symbol.h"
#include "types.h"

#ifndef TIGER_ENV
#define TIGER_ENV

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
    enum {
        E_varEntry, E_funEntry
    } kind;
    union {
        struct {
            Ty_ty ty;
            _Bool is_loop_var;
        } var;
        struct {
            Ty_tyList formals;
            Ty_ty result;
        } fun;
    } u;
};

E_enventry E_VarEntry(Ty_ty ty);

E_enventry E_LoopVarEntry(Ty_ty ty);

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result);

S_table E_base_tenv();

S_table E_base_venv();

#endif
