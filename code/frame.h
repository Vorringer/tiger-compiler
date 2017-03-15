
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "temp.h"
#include "tree.h"
#include "assem.h"


typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};
F_accessList F_AccessList(F_access head, F_accessList tail);

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {
            enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {F_frag head; F_fragList tail;};
F_fragList F_FragList(F_frag head, F_fragList tail);

int staticLinkNum;
Temp_map F_tempMap;
void precolor();
Temp_tempList F_registers(void);
Temp_tempList F_Registers(void);
string F_getlabel(F_frame frame);
T_exp F_Exp(F_access acc, T_exp framePtr);
F_access F_allocLocal(F_frame f, bool escape);
F_accessList F_formals(F_frame f);
Temp_label F_name(F_frame f);
Temp_temp F_EBP(void);//ebp
Temp_temp F_ESP(void);//esp
Temp_temp F_EAX(void);
Temp_temp F_EBX(void);
Temp_temp F_ECX(void);
Temp_temp F_EDX(void);
Temp_temp F_EDI(void);
Temp_temp F_ESI(void);
Temp_temp F_ZERO(void);
Temp_temp F_RA(void);
Temp_temp F_RV(void);
Temp_tempList F_CallerSave(void);
F_frame F_newFrame(Temp_label name, U_boolList formals);
T_exp F_externalCall(string s, T_expList args);
F_frag F_string(Temp_label lab, string str);
F_frag F_newProcFrag(T_stm body, F_frame frame);
T_stm F_procEntryExit1(F_frame frame, T_stm stm);
int getOffset(TAB_table table,F_frame f,Temp_temp temp);
// AS_instrList F_procEntryExit2(AS_instrList body);
// void F_procEntryExit3(AS_instrList *body);

#endif
