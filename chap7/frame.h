/*
 * frame.h - frame interface
 */

#ifndef TIGER_FRAME
#define TIGER_FRAME

#include <temp.h>
#include <util.h>
#include <tree.h>

typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {
    F_access head;
    F_accessList tail;
};

struct F_frame_ {
    Temp_label name;
    int n_frame_local;
    F_accessList formals;
};

F_frame F_newFrame(Temp_label name, U_boolList formals);

Temp_label F_name(F_frame frame);

F_accessList F_formals(F_frame frame);

F_access F_allocLocal(F_frame f, bool escape);

Temp_temp F_FP();

Temp_temp F_RV();

extern const int F_wordSize;
extern const int F_maxRegArg;

T_exp F_exp(F_access access, T_exp fp);

T_exp F_externalCall(string s, T_expList args);

typedef struct F_frag_ *F_frag;
struct F_frag_ {
    enum {
        F_stringFrag, F_procFrag
    } kind;

    union {
        struct {
            Temp_label label;
            string str;
        } stringg;

        struct {
            T_stm body;
            F_frame frame;
        } proc;
    } u;
};

F_frag F_StringFrag(Temp_label label, string str);

F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {
    F_frag head;
    F_fragList tail;
};

T_stm F_procEntryExit1(F_frame frame, T_stm stm);

F_fragList F_FragList(F_frag head, F_fragList tail);

#endif //TIGER_FRAME
