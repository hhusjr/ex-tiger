#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "semant.h"
#include "types.h"
#include "frame.h"
#include <stdlib.h>
#include <symbol.h>
#include <absyn.h>
#include <prabsyn.h>
#include <translate.h>
#include <printtree.h>

extern int yyparse(void);

extern A_exp absyn_root;

void parse(string fname) {
    EM_reset(fname);
    if (yyparse() == 0) /* parsing worked */ {
        fprintf(stderr, "Parsing successful!\n");
        SEM_transProg(absyn_root);
        F_fragList frags = Tr_getResult();
        for (; frags; frags = frags->tail) {
            printf(" =====================================================================\n");
            switch (frags->head->kind) {
                case F_stringFrag:
                    printf("String fragment %s\n", frags->head->u.stringg.label->name);
                    printf("%s\n", frags->head->u.stringg.str);
                    break;

                case F_procFrag:
                    printf("Proc fragment %s\n", frags->head->u.proc.frame->name->name);
                    printStmList(stdout, T_StmList(frags->head->u.proc.body, NULL));
                    break;
            }
        }
        pr_exp(fopen("ast.out", "w"), absyn_root, 4);
    } else fprintf(stderr, "Parsing failed\n");
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: a.out filename\n");
        exit(1);
    }
    parse(argv[1]);
    return 0;
}
