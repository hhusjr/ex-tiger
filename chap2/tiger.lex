%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

#define QS_BUF_SIZE 1024

int charPos=1;
 int linePos = 1;

int yywrap(void)
{
 charPos=1;
 return 1;
}


void adjust(int line_brk)
{
  EM_tokPos = charPos;
  EM_linePos = linePos;

  if (line_brk) {
    charPos = 1;
    linePos++;
  } else {
    charPos += yyleng;
  }
}

char qs_buf[QS_BUF_SIZE];
int qs_pos;
int comment_level = 0;

%}

digit [0-9]
alpha [a-zA-Z]
white [ \t\n\v\f\r]
%s INTIIAL S_STRING S_COMMENT S_QS_ESP

%%

 /* Comment */
<INITIAL>"/*" { adjust(0); BEGIN S_COMMENT; comment_level++; }
<S_COMMENT>"/*" { adjust(0); comment_level++; }
<S_COMMENT>"*/" { adjust(0); if (!--comment_level) BEGIN INITIAL; }
<S_COMMENT>. { adjust(0); }
<S_COMMENT>\n { adjust(1); }

 /* Symbols and keywords */
<INITIAL>"," { adjust(0); return COMMA; }
<INITIAL>":" { adjust(0); return COLON; }
<INITIAL>";" { adjust(0); return SEMICOLON; }
<INITIAL>"(" { adjust(0); return LPAREN; }
<INITIAL>")" { adjust(0); return RPAREN; }
<INITIAL>"[" { adjust(0); return LBRACK; }
<INITIAL>"]" { adjust(0); return RBRACK;}
<INITIAL>"{" { adjust(0); return LBRACE; }
<INITIAL>"}" { adjust(0); return RBRACE; }
<INITIAL>"." { adjust(0); return DOT; }
<INITIAL>"+" { adjust(0); return PLUS; }
<INITIAL>"-" { adjust(0); return MINUS; }
<INITIAL>"*" { adjust(0); return TIMES; }
<INITIAL>"/" { adjust(0); return DIVIDE; }
<INITIAL>"=" { adjust(0); return EQ; }
<INITIAL>":=" { adjust(0); return ASSIGN; }
<INITIAL>"!=" { adjust(0); return NEQ; }
<INITIAL>"<" { adjust(0); return LT; }
<INITIAL>">" { adjust(0); return GT; }
<INITIAL>"&" { adjust(0); return AND; }
<INITIAL>"|" { adjust(0); return OR; }
<INITIAL>"array" { adjust(0); return ARRAY; }
<INITIAL>"if" { adjust(0); return IF; }
<INITIAL>"then" { adjust(0); return THEN; }
<INITIAL>"else" { adjust(0); return ELSE; }
<INITIAL>"while" { adjust(0); return WHILE; }
<INITIAL>"for" { adjust(0); return FOR; }
<INITIAL>"to" { adjust(0); return TO; }
<INITIAL>"do" { adjust(0); return DO; }
<INITIAL>"let" { adjust(0); return LET; }
<INITIAL>"in" { adjust(0); return IN; }
<INITIAL>"end" { adjust(0); return END; }
<INITIAL>"of" { adjust(0); return OF; }
<INITIAL>"break" { adjust(0); return BREAK; }
<INITIAL>"nil" { adjust(0); return NIL; }
<INITIAL>"function" { adjust(0); return FUNCTION; }
<INITIAL>"var" { adjust(0); return VAR; }
<INITIAL>"type" { adjust(0); return TYPE; }

 /* Number Literal */
<INITIAL>{digit}* { adjust(0); yylval.ival = atoi(yytext); return INT; }

 /* Identifier */
<INITIAL>{alpha}({alpha}|{digit}|_)* { adjust(0); yylval.sval = String(yytext); return ID; }

 /* String Literal */
<INITIAL>\" { adjust(0); qs_pos = 0; BEGIN S_STRING; }
<S_STRING>\\\\ {
  adjust(0);
  qs_buf[qs_pos++] = '\\';
}
<S_STRING>\\n {
  adjust(0);
  qs_buf[qs_pos++] = '\n';
}
<S_STRING>\\t {
  adjust(0);
  qs_buf[qs_pos++] = '\t';
}
<S_STRING>\\\^{alpha} {
  adjust(0);
  qs_buf[qs_pos++] = yytext[2] - 'A' + 1;
}
<S_STRING>\\{digit}{digit}{digit} {
  adjust(0);
  qs_buf[qs_pos++] = (yytext[2] - '0') * 100 + (yytext[1] - '0') * 10 + (yytext[2] - '0');
}
<S_STRING>\\\" {
  adjust(0);
  qs_buf[qs_pos++] = '"';
}
<S_STRING>\\\n {
  adjust(1);
}
<S_STRING>\\ {
  adjust(0);
  BEGIN S_QS_ESP;
}
<S_QS_ESP>\n {
  adjust(1);
}
<S_QS_ESP>{white} {
  adjust(0);
}
<S_QS_ESP>\\ {
  adjust(0);
  BEGIN S_STRING;
}
<S_QS_ESP>. {
  adjust(0);
  EM_error(EM_tokPos, "illegal character in \\f___f\\ in escape string");
}
<S_STRING>\" { adjust(0); BEGIN INITIAL; qs_buf[qs_pos] = '\0'; yylval.sval = String(qs_buf); return STRING; }
<S_STRING>. { adjust(0); qs_buf[qs_pos++] = yytext[0]; }

 /* Others */
<INITIAL>\n { adjust(1); }
<INITIAL>{white} { adjust(0); }
<INITIAL>.	 { adjust(0); EM_error(EM_tokPos,"illegal token"); }

 /* EOF check */
<S_COMMENT><<EOF>> { adjust(0); EM_error(EM_tokPos, "EOF in comment error"); return 0; }
<S_STRING><<EOF>> { adjust(0); EM_error(EM_tokPos, "EOF in string error"); return 0; }
<S_QS_ESP><<EOF>> { adjust(0); EM_error(EM_tokPos, "EOF in escape string"); return 0; }
