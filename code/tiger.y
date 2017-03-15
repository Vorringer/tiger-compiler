%{
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h" 
#include "errormsg.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
 exit(1);
}
%}


%union {
	int pos;
	int ival;
	string sval;
	A_var var;
	A_exp exp;
	/* et cetera */
    A_oper oper;
    A_expList expList;
    A_efield efield;
    A_efieldList efieldList;
    A_decList decList;
    A_dec dec;
    A_nametyList nametyList;
    A_namety namety;
    A_ty ty;
    A_fundecList fundecList;
    A_fundec fundec;
    A_fieldList fieldList;
    A_field field;
}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%type <exp> exp program varExp nilExp intExp stringExp callExp opExp recordExp 
            seqExp assignExp ifExp whileExp forExp breakExp letExp arrayExp expseq
%type <expList> expList argList
%type <efield> efield
%type <efieldList> efieldList
%type <decList> decList
%type <dec> dec functionDec typeDec varDec
%type <nametyList> nametyList
%type <namety> nametyEntry
%type <ty> ty nameTy recordTy arrayTy
%type <fundecList> fundecList
%type <fundec> fundec
%type <fieldList> fieldList
%type <field> field
%type <var> lvalue
/* et cetera */

%start program
%nonassoc LOWER
%left SEMICOLON
%left COLON
%right COMMA
%nonassoc IF THEN WHILE DO FOR TO
%left OF
%left ELSE
%nonassoc ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE 
%left PLUS MINUS
%left TIMES DIVIDE
%nonassoc UMINUS

%%

program:   exp    {absyn_root=$1;}
    ;

exp:    varExp      { $$ = $1; }
    |   nilExp      { $$ = $1; }
    |   intExp      { $$ = $1; }
    |   stringExp   { $$ = $1; }
    |   callExp     { $$ = $1; }
    |   opExp       { $$ = $1; }
    |   recordExp   { $$ = $1; }
    |   seqExp      { $$ = $1; }
    |   assignExp   { $$ = $1; }
    |   ifExp       { $$ = $1; }
    |   whileExp    { $$ = $1; }
    |   forExp      { $$ = $1; }
    |   breakExp    { $$ = $1; }
    |   letExp      { $$ = $1; }
    |   arrayExp    { $$ = $1; }
    ;

varExp:     lvalue      { $$ = A_VarExp(EM_tokPos, $1); }
    ;

lvalue:     ID                        { $$ = A_SimpleVar(EM_tokPos,S_Symbol($1)); }
    |       lvalue DOT ID             { $$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3)); }
    |       lvalue LBRACK exp RBRACK  { $$ = A_SubscriptVar(EM_tokPos, $1, $3); }
    |       ID LBRACK exp RBRACK      { $$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos,S_Symbol($1)), $3); }
    ;

nilExp:     NIL     { $$ = A_NilExp(EM_tokPos); }
    ;

intExp:     INT     { $$ = A_IntExp(EM_tokPos, $1); }
    ;

stringExp:  STRING  { $$ = A_StringExp(EM_tokPos, $1); }
    ;

callExp:    ID LPAREN argList RPAREN    { $$ = A_CallExp(EM_tokPos, S_Symbol($1), $3); }
    ;

expList:    exp                   { $$ = A_ExpList($1, NULL); }
    |       exp SEMICOLON expList { $$ = A_ExpList($1, $3); } 
    |                             { $$ = NULL; }
    ;

argList:    exp                   { $$ = A_ExpList($1, NULL); }
    |       exp COMMA argList     { $$ = A_ExpList($1, $3); }
    |                             { $$ = NULL; }
    ;

expseq:     expList               { $$ = A_SeqExp(EM_tokPos, $1); }
    ;

opExp:      exp PLUS exp { $$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3); }
    |       exp MINUS exp { $$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3); }
    |       exp TIMES exp { $$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3); }
    |       exp DIVIDE exp { $$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3); }
    |       exp EQ exp { $$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3); }
    |       exp NEQ exp { $$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3); }
    |       exp LT exp { $$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3); }
    |       exp LE exp { $$ = A_OpExp(EM_tokPos, A_leOp, $1, $3); }
    |       exp GT exp { $$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3); }
    |       exp GE exp { $$ = A_OpExp(EM_tokPos, A_geOp, $1, $3); }
    |       MINUS exp  %prec UMINUS { $$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2); }
    ;

recordExp:  ID LBRACE efieldList RBRACE { $$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);}
    ;

efieldList: efield            { $$ = A_EfieldList($1, NULL); }
    |       efield COMMA efieldList { $$ = A_EfieldList($1, $3); }
    |                               { $$ = NULL; }
    ;

efield:     ID EQ exp  { $$ = A_Efield(S_Symbol($1), $3); }
    ;

seqExp:     LPAREN expList RPAREN           { $$ = A_SeqExp(EM_tokPos, $2); }
    ;

assignExp:  lvalue ASSIGN exp      { $$ = A_AssignExp(EM_tokPos, $1, $3); }
    ;

ifExp:      IF exp THEN exp  { $$ = A_IfExp(EM_tokPos, $2, $4, A_NilExp(EM_tokPos)); }
    |       IF exp THEN exp ELSE exp   { $$ = A_IfExp(EM_tokPos, $2, $4, $6); }
    |       exp AND exp      { $$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0)); }
    |       exp OR  exp      { $$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos, 1), $3); }
    ;

whileExp:   WHILE exp DO exp { $$ = A_WhileExp(EM_tokPos, $2, $4); }
    ;

forExp:     FOR ID ASSIGN exp TO exp DO exp { $$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8); }
    ;

breakExp:   BREAK     { $$ = A_BreakExp(EM_tokPos); }
    ;

letExp:     LET decList IN expseq END { $$ = A_LetExp(EM_tokPos, $2, $4); }
    ;

decList:    dec             { $$ = A_DecList($1, NULL); }
    |       dec decList     { $$ = A_DecList($1, $2); }
    ;

dec:        functionDec     { $$ = $1; }
    |       varDec          { $$ = $1; }
    |       typeDec         { $$ = $1; }
    ;

varDec:     VAR ID ASSIGN exp        { $$ = A_VarDec(EM_tokPos, S_Symbol($2), NULL, $4); }
    |       VAR ID COLON ID ASSIGN exp { $$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6); }
    ;

typeDec:    TYPE nametyList   { $$ = A_TypeDec(EM_tokPos, $2); }
    ;

nametyList: nametyEntry { $$ = A_NametyList($1, NULL); }
    |       nametyEntry nametyList { $$ = A_NametyList($1, $2); }
    ;

nametyEntry:     ID EQ ty      { $$ = A_Namety(S_Symbol($1), $3); }
    |            TYPE ID EQ ty { $$ = A_Namety(S_Symbol($2), $4); }
    ;

ty:         nameTy      { $$ = $1; }
    |       recordTy    { $$ = $1; }
    |       arrayTy     { $$ = $1; }
    ;

nameTy:     ID        { $$ = A_NameTy(EM_tokPos, S_Symbol($1)); }
    ;

recordTy:   LBRACE fieldList RBRACE   { $$ = A_RecordTy(EM_tokPos, $2); }
    ;

fieldList:  field           { $$ = A_FieldList($1, NULL); }
    |       field COMMA fieldList { $$ = A_FieldList($1, $3); }
    |                             { $$ = NULL; }
    ;

field:      ID COLON ID   { $$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3)); }
    ;

arrayTy:    ARRAY OF ID   { $$ = A_ArrayTy(EM_tokPos, S_Symbol($3)); }
    ;

functionDec:fundecList      { $$ = A_FunctionDec(EM_tokPos, $1); }
    ;

fundecList: fundec            { $$ = A_FundecList($1, NULL); }
    |       fundec fundecList { $$ = A_FundecList($1, $2); }
    ;

fundec:     FUNCTION ID LPAREN fieldList RPAREN EQ exp
                                {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL, $7); }
    |       FUNCTION ID LPAREN fieldList RPAREN COLON ID EQ exp
                                {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9); }
    ;

arrayExp:   ID LBRACK exp RBRACK OF exp { $$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6); }
    ;









	
