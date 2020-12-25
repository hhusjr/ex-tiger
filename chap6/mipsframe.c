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

#include <frame.h>

#define ARCH_WORD_SIZE (4)
#define ARCH_MAXREGARG (4)

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

struct F_access_ {
    enum {inFrame, inReg} kind;

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
    frame->formals = NULL;
    F_accessList tail = NULL;
    int offset = -ARCH_WORD_SIZE;
    int n_reg_alloca = 0;
    for (U_boolList p = formals; p; p = p->tail) {
        assert(n_reg_alloca <= ARCH_MAXREGARG);
        bool in_frame = p->head || n_reg_alloca >= ARCH_MAXREGARG;
        F_accessList entry = F_AccessList(in_frame ? InFrame(offset += ARCH_WORD_SIZE)
                                                   : (n_reg_alloca++, InReg(Temp_newtemp())), NULL);
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
        return InFrame(-(1 + f->n_frame_local++) * ARCH_WORD_SIZE);
    }

    return InReg(Temp_newtemp());
}

Temp_label F_name(F_frame frame) {
    return frame->name;
}

F_accessList F_formals(F_frame frame) {
    return frame->formals;
}
