/*
 * translate.c - translate code to IR
 */

#include <translate.h>
#include <frame.h>

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

static struct Tr_level_ troutmost = {NULL, NULL};

Tr_access Tr_Access(Tr_level level, F_access access) {
    Tr_access p = checked_malloc(sizeof(*p));
    p->level = level;
    p->access = access;
    return p;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

Tr_level Tr_outermost() { return &troutmost; }

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    Tr_level p = checked_malloc(sizeof(*p));
    p->parent = parent;
    p->frame = F_newFrame(name, U_BoolList(TRUE, formals));
    p->formals = NULL;
    Tr_accessList tail = NULL;
    for (F_accessList f_formals = F_formals(p->frame); f_formals; f_formals = f_formals->tail) {
        Tr_accessList entry =  Tr_AccessList(Tr_Access(p, f_formals->head), NULL);
        if (!p->formals) {
            p->formals = tail = entry;
        } else {
            tail->tail = entry;
            tail = entry;
        }
    }
    return p;
}

Tr_accessList Tr_formals(Tr_level level) { return level->formals; }

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    return Tr_Access(level, F_allocLocal(level->frame, escape));
}
