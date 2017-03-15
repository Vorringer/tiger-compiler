%{
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"

int charPos=1;
int count=0;
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
/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

%}
  /* You can add lex definitions here. */
%Start COMMENT

%%
  /* 
  * Below are some examples, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

<INITIAL>"/*"[^*/]*"*/" {adjust(); continue;}
<INITIAL>" "  {adjust(); continue;} 
<INITIAL>","	 {adjust(); return COMMA;}
<INITIAL>":"  {adjust(); return COLON;}
<INITIAL>";"  {adjust(); return SEMICOLON;}
<INITIAL>"("  {adjust(); return LPAREN;}
<INITIAL>")"  {adjust(); return RPAREN;}
<INITIAL>"["  {adjust(); return LBRACK;}
<INITIAL>"]"  {adjust(); return RBRACK;}
<INITIAL>"{"  {adjust(); return LBRACE;}
<INITIAL>"}"  {adjust(); return RBRACE;}
<INITIAL>"."  {adjust(); return DOT;}
<INITIAL>"+"  {adjust(); return PLUS;}
<INITIAL>"-"  {adjust(); return MINUS;}
<INITIAL>"*"  {adjust(); return TIMES;}
<INITIAL>"/"  {adjust(); return DIVIDE;}
<INITIAL>"="  {adjust(); return EQ;}
<INITIAL>"<>" {adjust(); return NEQ;}
<INITIAL>"<"  {adjust(); return LT;}
<INITIAL>"<=" {adjust(); return LE;}
<INITIAL>">"  {adjust(); return GT;}
<INITIAL>">=" {adjust(); return GE;}
<INITIAL>"&"  {adjust(); return AND;}
<INITIAL>"|"  {adjust(); return OR;}
<INITIAL>":=" {adjust(); return ASSIGN;}
<INITIAL>while     {adjust(); return WHILE;}
<INITIAL>for       {adjust(); return FOR;}
<INITIAL>to        {adjust(); return TO;}
<INITIAL>break     {adjust(); return BREAK;}
<INITIAL>let       {adjust(); return LET;}
<INITIAL>in        {adjust(); return IN;}
<INITIAL>end       {adjust(); return END;}
<INITIAL>function  {adjust(); return FUNCTION;}
<INITIAL>var       {adjust(); return VAR;}
<INITIAL>type      {adjust(); return TYPE;}
<INITIAL>array     {adjust(); return ARRAY;}
<INITIAL>if        {adjust(); return IF;}
<INITIAL>then      {adjust(); return THEN;}
<INITIAL>else      {adjust(); return ELSE;}
<INITIAL>do        {adjust(); return DO;}
<INITIAL>of        {adjust(); return OF;}
<INITIAL>nil       {adjust(); return NIL;}
<INITIAL>\n	 {adjust(); EM_newline(); continue;}
<INITIAL>\t   {adjust(); continue;}

<INITIAL>[0-9]+   {adjust(); yylval.ival=atoi(yytext); return INT;}
<INITIAL>[a-zA-Z][A-Za-z0-9_]* {adjust(); yylval.sval = String(yytext); return ID;}
<INITIAL>\"[!\n\\t>A-Za-z0-9 -]*("\\n"|"\""|".")*\" {
	adjust(); 
	yylval.sval = String(yytext);
	int i;
	for (i=0; i<strlen(yylval.sval)-1; i++)
		if (yylval.sval[i] == '\\' &&
			yylval.sval[i+1] == 'n') {
			yylval.sval[i] = '\n';
			int j;
			for (j=i+2; j<strlen(yylval.sval); j++)
				yylval.sval[j-1] = yylval.sval[j];
			yylval.sval[j-1] = '\0';
		}
		else if (yylval.sval[i] == '\\' &&
			yylval.sval[i+1] == 't') {
			yylval.sval[i] = '\t';
			int j;
			for (j=i+2; j<strlen(yylval.sval); j++)
				yylval.sval[j-1] = yylval.sval[j];
			yylval.sval[j-1] = '\0';
		}
	yylval.sval[strlen(yylval.sval)-1]='\0';
	yylval.sval++;
	if (strlen(yylval.sval) == 0) yylval.sval = "";
	return STRING;
}
<INITIAL>"/*"	{adjust();count++; BEGIN COMMENT;}
<COMMENT>"/*"	{adjust();count++;continue;}
<COMMENT>"*/"	{adjust();count--; if(count == 0) BEGIN INITIAL;}
<COMMENT>. 	{adjust(); continue; }
<COMMENT>\n 	{adjust(); EM_newline(); continue; }
%%


