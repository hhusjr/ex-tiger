/*
 * frame.h - frame interface
 */

#ifndef TIGER_FRAME
#define TIGER_FRAME

#include <temp.h>
#include <util.h>
#include <tree.h>

typedef struct F_frame_* F_frame;
typedef struct F_access_* F_access;
typedef struct F_accessList_* F_accessList;
struct F_accessList_ { F_access head; F_accessList tail; };

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
extern const int F_wordSize;
extern const int F_maxRegArg;

T_exp F_exp(F_access access, T_exp fp);

T_exp F_externalCall(string s, T_expList args);



#endif //TIGER_FRAME
