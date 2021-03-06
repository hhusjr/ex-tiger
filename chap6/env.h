#include "symbol.h"
#include "types.h"

#ifndef TIGER_ENV
#define TIGER_ENV

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
    enum {
        E_varEntry, E_funEntry, E_escapeEntry
    } kind;
    union {
        struct {
            Ty_ty ty;
            bool is_loop_var;
        } var;
        struct {
            Ty_tyList formals;
            Ty_ty result;
        } fun;
        struct {
            int level;
            bool* target;
        } escape;
    } u;
};

E_enventry E_VarEntry(Ty_ty ty);

E_enventry E_LoopVarEntry(Ty_ty ty);

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result);

E_enventry E_EscapeEntry(int level, bool* target);

S_table E_base_tenv();

S_table E_base_venv();

S_table E_base_eenv();

#endif
