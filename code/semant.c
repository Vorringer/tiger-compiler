#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/
int loop;
S_symbol loopvar;

struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

static U_boolList makeFormals(A_fieldList params) {
	U_boolList head = NULL, tail = NULL;
	A_fieldList p = params;
	for (; p; p = p->tail) {
		if (head) {
			tail->tail = U_BoolList(TRUE, NULL);
			tail = tail->tail;
		} else {
			head = U_BoolList(TRUE, NULL);
			tail = head;
		}
	}
	return head;
}

static A_exp transform(A_exp forExp) {
	// struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr;
	A_pos p = forExp->pos;
	S_symbol var = forExp->u.forr.var;
	S_symbol lim = S_Symbol("_LIMIT");
    A_var v = A_SimpleVar(p, var);
	A_dec varDec = A_VarDec(p, var, NULL, forExp->u.forr.lo);
	A_dec limDec = A_VarDec(p, lim, NULL, forExp->u.forr.hi);
	A_decList decs = A_DecList(varDec, A_DecList(limDec, NULL));
	A_exp test = A_OpExp(p, 7, A_VarExp(p, A_SimpleVar(p, var)), A_VarExp(p, A_SimpleVar(p, lim)));
	A_exp inc = A_AssignExp(p, v, A_OpExp(p, 0, A_VarExp(p, v), A_IntExp(p, 1)));
    A_expList tail = A_ExpList(inc, NULL);
    A_exp w = A_WhileExp(p, test, A_SeqExp(p, A_ExpList(forExp->u.forr.body, tail)));

	return A_LetExp(p, decs, w);
}

struct expty transVar(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
        case A_simpleVar: {
            // printf("%s\n", S_name(v->u.simple));
            E_enventry x = S_look(venv, v->u.simple);
            if (x && x->kind == E_varEntry) {
            	Tr_exp tr = Tr_simpleVar(x->u.var.access, level);
                return expTy(tr, actual_ty(x->u.var.ty));
            }
            else {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(Tr_noExp(), Ty_Int());
            }
        }
        case A_fieldVar: {
            struct expty lvalue = transVar(level, exp, venv, tenv, v->u.field.var);
            if (lvalue.ty->kind == Ty_record)  {
                Ty_fieldList list = lvalue.ty->u.record;
                int i = 0;
                while (list) {
                    Ty_field item = list->head;
                    if (S_name(item->name) == S_name(v->u.field.sym)) {
                    	Tr_exp tr = Tr_fieldVar(lvalue.exp, i);
                        return expTy(tr, actual_ty(item->ty));
                    }
                    i++;
                    list = list->tail;
                }
                EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
            } else 
                EM_error(v->pos, "not a record type");
            return expTy(Tr_noExp(), Ty_Int());
        }
        case A_subscriptVar: {
            struct expty lvalue = transVar(level, exp, venv, tenv, v->u.subscript.var);
            if (lvalue.ty->kind == Ty_array) {
                struct expty sub = transExp(level, exp, venv, tenv, v->u.subscript.exp);
                if (sub.ty->kind ==Ty_int) {
                	Tr_exp tr = Tr_subscriptVar(lvalue.exp, sub.exp);
                    return expTy(tr, actual_ty(lvalue.ty->u.array));
                }
            }
            EM_error(v->pos, "array type required");
            return expTy(Tr_noExp(), Ty_Int());
        }
    }
}

static count = 0;


struct expty transExp(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_exp e) {
    switch (e->kind) {
        case A_varExp: {
            return transVar(level, exp, venv, tenv, e->u.var); 
        }
        case A_nilExp: {
        	// printf("A_nilExp\n");
            return expTy(Tr_nilExp(), Ty_Nil());
        }
        case A_intExp: {
        	// printf("A_initExp\n");
            return expTy(Tr_intExp(e->u.intt), Ty_Int());
        }
        case A_stringExp: {
        	// printf("A_stringExp %s\n", e->u.stringg);
            return expTy(Tr_stringExp(e->u.stringg), Ty_String());
        }
        case A_callExp: {
            E_enventry f = S_look(venv, e->u.call.func);
		Tr_expList argList = NULL;
            if (f && f->kind == E_funEntry) {
                A_expList arg = e->u.call.args;
                Ty_tyList formal = f->u.fun.formals;
                while (arg) {
                    if (!formal) {
                        EM_error(arg->head->pos, "too many params in function %s", S_name(e->u.call.func));
                        break;
                    }
                    struct expty x = transExp(level, exp, venv, tenv, arg->head);
                    Tr_expList_prepend(x.exp, &argList);
                    //argList=Tr_ExpList(x.exp,argList);
                    if (actual_ty(x.ty)->kind != actual_ty(formal->head)->kind) {
                        // printf("%d %d\n", x.ty->kind, actual_ty(formal->head)->kind);
                        EM_error(arg->head->pos, "para type mismatch");
                    }
                    arg = arg->tail;
                    formal = formal->tail;
                }
                if (formal) 
                    EM_error(e->pos, "call exp too little args");
                argList=reverse_TrList(argList);
                Tr_exp tr = Tr_callExp(f->u.fun.label, f->u.fun.level, level, &argList, actual_ty(f->u.fun.result));
                return expTy(tr, f->u.fun.result);
            } else {
                EM_error(e->pos, "undefined function %s", S_name(e->u.call.func));
                return expTy(Tr_noExp(), Ty_Int());
            }
        }
        case A_opExp: {
        	// printf("A_opExp\n");
            A_oper op = e->u.op.oper;
            struct expty left = transExp(level, exp, venv, tenv, e->u.op.left);
            struct expty right = transExp(level, exp, venv, tenv, e->u.op.right);
            switch (op) {
                case A_plusOp:
                case A_minusOp:
                case A_timesOp:
                case A_divideOp: {
                    if (actual_ty(left.ty)->kind != Ty_int) {
                        EM_error(e->u.op.left->pos, "integer required");
                    	return expTy(Tr_noExp(), Ty_Int());
                    }
                    if (actual_ty(right.ty)->kind != Ty_int) {
                        EM_error(e->u.op.right->pos, "integer required");
                    	return expTy(Tr_noExp(), Ty_Int());
                    }
                    return expTy(Tr_arithExp(op, left.exp, right.exp), Ty_Int());
                }
                case A_eqOp:
                case A_neqOp: {
                    if (actual_ty(left.ty)->kind == Ty_int) {
                        if (actual_ty(right.ty)->kind != Ty_int && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        else
                        	return expTy(Tr_eqExp(op, left.exp, right.exp), Ty_Int());
                        return expTy(Tr_noExp(), Ty_Int());
                    }
                    if (actual_ty(left.ty)->kind == Ty_string) {
                        if (actual_ty(right.ty)->kind != Ty_string && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        else
                        	return expTy(Tr_eqStringExp(op, left.exp, right.exp), Ty_Int());
                        return expTy(Tr_noExp(), Ty_Int());
                    }
                    if (actual_ty(left.ty)->kind == Ty_array) {
                        if (actual_ty(right.ty)->kind != Ty_array && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                       	else
                       		return expTy(Tr_eqRef(op, left.exp, right.exp), Ty_Int());
                        return expTy(Tr_noExp(), Ty_Int());
                    }
                    if (actual_ty(left.ty)->kind == Ty_record) {
                        if (actual_ty(right.ty)->kind != Ty_record && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                       	else
                       		return expTy(Tr_eqRef(op, left.exp, right.exp), Ty_Int());
                        return expTy(Tr_noExp(), Ty_Int());
                    }
                }
                default: {
                    if (actual_ty(left.ty)->kind == Ty_int) {
                        if (actual_ty(right.ty)->kind != Ty_int && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        else
                        	return expTy(Tr_relExp(op, left.exp, right.exp), Ty_Int());
                        return expTy(Tr_noExp(), Ty_Int());
                    }
                    if (actual_ty(left.ty)->kind == Ty_string) {
                        if (actual_ty(right.ty)->kind != Ty_string && actual_ty(right.ty)->kind != Ty_nil)
                            EM_error(e->u.op.right->pos, "same type required");
                        else
                        	return expTy(Tr_eqStringExp(op, left.exp, right.exp), Ty_Int());
                        return expTy(Tr_noExp(), Ty_Int());
                    }
                }
            }
        }
        case A_recordExp: {
        	// printf("A_recordExp\n");
            Ty_ty x = actual_ty(S_look(tenv, e->u.record.typ));
            if (x && x->kind == Ty_record) {
            	int n = 0;
            	Tr_expList l = NULL;
                Ty_fieldList field = x->u.record;
                A_efieldList efield = e->u.record.fields;
                while (field) {
                    if (!efield || !field) {
                        EM_error(e->pos, "record field amount does not match");
                        goto something_wrong;
                    }
                    if (S_name(field->head->name) != S_name(efield->head->name)) {
                        EM_error(e->pos, "record field name does not match");
                        goto something_wrong;
                    }
                    struct expty val = transExp(level, exp, venv, tenv, efield->head->exp);
                    Tr_expList_prepend(val.exp, &l);
                    if (actual_ty(val.ty)->kind != actual_ty(field->head->ty)->kind &&
                        actual_ty(val.ty)->kind != Ty_nil) {
                        EM_error(e->pos, "record field type does not match");
                        goto something_wrong;
                    }
                    efield = efield->tail;
                    field = field->tail;
                    n++;
                }
                return expTy(Tr_recordExp(n, l), x);
            } else 
                EM_error(e->pos, "undefined type %s", S_name(e->u.record.typ));
            // if (!x) return expTy(Tr_noExp(), Ty_Record(NULL));
            something_wrong:
            return expTy(NULL, Ty_Record(NULL));
        }
        case A_seqExp: {
        	// printf("A_seqExp\n");
        	Tr_expList l = NULL;
            A_expList list = e->u.seq;
            struct expty result;
            if (!list)
				return expTy(Tr_noExp(), Ty_Void());
            while (list) {
                result = transExp(level, exp, venv, tenv, list->head);
                Tr_expList_prepend(result.exp, &l);
                list = list->tail;
            }
            return expTy(Tr_seqExp(l), result.ty);
        }
        case A_assignExp: {
        	// printf("A_assignExp\n");
            struct expty lvalue = transVar(level, exp, venv, tenv, e->u.assign.var);
            struct expty rvalue = transExp(level, exp, venv, tenv, e->u.assign.exp);
            if (lvalue.ty->kind != rvalue.ty->kind && rvalue.ty->kind !=Ty_nil) 
                EM_error(e->pos, "unmatched assign exp %d %d", lvalue.ty->kind, rvalue.ty->kind);
            if (e->u.assign.var->kind == A_simpleVar &&
                e->u.assign.var->u.simple == loopvar) 
                EM_error(e->pos, "loop variable can't be assigned");
            return expTy(Tr_assignExp(lvalue.exp, rvalue.exp), Ty_Void());
        }
        case A_ifExp: {
        	// printf("A_ifExp\n");
            struct expty exp1 = transExp(level, exp, venv, tenv, e->u.iff.test);
            struct expty exp2 = transExp(level, exp, venv, tenv, e->u.iff.then);
            struct expty exp3 = (e->u.iff.elsee) ? 
                                transExp(level, exp, venv, tenv, e->u.iff.elsee) : 
                                expTy(NULL, Ty_Void());
            if (exp1.ty->kind != Ty_int)
                EM_error(e->pos, "integer expression required ");
            if (exp3.ty->kind == Ty_void) {
                if (exp2.ty->kind != Ty_void) {
                    printf("%d\n", exp2.ty->kind);
                    EM_error(e->u.iff.then->pos, "if-then exp's body must produce no value");
                }
                return expTy(Tr_ifExp(exp1.exp, exp2.exp, exp3.exp), Ty_Void());
            }
            if (exp2.ty->kind != exp3.ty->kind && exp3.ty->kind != Ty_nil)
                EM_error(e->pos, "then exp and else exp type mismatch");
            if (exp2.ty->kind == exp3.ty->kind) 
                return expTy(Tr_ifExp(exp1.exp, exp2.exp, exp3.exp), exp2.ty);
            else if (exp3.ty->kind == Ty_nil)
                return expTy(Tr_ifExp(exp1.exp, exp2.exp, exp3.exp), exp2.ty);
            return expTy(Tr_ifExp(exp1.exp, exp2.exp, exp3.exp), Ty_Void());
        }
        case A_whileExp: {
        	// printf("A_whileExp\n");
            loop++;
            struct expty exp1 = transExp(level, exp, venv, tenv, e->u.whilee.test);
            Tr_exp done = Tr_doneExp();
            struct expty exp2 = transExp(level, done, venv, tenv, e->u.whilee.body);
            loop--;
            if (exp1.ty->kind != Ty_int)
                EM_error(e->pos, "integer expression required");
            if (exp2.ty->kind != Ty_nil && exp2.ty->kind != Ty_void)
                EM_error(e->pos, "while body must produce no value");
            return expTy(Tr_whileExp(exp1.exp, exp2.exp, done), Ty_Void());
        }
        case A_forExp: {
            A_exp forExp = transform(e);
            struct expty result = transExp(level, exp, venv, tenv, forExp);
            return expTy(result.exp, Ty_Void());
        }
        case A_breakExp: {
        	// printf("A_breakExp\n");
            if (!loop)
                EM_error(e->pos, "break alone");
            if (!exp) 
            	return expTy(Tr_noExp(), Ty_Void());
           	else
            	return expTy(Tr_breakExp(exp), Ty_Void());
        }
        case A_letExp: {
        	// printf("A_letExp\n");
        	Tr_expList l = NULL;
            S_beginScope(tenv);
            S_beginScope(venv);

            A_decList list = e->u.let.decs;
            while (list) {
                A_dec dec = list->head;

                switch (dec->kind) {
                    case A_functionDec: {
                        A_fundecList flist = dec->u.function;
                        for (; flist; flist = flist->tail) {
                            A_fundec x = flist->head;

                            E_enventry f = S_look(venv, x->name);
                            if (f) {
                                A_fundecList iter = dec->u.function;
                                int flag = 1;
                                for (; iter != flist; iter = iter->tail) { 
                                    if (S_name(iter->head->name) == S_name(x->name)) {
                                        flag = 0;
                                        break;
                                    }
                                }
                                if (!flag) {
                                    EM_error(x->pos, "two functions have the same name");
                                    continue;
                                }
                            }

                            Ty_tyList formals = NULL, current;
                            A_fieldList param = x->params;
                            while (param) {
                                A_field now = param->head;
                                Ty_ty typ = S_look(tenv, now->typ);
                                if (!typ) {
                                    EM_error(now->pos, "fundec params no such type");
                                    typ = Ty_Int();
                                }
                                Ty_tyList one = Ty_TyList(typ, NULL);
                                if (!formals) {
                                    formals = one;
                                    current = formals;
                                } else {
                                    current->tail = one;
                                    current = one;
                                }

                                param = param->tail;
                            }


                            Ty_ty result = Ty_Void();
                            if (x->result) {
                                result = S_look(tenv, x->result);
                                if (!result) {
                                    EM_error(x->pos, "fundec result no such type");
                                    result = Ty_Int();
                                }
                            }
                            Temp_label funLabel = Temp_namedlabel(S_name(x->name));
                            Tr_level l = Tr_newLevel(level, funLabel, makeFormals(x->params));
                            S_enter(venv, x->name, E_FunEntry(l, funLabel, formals, result));
                        }
                        Tr_expList_prepend(transDec(level, exp, venv, tenv, dec), &l);
                    } break;
                    case A_typeDec: {
                        A_nametyList nametyList = dec->u.type;
                        for (; nametyList; nametyList = nametyList->tail) {
                            A_namety now = nametyList->head;

                            Ty_ty x = S_look(tenv, now->name);
                            if (x) {
                                A_nametyList iter = dec->u.type;
                                int flag = 1;
                                for (; iter != nametyList; iter = iter->tail) { 
                                    if (S_name(iter->head->name) == S_name(now->name)) {
                                        flag = 0;
                                        break;
                                    }
                                }
                                if (!flag) {
                                    EM_error(dec->pos, "two types have the same name");
                                    continue;
                                }
                            }
                            else
                                S_enter(tenv, now->name, Ty_Name(now->name, NULL));
                        }
                        Tr_expList_prepend(transDec(level, exp, venv, tenv, dec), &l);
                    } break;
                    case A_varDec: 
                        Tr_expList_prepend(transDec(level, exp, venv, tenv, dec), &l);
                }
                list = list->tail;
            }

            struct expty body = transExp(level, exp, venv, tenv, e->u.let.body);
            Tr_expList_prepend(body.exp, &l);

            S_endScope(venv);
            S_endScope(tenv);

            return expTy(Tr_seqExp(l), body.ty);
        }
        case A_arrayExp: {
        	// printf("A_arrayExp\n");
            Ty_ty ty = actual_ty(S_look(tenv, e->u.array.typ));
            if (!ty || ty->kind != Ty_array)
                EM_error(e->pos, "not array type");
            struct expty exp1 = transExp(level, exp, venv, tenv, e->u.array.size);
            struct expty exp2 = transExp(level, exp, venv, tenv, e->u.array.init);
            if (exp1.ty->kind != Ty_int)
                EM_error(e->u.array.size->pos, "array size is not integer");
            if (ty && actual_ty(exp2.ty)->kind != actual_ty(ty->u.array)->kind)
                EM_error(e->u.array.init->pos, "type mismatch");
            return expTy(Tr_arrayExp(exp1.exp, exp2.exp), Ty_Array(ty->u.array));
        }
        default:
            EM_error(e->pos, "undefined expression");
    }
}

Tr_exp transDec(Tr_level level, Tr_exp exp, S_table venv, S_table tenv, A_dec d) {
    switch (d->kind) {
        case (A_functionDec): {
            A_fundecList function;
            for (function = d->u.function; function; function = function->tail) {
                A_fundec fundec = function->head;
                E_enventry funEntry = S_look(venv, fundec->name);
                S_beginScope(venv);
                Ty_tyList formalTys = funEntry->u.fun.formals, s;
                Tr_accessList acls = Tr_formals(funEntry->u.fun.level);

                A_fieldList params;
                for (params = fundec->params; 
                	 params && acls; 
           			 params = params->tail, acls = acls->tail) {
                    A_field param = params->head;
                    Ty_ty typ = actual_ty(S_look(tenv, param->typ));
                    if (!typ) {
                        EM_error(fundec->pos, "undefined type");
                        typ = Ty_Int();
                    }
                    S_enter(venv, param->name, E_VarEntry(acls->head, typ));
                }

                struct expty body = transExp(funEntry->u.fun.level, exp, venv, tenv, fundec->body);
                Tr_procEntryExit(funEntry->u.fun.level, body.exp, fundec->result);

                S_endScope(venv);
                Ty_ty result = Ty_Void();
                if (fundec->result) 
                   result = actual_ty(S_look(tenv, fundec->result));
                if (result) {
                    if (result->kind != body.ty->kind) {
                        if (result->kind == Ty_void)
                            EM_error(fundec->pos, "procedure returns value");
                        else
                            EM_error(fundec->body->pos, "fundec body type conflict");
                    }
                } else
                    EM_error(fundec->pos, "fundec no such return type");
            }
            return Tr_noExp();
        } break;
        case (A_typeDec): {
            A_nametyList type;
            for (type = d->u.type; type; type = type->tail) {
                A_namety now = type->head;
                Ty_ty namety = S_look(tenv, now->name);
                Ty_ty ty = transTy(tenv, now->ty);
                namety->u.name.ty = ty;
                while (ty && ty->kind == Ty_name) {
                    if (ty->u.name.ty == namety) {
                        EM_error(d->pos, "illegal type cycle");
                        break;
                    }
                    ty = ty->u.name.ty;
                }
            }
            return Tr_noExp();
        } break;
        case (A_varDec): {
            Ty_ty typ = Ty_Nil();
            if (d->u.var.typ) {
                typ = actual_ty(S_look(tenv, d->u.var.typ));
                if (!typ) {
                    EM_error(d->pos, "vardec error");
                    typ = Ty_Void();
                }
            }
            struct expty init = transExp(level, exp, venv, tenv, d->u.var.init);
            Tr_access ac = Tr_allocLocal(level, d->u.var.escape);
            if (init.ty->kind != typ->kind && typ->kind != Ty_nil && init.ty->kind != Ty_nil) {
                EM_error(d->pos, "type mismatch");
            }
            if (init.ty->kind == typ->kind) {
                if (typ->kind == Ty_record && S_name(d->u.var.typ) != S_name(d->u.var.init->u.record.typ))
                    EM_error(d->pos, "type mismatch");               
                if (typ->kind == Ty_array) {
                    S_symbol sx = d->u.var.typ, sy = d->u.var.init->u.array.typ;
                    Ty_ty y = S_look(tenv, d->u.var.init->u.array.typ);
                    while (y->kind == Ty_name) {
                        sy = y->u.name.sym;
                        if (y->u.name.ty->kind != Ty_name)
                            break;
                        y = y->u.name.ty;
                    }
                    if (S_name(sx) != S_name(sy))
                        EM_error(d->pos, "type mismatch");               
                }
            }

            if (init.ty->kind == Ty_nil && !d->u.var.typ)
                EM_error(d->pos, "init should not be nil without type specified");

            if (typ->kind == Ty_nil)
                S_enter(venv, d->u.var.var, E_VarEntry(ac, init.ty));
            else
                S_enter(venv, d->u.var.var, E_VarEntry(ac, typ));
            return Tr_assignExp(Tr_simpleVar(ac, level), init.exp);
        }
    }
}

Ty_ty transTy(S_table tenv, A_ty a) {
    switch (a->kind) {
        case (A_nameTy): {
            Ty_ty ty = S_look(tenv, a->u.name);
            if (!ty)
                EM_error(a->pos, "typedec namety error");
            return ty;
        }
        case (A_recordTy): {
            A_fieldList record;
            Ty_fieldList fieldList = NULL, current;
            for (record = a->u.record; record; record = record->tail) {
                A_field entry = record->head;
                Ty_ty ty = S_look(tenv, entry->typ);
                if (!ty) {
                    EM_error(entry->pos, "undefined type %s", S_name(entry->typ));
                    //return NULL;
		            exit(0);
                }
                Ty_fieldList field = Ty_FieldList(Ty_Field(entry->name, ty), NULL);
                if (!fieldList) {
                    fieldList = field;
                    current = fieldList;
                } else {
                    current->tail = field;
                    current = field;
                }
            }
            return Ty_Record(fieldList);
        }
        case (A_arrayTy): {
            Ty_ty ty = S_look(tenv, a->u.array);
            if (!ty) {
                EM_error(a->pos, "typedec array error");
                ty = Ty_Int();
            }
            return Ty_Array(ty);
        }
    }
}

Ty_ty actual_ty(Ty_ty ty) {
    while (ty && ty->kind == Ty_name) {
        ty = ty->u.name.ty;
        if (!ty) break;
    }
    return ty;
}

F_fragList SEM_transProg(A_exp exp){
    S_table venv = E_base_venv();
    S_table tenv = E_base_tenv();
    loop = 0;
    S_enter(tenv, S_Symbol("int"), Ty_Int());
    S_enter(tenv, S_Symbol("string"), Ty_String());
    S_enter(venv, S_Symbol("getchar"), E_FunEntry(Tr_outermost(), Temp_namedlabel("getchar"), NULL, Ty_String()));
    S_enter(venv, S_Symbol("ord"), E_FunEntry(Tr_outermost(), Temp_namedlabel("ord"), Ty_TyList(Ty_String(), NULL), Ty_Int()));
    S_enter(venv, S_Symbol("print"), E_FunEntry(Tr_outermost(), Temp_namedlabel("print"), Ty_TyList(Ty_String(), NULL), Ty_Void()));
    S_enter(venv, S_Symbol("printi"), E_FunEntry(Tr_outermost(), Temp_namedlabel("printi"), Ty_TyList(Ty_Int(), NULL), Ty_Void()));
    S_enter(venv, S_Symbol("chr"), E_FunEntry(Tr_outermost(), Temp_namedlabel("chr"), Ty_TyList(Ty_Int(), NULL), Ty_String()));


    Tr_level outermost = Tr_outermost();
    struct expty program = transExp(outermost, NULL, venv, tenv, exp);
    Tr_procEntryExit(outermost, program.exp, NULL);
    
    return Tr_getResult();
}

