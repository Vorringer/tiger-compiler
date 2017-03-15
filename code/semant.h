#ifndef SEMANT_H
#define SEMANT_H

#include "frame.h"
#include "translate.h"
#include "types.h"

// typedef void *Tr_exp; // avoid IR

struct expty {Tr_exp exp; Ty_ty ty;};

struct expty expTy(Tr_exp exp, Ty_ty ty);

struct expty transVar(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_var v);

struct expty transExp(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_exp e);

Tr_exp       transDec(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_dec d);

struct expty transVarmak(S_table venv, S_table tenv, A_var v);

       Ty_ty transTy(               S_table tenv, A_ty a);

       Ty_ty actual_ty(Ty_ty ty);

F_fragList SEM_transProg(A_exp exp);

#endif
