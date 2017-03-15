#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/
const int F_wordSize = 4;
const int F_MAX_REG = 6;
static Temp_temp ebp = NULL, esp = NULL, eax = NULL, ebx = NULL, 
                 ecx = NULL, edx = NULL, esi = NULL, edi = NULL;

struct F_frame_ {
    int local;
    Temp_label name;
    F_accessList formals;
};

struct F_access_ {
    enum {inFrame, inReg} kind;
    union {
        int offset;
        Temp_temp reg;
    } u;
};

static F_access InFrame(int offs) {
    F_access a = checked_malloc(sizeof(*a));
    a->kind = inFrame;
    a->u.offset = offs;
    return a;
}

static F_access InReg(Temp_temp t) {
    F_access a = checked_malloc(sizeof(*a));
    a->kind = inReg;
    a->u.reg = t;
    return a;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList f = checked_malloc(sizeof(*f));
    f->head = head;
    f->tail = tail;
    return f;
}

F_frag F_StringFrag(Temp_label label, string str) {
    F_frag f = checked_malloc(sizeof(*f));
    f->kind = F_stringFrag;
    f->u.stringg.label = label;
    f->u.stringg.str = str;
    return f;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
    F_frag f = checked_malloc(sizeof(*f));
    f->kind = F_procFrag;
    f->u.proc.body = body;
    f->u.proc.frame = frame;
	return f;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
    F_fragList f = checked_malloc(sizeof(*f));
    f->head = head;
    f->tail = tail;
	return f;
}

string F_getlabel(F_frame frame) {
    return Temp_labelstring(frame->name);
}

F_access F_allocLocal(F_frame f, bool escape) {
    f->local++;
    if (escape) {
        return InFrame(-F_wordSize * f->local);
    } else {
        return InReg(Temp_newtemp());
    }
}

F_accessList F_formals(F_frame f) {
    return f->formals;
}

static F_accessList makeFormalAccessList(F_frame f, U_boolList formals) {
    U_boolList fmls = formals;
    F_accessList head = NULL, tail = NULL;
    int i = 0;
    for (; fmls; fmls = fmls->tail, i++) {
        F_access ac = NULL;
        if (i < F_MAX_REG && !fmls->head) {
            ac = InReg(Temp_newtemp());
        } else {
            /*keep a space for return*/
            ac = InFrame((i + 2) * F_wordSize);
        }
        if (head) {
            tail->tail = F_AccessList(ac, NULL);
            tail = tail->tail;
        } else {
            head = F_AccessList(ac, NULL);
            tail = head;
        }
    }
    return head;
}

Temp_label F_name(F_frame f) {
    return f->name;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame f = checked_malloc(sizeof(*f));
    f->name = name;
    f->formals = makeFormalAccessList(f, formals);
    f->local = 0;
    return f;
}

F_frag F_string(Temp_label lab, string str) {
    F_frag f = checked_malloc(sizeof(*f));
    f->kind = F_stringFrag;
    f->u.stringg.label = lab;
    f->u.stringg.str = str;
    return f;
}

F_frag F_newProcFrag(T_stm body, F_frame frame) {
    F_frag f = checked_malloc(sizeof(*f));
    f->kind = F_procFrag;
    f->u.proc.body = body;
    f->u.proc.frame = frame;
    return f;
}

Temp_temp F_EBP(void) {
    if (!ebp) 
        ebp = Temp_newtemp();
    return ebp;
}

Temp_temp F_ESP(void) {
    if (!esp) 
        esp = Temp_newtemp();
    return esp;
}

Temp_temp F_EAX(void) {
    if (!eax) 
        eax = Temp_newtemp();
    return eax;
}

Temp_temp F_EBX(void) {
    if (!ebx) 
        ebx = Temp_newtemp();
    return ebx;
}

Temp_temp F_ECX(void) {
    if (!ecx) 
        ecx = Temp_newtemp();
    return ecx;
}

Temp_temp F_EDX(void) {
    if (!edx) 
        edx = Temp_newtemp();
    return edx;
}

Temp_temp F_ESI(void) {
    if (!esi) 
        esi = Temp_newtemp();
    return esi;
}

Temp_temp F_EDI(void) {
    if (!edi) 
        edi = Temp_newtemp();
    return edi;
}

Temp_tempList F_CallerSave(void) {
    return Temp_TempList(F_EAX(), Temp_TempList(F_EDX(), Temp_TempList(F_ECX(), NULL)));
}

T_exp F_Exp(F_access access, T_exp framePtr) {
    if (access->kind == inFrame) 
        return T_Mem(T_Binop(T_plus, framePtr, T_Const(access->u.offset)));
    else 
        return T_Temp(access->u.reg);
}

T_exp F_externalCall(string str, T_expList args) {
    return T_Call(T_Name(Temp_namedlabel(str)), args);
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
    F_access ebx = F_allocLocal(frame, TRUE);
    F_access esi = F_allocLocal(frame, TRUE);
    F_access edi = F_allocLocal(frame, TRUE);
    T_stm pushEBX = T_Move(T_Binop(T_minus, T_Temp(F_EBP()), T_Const(ebx->u.offset)), T_Temp(F_EBX()));
    T_stm pushESI = T_Move(T_Binop(T_minus, T_Temp(F_EBP()), T_Const(esi->u.offset)), T_Temp(F_ESI()));
    T_stm pushEDI = T_Move(T_Binop(T_minus, T_Temp(F_EBP()), T_Const(edi->u.offset)), T_Temp(F_EDI()));
    T_stm popEBX = T_Move(T_Temp(F_EBX()), T_Binop(T_minus, T_Temp(F_EBP()), T_Const(ebx->u.offset)));
    T_stm popESI = T_Move(T_Temp(F_ESI()), T_Binop(T_minus, T_Temp(F_EBP()), T_Const(esi->u.offset)));
    T_stm popEDI = T_Move(T_Temp(F_EDI()), T_Binop(T_minus, T_Temp(F_EBP()), T_Const(edi->u.offset)));
    return T_Seq(pushEBX, T_Seq(pushESI, T_Seq(pushEDI, T_Seq(stm, T_Seq(popEBX, T_Seq(popESI, popEDI))))));
}

// AS_instrList F_procEntryExit2(AS_instrList body) {
//     //TO DO:
// }

// void F_procEntryExit3(AS_instrList *body) {
//     AS_instr prologue = AS_Oper("pushl %%ebp\nmovl %%esp, %%ebp\nsubl $64, %%esp\n", NULL, NULL, NULL);
//     AS_instr epilogue = AS_Oper("leave\nret\n", NULL, NULL, NULL);
//     *body = AS_InstrList(prologue, AS_InstrList(*body, AS_InstrList(epilogue, NULL)));
// }

static Temp_tempList registers = NULL;
static Temp_tempList Registers = NULL;
void precolor() {
    Temp_enter(F_tempMap, F_EAX(), "%eax");
    Temp_enter(F_tempMap, F_EBX(), "%ebx");
    Temp_enter(F_tempMap, F_ECX(), "%ecx");
    Temp_enter(F_tempMap, F_EDX(), "%edx");
    Temp_enter(F_tempMap, F_ESI(), "%esi");
    Temp_enter(F_tempMap, F_EDI(), "%edi");
    Temp_enter(F_tempMap, F_EBP(), "%ebp");
    Temp_enter(F_tempMap, F_ESP(), "%esp");
}

Temp_tempList F_registers() {
    if (!registers) {
        registers = Temp_TempList(F_EAX(),
                    Temp_TempList(F_EBX(),
                    Temp_TempList(F_ECX(),
                    Temp_TempList(F_EDX(),
                    Temp_TempList(F_ESI(),
                    Temp_TempList(F_EDI(), NULL))))));
    }
    precolor();
    return registers;
}
Temp_tempList F_Registers() {
    if (!Registers) {
        Registers = Temp_TempList(F_EAX(),
                    Temp_TempList(F_EBX(),
                    Temp_TempList(F_ECX(),
                    Temp_TempList(F_EDX(),
                    Temp_TempList(F_ESI(),
                    Temp_TempList(F_EDI(), NULL))))));
    }
    precolor();
    return Registers;
}

int getOffset(TAB_table table,F_frame f,Temp_temp temp){
    F_access f_access = TAB_look(table, temp);
    if (f_access == NULL){
        f_access = F_allocLocal(f, TRUE);
        TAB_enter(table, temp, f_access);
    }
    return f_access->u.offset*F_wordSize;
}
