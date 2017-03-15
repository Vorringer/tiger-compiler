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
#include "liveness.h"
#include "table.h"

static Temp_tempList regs;
static TAB_table NodeMap;

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}


Temp_temp Live_gtemp(G_node n) {
	//your code here.

	return (Temp_temp)G_nodeInfo(n);
}

struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	struct Live_graph lg;;
	lg.cg=G_Graph();
	lg.fg=flow;
	lg.moves=NULL;
	G_nodeList nodes=G_nodes(flow);
	G_nodeList curr=nodes;
	G_table gt=G_empty();
	calculateLiveMap(gt,curr);
	buildConflictGraph(lg.cg,flow,gt);
	//G_show(stdout, G_nodes(lg->graph), showTemp);
	return lg;
}

void showTemp(void* info)
{
	Temp_temp temp=info;
	fprintf(stdout, "Temp: %d\n", Temp_data(temp));
}

void calculateLiveMap(G_table t, G_nodeList nodes)
{
	G_nodeList curr=nodes;
	int n=0;
	while (curr){n++;curr=curr->tail;}
	curr=nodes;
	Temp_tempList in[500];
	Temp_tempList out[500];
	for (int i=0;i<n;i++)
	{
		in[i]=NULL;
		out[i]=NULL;
	}
	bool reachedFixedPoint=TRUE;
	do
	{
		reachedFixedPoint=TRUE;
		Temp_tempList inn=NULL;
		Temp_tempList outt=NULL;
		curr=nodes;
		int i=0;
		for (;curr;curr=curr->tail)
		{
			inn=Temp_copyList(in[i]);
			outt=Temp_copyList(out[i]);
			in[i]=unionn(FG_use(curr->head),except(out[i],FG_def(curr->head)));
			G_nodeList succ = G_succ(curr->head);
			out[i]=NULL;
			for (; succ; succ = succ->tail) 
			{
				int index=findGnode(nodes,succ->head);
				assert(index!=-1);
				out[i] = unionn(out[i], in[index]);
			}
			if (!isequalTempList(in[i],inn) || !isequalTempList(out[i],outt)) reachedFixedPoint=FALSE;
			i++;
		}
	}while (!reachedFixedPoint);
	curr=nodes;
	for (int i=0;i<n;i++)
	{
		enterLiveMap(t,curr->head,out[i]);
		curr=curr->tail;
	}
}

static void printTempList(Temp_tempList l)
{
    Temp_tempList ll=l;
    for (;ll;ll=ll->tail) printf("%d->",(ll->head)->num);
    printf("\n");
}

static void getAllRegs(G_graph g) {
	regs = NULL;
	G_nodeList l = G_nodes(g);
	for (; l; l = l->tail) {
		G_node n = l->head;
		regs = unionn(regs, unionn(FG_use(n), FG_def(n)));
	}
}

static void Conflict_initial(G_graph g) {
	NodeMap = TAB_empty();
	Temp_tempList i;
	for (i = regs; i; i = i->tail) {
		G_node n = G_Node(g, i->head);
		TAB_enter(NodeMap, i->head, n);
	}
}

void buildConflictGraph(G_graph cg, G_graph flow, G_table liveMap)
{
	getAllRegs(flow);
	Conflict_initial(cg);
	G_nodeList curr=G_nodes(flow);
	for (;curr;curr=curr->tail)
	{
		AS_instr instr = (AS_instr)G_nodeInfo(curr->head);
		Temp_tempList def=FG_def(curr->head);
		Temp_tempList use=FG_use(curr->head);
		Temp_tempList liveTempp=lookupLiveMap(liveMap,curr->head);
	        //if (def)getGNode(lg->graph,def->head);
		//if (liveTempp)getGNode(lg->graph,liveTempp->head);
		//if (use)getGNode(lg->graph,use->head);
		for (;def;def=def->tail)
		{
                        Temp_tempList liveTemp=liveTempp;
			for (;liveTemp;liveTemp=liveTemp->tail)
			{
				G_node d = TAB_look(NodeMap, def->head);
				G_node t = TAB_look(NodeMap, liveTemp->head);
				if (d != t) 
				{

					if (instr->kind == I_MOVE) 
					{
						Temp_tempList uses = FG_use(curr->head);

						G_node c = uses ? TAB_look(NodeMap, uses->head) : NULL;
							if (t != c) 
								G_addEdge(d, t);

					} 
					else if (instr->kind == I_OPER || instr->kind == I_LABEL) 
					{

						G_addEdge(d, t);

					} else assert(0);
				}
			}
		}
	}
	G_nodeList nlist = G_nodes(cg);
	for (; nlist; nlist = nlist->tail) {
		Temp_temp t = G_nodeInfo(nlist->head);
		printf("\n%d:\n\t", t->num);
		G_nodeList adj = G_adj(nlist->head);
		for (; adj; adj = adj->tail) {
			t = G_nodeInfo(adj->head);
			printf("%d, ", t->num);
		}
	}
	
	
	
}

G_node getGNode(G_graph g, Temp_temp info)
{
	G_nodeList nl=G_nodes(g);
	while (nl)
	{
		Temp_temp temp=G_nodeInfo(nl->head);
		if (info!=NULL && Temp_data(temp)==Temp_data(info)) return nl->head;
		nl=nl->tail;
	}
	return (G_node)G_Node(g, info);
}

int findGnode(G_nodeList nodes, G_node node)
{
	G_nodeList curr=nodes;
	int index=0;
	while (curr)
	{
		if (curr->head==node) return index;
		curr=curr->tail;
		index++;
	}
	return -1;
}

static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps)
{
	G_enter(t,flowNode,temps);
}

Temp_tempList lookupLiveMap(G_table t, G_node flowNode)
{
	return (Temp_tempList)G_look(t,flowNode);
}

Live_moveList delete_move(G_node src,G_node dst, Live_moveList mlist)
{
	assert(src && dst && mlist);
	if (src==mlist->src && dst==mlist->dst) return mlist->tail;
	else return Live_MoveList(mlist->src, mlist->dst, delete_move(src,dst, mlist->tail));
}

static Live_moveList cat_move(Live_moveList a, Live_moveList b) {
  if (a==NULL) return b;
  else return Live_MoveList(a->src, a->dst, cat_move(a->tail, b));
}

Live_moveList union_move(Live_moveList l1, Live_moveList l2)
{
    Live_moveList l=cat_move(l1,l2);
    Live_moveList ll=l;
    for (; l; l = l->tail) 
    {
        Live_moveList tmpl = l->tail;
        G_node tmp = l->src;
        G_node tmp2=l->dst;
        Live_moveList parent = l;
        for (; tmpl; tmpl = tmpl->tail) 
        {
            if (tmp == tmpl->src && tmp2==tmpl->dst) 
            {
              parent->tail = tmpl->tail; 
            } 
            else 
            {
              parent = tmpl;
            }
        }
    }
    return ll;
}

Live_moveList intersect_move(Live_moveList l1, Live_moveList l2)
{
	Live_moveList r=NULL;
	Live_moveList ll1=l1;
	Live_moveList ll2=l2;
	for (;ll1;ll1=ll1->tail)
	{
		ll2=l2;
		for (;ll2;ll2=ll2->tail)
		{
			if (ll1->src==ll2->src && ll1->dst==ll2->dst)
			{
				r=Live_MoveList(ll1->src,ll1->dst,r);
			}
		}
	}
	return r;
}

bool inMoveList(G_node src, G_node dst, Live_moveList l) {
  Live_moveList p;
  for(p=l; p!=NULL; p=p->tail)
    if (p->src==src && p->dst==dst) return TRUE;
  return FALSE;
}

