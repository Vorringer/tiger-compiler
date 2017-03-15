#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//Lab 6: your code here
static AS_instrList iList = NULL, last = NULL;

static void emit(AS_instr);
static void munchStm(T_stm);
static Temp_temp munchExp(T_exp);
static Temp_tempList munchArgs(unsigned int, T_expList);

static void emit(AS_instr inst) {
    if (last)
        last = last->tail = AS_InstrList(inst, NULL);
    else
        last = iList = AS_InstrList(inst, NULL);
}

static Temp_temp munchExp(T_exp exp) {
	Temp_temp r = NULL;
	switch (exp->kind) {
		case T_BINOP: {
			string op;
			switch (exp->u.BINOP.op) {
				case T_plus: {
					op = "addl";
				} break;
				case T_minus: {
					op = "subl";
				} break;
				case T_mul: {
					op = "imull";
				} break;
				case T_div: {
					op = "idivl";
				} break;
				default:
					assert(0);
			}
			if (exp->u.BINOP.op==T_div)
			{
				r=munchExp(exp->u.BINOP.left);
				emit(AS_Move(String_format("movl `s0, `d0\n"),
						Temp_TempList(F_EAX(), NULL), Temp_TempList(r, NULL)));
				emit(AS_Oper(String_format("cltd\n"),
					Temp_TempList(F_EDX(), NULL), Temp_TempList(F_EAX(),NULL), NULL));
				Temp_temp t=munchExp(exp->u.BINOP.right);
				emit(AS_Oper(String_format("idivl `s0\n"),
					Temp_TempList(F_EAX(), Temp_TempList(F_EDX(),NULL)), 
					    Temp_TempList(t, Temp_TempList(F_EDX(),Temp_TempList(F_EAX(),NULL))), NULL));
				return F_EAX();
			}
			else
			if (exp->u.BINOP.left->kind == T_CONST) {
				T_exp e2 = exp->u.BINOP.right;
				r = munchExp(e2);
				int n = exp->u.BINOP.left->u.CONST;
				emit(AS_Oper(String_format("%s $%d, `d0\n", op, n),
					Temp_TempList(r, NULL), NULL, NULL));
				return r;
			} else 
			if (exp->u.BINOP.right->kind == T_CONST) {
				T_exp e2 = exp->u.BINOP.left;
				r = munchExp(e2);
				int n = exp->u.BINOP.right->u.CONST;
				emit(AS_Oper(String_format("%s $%d, `d0\n", op, n),
					Temp_TempList(r, NULL), NULL, NULL));
				return r;
			} else {
				T_exp e1 = exp->u.BINOP.left, e2 = exp->u.BINOP.right;
				r = munchExp(e2);
				Temp_temp t = munchExp(e1);
				emit(AS_Oper(String_format("%s `s0, `d0\n", op), 
					Temp_TempList(t, NULL), Temp_TempList(r, Temp_TempList(t, NULL)), NULL));
				return t;
			}
			assert(0);
		} break;
		case T_MEM: {
			T_exp loc = exp->u.MEM;
			if (loc->kind == T_BINOP && loc->u.BINOP.op == T_plus) {
				if (loc->u.BINOP.left->kind == T_CONST) {
					r = Temp_newtemp();
					T_exp e2 = loc->u.BINOP.right;
					int n = loc->u.BINOP.left->u.CONST;
					emit(AS_Move(String_format("movl %d(`s0), `d0\n", n),
						Temp_TempList(r, NULL), Temp_TempList(munchExp(e2), NULL)));
					return r;
				} else if (loc->u.BINOP.right->kind == T_CONST) {
					r = Temp_newtemp();
					T_exp e2 = loc->u.BINOP.left;
					int n = loc->u.BINOP.right->u.CONST;
					emit(AS_Move(String_format("movl %d(`s0), `d0\n", n),
						Temp_TempList(r, NULL), Temp_TempList(munchExp(e2), NULL)));
					return r;
				} else if (loc->u.BINOP.right->kind == T_BINOP){
					r = Temp_newtemp();
					Temp_temp e1 = munchExp(loc->u.BINOP.right),
							  e2 = munchExp(loc->u.BINOP.left);
					emit(AS_Oper("addl `s0, `d0\n", Temp_TempList(e2, NULL),
													Temp_TempList(e1, NULL), NULL));

					emit(AS_Move(String_format("movl 0(`s0), `d0\n"), 
								 Temp_TempList(r, NULL),
								 Temp_TempList(e2, NULL)));
					return r;
				} else
					assert(0);
			}
			else if (loc->kind == T_CONST) {
				r = Temp_newtemp();
				int n = loc->u.CONST;
				emit(AS_Move(String_format("movl $%d, `d0\n", n), 
							 Temp_TempList(r, NULL),NULL));
				return r; 
			} else {
				r = Temp_newtemp();
				T_exp e1 = loc->u.MEM;
				emit(AS_Move(String_format("movl 0(`s0), `d0\n"), 
							 Temp_TempList(r, NULL), Temp_TempList(munchExp(e1), NULL)));
				return r;
			}
		} break;
		case T_TEMP: {
			return exp->u.TEMP;
		} break;
		case T_ESEQ: {
			munchStm(exp->u.ESEQ.stm);
			return munchExp(exp->u.ESEQ.exp);
		} break;
		case T_NAME: {
			r = Temp_newtemp();
			Temp_enter(F_tempMap, r, Temp_labelstring(exp->u.NAME));
			return r;
		} break;
		case T_CONST: {
			r = Temp_newtemp();
			emit(AS_Move(String_format("movl $%d, `d0\n", exp->u.CONST),
				Temp_TempList(r, NULL), NULL));
			return r;
		} break;
		case T_CALL: {
			Temp_temp r = munchExp(exp->u.CALL.fun);
            emit(AS_Oper(String("pushl `s0\n"), NULL, Temp_TempList(F_EDX(), NULL), NULL));
			emit(AS_Oper(String("pushl `s0\n"), NULL, Temp_TempList(F_ECX(), NULL), NULL));
			Temp_tempList args = munchArgs(0, exp->u.CALL.args);
			emit(AS_Oper("call `s0\n", F_CallerSave(), Temp_TempList(r, args), NULL));
			int incre=0;
			for (;args;args=args->tail)
			{
				//emit(AS_Oper(String("popl `s0\n"), NULL, Temp_TempList(F_EDX(), NULL), NULL));
				incre+=4;

			}
			if (incre!=0) emit(AS_Oper(String_format("addl $%d, `d0\n", incre),
					Temp_TempList(F_ESP(), NULL), NULL, NULL));
			emit(AS_Oper(String("popl `s0\n"), NULL, Temp_TempList(F_ECX(), NULL), NULL));
			emit(AS_Oper(String("popl `s0\n"), NULL, Temp_TempList(F_EDX(), NULL), NULL));
			return F_EAX();
		} break;
		default:
			assert(0);
	}
}

static void munchStm(T_stm stm) {
	switch (stm->kind) {
		case T_SEQ: {
			munchStm(stm->u.SEQ.left);
			munchStm(stm->u.SEQ.right);
		} break;
		case T_LABEL: {
			emit(AS_Label(String_format("%s:\n", Temp_labelstring(stm->u.LABEL)), 
						  stm->u.LABEL));
		} break;
		case T_JUMP: {
			Temp_label label = stm->u.JUMP.jumps->head;
			emit(AS_Oper(String_format("jmp %s\n", Temp_labelstring(label)), 
				NULL, NULL, AS_Targets(stm->u.JUMP.jumps)));
		} break;
		case T_CJUMP: {
			string jmp;
			Temp_temp left = munchExp(stm->u.CJUMP.left);
			Temp_temp right = munchExp(stm->u.CJUMP.right);
			emit(AS_Oper("cmp `s0,`s1\n", NULL, 
				 		 Temp_TempList(right, Temp_TempList(left, NULL)), NULL));
			Temp_label t = stm->u.CJUMP.true;
			switch (stm->u.CJUMP.op) {
				case T_eq:
					jmp = "je ";
					break;
				case T_ne:
					jmp = "jne ";
					break;
				case T_lt: 
					jmp = "jl ";
					break;
				case T_gt:
					jmp = "jg ";
					break;
				case T_le:
					jmp = "jle ";
					break;
				case T_ge:
					jmp = "jge ";
					break;
				default:
					assert(0);
			}
			emit(AS_Oper(String_format("%s %s\n", jmp, Temp_labelstring(t)), 
							 NULL, NULL, 
							 AS_Targets(Temp_LabelList(t, NULL))));
		} break;
		case T_MOVE: {
			T_exp dst = stm->u.MOVE.dst, src = stm->u.MOVE.src;
			if (dst->kind == T_MEM) {
				if (dst->u.MEM->kind == T_BINOP &&
					dst->u.MEM->u.BINOP.op == T_plus &&
					dst->u.MEM->u.BINOP.right->kind == T_CONST) {
					T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src;
					int c = dst->u.MEM->u.BINOP.right->u.CONST;
					emit(AS_Move(String_format("movl `s1, %d(`s0)\n", c),
							NULL, Temp_TempList(munchExp(e1), 
								  Temp_TempList(munchExp(e2), NULL))));
				} else
				if (dst->u.MEM->kind == T_BINOP &&
					dst->u.MEM->u.BINOP.op == T_plus &&
					dst->u.MEM->u.BINOP.left->kind == T_CONST) {
					T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
					int c = dst->u.MEM->u.BINOP.left->u.CONST;
					emit(AS_Move(String_format("movl `s1, %d(`s0)\n", c),
							NULL, Temp_TempList(munchExp(e1), 
								  Temp_TempList(munchExp(e2), NULL))));
				} else 
				if (src->kind == T_MEM) {
					T_exp e1 = dst->u.MEM, e2 = src;
					emit(AS_Move(String_format("movl `s1, 0(`s0)\n"),
							NULL, Temp_TempList(munchExp(e1), 
								  Temp_TempList(munchExp(e2), NULL))));
				} 
				else if (src->kind==T_NAME)
				{
					T_exp e1 = dst->u.MEM, e2 = src;
					emit(AS_Move(String_format("movl $`s1, 0(`s0)\n"),
							NULL, Temp_TempList(munchExp(e1), 
								  Temp_TempList(munchExp(e2), NULL))));
				}
				else {
					T_exp e1 = dst->u.MEM, e2 = src;
					emit(AS_Move(String_format("movl `s1, 0(`s0)\n"),
							NULL, Temp_TempList(munchExp(e1), 
								  Temp_TempList(munchExp(e2), NULL))));
				}
			} else
			if (dst->kind == T_TEMP) {
				if (src->kind == T_CONST) 
				{
					int c = src->u.CONST;
					emit(AS_Move(String_format("movl $%d, `d0\n", c),
								 Temp_TempList(munchExp(dst), NULL), NULL));

				} 
				else if (src->kind==T_NAME)
				{
					T_exp e2 = src;
					emit(AS_Move(String_format("movl $`s0, `d0\n"),
								 Temp_TempList(munchExp(dst), NULL),
									  Temp_TempList(munchExp(e2), NULL)));
				}
				else if (src->kind==T_BINOP && 
					(src->u.BINOP.left->kind==T_TEMP && src->u.BINOP.right->kind==T_CONST))
				{
					emit(AS_Move(String_format("leal %d(`s0), `d0\n",src->u.BINOP.right->u.CONST),
								Temp_TempList(munchExp(dst), NULL),
									Temp_TempList(munchExp(src->u.BINOP.left), NULL)));
				}
				else if (src->kind==T_BINOP && 
					(src->u.BINOP.right->kind==T_TEMP && src->u.BINOP.left->kind==T_CONST))
				{
					T_exp e2=src;
					emit(AS_Move(String_format("leal %d(`s0), `d0\n",src->u.BINOP.left->u.CONST),
								Temp_TempList(munchExp(dst), NULL),
									Temp_TempList(munchExp(src->u.BINOP.right), NULL)));
				}
				else {
					T_exp e2 = src;
					emit(AS_Move(String_format("movl `s0, `d0\n"),
								 Temp_TempList(munchExp(dst), NULL),
									  Temp_TempList(munchExp(e2), NULL)));
				}
			} else
				assert(0);
		} break;
		case T_EXP:
			munchExp(stm->u.EXP);
			break;
		case T_CLTD:
		    emit(AS_Move(String_format("cltd\n"),
								 Temp_TempList(F_EDX(), NULL),
									  Temp_TempList(F_EAX(), NULL)));
		    break;
		default:
			assert(0);
	}
}

static Temp_tempList munchArgs(unsigned int i, T_expList args) {
	if (!args) 
		return NULL;

	Temp_tempList tail = munchArgs(i + 1, args->tail);
	Temp_temp r = munchExp(args->head);
	emit(AS_Oper(String("pushl `s0\n"), NULL, Temp_TempList(r, NULL), NULL));	

	return Temp_TempList(r, tail);
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;
    T_stmList sl;
    for (sl = stmList; sl; sl = sl->tail)
        munchStm(sl->head);
    list = iList;
    iList = last = NULL;
    return list;
}


