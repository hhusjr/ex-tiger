#ifndef TIGER_ENV
#define TIGER_ENV

#include "symbol.h"
#include "types.h"
#include "translate.h"

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
    enum {
        E_varEntry, E_funEntry, E_escapeEntry
    } kind;
    union {
        struct {
            Ty_ty ty;
            bool is_loop_var;
            Tr_access access;
        } var;
        struct {
            Ty_tyList formals;
            Ty_ty result;
            Tr_level level;
        } fun;
        struct {
            int level;
            bool *target;
        } escape;
    } u;
};

E_enventry E_VarEntry(Ty_ty ty, Tr_access access);

E_enventry E_LoopVarEntry(Ty_ty ty, Tr_access access);

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result, Tr_level level);

E_enventry E_EscapeEntry(int level, bool *target);

S_table E_base_tenv();

S_table E_base_venv();

S_table E_base_eenv();

#endif
