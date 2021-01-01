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
#include "assem.h"

static struct Temp_temp_ f_ra = {0}, f_fp = {1}, f_sp = {2}, f_at = {3}, f_rv = {4}, f_zero = {5};
static struct Temp_temp_ f_a0 = {6}, f_a1 = {7}, f_a2 = {8}, f_a3 = {9};
static struct Temp_temp_ f_s0 = {10}, f_s1 = {11}, f_s2 = {12}, f_s3 = {13},
                         f_s4 = {14}, f_s5 = {15}, f_s6 = {16}, f_s7 = {17};
static struct Temp_temp_ f_t0 = {18}, f_t1 = {19}, f_t2 = {20}, f_t3 = {21},
                         f_t4 = {22}, f_t5 = {23}, f_t6 = {24}, f_t7 = {25},
                         f_t8 = {26}, f_t9 = {27};

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

Temp_temp F_RA() {
    return &f_ra;
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

AS_instrList F_procEntryExit2(AS_instrList body) {
    // "Sink" instruction
    static Temp_tempList returnSink = NULL;
    if (!returnSink) {
        returnSink = F_Specialregs();
        Temp_tempList tail = returnSink;
        assert(tail); // Special regs will not be empty
        for (; tail->tail; tail = tail->tail);
        tail->tail = F_Calleesaves();
    }
    return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));
}

Temp_map F_TempMap() {
    Temp_map map = Temp_empty();
    Temp_enter(map, F_FP(), "$fp");
    Temp_enter(map, F_SP(), "$sp");
    Temp_enter(map, F_AT(), "$at");
    Temp_enter(map, F_ZERO(), "$zero");
    Temp_enter(map, F_RV(), "$v0");
    Temp_enter(map, F_RA(), "$ra");

    Temp_enter(map, &f_a0, "$a0");
    Temp_enter(map, &f_a1, "$a1");
    Temp_enter(map, &f_a2, "$a2");
    Temp_enter(map, &f_a3, "$a3");
    Temp_enter(map, &f_s0, "$s0");
    Temp_enter(map, &f_s1, "$s1");
    Temp_enter(map, &f_s2, "$s2");
    Temp_enter(map, &f_s3, "$s3");
    Temp_enter(map, &f_s4, "$s4");
    Temp_enter(map, &f_s5, "$s5");
    Temp_enter(map, &f_s6, "$s6");
    Temp_enter(map, &f_s7, "$s7");
    Temp_enter(map, &f_t0, "$t0");
    Temp_enter(map, &f_t1, "$t1");
    Temp_enter(map, &f_t2, "$t2");
    Temp_enter(map, &f_t3, "$t3");
    Temp_enter(map, &f_t4, "$t4");
    Temp_enter(map, &f_t5, "$t5");
    Temp_enter(map, &f_t6, "$t6");
    Temp_enter(map, &f_t7, "$t7");
    Temp_enter(map, &f_t8, "$t8");
    Temp_enter(map, &f_t9, "$t9");
    return map;
}

Temp_tempList F_Argregs() {
    static Temp_tempList argregs = NULL;
    if (!argregs) {
        argregs = Temp_TempList(&f_a0, Temp_TempList(&f_a1, Temp_TempList(&f_a2, Temp_TempList(&f_a3, NULL))));
    }
    return argregs;
}

Temp_tempList F_Calleesaves() {
    static Temp_tempList calleesaves = NULL;
    if (!calleesaves) {
        calleesaves = Temp_TempList(&f_s0, Temp_TempList(&f_s1, Temp_TempList(&f_s2, Temp_TempList(&f_s3,
                                                                                                   Temp_TempList(&f_s4, Temp_TempList(&f_s5, Temp_TempList(&f_s6, Temp_TempList(&f_s7, NULL))))))));
    }
    return calleesaves;
}

Temp_tempList F_Callersaves() {
    static Temp_tempList callersaves = NULL;
    if (!callersaves) {
        callersaves = Temp_TempList(&f_t0, Temp_TempList(&f_t1, Temp_TempList(&f_t2, Temp_TempList(&f_t3,
                                                                                                   Temp_TempList(&f_t4, Temp_TempList(&f_t5, Temp_TempList(&f_t6, Temp_TempList(&f_t7,
                                                                                                                                                                                Temp_TempList(&f_t8, Temp_TempList(&f_t9, NULL))))))))));
    }
    return callersaves;
}

Temp_tempList F_Specialregs() {
    static Temp_tempList specialregs = NULL;
    if (!specialregs) {
        specialregs = Temp_TempList(&f_ra, Temp_TempList(&f_fp, Temp_TempList(&f_sp, Temp_TempList(&f_at, Temp_TempList(&f_rv, Temp_TempList(&f_zero, NULL))))));
    }
    return specialregs;
}
