/*
 * errormsg.c - functions used in all phases of the compiler to give
 *              error messages about the Tiger program.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "util.h"
#include "errormsg.h"


bool anyErrors= FALSE;

static string fileName = "";

int EM_tokPos=0;
int EM_linePos = 0;

extern FILE *yyin;

void EM_error(int pos, char *message,...)
{va_list ap;

  anyErrors=TRUE;
  if (fileName) fprintf(stderr,"%s:",fileName);
  if (EM_linePos) fprintf(stderr,"%d.%d: ", EM_linePos, EM_tokPos);
  va_start(ap,message);
  vfprintf(stderr, message, ap);
  va_end(ap);
  fprintf(stderr,"\n");

}

void EM_reset(string fname)
{
 anyErrors=FALSE; fileName=fname;
 yyin = fopen(fname,"r");
 if (!yyin) {EM_error(0,"cannot open"); exit(1);}
}
