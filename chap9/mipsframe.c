/*
 * mipsframe.c - MIPS stack frame (implements frame.h interface)
 */

/*
 * Stack frame layout
 *                          High address
 *      Argn                                    |
 *      Argn-1                                  |
 *      ...                                     |   Old stack frame
 *      Arg1                                    |
 *      Static link         <- Frame pointer    |
 *      Localvar1                               +
 *      Localvar2                               +
 *      ...                                     +
 *      Localvarn                               +
 *      Return address                          +
 *      Tempvar1                                +
 *      Tempvar2                                +
 *      ...                                     +   New stack frame
 *      Tempvarn                                +
 *      Saved registers                         +
 *      Argn                                    +
 *      Argn-1                                  +
 *      ...                                     +
 *      Arg1                                    +
 *      Static link         <- Stack pointer    +
 */

#include "frame.h"
#include "temp.h"

static struct Temp_temp_ f_fp = {1}, f_sp = {2}, f_at = {3}, f_rv = {4}, f_zero = {5};
static struct Temp_temp_ f_a0 = {6}, f_a1 = {7}, f_a2 = {8}, f_a3 = {9};

const int F_wordSize = 4;
const int F_maxRegArg = 4;

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

struct F_access_ {
    enum {
        inFrame, inReg
    } kind;

    union {
        int offset;         // inFrame
        Temp_temp reg;      // inReg
    };
};

static F_access InFrame(int offset) {
    F_access access = checked_malloc(sizeof(*access));
    access->kind = inFrame;
    access->offset = offset;
    return access;
}

static F_access InReg(Temp_temp reg) {
    F_access access = checked_malloc(sizeof(*access));
    access->kind = inReg;
    access->reg = reg;
    return access;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame frame = checked_malloc(sizeof(*frame));
    frame->name = name;
    frame->formals = NULL;
    F_accessList tail = NULL;
    int offset = -F_wordSize;
    for (U_boolList p = formals; p; p = p->tail) {
        F_accessList entry = F_AccessList(p->head ? InFrame(offset += F_wordSize)
                                                        : InReg(Temp_newtemp()), NULL);
        if (!frame->formals) {
            frame->formals = tail = entry;
        } else {
            tail->tail = entry;
            tail = entry;
        }
    }
    return frame;
}

F_access F_allocLocal(F_frame f, bool escape) {
    if (escape) {
        return InFrame(-(1 + f->n_frame_local++) * F_wordSize);
    }

    return InReg(Temp_newtemp());
}

Temp_label F_name(F_frame frame) {
    return frame->name;
}

F_accessList F_formals(F_frame frame) {
    return frame->formals;
}

Temp_temp F_FP() {
    return &f_fp;
}

Temp_temp F_RV() {
    return &f_rv;
}

Temp_temp F_AT() {
    return &f_at;
}

Temp_temp F_SP() {
    return &f_sp;
}

Temp_temp F_ZERO() {
    return &f_zero;
}

T_exp F_exp(F_access access, T_exp fp) {
    return access->kind == inFrame ? T_Mem(T_Binop(T_plus, fp, T_Const(access->offset))) // inframe
                                   : T_Temp(access->reg); // inreg
}

T_exp F_externalCall(string s, T_expList args) {
    return T_Call(T_Name(Temp_namedlabel(s)), args);
}

F_frag F_StringFrag(Temp_label label, string str) {
    F_frag p = checked_malloc(sizeof(*p));
    p->kind = F_stringFrag;
    p->u.stringg.label = label;
    p->u.stringg.str = str;
    return p;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
    F_frag p = checked_malloc(sizeof(*p));
    p->kind = F_procFrag;
    p->u.proc.body = body;
    p->u.proc.frame = frame;
    return p;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
    F_fragList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
    // TODO: Finish this
    return stm;
}

Temp_map F_TempMap() {
    Temp_map map = Temp_empty();
    Temp_enter(map, F_FP(), "$fp");
    Temp_enter(map, F_AT(), "$at");
    Temp_enter(map, F_ZERO(), "$zero");
    Temp_enter(map, F_RV(), "$v0");
    Temp_enter(map, &f_a0, "$a0");
    Temp_enter(map, &f_a1, "$a1");
    Temp_enter(map, &f_a2, "$a2");
    Temp_enter(map, &f_a3, "$a3");
    return map;
}

Temp_tempList F_Argregs() {
    return Temp_TempList(&f_a0, Temp_TempList(&f_a1, Temp_TempList(&f_a2, Temp_TempList(&f_a3, NULL))));
}