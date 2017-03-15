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
#include "flowgraph.h"
#include "liveness.h"
#include "color.h"
#include "table.h"

static G_nodeList precolored=NULL;
static G_nodeList simplifyWorklist=NULL;
static G_nodeList freezeWorklist=NULL;
static G_nodeList spillWorklist=NULL;
static G_nodeList spilledNodes=NULL;
static G_nodeList coalescedNodes=NULL;
static G_nodeList coloredNodes=NULL;
static G_nodeList selectStack=NULL;
static AS_instrList coalescedMoves=NULL;
static AS_instrList constrainedMoves=NULL;
static AS_instrList frozenMoves=NULL;
static AS_instrList worklistMoves=NULL;
static AS_instrList activeMoves=NULL;

static int K;
static G_nodeList* adjList = NULL;
static bool** adjSet = NULL;
static int* degree = NULL;
static TAB_table tempNode=NULL;
static G_table moveList = NULL;
static G_table alias = NULL;
static G_table color = NULL;
static Temp_tempList registers = NULL;

static int getRegIndex(Temp_map initial, G_node precoloredNode)
{
	Temp_temp temp=precoloredNode->info;
	string regStr=Temp_look(initial,temp);
	if (!strcmp(regStr,"%eax")) return 0;
	if (!strcmp(regStr,"%ebx")) return 1;
	if (!strcmp(regStr,"%ecx")) return 2;
	if (!strcmp(regStr,"%edx")) return 3;
	if (!strcmp(regStr,"%esi")) return 4;
	if (!strcmp(regStr,"%edi")) return 5;
	if (!strcmp(regStr,"%ebp")) return 0;
	if (!strcmp(regStr,"%esp")) return 0;
	return -1;


}

static int getColorIndex(G_node n) {
    counter c = G_look(color, n);
    if (!c) return -1;
    return c->num;
}

static int putColor(G_node n, int v) {
    counter c = Counter(v);
    G_enter(color, n, c);
}

String_list String_List(string str, String_list tail)
{
	String_list slist=(String_list)checked_malloc(sizeof(struct String_list_));
	slist->str=str;
	slist->tail=tail;
	return slist;
}

String_list delete_string(string str,String_list sl)
{
	if (str==sl->str) return sl->tail;
    else return String_List(sl->str, delete_string(str, sl->tail));
}

static void printWorklist(G_nodeList l)
{
    G_nodeList ll=l;
    for (;ll;ll=ll->tail) printf("%d->",((Temp_temp)(ll->head->info))->num);
    printf("\n");
}

struct COL_result COL_color(G_graph flow, Temp_map initial, Temp_tempList regs) {
	//your code here.
	struct Live_graph live_graph=Live_liveness(flow);
	init(flow->nodecount,initial,regs);
	initTempNode(live_graph.cg);


	int i = 0;

	G_nodeList nnlist=G_nodes(live_graph.cg);
	unsigned int addr=(unsigned int)nnlist;
	G_nodeList nlist=nnlist;
	for (;nlist;nlist=nlist->tail)
	{
		if (Temp_look(initial,nlist->head->info)) 
        {
                precolored=G_NodeList(nlist->head,precolored);
				int index=getRegIndex(initial,nlist->head);
                if (index==-1)continue;
                putColor(nlist->head,index);
        }
	}
	Build(live_graph.fg,live_graph.cg);

	printf("SIMP");printWorklist(simplifyWorklist);printf("\n");
	printf("FREEZE");printWorklist(freezeWorklist);printf("\n");
	printf("SPILL");printWorklist(spillWorklist);printf("\n");
	printf("PRECOLORED");printWorklist(precolored);printf("\n");
	MakeWorklist(live_graph.cg);
	do
	{
		if (simplifyWorklist) Simplify();
		else if(worklistMoves) Coalesce();
		else if (freezeWorklist) Freeze();
		else if (spillWorklist) SelectSpill();

	}while (simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist);
printWorklist(selectStack);
	AssignColors();
	struct COL_result ret;
	Temp_tempList spills = NULL;
	while (spilledNodes != NULL){
		spills = Temp_TempList(G_nodeInfo(spilledNodes->head),spills);
		spilledNodes = spilledNodes->tail;
	}
	ret.spills=spills;

	Temp_temp regArray[K];
	i = 0;
	for (; regs; regs = regs->tail) regArray[i++] = regs->head;
	TAB_table tab = TAB_empty();
	nlist = (G_nodeList)addr;
	for (; nlist; nlist = nlist->tail) 
	{
		G_node n = nlist->head;
		int index=getColorIndex(n);
		if (index==-1)continue;
		string s = Temp_look(initial, regArray[index]);
		TAB_enter(tab, G_nodeInfo(n), s);
	}
	ret.coloring = newMap(tab, NULL);


	return ret;
}

void initTempNode(G_graph cg)
{
	tempNode=TAB_empty();
	G_nodeList curr=G_nodes(cg);
	for (;curr;curr=curr->tail)
	{
		Temp_temp temp=G_nodeInfo(curr->head);
		if (temp)
		{
			TAB_enter(tempNode,temp,curr->head);
		}
	}
}

void initAdjSet(int n){
	adjSet = checked_malloc(n*sizeof(bool*));
	int i=0;
	for (i=0;i<n;i++) adjSet[i]=checked_malloc(n*sizeof(bool));
	int j=0;
    i=0;
	for(i=0;i<n;i++){
		for (j=0;j<n;j++)
		adjSet[i][j] = FALSE;
	}
}
void initAdjList(int n){
	adjList = checked_malloc(n*sizeof(G_nodeList));
	int i;
	for(i = 0;i < n;i++){
		adjList[i] = NULL;
	}
}
void initDegree(int n){
	degree = checked_malloc(n*sizeof(int));
	int i;
	for(i = 0;i < n;i++){
		degree[i] = 0;
	}
}

void init(int n,Temp_map initial,Temp_tempList regs)
{
	precolored=NULL;
	simplifyWorklist=NULL;
	freezeWorklist=NULL;
	spillWorklist=NULL;
	spilledNodes=NULL;
	coalescedNodes=NULL;
	coloredNodes=NULL;
	selectStack=NULL;
	coalescedMoves=NULL;
	constrainedMoves=NULL;
	frozenMoves=NULL;
	worklistMoves=NULL;
	activeMoves=NULL;
	K=lengthOfTempList(regs);
	initAdjSet(n);
	initAdjList(n);
	initDegree(n);
	moveList = G_empty();
	color = G_empty();
	alias = G_empty();
	registers = regs;

}

/*
void Build(G_graph flow, G_graph cg)
{
	G_nodeList curr=G_nodes(flow);
	G_table liveMap=G_empty();
	G_nodeList reversedd=G_copyList(curr);
	reversedd=reverseList(reversedd);
	calculateLiveMap(liveMap,curr);
	G_graph conflictGraph=G_Graph();
	for (;curr;curr=curr->tail)
	{
		G_nodeList reversed=reversedd;
		Temp_tempList live=lookupLiveMap(liveMap,curr->head);
		for (;reversed;reversed=reversed->tail)
		{
			AS_instr instr=G_nodeInfo(reversed->head);
			Temp_tempList def=FG_def(reversed->head);
			Temp_tempList use=FG_use(reversed->head);
			if (instr->kind==I_MOVE)
			{
				live=except(live,use);
				Temp_tempList unioned=unionn(use,def);
				for (;unioned;unioned=unioned->tail)
				{
					G_node node=TAB_look(tempNode,unioned->head);
					AS_instrList move=G_look(moveList,node);
					move=AS_InstrList(instr,move);
					G_enter(moveList,node,move);
				}
				worklistMoves=AS_InstrList(instr,worklistMoves);
			}
			live=unionn(live,def);
			Temp_tempList deff=def;
			Temp_tempList livee=live;
			for (;deff;deff=deff->tail)
			{
				for (;livee;livee=livee->tail)
				{
					G_node d=getGNode(cg,deff->head);
					G_node t=getGNode(cg,livee->head);
					AddEdge(d,t);
				}
			}
			live=unionn(use,except(live,def));
		}
	}
}
*/
void Build(G_graph fg, G_graph cg)
{

	G_nodeList curr=G_nodes(fg);
	G_nodeList clist=G_nodes(cg);
	//G_nodeList reversed=G_copyList(curr);
	//reversed=reverseList(reversed);
	for (;curr;curr=curr->tail)
	{
		AS_instr instr=G_nodeInfo(curr->head);
		Temp_tempList def=FG_def(curr->head);
		Temp_tempList use=FG_use(curr->head);
		if (instr->kind==I_MOVE)
		{
			Temp_tempList unioned=unionn(use,def);
			for (;unioned;unioned=unioned->tail)
			{
				G_node node=TAB_look(tempNode,unioned->head);
				AS_instrList move=G_look(moveList,node);
				move=AS_InstrList(instr,move);
				G_enter(moveList,node,move);
			}
			if (instr->u.MOVE.src && instr->u.MOVE.dst)
			worklistMoves=AS_InstrList(instr,worklistMoves);
		}
	}
	G_nodeList p1=clist;
	G_nodeList p2=clist;
	for (;p1;p1=p1->tail)
		{
                        p2=clist;
			for (;p2;p2=p2->tail)
			{
				if (G_goesTo(p1->head,p2->head))
					AddEdge(p1->head,p2->head);
				if (G_goesTo(p2->head,p1->head))
					AddEdge(p2->head,p1->head);
			}
		}

}

void AddEdge(G_node src, G_node dst)
{
	int u=src->mykey;
	int v=dst->mykey;
	if (!adjSet[u][v] && src!=dst)
	{
		adjSet[u][v]=TRUE;
		adjSet[v][u]=TRUE;
		if (!G_inNodeList(src,precolored))
		{
			adjList[u]=G_NodeList(dst,adjList[u]);
			degree[u]++;
		}
		if (!G_inNodeList(dst,precolored))
		{
			adjList[v]=G_NodeList(src,adjList[v]);
			degree[v]++;
		}
	}
}

void MakeWorklist(G_graph graph)
{
	G_nodeList init=G_nodes(graph);
	for (;init;init=init->tail)
	{
		if (degree[init->head->mykey]>=K)
		{
			spillWorklist=G_NodeList(init->head,spillWorklist);
		}
		else if(MoveRelated(init->head))
		{
			freezeWorklist=G_NodeList(init->head,freezeWorklist);
		}
		else
		{
			simplifyWorklist=G_NodeList(init->head,simplifyWorklist);
		}
	}
}

G_nodeList Adjacent(G_node n)
{
	return G_except(adjList[n->mykey],G_union(selectStack,coalescedNodes));
}

AS_instrList NodeMoves(G_node n)
{
	AS_instrList move=G_look(moveList,n);
	return intersect_instr(move,union_instr(activeMoves,worklistMoves));
}

bool MoveRelated(G_node n)
{
	return NodeMoves(n)!=NULL;
}

void Simplify()
{
	G_node curr=simplifyWorklist->head;
	simplifyWorklist=delete(curr,simplifyWorklist);
	selectStack=G_NodeList(curr,selectStack);
	G_nodeList adj=Adjacent(curr);
	G_nodeList adjj=adj;
	for (;adjj;adjj=adjj->tail)
	{
		DecrementDegree(adjj->head);
	}
}

void DecrementDegree(G_node m)
{
	int d=degree[m->mykey];
	degree[m->mykey]--;
	if (d==K)
	{
		EnableMoves(G_NodeList(m,Adjacent(m)));
		spillWorklist=delete(m,spillWorklist);
		if (MoveRelated(m))
		{
			freezeWorklist=G_NodeList(m,freezeWorklist);
		}
		else
		{
			simplifyWorklist=G_NodeList(m,simplifyWorklist);
		}
	}
}

void EnableMoves(G_nodeList nlist)
{
	G_nodeList curr=nlist;
	for (;curr;curr=curr->tail)
	{
		AS_instrList nodem=NodeMoves(curr->head);
		for (;nodem;nodem=nodem->tail)
		{
			if (inASinstrList(nodem->head,activeMoves))
			{
				activeMoves=delete_instr(nodem->head,activeMoves);
				worklistMoves=AS_InstrList(nodem->head,worklistMoves);
			}
		}
	}
}

static bool isEBP(G_node x, G_node y)
{
	return x->info==F_EBP() || y->info==F_EBP();
}

static bool isESP(G_node x, G_node y)
{
	return x->info==F_ESP() || y->info==F_ESP();
}


void Coalesce()
{
	AS_instrList mlist=worklistMoves;
	AS_instr instr=mlist->head;
	G_node src=TAB_look(tempNode,instr->u.MOVE.src->head);
	G_node dst=TAB_look(tempNode,instr->u.MOVE.dst->head);
	G_node x=GetAlias(src);
	G_node y=GetAlias(dst);
	G_node u,v;
	if (G_inNodeList(y,precolored))
	{
		u=y;
		v=x;
	}
	else
	{
		u=x;
		v=y;
	}
	worklistMoves=delete_instr(instr,worklistMoves);
	if (u==v)
	{
		coalescedMoves=AS_InstrList(instr,coalescedMoves);
		AddWorkList(u);
	}
	else if(G_inNodeList(v,precolored) || adjSet[u->mykey][v->mykey] || isEBP(u,v) || isESP(u,v))
	{
		constrainedMoves=AS_InstrList(instr,constrainedMoves);
		AddWorkList(u);
		AddWorkList(v);
	}
	else if (G_inNodeList(u,precolored))
	{
		G_nodeList adjv=Adjacent(v);
		G_nodeList adjvv=adjv;
		bool flag=TRUE;
		for (;adjvv;adjvv=adjvv->tail)
		{
			if (!OK(adjvv->head,u))
			{
				flag=FALSE;
				break;
			}
		}
		if (flag || (!G_inNodeList(u,precolored) && Conservative(G_union(Adjacent(u),Adjacent(v)))))
		{
			coalescedMoves=AS_InstrList(instr,coalescedMoves);
			Combine(u,v);
			AddWorkList(u);
		}
	}
	else
	{
		activeMoves=AS_InstrList(instr,activeMoves);
	}
}

void AddWorkList(G_node u)
{
	if (!G_inNodeList(u,precolored) && !MoveRelated(u) && degree[u->mykey]<K)
	{
		freezeWorklist=delete(u,freezeWorklist);
		simplifyWorklist=G_NodeList(u,simplifyWorklist);
	}
}

bool OK(G_node t, G_node r)
{
	return degree[t->mykey]<K || G_inNodeList(t,precolored) || adjSet[t->mykey][r->mykey];
}

bool Conservative(G_nodeList nlist)
{
	int k=0;
	G_nodeList curr=nlist;
	for (;curr;curr=curr->tail)
	{
		if (degree[curr->head->mykey]>=K) k++;
	}
	return k<K;
}

G_node GetAlias(G_node n)
{
	if (G_inNodeList(n,coalescedNodes))
	{
		G_node nn=G_look(alias,n);
		return GetAlias(nn);
	}
	else
	{
		return n;
	}
}

void Combine(G_node u, G_node v)
{
	if (G_inNodeList(v,freezeWorklist))
	{
		freezeWorklist=delete(v,freezeWorklist);
	}
	else
	{
		spillWorklist=delete(v,spillWorklist);
	}
	coalescedNodes=G_NodeList(v,coalescedNodes);
	G_enter(alias,v,u);
	AS_instrList moveu=G_look(moveList,u);
	AS_instrList movev=G_look(moveList,v);
	moveu=union_instr(moveu,movev);
	G_enter(moveList,u,moveu);
	EnableMoves(G_NodeList(v,NULL));
	G_nodeList nlist=adjList[v->mykey];
	G_nodeList curr=nlist;
	for (;curr;curr=curr->tail)
	{
		AddEdge(curr->head,u);
		if (!G_inNodeList(curr->head,selectStack) && !G_inNodeList(curr->head,coalescedNodes))
		DecrementDegree(curr->head);
	}
	if (degree[u->mykey]>= K && G_inNodeList(u,freezeWorklist))
	{
		freezeWorklist=delete(u,freezeWorklist);
		spillWorklist=G_NodeList(u,spillWorklist);
	}
}

void Freeze()
{
	G_node u=freezeWorklist->head;
	freezeWorklist=delete(u,freezeWorklist);
	simplifyWorklist=G_NodeList(u,simplifyWorklist);
	FreezeMoves(u);
}

void FreezeMoves(G_node u)
{
	AS_instrList curr=NodeMoves(u);
	G_node v;
	for (;curr;curr=curr->tail)
	{
		AS_instr instr=curr->head;
		G_node x=TAB_look(tempNode,instr->u.MOVE.src->head);
		G_node y=TAB_look(tempNode,instr->u.MOVE.dst->head);
		if (GetAlias(y)==GetAlias(u))
		{
			v=GetAlias(x);
		}
		else
		{
			v=GetAlias(y);
		}
		activeMoves=delete_instr(instr,activeMoves);
		frozenMoves=AS_InstrList(instr,frozenMoves);
		if (NodeMoves(v)==NULL && degree[v->mykey]<K)
		{
			freezeWorklist=delete(v,freezeWorklist);
			simplifyWorklist=G_NodeList(v,simplifyWorklist);
		}
	}
}



void SelectSpill()
{
	G_node m=spillWorklist->head;
	spillWorklist=delete(m,spillWorklist);
	simplifyWorklist=G_NodeList(m,simplifyWorklist);
	FreezeMoves(m);
}

/*
String_list getAllColors(){
	String_list head = NULL;
	Temp_tempList regs = registers;
	for (;regs;regs=regs->tail)
	{
		string colorStr=Temp_look(color,regs->head);
		head=String_List(colorStr,head);
	}
	return head;
}
*/

void AssignColors()
{
	while (selectStack)
	{
		//String_list okColors=getAllColors();
		G_node n=selectStack->head;
		selectStack=delete(n,selectStack);
		int okColors[K];
		int i=0;
		for (i=0;i<K;i++)okColors[i]=0;
		G_nodeList adjn=adjList[n->mykey];
		G_nodeList curr=adjn;
		for (;curr;curr=curr->tail)
		{
		    G_node w = GetAlias(curr->head);
		    if (G_inNodeList(w, coloredNodes) ||
			G_inNodeList(w, precolored)) 
		    {
			okColors[getColorIndex(w)] = 1;
		    }
		}
                int available=K;
		for (i = 0; i < K; i++) available -= okColors[i];
		if (available == 0) spilledNodes=G_NodeList(n,spilledNodes);
		else
		{
			coloredNodes=G_NodeList(n,coloredNodes);
			for (i = 0; i < K; i++)
		        if (!okColors[i]) 
			{
		            putColor(n, i);
		            break;
		        }
		}
	}
	G_nodeList nlist = coalescedNodes;
	for (; nlist; nlist = nlist->tail) 
	{
            G_node n = nlist->head, a = GetAlias(n);
	    putColor(n, getColorIndex(a));
	}
}
