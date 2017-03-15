#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"


Temp_tempList FG_def(G_node n) {
	//your code here.
    AS_instr x = G_nodeInfo(n);
    switch (x->kind) {
        case I_OPER:
            return x->u.OPER.dst;
        case I_MOVE:
            return x->u.MOVE.dst;
	    case I_LABEL:
	        return NULL;
        default:
            assert(0);
    }
	return NULL;
}

Temp_tempList FG_use(G_node n) {
	//your code here.
    AS_instr x = G_nodeInfo(n);
    switch (x->kind) {
        case I_OPER:
            return x->u.OPER.src;
        case I_MOVE:
            return x->u.MOVE.src;
	    case I_LABEL:
	        return NULL;
        default:
            assert(0);
    }
	return NULL;
}

bool FG_isMove(G_node n) {
	//your code here.
    AS_instr x = G_nodeInfo(n);
	return x && x->kind == I_MOVE;
}

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
	//your code here.
    G_graph g = G_Graph();
    G_table t = G_empty();
    G_nodeList nlist = NULL;
    G_node pre = NULL;
    AS_instrList i;
    for (i = il; i; i = i->tail) {
        AS_instr inst = i->head;
        G_node now = G_Node(g, inst);
        if (pre) G_addEdge(pre, now);
        switch (inst->kind) {
            case I_OPER: 
                nlist = G_NodeList(now, nlist);
                break;
            case I_LABEL:
                TAB_enter(t, inst->u.LABEL.label, now);
                break;
            case I_MOVE:
                break;
            default:
                assert(0);
        }
        pre = now;
    }
    for (; nlist; nlist = nlist->tail) {
        AS_instr inst = G_nodeInfo(nlist->head);
        if (!inst->u.OPER.jumps) continue;
        Temp_labelList jumps = inst->u.OPER.jumps->labels;
        Temp_labelList j;
        for (j = jumps; j; j = j->tail) {
            G_node n = TAB_look(t, j->head);
            G_addEdge(nlist->head, n);
        }
    }
	return g;
}
