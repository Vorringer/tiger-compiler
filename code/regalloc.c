#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "color.h"
#include "liveness.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"


static Temp_temp getNewTemp(TAB_table table, Temp_temp oldTemp){
    Temp_temp newTemp = TAB_look(table, oldTemp);
    if (newTemp == NULL){
        newTemp = Temp_newtemp();
        TAB_enter(table, oldTemp, newTemp);
    }
    return newTemp;
}

static void rewriteProgram(F_frame f,Temp_tempList temp_tempList,AS_instrList il){
    AS_instrList pre = NULL, cur = il;
        TAB_table tempMapOffset = TAB_empty();
    while (cur != NULL){
        AS_instr as_Instr = cur->head;
                Temp_tempList defTempList = NULL;
                Temp_tempList useTempList = NULL;
        switch (as_Instr->kind){
        case I_OPER:
                  defTempList = as_Instr->u.OPER.dst;
                  useTempList = as_Instr->u.OPER.src;
                  break;
        case I_MOVE:
                  defTempList = as_Instr->u.MOVE.dst;
                  useTempList = as_Instr->u.MOVE.src;
                  break;
                default:
                  break;
                }
                if(useTempList!=NULL||defTempList!=NULL){
            TAB_table oldMapNew = TAB_empty();
            while (useTempList != NULL){
                if (inTemp_tempList(useTempList->head, temp_tempList)){
                    assert(pre);
                    Temp_temp newTemp = getNewTemp(oldMapNew, useTempList->head);
                    int offset = getOffset(tempMapOffset,f,useTempList->head);
                    string instr = String_format("movl %d(`s0),`d0\n", offset);
                    AS_instr as_instr = AS_Move(instr, Temp_TempList(newTemp,NULL),Temp_TempList(F_EBP(),NULL));
                                        useTempList->head = newTemp;
                                        pre = pre->tail = AS_InstrList(as_instr,cur);
                }
                useTempList = useTempList->tail;
            }
            while (defTempList != NULL){
                if (inTemp_tempList(defTempList->head, temp_tempList)){
                    assert(pre);
                    Temp_temp newTemp = getNewTemp(oldMapNew, defTempList->head);
                    int offset = getOffset(tempMapOffset, f, defTempList->head);
                    string instr = String_format("movl `s0,%d(`s1)\n", offset);
                    AS_instr as_instr = AS_Move(instr,NULL, Temp_TempList(newTemp,Temp_TempList(F_EBP(),NULL)));
                    cur->tail = AS_InstrList(as_instr, cur->tail);
                                        defTempList->head = newTemp;
                }
                defTempList = defTempList->tail;
            }
                }
                pre = cur;
                cur = cur->tail;
    }
}

static void removeRedundantMoves(Temp_map m,AS_instrList il){
  AS_instrList pre = NULL;
  while(il!=NULL){
    AS_instr as_instr = il->head;
    if(as_instr->kind==I_MOVE&&strcmp(as_instr->u.MOVE.assem,"movl `s0, `d0\n")==0){
      Temp_tempList dst = as_instr->u.MOVE.dst;
      Temp_tempList src = as_instr->u.MOVE.src;
      string temp1 = Temp_look(m,dst->head);
      string temp2 = Temp_look(m,src->head);
      assert(temp1&&temp2);
      if(temp1==temp2){
        assert(pre);
        pre->tail = il->tail;
        il = il->tail;
        continue;
      }
    }
    pre = il;
    il = il->tail;
  }
}

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here.
	struct RA_result ret;
	G_graph fg = FG_AssemFlowGraph(il, f);
	Temp_tempList regs = F_Registers();
	Temp_map initial = Temp_layerMap(Temp_empty(), F_tempMap);
	struct COL_result col_result = COL_color(fg, initial, regs);
    if(col_result.spills!=NULL){
      rewriteProgram(f, col_result.spills, il);
      return RA_regAlloc(f,il);
    }
	ret.coloring = col_result.coloring;
    removeRedundantMoves(ret.coloring,il);
	ret.il = il;

	return ret;
}
