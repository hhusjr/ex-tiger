%{
#include <string.h>
#include "util.h"
#include "y.tab.h"
#include "errormsg.h"

#define QS_BUF_SIZE 1024

int charPos=1;

int yywrap(void)
{
    charPos=1;
    return 1;
}


void adjust(void)
{
    EM_tokPos=charPos;
    charPos+=yyleng;
}


char qs_buf[QS_BUF_SIZE];
int qs_pos;
int comment_level = 0;

%}

digit [0-9]
alpha [a-zA-Z]
white (" "|"\t"|"\n"|"\v"|"\f"|"\r")
%x INTIIAL S_STRING S_COMMENT S_QS_ESP

%%

 /* Comment */
<INITIAL>"/*" { adjust(); BEGIN S_COMMENT; comment_level++; }
<S_COMMENT>"/*" { adjust(); comment_level++; }
<S_COMMENT>"*/" { adjust(); if (!--comment_level) BEGIN INITIAL; }
<S_COMMENT>. { adjust(); }
<S_COMMENT>"\n" { adjust(); EM_newline(); }

 /* Symbols and keywords */
"," { adjust(); return COMMA; }
":" { adjust(); return COLON; }
";" { adjust(); return SEMICOLON; }
"(" { adjust(); return LPAREN; }
")" { adjust(); return RPAREN; }
"[" { adjust(); return LBRACK; }
"]" { adjust(); return RBRACK;}
"{" { adjust(); return LBRACE; }
"}" { adjust(); return RBRACE; }
"." { adjust(); return DOT; }
"+" { adjust(); return PLUS; }
"-" { adjust(); return MINUS; }
"*" { adjust(); return TIMES; }
"/" { adjust(); return DIVIDE; }
"=" { adjust(); return EQ; }
":=" { adjust(); return ASSIGN; }
"!=" { adjust(); return NEQ; }
"<" { adjust(); return LT; }
">" { adjust(); return GT; }
"<=" { adjust(); return LE; }
">=" { adjust(); return GE; }
"&" { adjust(); return AND; }
"|" { adjust(); return OR; }
"array" { adjust(); return ARRAY; }
"if" { adjust(); return IF; }
"then" { adjust(); return THEN; }
"else" { adjust(); return ELSE; }
"while" { adjust(); return WHILE; }
"for" { adjust(); return FOR; }
"to" { adjust(); return TO; }
"do" { adjust(); return DO; }
"let" { adjust(); return LET; }
"in" { adjust(); return IN; }
"end" { adjust(); return END; }
"of" { adjust(); return OF; }
"break" { adjust(); return BREAK; }
"nil" { adjust(); return NIL; }
"function" { adjust(); return FUNCTION; }
"var" { adjust(); return VAR; }
"type" { adjust(); return TYPE; }

 /* Number Literal */
{digit}* { adjust(); yylval.ival = atoi(yytext); return INT; }

 /* Identifier */
{alpha}({alpha}|{digit}|_)* { adjust(); yylval.sval = String(yytext); return ID; }

 /* String Literal */
"\"" { adjust(); qs_pos = 0; BEGIN S_STRING; }
<S_STRING>"\\\\" { adjust(); qs_buf[qs_pos++] = '\\'; }
<S_STRING>"\\n" { adjust(); qs_buf[qs_pos++] = '\n'; }
<S_STRING>"\\t" { adjust(); qs_buf[qs_pos++] = '\t'; }
<S_STRING>"\\^"{alpha} { adjust(); qs_buf[qs_pos++] = yytext[2] - 'A' + 1; }
<S_STRING>"\\"{digit}{digit}{digit} {
    adjust();
    qs_buf[qs_pos++] = (yytext[2] - '0') * 100 + (yytext[1] - '0') * 10 + (yytext[2] - '0');
}
<S_STRING>"\\\"" { adjust(); qs_buf[qs_pos++] = '"'; }
<S_STRING>"\\\n" { adjust(); EM_newline(); }
<S_STRING>"\\" { adjust(); BEGIN S_QS_ESP; }
<S_QS_ESP>"\n" { adjust(); EM_newline(); }
<S_QS_ESP>{white} { adjust(); }
<S_QS_ESP>"\\" { adjust(); BEGIN S_STRING; }
<S_QS_ESP>. {
    adjust();
    EM_error(EM_tokPos, "illegal character in \\f___f\\ in escape string");
}
<S_STRING>"\"" { adjust(); BEGIN INITIAL; qs_buf[qs_pos] = '\0'; yylval.sval = String(qs_buf); return STRING; }
<S_STRING>. { adjust(); qs_buf[qs_pos++] = yytext[0]; }

 /* Others */
"\n" { adjust(); EM_newline(); }
{white} { adjust(); }
. { adjust(); EM_error(EM_tokPos,"illegal token"); }

 /* EOF check */
<S_COMMENT><<EOF>> { adjust(); EM_error(EM_tokPos, "EOF in comment error"); return 0; }
<S_STRING><<EOF>> { adjust(); EM_error(EM_tokPos, "EOF in string error"); return 0; }
<S_QS_ESP><<EOF>> { adjust(); EM_error(EM_tokPos, "EOF in escape string"); return 0; }
