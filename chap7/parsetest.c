#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "semant.h"
#include "types.h"
#include <stdlib.h>
#include <symbol.h>
#include <absyn.h>
#include <prabsyn.h>

extern int yyparse(void);

extern A_exp absyn_root;

void parse(string fname) {
    EM_reset(fname);
    if (yyparse() == 0) /* parsing worked */ {
        fprintf(stderr, "Parsing successful!\n");
        SEM_transProg(absyn_root);
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
