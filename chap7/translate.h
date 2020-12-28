/*
 * translate.h - translate code to IR
 */

#ifndef TIGER_TRANSLATE
#define TIGER_TRANSLATE

#include "temp.h"
#include "frame.h"

typedef struct Tr_access_ *Tr_access;
typedef struct Tr_accessList_ *Tr_accessList;
typedef struct Tr_level_ *Tr_level;

struct Tr_level_ {
    Tr_level parent;
    F_frame frame;
    Tr_accessList formals;
};

struct Tr_access_ {
    Tr_level level;
    F_access access;
};

struct Tr_accessList_ {
    Tr_access head;
    Tr_accessList tail;
};

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_expList_ *Tr_expList;

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

typedef enum {
    Tr_plus, Tr_minus, Tr_times, Tr_divide,
    Tr_eq, Tr_neq, Tr_lt, Tr_le, Tr_gt, Tr_ge
} Tr_oper;

typedef struct patchList_ *patchList;
typedef struct Cx_ Cx;

struct Cx_ {
    patchList trues, falses;
    T_stm stm;
};

struct patchList_ {
    Temp_label *head;
    patchList tail;
};

struct Tr_exp_ {
    enum {
        Tr_ex, Tr_nx, Tr_cx
    } kind;

    union {
        T_exp ex;
        T_stm nx;
        Cx cx;
    } u;
};

struct Tr_expList_ {
    Tr_exp head;
    Tr_expList tail;
};

Tr_exp Tr_assign(Tr_exp lhs, Tr_exp rhs);

Tr_exp Tr_functionCall(S_symbol name, Tr_level callee, Tr_level caller, Tr_expList args, bool is_proc);

Tr_exp Tr_for(Tr_access var, Tr_level cur_level, Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done);

Tr_exp Tr_while(Tr_exp test, Tr_exp body, Temp_label done);

Tr_exp Tr_newArray(Tr_exp n, Tr_exp initializer);

Tr_exp Tr_newRecord(int n_field, Tr_expList initializers);

Tr_exp Tr_ifthen(Tr_exp test, Tr_exp then);

Tr_exp Tr_ifthenelse(Tr_exp test, Tr_exp then, Tr_exp elsee);

Tr_exp Tr_op(Tr_oper op, Tr_exp l, Tr_exp r);

Tr_exp Tr_fieldVar(Tr_exp var, int field_idx);

Tr_exp Tr_subscriptVar(Tr_exp var, Tr_exp idx);

Tr_exp Tr_nop();

Tr_exp Tr_simpleVar(Tr_access access, Tr_level cur_level);

Tr_exp Tr_const(int n);

Tr_exp Tr_string(string s);

Tr_exp Tr_strCmp(Tr_oper op, Tr_exp l, Tr_exp r);

Tr_exp Tr_break(Temp_label done);

Tr_exp Tr_stmtSeq(Tr_exp left, Tr_exp right);

Tr_exp Tr_seq(Tr_exp stm, Tr_exp res);

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);

F_fragList Tr_getResult();

#endif //TIGER_TRANSLATE
